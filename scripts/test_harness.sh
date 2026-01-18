#!/usr/bin/env bash
# Automated test harness for C-EDASM
# Tests all sample source files and generates a summary report

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"
TEST_RESULTS_DIR="${TEST_RESULTS_DIR:-/tmp/edasm-test-results}"

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
CYAN='\033[0;36m'
MAGENTA='\033[0;35m'
NC='\033[0m'

# Test results
declare -a PASSED_TESTS=()
declare -a FAILED_TESTS=()
declare -a SKIPPED_TESTS=()

echo -e "${CYAN}╔══════════════════════════════════════════╗${NC}"
echo -e "${CYAN}║  C-EDASM Automated Test Harness         ║${NC}"
echo -e "${CYAN}╔══════════════════════════════════════════╗${NC}"
echo ""

# Setup test results directory
setup_results_dir() {
    echo -e "${BLUE}Setting up test results directory${NC}"
    rm -rf "${TEST_RESULTS_DIR}"
    mkdir -p "${TEST_RESULTS_DIR}"
    echo -e "${GREEN}✓ Results directory: ${TEST_RESULTS_DIR}${NC}"
    echo ""
}

# Find all test source files
find_test_files() {
    local test_files=()

    # Find all test_*.src files in project root
    while IFS= read -r -d '' file; do
        test_files+=("${file}")
    done < <(find "${PROJECT_ROOT}" -maxdepth 1 -name "test_*.src" -print0 | sort -z)

    echo "${test_files[@]}"
}

# Portable file size function
get_file_size() {
    local file="${1}"
    if [[ "$(uname)" == "Darwin" ]]; then
        stat -f%z "${file}" 2>/dev/null
    else
        stat -c%s "${file}" 2>/dev/null
    fi
}

# Test a single source file
test_single_file() {
    local src_file="${1}"
    local basename=$(basename "${src_file}" .src)
    local output_file="${TEST_RESULTS_DIR}/${basename}.bin"
    local log_file="${TEST_RESULTS_DIR}/${basename}.log"

    echo -e "${BLUE}Testing: ${basename}${NC}"

    # Run assembler
    cd "${PROJECT_ROOT}"
    if python3 scripts/assemble_helper.py "${src_file}" "${output_file}" >"${log_file}" 2>&1; then
        echo -e "${GREEN}  ✓ Assembly successful${NC}"

        # Get size
        local size=$(get_file_size "${output_file}")
        echo "  Size: ${size} bytes"

        PASSED_TESTS+=("${basename}")
        return 0
    else
        echo -e "${RED}  ✗ Assembly failed${NC}"
        echo "  See: ${log_file}"

        FAILED_TESTS+=("${basename}")
        return 1
    fi
}

