#!/bin/sh
PDFTOEDN=../src/pdftoedn
TMPFILE=edn.tmp
TESTDOC=docs/HUN.pdf
TESTREF=docs/HUN.edn

DIFF="diff"
RM="rm -f"
SED="sed"
CAT="cat"
BZIP2="bzip2 -k"
BUNZIP2="$BZIP2 -d"

test_start() {
    echo > /dev/null
}


test_end() {
    $RM $TMPFILE
}
