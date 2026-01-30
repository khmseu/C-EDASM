#!/usr/bin/env bash
set -euo pipefail
set -x

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_DIR="${ROOT}/build"

BINARY_PATH="third_party/artifacts/edasm/EDASM/edasm"

# Ensure the build directory is configured before building.
if [[ ! -f "${BUILD_DIR}/CMakeCache.txt" ]]; then
    "${ROOT}/scripts/configure.sh"
fi

cmake --build "${BUILD_DIR}" --target emulator_runner

TESTDIR="${ROOT}/EDASM.TEST"
rm -rf "${TESTDIR}"
mkdir -vp "${TESTDIR}"
for i in "${BINARY_PATH}/"*; do
    fname="${i##*/}"
    fname_nohash="${fname%%#*}"
    cp -avi "${i}" "${TESTDIR}/${fname_nohash}"
done
cd "${TESTDIR}"
touch EDASM.SWAP
"${BUILD_DIR}/emulator_runner" --binary EDASM.SYSTEM "$@" &>emulator_runner.log
