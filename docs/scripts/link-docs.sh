#!/bin/bash
# Set up the documentation (mostly via symbolic links) in the "www" directory.
# Note that the demos must be built in build before running this script.
set -e
mkdir -p www
cd www
rm -rf *
ln -s ../docs/* .
unzip local-favicon.zip && rm *.zip
ln -s ../build/*.js ../build/*.wasm .
ln -s ../src/demos/* .
../scripts/build-dot-files.sh
