#!/usr/bin/env bash

# This script installs dependencies needed for compilation and linking.
# Running the script requires root privileges, use `sudo` for instance.

set -e  # Exit on error
# set -x  # Display commands

apt-get install libboost-filesystem-dev libboost-system-dev libboost-thread-dev

