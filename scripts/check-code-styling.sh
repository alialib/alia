#!/bin/bash
# Check that all C++ source files match out .clang-format styling.
# Note that this should be run from the root directory.
find . -name *.[chi]pp | xargs -n1 clang-format -style=file -output-replacements-xml | grep "<replacement " >/dev/null
if [ $? -ne 1 ]
then
    echo "Some files contain style violations."
    echo "Please run scripts/apply-code-styling.sh."
    exit 1;
fi
