#!/bin/bash
# Install a recent CMake on older Ubuntu systems.

set -x -e

sudo apt-add-repository 'deb https://apt.kitware.com/ubuntu/ bionic-rc main'
sudo apt-get update
sudo apt-get install --upgrade cmake
