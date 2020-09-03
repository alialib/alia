#!/bin/bash
# Install a recent CMake on older Ubuntu systems.

set -x -e

sudo apt-get remove cmake

curl -sSL https://cmake.org/files/v3.17/cmake-3.17.2-Linux-x86_64.sh -o install-cmake.sh
chmod +x install-cmake.sh
./install-cmake.sh --prefix=/usr/local --skip-license
