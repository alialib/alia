#!/bin/bash
# Set up Travis for a "normal" CI build.
# This should be run with sudo.
set -x -e
scripts/set-up-system.sh
pip install -r requirements.txt
sudo -u $SUDO_USER bash scripts/set-up-conan.sh