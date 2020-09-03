#!/bin/bash
# Install a recent CMake on older Ubuntu systems.

set -x -e

rm -rf /usr/local/cmake-*

curl -sSL https://cmake.org/files/v3.18/cmake-3.18.2-Linux-x86_64.sh -o install-cmake.sh
chmod +x install-cmake.sh
./install-cmake.sh --prefix=/usr/local --skip-license
