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

# Keyboard input
tr '\n' '\r' >"${TESTDIR}/test_input.txt" <<'EOF'
***
EOF

# Copy command file
tr '\n' '\r' >"${TESTDIR}/EDASM.AUTOST" <<'EOF'
ASM INPUT.SRC
CAT
END
EOF

# Test assembler input
tr '\n' '\r' <"${ROOT}/tests/fixtures/test_lst.src" >"${TESTDIR}/INPUT.SRC"

cd "${TESTDIR}"
# touch EDASM.SWAP
"${BUILD_DIR}/emulator_runner" --binary EDASM.SYSTEM --input-file test_input.txt "$@" &>emulator_runner.log
echo "Emulator stopped with no errors."
