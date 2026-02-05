#!/usr/bin/env bash
set -euo pipefail
set -x

trap 'echo "ERROR: Script aborted at line $LINENO (exit code: $?)"' ERR

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

# Copy test command file
for i in test_input.txt test_simple.src; do
    tr '\n' '\r' <"${ROOT}/tests/fixtures/${i}" >"${TESTDIR}/${i}"
done
mv -vi "${TESTDIR}/test_input.txt" "${TESTDIR}/EDASM.AUTOST"
echo "***" >"${TESTDIR}/test_input.txt"

cd "${TESTDIR}"
touch EDASM.SWAP
"${BUILD_DIR}/emulator_runner" --binary EDASM.SYSTEM --input-file test_input.txt "$@" &>emulator_runner.log
echo "Emulator stopped with no errors."
