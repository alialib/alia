#!/bin/bash
# Do "normal" CI testing.
set -x -e
export CTEST_OUTPUT_ON_FAILURE=1
python scripts/generate-distributables.py
cp alia.hpp single_header_tests
python scripts/generate-single-header-tests.py
./fips make test linux-make-debug
./fips make test linux-make-release
