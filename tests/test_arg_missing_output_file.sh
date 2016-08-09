#!/bin/sh

. ./common.sh

test_start

# test missing -o option
$PDFTOEDN $TESTDOC 2> /dev/null
RET=$?


test_end
if [ $RET -ne 0 ]; then
    exit 0
fi
echo "Error missing output file was accepted!"
exit 1
