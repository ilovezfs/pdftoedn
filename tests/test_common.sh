#!/bin/sh

TMPFILE=edn.tmp
TESTDOC=${TESTS_DIR}/docs/HUN.pdf

DIFF="diff"
RM="rm -f"
BZIP2="bzip2 -k"
BUNZIP2="$BZIP2 -d"

# set the pdftoedn binary
test_start () {
    if [ ! -x "$PDFTOEDN" ]; then
        export PDFTOEDN=`which pdftoedn`

        if [ -z "$PDFTOEDN" ]; then
            echo "No pdftoedn binary found"
            exit -1
        fi

        echo "** Missing build pdftoedn exec - Using $PDFTOEDN **"
    fi
}

# remove contents of :filename and :versions hashes from data output
# so diff can compare everything else
filter_meta () {
    SRC="$1"
    DST="$2"
    cat "$SRC" | sed 's/:versions {.*}/:versions {}/' | sed 's/:filename ".*"/:filename ""/' > "$DST"
}

# output and execute the command
run_cmd () {
    cmd=("$@")
    echo $cmd
    $cmd
    return $?
}


# cleanup
test_end () {
    [ -f "$TMPFILE" ] && $RM "$TMPFILE"
}
