#!/bin/bash
# Build the documentation in the "www" directory.
# Note that the asm-dom examples must be built in examples/asm-dom/build before
# running this script.
set -e
mkdir -p www
cd www
rm -rf *
ln -s ../docs/* .
unzip favicon_io.zip && rm favicon_io.zip
ln -s ../examples/asm-dom/build/*.js ../examples/asm-dom/build/*.wasm .
ln -s ../examples/asm-dom/snippets/* .
