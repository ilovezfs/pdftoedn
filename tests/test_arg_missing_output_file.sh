#!/bin/sh

[ "x${TESTS_DIR}" = "x" ] && TESTS_DIR="."
. ${TESTS_DIR}/test_common.sh

EXPECTED_SUBSTR="the option '--output_file' is required but missing"

test_start

# test missing -o option
run_cmd "$PDFTOEDN "$TESTDOC""
status=$?

test_end

flag_set $status $CODE_INIT_ERROR && \
    check_stdout "$EXPECTED_SUBSTR" && \
    exit 0

echo "unexpected return value $status"
exit $status
