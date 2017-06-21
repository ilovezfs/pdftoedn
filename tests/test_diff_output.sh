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

    # enc_test.pdf is encrypted so pass the password using -u
    if [ "${SRCPDF#*enc_test.pdf}" != "$SRCPDF" ]; then
        # encrypted test - "enc_test.pdf" is the password
        ARGS="$ARGS -u enc_test.pdf"
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
    filter_meta "$TMPFILE" t1.tmp

    # diff them
    $DIFF t1.tmp "$REFEDN" &> /dev/null
    status=$?

    $RM t1.tmp
    if [ $status -ne 0 ]; then
        echo " -> File output for $SRCPDF did not match reference output $REFEDN"
        break
    fi

    echo
done

test_end

# returns last file's status
exit $status
