#!/bin/sh

[ "x${TESTS_DIR}" = "x" ] && TESTS_DIR="."
. ${TESTS_DIR}/test_common.sh

PDFTOEDN=$1

if [ -z $PDFTOEDN ]; then
    PDFTOEDN=`which pdftoedn`
fi

# regenerate output for files that have bzip2'd edn output
for file in ${TESTS_DIR}/docs/*.edn.bz2; do
    REFEDN="${file%.*}"
    SRCPDF="${REFEDN%.*}.pdf"
    FONTMAP="${REFEDN%.*}.json"

    ARGS="-f"

    if [ -f "$FONTMAP" ]; then
        # use the specified fontmap if it exists
        ARGS="$ARGS -m "$FONTMAP""
    fi

    # process EDN
    run_cmd "${PDFTOEDN} $ARGS -o "$REFEDN" "$SRCPDF""

    if [ $? -eq 0 ]; then
        # compress ouptut
        bzip2 -f "$REFEDN"
        [ $? -eq 0 ] && echo "Compressed $REFEDN"
    fi
done
