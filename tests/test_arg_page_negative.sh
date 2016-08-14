#!/bin/sh

[ "x${TESTS_DIR}" = "x" ] && TESTS_DIR="."
. ${TESTS_DIR}/test_common.sh

test_start

# try to pass a negative page argument
run_cmd "$PDFTOEDN -p -1 -o "$TMPFILE" "$TESTDOC""
status=$?

test_end

if [ $status -eq 1 ]; then
    exit 0
fi
echo "Error page -1 was accepted!"
exit $status
