#!/bin/bash
# Install additional Conan remotes that are needed to build alia.
echo "Setting up Conan..."
set -x -e
conan remote add conan-community https://api.bintray.com/conan/conan/conan-community
conan remote add conan-transit https://api.bintray.com/conan/conan/conan-transit
