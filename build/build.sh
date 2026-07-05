#!/usr/bin/env bash
set -euo pipefail

# Build script for the repository layout:
#   starlink-htsim/
#     build/build.sh
#     src/Makefile
#     src/starlink/*.cpp
#
# It first rebuilds the common htsim library and parse_output in ../src,
# then rebuilds the Starlink experiment binary in ../build.

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"
SRC_DIR="${PROJECT_ROOT}/src"
BUILD_DIR="${PROJECT_ROOT}/build"

cd "${SRC_DIR}"
make clean
make

cd "${BUILD_DIR}"
make clean
make