# Run all tests
run_all_tests() {
    local test_files=($(find_test_files))

    if [[ ${#test_files[@]} -eq 0 ]]; then
        echo -e "${YELLOW}No test files found${NC}"
        return
    fi

    echo -e "${MAGENTA}Found ${#test_files[@]} test files${NC}"
    echo ""

    local count=0
    for src_file in "${test_files[@]}"; do
        count=$((count + 1))
        echo -e "${CYAN}[${count}/${#test_files[@]}]${NC}"
        test_single_file "${src_file}"
        echo ""
    done
}

# Generate summary report
generate_summary() {
    local total_tests=$((${#PASSED_TESTS[@]} + ${#FAILED_TESTS[@]} + ${#SKIPPED_TESTS[@]}))
    local pass_rate=0

    if [[ ${total_tests} -gt 0 ]]; then
        pass_rate=$((${#PASSED_TESTS[@]} * 100 / total_tests))
    fi

    echo ""
    echo -e "${CYAN}╔══════════════════════════════════════════╗${NC}"
    echo -e "${CYAN}║           Test Results Summary           ║${NC}"
    echo -e "${CYAN}╚══════════════════════════════════════════╝${NC}"
    echo ""

    echo -e "${GREEN}Passed:  ${#PASSED_TESTS[@]}${NC}"
    echo -e "${RED}Failed:  ${#FAILED_TESTS[@]}${NC}"
    echo -e "${YELLOW}Skipped: ${#SKIPPED_TESTS[@]}${NC}"
    echo -e "${BLUE}Total:   ${total_tests}${NC}"
    echo ""
    echo -e "${MAGENTA}Pass Rate: ${pass_rate}%${NC}"
    echo ""

    if [[ ${#PASSED_TESTS[@]} -gt 0 ]]; then
        echo -e "${GREEN}✓ Passed Tests:${NC}"
        for test in "${PASSED_TESTS[@]}"; do
            echo "  • ${test}"
        done
        echo ""
    fi

    if [[ ${#FAILED_TESTS[@]} -gt 0 ]]; then
        echo -e "${RED}✗ Failed Tests:${NC}"
        for test in "${FAILED_TESTS[@]}"; do
            echo "  • ${test}"
        done
        echo ""
    fi

    if [[ ${#SKIPPED_TESTS[@]} -gt 0 ]]; then
        echo -e "${YELLOW}⊘ Skipped Tests:${NC}"
        for test in "${SKIPPED_TESTS[@]}"; do
            echo "  • ${test}"
        done
        echo ""
    fi
}

# Save detailed report
save_detailed_report() {
    local report_file="${TEST_RESULTS_DIR}/test_report.txt"

    echo "C-EDASM Automated Test Report" >"${report_file}"
    echo "=============================" >>"${report_file}"
    echo "" >>"${report_file}"
    echo "Date: $(date)" >>"${report_file}"
    echo "Project: C-EDASM" >>"${report_file}"
    echo "" >>"${report_file}"
    echo "Summary" >>"${report_file}"
    echo "-------" >>"${report_file}"
    echo "Passed:  ${#PASSED_TESTS[@]}" >>"${report_file}"
    echo "Failed:  ${#FAILED_TESTS[@]}" >>"${report_file}"
    echo "Skipped: ${#SKIPPED_TESTS[@]}" >>"${report_file}"
    echo "Total:   $((${#PASSED_TESTS[@]} + ${#FAILED_TESTS[@]} + ${#SKIPPED_TESTS[@]}))" >>"${report_file}"
    echo "" >>"${report_file}"

    if [[ ${#PASSED_TESTS[@]} -gt 0 ]]; then
        echo "Passed Tests" >>"${report_file}"
        echo "------------" >>"${report_file}"
        for test in "${PASSED_TESTS[@]}"; do
            echo "  • ${test}" >>"${report_file}"
            local bin_file="${TEST_RESULTS_DIR}/${test}.bin"
            if [[ -f ${bin_file} ]]; then
                local size=$(get_file_size "${bin_file}")
                echo "    Size: ${size} bytes" >>"${report_file}"
            fi
        done
        echo "" >>"${report_file}"
    fi

    if [[ ${#FAILED_TESTS[@]} -gt 0 ]]; then
        echo "Failed Tests" >>"${report_file}"
        echo "------------" >>"${report_file}"
        for test in "${FAILED_TESTS[@]}"; do
            echo "  • ${test}" >>"${report_file}"
            local log_file="${TEST_RESULTS_DIR}/${test}.log"
            if [[ -f ${log_file} ]]; then
                echo "    Log: ${log_file}" >>"${report_file}"
            fi
        done
        echo "" >>"${report_file}"
    fi

    echo "" >>"${report_file}"
    echo "Detailed Logs" >>"${report_file}"
    echo "-------------" >>"${report_file}"
    for test in "${PASSED_TESTS[@]}" "${FAILED_TESTS[@]}"; do
        local log_file="${TEST_RESULTS_DIR}/${test}.log"
        if [[ -f ${log_file} ]]; then
            echo "" >>"${report_file}"
            echo "=== ${test} ===" >>"${report_file}"
            cat "${log_file}" >>"${report_file}"
        fi
    done

    echo ""
    echo -e "${GREEN}✓ Detailed report saved: ${report_file}${NC}"
}

# List available binaries
list_binaries() {
    echo ""
    echo -e "${BLUE}Generated Binaries:${NC}"
    echo "Location: ${TEST_RESULTS_DIR}/"
    echo ""

    local bin_files=($(find "${TEST_RESULTS_DIR}" -name "*.bin" -type f))

    if [[ ${#bin_files[@]} -eq 0 ]]; then
        echo "  (none)"
    else
        for bin_file in "${bin_files[@]}"; do
            local size=$(get_file_size "${bin_file}")
            local basename=$(basename "${bin_file}")
            echo "  • ${basename} (${size} bytes)"
        done
    fi
    echo ""
}

# Main test execution
main() {
    local mode="${1:-all}"

    # Check if C-EDASM is built
    if [[ ! -f "${PROJECT_ROOT}/build/test_asm" ]]; then
        echo -e "${RED}Error: C-EDASM not built${NC}"
        echo "Run: ./scripts/build.sh"
        exit 1
    fi

    setup_results_dir

    case "${mode}" in
    all)
        run_all_tests
        ;;

    single)
        if [[ $# -lt 2 ]]; then
            echo -e "${RED}Error: single mode requires filename${NC}"
            echo "Usage: ${0} single <test_file.src>"
            exit 1
        fi
        test_single_file "${2}"
        ;;

    *)
        echo -e "${RED}Error: Unknown mode: ${mode}${NC}"
        echo "Usage: ${0} [all|single <file>]"
        exit 1
        ;;
    esac

    # Generate reports
    generate_summary
    save_detailed_report
    list_binaries

    echo -e "${CYAN}╔══════════════════════════════════════════╗${NC}"
    echo -e "${CYAN}║         Test Harness Complete            ║${NC}"
    echo -e "${CYAN}╚══════════════════════════════════════════╝${NC}"
    echo ""

    # Exit with appropriate code
    if [[ ${#FAILED_TESTS[@]} -gt 0 ]]; then
        exit 1
    else
        exit 0
    fi
}

main "$@"
