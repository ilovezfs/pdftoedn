#!/bin/sh

[ "x${TESTS_DIR}" = "x" ] && TESTS_DIR="."
. ${TESTS_DIR}/test_common.sh

test_start

# process the PDFs in the docs and compare their output to the one
# saved
status=0
for file in "${TESTS_DIR}/docs"/*.bz2
do
    REFEDN="${file%.*}"
    SRCPDF="${REFEDN%.*}.pdf"
    FONTMAP="${REFEDN%.*}.json"
    ARGS="-f"

    # uncompress the reference output if needed
    if [ ! -f "$REFEDN" ]; then
        $BUNZIP2 "$file"
    fi

    # if there's a fontmap available for the doc, use it
    if [ -f "$FONTMAP" ]; then
        ARGS="$ARGS -m "$FONTMAP""
    fi

    # process the PDF
    run_cmd "$PDFTOEDN $ARGS -o "$TMPFILE" "$SRCPDF""
    status=$?

    if [ $status -ne 0 ]; then
        echo "\tError processing file $file"
        break
    fi

    # remove the filename string and version strings hash so it
    # doesn't cause diff output on version bumps
    filter_meta $TMPFILE t1.tmp
    filter_meta $REFEDN t2.tmp

    # diff them
    $DIFF t1.tmp t2.tmp &> /dev/null
    status=$?

    $RM t1.tmp t2.tmp
    if [ $status -ne 0 ]; then
        echo " -> File output did not match reference output"
    fi
done

test_end

# returns last file's status
exit $status
