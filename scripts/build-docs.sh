#!/bin/bash
# Build the documentation in the "www" directory.
# Note that the demos must be built in build before running this script.
set -e
mkdir -p www
cd www
rm -rf *
cp ../docs/* .
unzip favicon.zip && rm *.zip
cp ../build/*.js ../build/*.wasm .
wget https://github.com/alialib/alia/releases/latest/download/alia.hpp
../scripts/build-dot-files.sh
touch .nojekyll
cd ..
python3 scripts/process-docs.py
