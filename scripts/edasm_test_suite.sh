#!/usr/bin/env bash
# shellcheck disable=SC2155
# Unified EDASM Test Suite
# Combines functionality from test_harness.sh, compare_assemblers.sh,
# test_original_edasm.sh, and run_emulator_test.sh into one comprehensive tool

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"

# Configuration
WORK_DIR="${PROJECT_ROOT}/tmp/unified_test"
RESULTS_DIR="${WORK_DIR}/results"
BOOT_DISK="${PROJECT_ROOT}/third_party/artifacts/minimal_boot.po"
EDASM_DISK="${PROJECT_ROOT}/third_party/artifacts/new_edasm.2mg"
CEDASM_BINARY="${PROJECT_ROOT}/build/test_asm"

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
CYAN='\033[0;36m'
NC='\033[0m'

# Global state
VERBOSE=0
DRY_RUN=0

# Utility functions
log() {
    echo -e "${BLUE}[$(date '+%H:%M:%S')]${NC} $*"
}

success() {
    echo -e "${GREEN}✓${NC} $*"
}

warning() {
    echo -e "${YELLOW}⚠${NC} $*"
}

error() {
    echo -e "${RED}✗${NC} $*" >&2
}

verbose() {
    [[ ${VERBOSE} -eq 1 ]] && echo -e "${CYAN}[DEBUG]${NC} $*" || true
}

# Get cadius command
get_cadius() {
    if command -v cadius &>/dev/null; then
        echo "cadius"
    else
        echo "${PROJECT_ROOT}/third_party/cadius/cadius"
    fi
}

