#!/bin/bash
# Build the documentation in the "html" directory.
# Note that the asm-dom examples must be built in examples/asm-dom/build before
# running this script.
set -x -e
sphinx-build docs html
cp examples/asm-dom/build/*.js examples/asm-dom/build/*.wasm html
touch html/.nojekyll
