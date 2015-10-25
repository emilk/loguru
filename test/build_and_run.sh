#!/bin/bash
set -e # Fail on error

ROOT_DIR=$(cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd)

cd "$ROOT_DIR"
mkdir -p build
cd build
cmake ..
make

echo "---------------------------------------------------------"
echo "Testing failures..."
echo "---------------------------------------------------------"
./loguru_test ABORT_F             && echo "Expected command to fail!" && exit 1
./loguru_test assert              && echo "Expected command to fail!" && exit 1
./loguru_test CHECK_NOTNULL_F     && echo "Expected command to fail!" && exit 1
./loguru_test CHECK_F             && echo "Expected command to fail!" && exit 1
./loguru_test CHECK_EQ_F_int      && echo "Expected command to fail!" && exit 1
./loguru_test CHECK_EQ_F_unsigned && echo "Expected command to fail!" && exit 1
./loguru_test CHECK_EQ_F_size_t   && echo "Expected command to fail!" && exit 1
./loguru_test CHECK_EQ_F          && echo "Expected command to fail!" && exit 1
./loguru_test CHECK_EQ_F_message  && echo "Expected command to fail!" && exit 1
./loguru_test CHECK_EQ_F          && echo "Expected command to fail!" && exit 1
./loguru_test CHECK_EQ_S          && echo "Expected command to fail!" && exit 1
./loguru_test CHECK_LT_S          && echo "Expected command to fail!" && exit 1
./loguru_test CHECK_LT_S_message  && echo "Expected command to fail!" && exit 1
./loguru_test deep_abort          && echo "Expected command to fail!" && exit 1
./loguru_test SIGSEGV             && echo "Expected command to fail!" && exit 1
echo "---------------------------------------------------------"
echo "ALL TESTS PASSED!"
echo "---------------------------------------------------------"

./loguru_test $@
