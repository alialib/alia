#!/bin/bash
# Set up a fresh Ubuntu Xenial installation so that it can build alia.
# This should be run with sudo.
echo "Setting up system..."
set -x -e
apt-get install -y --upgrade g++-5 gcc-5 lcov cmake git curl clang-4.0 llvm-4.0
