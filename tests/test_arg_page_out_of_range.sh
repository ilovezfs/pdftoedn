#!/bin/sh

. ./common.sh

test_start


# document has 6 pages - try to process page 7
$PDFTOEDN -p 6 -o $TMPFILE $TESTDOC 2> /dev/null
status=$?


test_end
if [ $status -ne 0 ]; then
    exit 0
fi
echo "Error page outside of range was accepted!"
exit 1
