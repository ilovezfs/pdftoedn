#!/bin/sh

[[ "x${TESTS_DIR}" == "x" ]] && TESTS_DIR="."
. ${TESTS_DIR}/test_common.sh

echo "$0"

test_start

# test processing an invalid PDF
$PDFTOEDN -o dummy.edn "$0"
status=$?

echo status is $status

test_end
if [ $status -eq 5 ]; then
    exit 0
fi
echo "Error invalid PDF error not detected"
exit $status
