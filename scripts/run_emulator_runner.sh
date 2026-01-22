#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_DIR="${ROOT}/build"

if [[ $# -gt 0 ]]; then
  BINARY_PATH="$1"
  shift
else
  BINARY_PATH="third_party/artifacts/edasm/EDASM/edasm/EDASM.SYSTEM#FF0000"
fi

# Ensure the build directory is configured before building.
if [[ ! -f "${BUILD_DIR}/CMakeCache.txt" ]]; then
  "${ROOT}/scripts/configure.sh"
fi

cmake --build "${BUILD_DIR}" --target emulator_runner

"${BUILD_DIR}/emulator_runner" --binary "${BINARY_PATH}" "$@" |& tee tests/emulator_runner.log
