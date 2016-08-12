#!/bin/sh

TMPFILE=edn.tmp
TESTDOC=${TESTS_DIR}/docs/HUN.pdf

DIFF="diff"
RM="rm -f"
BZIP2="bzip2 -k"
BUNZIP2="$BZIP2 -d"

test_start() {
    if [ ! -x "$PDFTOEDN" ]; then
        export PDFTOEDN=`which pdftoedn`

        if [ -z "$PDFTOEDN" ]; then
            echo "No pdftoedn binary found"
            exit -1
        fi

        echo "** Missing build pdftoedn exec - Using $PDFTOEDN **"
    fi
}


test_end() {
    [[ -f "$TMPFILE" ]] && $RM "$TMPFILE"
}
