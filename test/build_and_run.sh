#!/bin/bash
set -e # Fail on error

ROOT_DIR=$(cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd)

$ROOT_DIR/build.sh

cd $ROOT_DIR/build/

function test_failure
{
    echo ""
    ./loguru_test $1 && echo "Expected command to fail!" && exit 1
    echo ""
    echo ""
}

function test_success
{
    echo ""
    ./loguru_test $1 || echo "Expected command to succeed!"
    echo ""
    echo ""
}

echo "---------------------------------------------------------"
echo "Testing failures..."
echo "---------------------------------------------------------"
test_failure "ABORT_F"
test_failure "ABORT_S"
test_failure "assert"
test_failure "LOG_F_FATAL"
test_failure "LOG_S_FATAL"
test_failure "CHECK_NOTNULL_F"
test_failure "CHECK_F"
test_failure "CHECK_EQ_F_int"
test_failure "CHECK_EQ_F_unsigned"
test_failure "CHECK_EQ_F_size_t"
test_failure "CHECK_EQ_F"
test_failure "CHECK_EQ_F_message"
test_failure "CHECK_EQ_S"
test_failure "CHECK_LT_S"
test_failure "CHECK_LT_S_message"
test_failure "deep_abort"
test_failure "SIGSEGV"
test_failure "abort"
test_failure "error_context"
test_failure "throw_on_fatal"
test_failure "throw_on_signal"
test_success "callback"
echo "---------------------------------------------------------"
echo "ALL TESTS PASSED!"
echo "---------------------------------------------------------"

./loguru_test $@

./loguru_test hang
