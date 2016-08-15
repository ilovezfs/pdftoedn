#!/bin/sh

[ "x${TESTS_DIR}" = "x" ] && TESTS_DIR="."
. ${TESTS_DIR}/test_common.sh

EXPECTED_SUBSTR="does not exist"

test_start

# test invalid -m option
run_cmd "$PDFTOEDN -m "$TMPFILE" -o "$TMPFILE" "$TESTDOC""
status=$?

test_end

flag_set $status $CODE_INIT_ERROR && \
    check_stdout "$EXPECTED_SUBSTR" && \
    exit 0

echo "unexpected return value $status"
exit $status
