#!/bin/bash

[[ "x${TESTS_DIR}" == "x" ]] && TESTS_DIR="."

PDFTOEDN=$1

if [[ -z $PDFTOEDN ]]; then
    PDFTOEDN=`which pdftoedn`
fi

for file in ${TESTS_DIR}/docs/*.edn.bz2; do
    REFEDN="${file%.*}"
    SRCPDF="${REFEDN%.*}.pdf"
    FONTMAP="${REFEDN%.*}.json"

    ARGS="-f"

    if [[ -f "$FONTMAP" ]]; then
        ARGS="$ARGS -m "$FONTMAP""
    fi

    # process EDN
    cmd="${PDFTOEDN} $ARGS -o $REFEDN $SRCPDF"
    echo $cmd
    `$cmd`

    if [[ $? -eq 0 ]]; then
        # compress ouptut
        bzip2 -f $REFEDN
        [[ $? -eq 0 ]] && echo "Compressed $REFEDN"
    fi
done
