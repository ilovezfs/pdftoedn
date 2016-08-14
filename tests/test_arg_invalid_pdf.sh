#!/bin/sh

[ "x${TESTS_DIR}" = "x" ] && TESTS_DIR="."
. ${TESTS_DIR}/test_common.sh

test_start

# test processing an invalid PDF
run_cmd "${PDFTOEDN} -o dummy.edn $0"
status=$?

test_end

if [ $status -eq 5 ]; then
    exit 0
fi
echo "Error invalid PDF error not detected"
exit $status
