#!/bin/bash
# Set up the documentation (mostly via symbolic links) in the "www" directory.
# Note that the demos must be built in `<project-root>/build/emscripten` before
# running this script.
set -e
mkdir -p www
cd www
rm -rf *
ln -s ../docs/* .
unzip local-favicon.zip && rm *.zip
ln -s ../../build/emscripten/asm-dom.js .
ln -s ../../build/emscripten/docs/demos.js .
ln -s ../../build/emscripten/docs/demos.wasm .
ln -s ../src/demos/* .
../scripts/build-dot-files.sh
