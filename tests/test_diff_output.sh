#!/bin/sh

. ./common.sh

test_start

# process the PDFs in the docs and compare their output to the one
# saved
for file in "docs"/*.pdf
do
    REFEDN="${file%.*}.edn"

    # uncompress the reference output if needed
    if [ ! -f $REFEDN ]; then
        $BUNZIP2 "$REFEDN.bz2"
    fi

    cmd="$PDFTOEDN -f -o $TMPFILE $file"
    echo $cmd
    `$cmd`

    # remove the version strings hash so it doesn't cause diff output
    # on version bumps
    $CAT $TMPFILE | $SED 's/:versions {[a-z0-9\:\"\.\, ]*}/:versions {}/' > t1.tmp
    $CAT $REFEDN  | $SED 's/:versions {[a-z0-9\:\"\.\, ]*}/:versions {}/' > t2.tmp
    $DIFF t1.tmp t2.tmp 2> /dev/null
    status=$?

    $RM t1.tmp t2.tmp
    if [ $status -ne 0 ]; then
        echo "Output did not match"
        break
    fi
done

test_end

exit $status
