#!/bin/bash
set -e # Fail on error

ROOT_DIR=$(cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd)

cd "$ROOT_DIR"
mkdir -p build
cd build

cmake ..

# Use GCC:
# cmake -DCMAKE_C_COMPILER=/usr/bin/gcc -DCMAKE_CXX_COMPILER=/usr/bin/g++ ..

# Use GCC5:
# cmake -DCMAKE_C_COMPILER=/usr/bin/gcc-5 -DCMAKE_CXX_COMPILER=/usr/bin/g++-5 ..

# Use clang-3.7:
# cmake -DCMAKE_C_COMPILER=/usr/bin/clang-3.7 -DCMAKE_CXX_COMPILER=/usr/bin/clang++-3.7 ..

make
