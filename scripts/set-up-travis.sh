#!/bin/bash
# Set up Travis for a "normal" CI build.
set -x -e

curl -sSL https://cmake.org/files/v3.17/cmake-3.17.2-Linux-x86_64.sh -o install-cmake.sh
chmod +x install-cmake.sh
./install-cmake.sh --prefix=/usr/local --skip-license

pip install -r requirements.txt

scripts/set-up-conan.sh
