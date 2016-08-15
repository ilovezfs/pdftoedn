#!/bin/sh

[ "x${TESTS_DIR}" = "x" ] && TESTS_DIR="."
. ${TESTS_DIR}/test_common.sh

EXPECTED_SUBSTR="does not look like a valid PDF"

test_start

# test processing an invalid PDF
run_cmd "${PDFTOEDN} -o dummy.edn $0"
status=$?

test_end

flag_set $status $CODE_INIT_ERROR && \
    check_stdout "$EXPECTED_SUBSTR" && \
    exit 0

echo "unexpected return value $status"
exit $status
