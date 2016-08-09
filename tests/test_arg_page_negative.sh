#!/bin/sh

. ./common.sh

test_start


# try to pass a negative page argument
$PDFTOEDN -p -1 -o $TMPFILE $TESTDOC 2> /dev/null
status=$?


test_end

if [ $status -ne 0 ]; then
    exit 0
fi
echo "Error page -1 was accepted!"
exit 1
