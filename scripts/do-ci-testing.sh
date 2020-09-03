#!/bin/bash
# Do "normal" CI testing.

set -x -e

export CTEST_OUTPUT_ON_FAILURE=1

python scripts/generate-distributables.py
cp alia.hpp single_header_tests

python scripts/generate-single-header-tests.py

mkdir debug-build
cd debug-build
cmake -DCMAKE_BUILD_TYPE=Debug -G"Unix Makefiles" ..
cmake --build . --target ctest
cd ..

mkdir release-build
cd release-build
cmake -DCMAKE_BUILD_TYPE=Release -G"Unix Makefiles" ..
cmake --build . --target ctest
cd ..
