#!/bin/sh

[ "x${TESTS_DIR}" = "x" ] && TESTS_DIR="."
. ${TESTS_DIR}/test_common.sh

EXPECTED_SUBSTR="file was encrypted and password was incorrect or not supplied"

test_start

# test processing an encrypted file with the wrong password
run_cmd "${PDFTOEDN} -u badpassword -o dummy.edn "$TEST_ENCDOC""
status=$?

test_end

flag_set $status $CODE_INIT_ERROR && \
    check_stdout "$EXPECTED_SUBSTR" && \
    exit 0

echo "unexpected return value $status"
exit $status
