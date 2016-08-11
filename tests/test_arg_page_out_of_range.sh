#!/bin/sh

[[ "x${TESTS_DIR}" == "x" ]] && TESTS_DIR="."
. ${TESTS_DIR}/test_common.sh

test_start


# document has 6 pages - try to process page 7. Note page arg is
# 0-indexed
pg=6
$PDFTOEDN -p $pg -o $TMPFILE $TESTDOC
status=$?


test_end
if [[ $status -eq 1 ]]; then
    exit 0
fi
echo "Error page ($pg) outside of range was accepted!"
exit $status
