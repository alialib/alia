#!/bin/bash
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
