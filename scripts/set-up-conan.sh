#!/bin/bash
# Install additional Conan remotes that are needed to build alia.
echo "Setting up Conan..."
set -x -e
conan remote add bincrafters https://api.bintray.com/conan/bincrafters/public-conan