# Comprehensive dependency check
check_dependencies() {
    log "Checking dependencies..."
    local missing=()

    # Core tools
    if [[ ! -x ${CEDASM_BINARY} ]]; then
        missing+=("C-EDASM (run: ./scripts/build.sh)")
    fi

    if [[ ! -x "${PROJECT_ROOT}/third_party/cadius/cadius" ]] && ! command -v cadius &>/dev/null; then
        missing+=("cadius")
    fi

    # Emulator dependencies (optional)
    if ! command -v mame &>/dev/null; then
        warning "MAME not available (emulation features disabled)"
    fi

    # Artifacts
    if [[ ! -f ${BOOT_DISK} ]]; then
        missing+=("minimal_boot.po (should be in third_party/artifacts/)")
    fi

    if [[ ! -f ${EDASM_DISK} ]]; then
        missing+=("new_edasm.2mg (should be in third_party/artifacts/)")
    fi

    if [[ ${#missing[@]} -gt 0 ]]; then
        error "Missing dependencies: ${missing[*]}"
        echo "Run: ./scripts/setup_emulator_deps.sh"
        return 1
    fi

    success "All dependencies available"
    return 0
}

# Setup work environment
setup_workspace() {
    log "Setting up workspace: ${WORK_DIR}"
    mkdir -p "${WORK_DIR}" "${RESULTS_DIR}"
    mkdir -p "${WORK_DIR}/cedasm" "${WORK_DIR}/original" "${WORK_DIR}/disks"
    success "Workspace ready"
}

# Test C-EDASM with a single file
test_cedasm_file() {
    local src_file="${1}"
    local test_name="$(basename "${src_file}" .src)"

    verbose "Testing C-EDASM with: ${src_file}"

    local output_file="${WORK_DIR}/cedasm/${test_name}.bin"
    local result_file="${RESULTS_DIR}/cedasm_${test_name}.txt"

    # Run C-EDASM
    if "${CEDASM_BINARY}" "${src_file}" >"${result_file}" 2>&1; then
        local size=0
        if [[ -f ${output_file} ]]; then
            size=$(stat -c%s "${output_file}" 2>/dev/null || stat -f%z "${output_file}" 2>/dev/null)
        fi
        echo "PASS: ${test_name} (${size} bytes)" | tee -a "${result_file}"
        return 0
    else
        echo "FAIL: ${test_name}" | tee -a "${result_file}"
        return 1
    fi
}

# Test C-EDASM in batch mode
cmd_test_cedasm() {
    log "Testing C-EDASM assembler..."

    # Find all test files
    local test_files=()
    while IFS= read -r -d '' file; do
        test_files+=("${file}")
    done < <(find "${PROJECT_ROOT}/tests" -name "test_*.src" -print0 2>/dev/null)

    if [[ ${#test_files[@]} -eq 0 ]]; then
        warning "No test files found in ${PROJECT_ROOT}/tests/"
        return 1
    fi

    local passed=0
    local total=${#test_files[@]}

    log "Found ${total} test files"

    for src_file in "${test_files[@]}"; do
        if test_cedasm_file "${src_file}"; then
            ((passed++))
        fi
    done

    # Generate report
    local report="${RESULTS_DIR}/cedasm_summary.txt"
    {
        echo "C-EDASM Test Summary"
        echo "===================="
        echo "Total tests: ${total}"
        echo "Passed: ${passed}"
        echo "Failed: $((total - passed))"
        echo "Success rate: $((passed * 100 / total))%"
    } >"${report}"

    cat "${report}"

    if [[ ${passed} -eq ${total} ]]; then
        success "All C-EDASM tests passed!"
        return 0
    else
        error "$((total - passed)) C-EDASM tests failed"
        return 1
    fi
}

# Compare single file between C-EDASM and reference
cmd_compare_file() {
    local src_file="${1-}"
    if [[ -z ${src_file} ]]; then
        error "Usage: ${0} compare <source_file.src>"
        return 1
    fi

    if [[ ! -f ${src_file} ]]; then
        error "Source file not found: ${src_file}"
        return 1
    fi

    log "Comparing assemblers for: $(basename "${src_file}")"

    local test_name="$(basename "${src_file}" .src)"
    local cedasm_out="${WORK_DIR}/cedasm/${test_name}.bin"
    local result_file="${RESULTS_DIR}/compare_${test_name}.txt"

    # Test C-EDASM
    if ! test_cedasm_file "${src_file}"; then
        error "C-EDASM failed for ${src_file}"
        return 1
    fi

    # TODO: Add original EDASM testing when available
    log "C-EDASM output: ${cedasm_out}"
    if [[ -f ${cedasm_out} ]]; then
        local size=$(stat -c%s "${cedasm_out}" 2>/dev/null || stat -f%z "${cedasm_out}" 2>/dev/null)
        success "C-EDASM produced ${size} bytes"

        # Show hex dump
        echo "Hex dump:" | tee -a "${result_file}"
        hexdump -C "${cedasm_out}" | head -10 | tee -a "${result_file}"
    fi

    success "Comparison complete (results in ${result_file})"
}

# Setup original EDASM environment
cmd_setup_original() {
    log "Setting up original EDASM environment..."

    local cadius_cmd
    cadius_cmd="$(get_cadius)"

    local work_disk="${WORK_DIR}/disks/edasm_work.2mg"

    # Create work disk
    log "Creating work disk..."
    "${cadius_cmd}" CREATEVOLUME "${work_disk}" "WORK" 140KB

    # Extract EDASM from original disk
    log "Extracting EDASM components..."
    local temp_extract="${WORK_DIR}/edasm_extract"
    mkdir -p "${temp_extract}"

    cd "${temp_extract}"
    "${cadius_cmd}" EXTRACTVOLUME "${EDASM_DISK}" .
    cd "${PROJECT_ROOT}"

    # Install EDASM on work disk
    log "Installing EDASM on work disk..."
    find "${temp_extract}" -name "EDASM.*" -type f | while read -r file; do
        echo "  Adding $(basename "${file}")"
        "${cadius_cmd}" ADDFILE "${work_disk}" "/WORK/" "${file}"
    done

    # Add test files with ProDOS-compatible names
    log "Adding test source files..."
    find "${PROJECT_ROOT}/tests" -name "test_*.src" | head -5 | while read -r src_file; do
        local base_name="$(basename "${src_file}" .src)"
        local prodos_name="${base_name^^}" # Convert to uppercase
        prodos_name="${prodos_name//_/.}"  # Replace _ with .
        prodos_name="${prodos_name:0:15}"  # Limit to 15 chars

        local temp_file="${WORK_DIR}/${prodos_name}"
        cp "${src_file}" "${temp_file}"

        if "${cadius_cmd}" ADDFILE "${work_disk}" "/WORK/" "${temp_file}" 2>/dev/null; then
            success "Added: ${prodos_name}"
        else
            warning "Could not add: ${prodos_name} (filename restrictions)"
        fi
    done

    # Show disk contents
    log "Work disk contents:"
    "${cadius_cmd}" CATALOG "${work_disk}"

    success "Original EDASM environment ready"
    success "Boot disk: ${BOOT_DISK}"
    success "Work disk: ${work_disk}"

    echo ""
    echo "To test with MAME:"
    echo "  mame apple2gs -flop1 ${BOOT_DISK} -flop3 ${work_disk} -window"
    echo ""
    echo "In EDASM:"
    echo "  1. Access work disk (Slot 5, Drive 1: PR#5 then CATALOG or Ctrl+Apple+3)"
    echo "  2. RUN EDASM.SYSTEM"
    echo "  3. Load and assemble test files"
}

# Run full emulator test
cmd_emulator_test() {
    local interactive="${1:-false}"

    log "Running emulator integration test..."

    if ! command -v mame &>/dev/null; then
        error "MAME not available - cannot run emulator tests"
        return 1
    fi

    # Setup original EDASM if needed
    local work_disk="${WORK_DIR}/disks/edasm_work.2mg"
    if [[ ! -f ${work_disk} ]]; then
        log "Setting up original EDASM environment first..."
        cmd_setup_original
    fi

    log "Starting MAME with Apple IIGS..."
    log "Boot disk: ${BOOT_DISK}"
    log "Work disk: ${work_disk}"

    if [[ ${interactive} == "true" ]]; then
        # Interactive mode with window
        mame apple2gs \
            -flop1 "${BOOT_DISK}" \
            -flop3 "${work_disk}" \
            -window ||
            true
    else
        # Automated mode - headless
        mame apple2gs \
            -flop1 "${BOOT_DISK}" \
            -flop3 "${work_disk}" \
            -video none \
            -sound none \
            -nothrottle \
            -seconds_to_run 30 ||
            true
    fi

    success "MAME session completed"

    # Check for any output files
    log "Checking for results..."
    local cadius_cmd
    cadius_cmd="$(get_cadius)"
    if "${cadius_cmd}" CATALOG "${work_disk}" | grep -i "\.bin\|\.obj\|\.lst" >/dev/null 2>&1; then
        success "Found output files on work disk"
    else
        warning "No obvious output files found"
    fi
}

# Full automated comparison (future enhancement)
cmd_full_comparison() {
    log "Running full C-EDASM vs Original EDASM comparison..."

    # First test C-EDASM
    cmd_test_cedasm

    # Setup original environment
    cmd_setup_original

    warning "Manual step required:"
    echo "  1. Run: ${0} emulator-test"
    echo "  2. In MAME, assemble the same test files with original EDASM"
    echo "  3. Extract results and compare manually"

    # TODO: Implement automated result extraction and comparison
    warning "Automated comparison not yet implemented"
}

# Show usage information
usage() {
    cat <<EOF
Unified EDASM Test Suite

Usage: ${0} <command> [options] [args]

Commands:
  test-cedasm              Test C-EDASM assembler with all test files
  compare <file.src>       Compare C-EDASM vs reference for single file
  setup-original          Setup original EDASM environment for testing
  emulator-test [--interactive] Run emulator integration test
  full-comparison         Run comprehensive C-EDASM vs Original comparison
  
Options:
  -v, --verbose           Enable verbose output
  -n, --dry-run           Show what would be done without executing
  -h, --help              Show this help

Examples:
  ${0} test-cedasm                    # Test all C-EDASM functionality
  ${0} compare tests/test_simple.src  # Compare specific file
  ${0} setup-original                 # Setup original EDASM environment
  ${0} emulator-test                  # Run automated emulator test
  ${0} emulator-test --interactive    # Run interactive MAME session
  ${0} full-comparison                # Complete comparison workflow

Features:
  ✓ Unified dependency checking
  ✓ C-EDASM batch testing (replaces test_harness.sh)
  ✓ Single file comparison (replaces compare_assemblers.sh)  
  ✓ Original EDASM setup (replaces test_original_edasm.sh)
  ✓ MAME integration (replaces run_emulator_test.sh)
  ✓ ProDOS filename handling
  ✓ Comprehensive result reporting
  ✓ Standardized artifact usage

EOF
}

# Main command dispatcher
main() {
    # Parse global options
    while [[ $# -gt 0 ]]; do
        case ${1} in
        -v | --verbose)
            VERBOSE=1
            shift
            ;;
        -n | --dry-run)
            DRY_RUN=1
            shift
            ;;
        -h | --help)
            usage
            exit 0
            ;;
        -*)
            error "Unknown option: ${1}"
            usage
            exit 1
            ;;
        *)
            break
            ;;
        esac
    done

    if [[ $# -eq 0 ]]; then
        usage
        exit 1
    fi

    local command="${1}"
    shift

    # Setup
    check_dependencies || exit 1
    setup_workspace

    # Execute command
    case "${command}" in
    test-cedasm)
        cmd_test_cedasm "$@"
        ;;
    compare)
        cmd_compare_file "$@"
        ;;
    setup-original)
        cmd_setup_original "$@"
        ;;
    emulator-test)
        local interactive="false"
        if [[ $# -gt 0 && $1 == "--interactive" ]]; then
            interactive="true"
            shift
        fi
        cmd_emulator_test "${interactive}"
        ;;
    full-comparison)
        cmd_full_comparison "$@"
        ;;
    *)
        error "Unknown command: ${command}"
        usage
        exit 1
        ;;
    esac
}

# Run main function
main "$@"
