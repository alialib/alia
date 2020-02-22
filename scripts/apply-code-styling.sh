#!/bin/bash
# Apply our .clang-format styling to all C++ source files.
# Note that this should be run from the root directory.
find src unit_tests examples -path examples/asm-dom/build -prune -o \
    -name *.[chi]pp | xargs -n1 clang-format -i -style=file
