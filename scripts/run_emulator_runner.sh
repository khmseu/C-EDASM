#!/usr/bin/env bash
set -euo pipefail
set -x

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_DIR="${ROOT}/build"

BINARY_PATH="third_party/artifacts/edasm/EDASM/edasm/EDASM.SYSTEM#FF0000"

# Ensure the build directory is configured before building.
if [[ ! -f "${BUILD_DIR}/CMakeCache.txt" ]]; then
  "${ROOT}/scripts/configure.sh"
fi

cmake --build "${BUILD_DIR}" --target emulator_runner

"${BUILD_DIR}/emulator_runner" --binary "${BINARY_PATH}" "$@" |& tee tests/emulator_runner.log
