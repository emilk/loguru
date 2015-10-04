#!/bin/bash
set -e # Fail on error

ROOT_DIR=$(cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd)

cd "$ROOT_DIR"
mkdir -p build
cd build
cmake ..
make

./glog_bench $@ 2>/dev/null
