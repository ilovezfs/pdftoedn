#!/bin/sh

[ "x${TESTS_DIR}" = "x" ] && TESTS_DIR="."
. ${TESTS_DIR}/test_common.sh

EXPECTED_SUBSTR="JSON parse error"

test_start

# make a json map with a syntax error
FONTMAP=json.tmp
echo '{ "fontMaps" { } }' > $FONTMAP

# test -m option with a invalid json syntax file
run_cmd "$PDFTOEDN -m "$FONTMAP" -o "$TMPFILE" "$TESTDOC""
status=$?

test_end

# check error output and returned code
flag_set $status $CODE_INIT_ERROR && \
    check_stdout "$EXPECTED_SUBSTR" && \
    exit 0

echo "unexpected return value $status"
exit $status
