#!/bin/bash
# Set up a fresh Ubuntu installation so that it can build alia.
# This should be run with sudo.
# (This is currently tuned to support Ubuntu Trusty.)
echo "Setting up system..."
set -x -e
apt-get update -qy
apt-get install -y software-properties-common
add-apt-repository -y ppa:ubuntu-toolchain-r/test
add-apt-repository -y ppa:deadsnakes/ppa
apt-get update -qy
apt-get install -y --upgrade python3.5 python3.5-dev g++-5 gcc-5 lcov cmake git curl
curl --silent --show-error --retry 5 https://bootstrap.pypa.io/get-pip.py | python3.5
python3.5 -m pip install virtualenv
curl -sL https://deb.nodesource.com/setup_4.x | bash -
apt-get install -y nodejs
npm install -g clang-format
