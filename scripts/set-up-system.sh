#!/bin/bash
# Set up a fresh Ubuntu installation so that it can build alia.
# This should be run with sudo.
echo "Setting up system..."
set -x -e
apt-get update -qy
apt-get install -y software-properties-common
add-apt-repository -y ppa:ubuntu-toolchain-r/test
apt-get update -qy
apt-get install -y --upgrade --allow-unauthenticated g++-5 gcc-5 lcov cmake git curl clang-4.0
pip install virtualenv
