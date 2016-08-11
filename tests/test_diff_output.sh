#!/bin/sh

[[ "x${TESTS_DIR}" == "x" ]] && TESTS_DIR="."
. ${TESTS_DIR}/test_common.sh

test_start

# process the PDFs in the docs and compare their output to the one
# saved
status=0
for file in "${TESTS_DIR}/docs"/*.pdf
do
    REFEDN="${file%.*}.edn"

    # uncompress the reference output if needed
    if [ ! -f $REFEDN ]; then
        $BUNZIP2 "$REFEDN.bz2"
    fi

    cmd="$PDFTOEDN -f -o $TMPFILE $file"
    echo $cmd
    `$cmd`
    status=$?

    if [ $status -ne 0 ]; then
        echo "\tError processing file $file"
        break
    fi


    # remove the filename string and version strings hash so it
    # doesn't cause diff output on version bumps
    cat $TMPFILE | sed 's/:versions {[a-z0-9\:\"\.\, \-]*}/:versions {}/' | sed 's/:filename "[a-zA-z0-9\.\-\/]*"/:filename ""/' > t1.tmp
    cat $REFEDN  | sed 's/:versions {[a-z0-9\:\"\.\, \-]*}/:versions {}/' | sed 's/:filename "[a-zA-z0-9\.\-\/]*"/:filename ""/' > t2.tmp

    $DIFF t1.tmp t2.tmp &> /dev/null
    status=$?

    $RM t1.tmp t2.tmp
    if [ $status -ne 0 ]; then
        echo "\tFile output did not match reference output"
    fi
done

test_end

# returns last file's status
exit $status
