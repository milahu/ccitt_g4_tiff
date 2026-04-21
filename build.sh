#!/bin/sh

cd "$(dirname "$0")"

mkdir -p build
cd build
set -x
cmake ..
# make
cmake --build . --verbose
