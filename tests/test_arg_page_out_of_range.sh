#!/bin/sh

[ "x${TESTS_DIR}" = "x" ] && TESTS_DIR="."
. ${TESTS_DIR}/test_common.sh

EXPECTED_SUBSTR="Error: requested page number"

test_start

# document has 6 pages - try to process page 7. Note page arg is
# 0-indexed
pg=6
run_cmd "$PDFTOEDN -p $pg -o "$TMPFILE" "$TESTDOC""
status=$?

test_end

flag_set $status $CODE_INIT_ERROR && \
    check_stdout "$EXPECTED_SUBSTR" && \
    exit 0

echo "unexpected return value $status"
exit $status
