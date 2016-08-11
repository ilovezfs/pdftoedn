#!/bin/sh

[[ "x${TESTS_DIR}" == "x" ]] && TESTS_DIR="."
. ${TESTS_DIR}/test_common.sh

test_start

# test missing -o option
$PDFTOEDN $TESTDOC
status=$?


test_end
if [ $status -eq 1 ]; then
    exit 0
fi
echo "Error missing output file was accepted!"
exit $status
