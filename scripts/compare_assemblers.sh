#!/usr/bin/env bash
# Comparison tool for C-EDASM vs original EDASM output
# Runs both assemblers and compares their outputs

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"
WORK_DIR="${WORK_DIR:-${PROJECT_ROOT}/tmp/edasm-comparison}"

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
CYAN='\033[0;36m'
NC='\033[0m'

echo -e "${CYAN}=== EDASM Comparison Tool ===${NC}"
echo ""

# Check if C-EDASM is built
check_cedasm() {
    if [[ ! -f "${PROJECT_ROOT}/build/test_asm" ]]; then
        echo -e "${YELLOW}C-EDASM not built. Building...${NC}"
        cd "${PROJECT_ROOT}"
        ./scripts/configure.sh
        ./scripts/build.sh
        cd - >/dev/null
    fi
    echo -e "${GREEN}✓ C-EDASM available${NC}"
}

# Check emulator dependencies
check_emulator() {
    local missing=()

    if ! command -v mame &>/dev/null; then
        missing+=("mame")
    fi

    if ! command -v cadius &>/dev/null && [[ ! -x "${PROJECT_ROOT}/tmp/cadius/cadius" ]]; then
        missing+=("cadius")
    fi

    if [[ ${#missing[@]} -gt 0 ]]; then
        echo -e "${RED}✗ Missing dependencies: ${missing[*]}${NC}"
        echo "Run: ./scripts/setup_emulator_deps.sh"
        return 1
    fi

    echo -e "${GREEN}✓ Emulator tools available${NC}"
    return 0
}

# Setup work directory
setup_work_dir() {
    echo ""
    echo -e "${BLUE}Setting up work directory: ${WORK_DIR}${NC}"
    mkdir -p "${WORK_DIR}/cedasm"
    mkdir -p "${WORK_DIR}/original"
    mkdir -p "${WORK_DIR}/disks"
    echo -e "${GREEN}✓ Work directory ready${NC}"
}

# Assemble with C-EDASM
assemble_cedasm() {
    local src_file="${1}"
    local basename=$(basename "${src_file}" .src)
    local output="${WORK_DIR}/cedasm/${basename}.bin"
    local listing="${WORK_DIR}/cedasm/${basename}.lst"

    echo ""
    echo -e "${BLUE}Assembling with C-EDASM: $(basename "${src_file}")${NC}"

    # Run C-EDASM assembler via Python helper
    cd "${PROJECT_ROOT}"
    if python3 scripts/assemble_helper.py "${src_file}" "${output}" >"${listing}" 2>&1; then
        echo -e "${GREEN}✓ C-EDASM assembly successful${NC}"

        # Show file info
        if [[ -f ${output} ]]; then
            local size=$(stat -f%z "${output}" 2>/dev/null || stat -c%s "${output}" 2>/dev/null)
            echo "  Output: ${output} (${size} bytes)"
        fi

        return 0
    else
        echo -e "${RED}✗ C-EDASM assembly failed${NC}"
        echo "See: ${listing}"
        cat "${listing}"
        return 1
    fi
}

# Assemble with original EDASM (via emulator)
assemble_original() {
    local src_file="${1}"
    local basename=$(basename "${src_file}" .src)

    echo ""
    echo -e "${BLUE}Assembling with original EDASM: $(basename "${src_file}")${NC}"
    echo -e "${YELLOW}⚠ Note: Original EDASM assembly via emulator not yet fully automated${NC}"
    echo ""

    # For now, this is a placeholder
    # Full implementation would:
    # 1. Create disk with source file
    # 2. Run MAME with assembly automation
    # 3. Extract output binary

    echo "Would perform:"
    echo "  1. Create test disk: ${WORK_DIR}/disks/${basename}.2mg"
    echo "  2. Inject source: ${src_file}"
    echo "  3. Run MAME with assemble_test.lua"
    echo "  4. Extract output to: ${WORK_DIR}/original/${basename}.bin"
    echo ""
    echo -e "${YELLOW}Skipping original EDASM assembly (manual step required)${NC}"

    return 1 # Not implemented yet
}

# Compare outputs
compare_outputs() {
    local basename="${1}"
    local cedasm_output="${WORK_DIR}/cedasm/${basename}.bin"
    local original_output="${WORK_DIR}/original/${basename}.bin"

    echo ""
    echo -e "${CYAN}=== Comparison Results ===${NC}"
    echo ""

    # Check if both files exist
    local cedasm_exists=false
    local original_exists=false

    if [[ -f ${cedasm_output} ]]; then
        cedasm_exists=true
        local cedasm_size=$(stat -f%z "${cedasm_output}" 2>/dev/null || stat -c%s "${cedasm_output}" 2>/dev/null)
        echo -e "${GREEN}✓${NC} C-EDASM output: ${cedasm_size} bytes"
    else
        echo -e "${RED}✗${NC} C-EDASM output: Not found"
    fi

    if [[ -f ${original_output} ]]; then
        original_exists=true
        local original_size=$(stat -f%z "${original_output}" 2>/dev/null || stat -c%s "${original_output}" 2>/dev/null)
        echo -e "${GREEN}✓${NC} Original EDASM output: ${original_size} bytes"
    else
        echo -e "${YELLOW}⚠${NC} Original EDASM output: Not found (manual assembly required)"
    fi

    echo ""

    if [[ ${cedasm_exists} == true && ${original_exists} == true ]]; then
        echo "Performing byte-by-byte comparison..."

        if cmp -s "${cedasm_output}" "${original_output}"; then
            echo -e "${GREEN}✓✓✓ IDENTICAL! Outputs match perfectly! ✓✓✓${NC}"
            return 0
        else
            echo -e "${RED}✗ Outputs differ${NC}"
            echo ""
            echo "Differences (first 20):"
            cmp -l "${cedasm_output}" "${original_output}" | head -20
            echo ""

            # Hex dump comparison
            echo "Hex dump (first 64 bytes):"
            echo ""
            echo "C-EDASM:"
            hexdump -C "${cedasm_output}" | head -5
            echo ""
            echo "Original EDASM:"
            hexdump -C "${original_output}" | head -5

            return 1
        fi
    elif [[ ${cedasm_exists} == true ]]; then
        echo -e "${BLUE}C-EDASM assembly successful. Place original output at:${NC}"
        echo "  ${original_output}"
        echo ""
        echo "Then re-run comparison."
        return 2
    else
        echo -e "${RED}No outputs to compare${NC}"
        return 3
    fi
}

# Generate comparison report
generate_report() {
    local src_file="${1}"
    local basename=$(basename "${src_file}" .src)
    local report_file="${WORK_DIR}/report_${basename}.txt"

    echo ""
    echo -e "${BLUE}Generating comparison report: ${report_file}${NC}"

    {
        echo "EDASM Comparison Report"
        echo "======================"
        echo ""
        echo "Source file: ${src_file}"
        echo "Date: $(date)"
        echo ""
        echo "C-EDASM Output"
        echo "--------------"
        if [[ -f "${WORK_DIR}/cedasm/${basename}.bin" ]]; then
            echo "Status: Success"
            echo "Size: $(stat -f%z "${WORK_DIR}/cedasm/${basename}.bin" 2>/dev/null || stat -c%s "${WORK_DIR}/cedasm/${basename}.bin") bytes"
            echo "Location: ${WORK_DIR}/cedasm/${basename}.bin"
        else
            echo "Status: Failed"
        fi
        echo ""
        echo "Original EDASM Output"
        echo "--------------------"
        if [[ -f "${WORK_DIR}/original/${basename}.bin" ]]; then
            echo "Status: Success"
            echo "Size: $(stat -f%z "${WORK_DIR}/original/${basename}.bin" 2>/dev/null || stat -c%s "${WORK_DIR}/original/${basename}.bin") bytes"
            echo "Location: ${WORK_DIR}/original/${basename}.bin"
        else
            echo "Status: Not available"
        fi
        echo ""
        echo "Comparison"
        echo "----------"
        if [[ -f "${WORK_DIR}/cedasm/${basename}.bin" && -f "${WORK_DIR}/original/${basename}.bin" ]]; then
            if cmp -s "${WORK_DIR}/cedasm/${basename}.bin" "${WORK_DIR}/original/${basename}.bin"; then
                echo "Result: IDENTICAL ✓"
            else
                echo "Result: DIFFER ✗"
            fi
        else
            echo "Result: Cannot compare (missing output)"
        fi
    } >"${report_file}"

    cat "${report_file}"
    echo ""
    echo -e "${GREEN}✓ Report saved: ${report_file}${NC}"
}

# Main comparison workflow
main() {
    local src_file="${1-}"

    if [[ -z ${src_file} ]]; then
        echo -e "${RED}Error: No source file specified${NC}"
        echo ""
        echo "Usage: ${0} <source.src>"
        echo ""
        echo "Example:"
        echo "  ${0} tests/test_simple.src"
        echo ""
        exit 1
    fi

    if [[ ! -f ${src_file} ]]; then
        echo -e "${RED}Error: File not found: ${src_file}${NC}"
        exit 1
    fi

    # Pre-flight checks
    check_cedasm
    check_emulator || echo -e "${YELLOW}⚠ Emulator not available (C-EDASM only mode)${NC}"

    # Setup
    setup_work_dir

    # Assemble with both
    local basename=$(basename "${src_file}" .src)

    assemble_cedasm "${src_file}"
    local cedasm_status=$?

    # Original EDASM assembly would go here
    # assemble_original "${src_file}"
    # local original_status=$?

    # Compare
    if [[ ${cedasm_status} -eq 0 ]]; then
        compare_outputs "${basename}"
        generate_report "${src_file}"
    fi

    echo ""
    echo -e "${CYAN}=== Comparison Complete ===${NC}"
    echo ""
    echo "Work directory: ${WORK_DIR}"
    echo "  C-EDASM outputs: ${WORK_DIR}/cedasm/"
    echo "  Original outputs: ${WORK_DIR}/original/ (manual)"
    echo ""
}

main "$@"
