#!/usr/bin/env bash
# Wrapper script for running MAME-based EDASM tests
# This script automates the process of:
# 1. Creating a test disk with sample .src files
# 2. Running MAME with the original EDASM
# 3. Extracting results for comparison

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
TEST_WORK_DIR="${TEST_WORK_DIR:-$PROJECT_ROOT/tmp/edasm-emulator-test}"
EDASM_DISK="$PROJECT_ROOT/third_party/EdAsm/EDASM_SRC.2mg"
PRODOS_DISK="${PRODOS_DISK:-$PROJECT_ROOT/tmp/prodos.po}"
PRODOS_URL="${PRODOS_URL:-https://releases.prodos8.com/ProDOS_2_4_3.po}"
MAME_SYSTEM="${MAME_SYSTEM:-apple2gs}"
MAME_SECONDS="${MAME_SECONDS:-}"
MAME_TIMEOUT="${MAME_TIMEOUT:-60}"
ROM_DIR="${MAME_ROM_PATH:-$HOME/mame/roms}"
MAME_VIDEO="${MAME_VIDEO:-none}"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

echo "=== EDASM Emulator Test Runner ==="
echo ""

# Check dependencies
check_dependencies() {
    local missing=()

    if ! command -v mame &>/dev/null; then
        missing+=("mame")
    fi

    if ! command -v curl &>/dev/null; then
        missing+=("curl")
    fi

    # Check for required tools (MAME already checked by check_dependencies)
        # Try common Go install locations
        local go_bin
        go_bin="$(go env GOBIN 2>/dev/null || true)"
        local go_path
        go_path="$(go env GOPATH 2>/dev/null || echo "$HOME/go")"

        if [[ -n $go_bin && -f "$go_bin/diskm8" ]]; then
            export PATH="$PATH:$go_bin"
        elif [[ -f "$go_path/bin/diskm8" ]]; then
            export PATH="$PATH:$go_path/bin"
        else
            missing+=("diskm8")
        fi
    fi

    if [[ ${#missing[@]} -gt 0 ]]; then
        echo -e "${RED}Error: Missing dependencies: ${missing[*]}${NC}"
        echo "Run: ./scripts/setup_emulator_deps.sh"
        return 1
    fi

    echo -e "${GREEN}✓ All dependencies available${NC}"
    return 0
}

# Check if EdAsm submodule is initialized
check_submodule() {
    if [[ ! -f $EDASM_DISK ]]; then
        echo -e "${RED}Error: EDASM_SRC.2mg not found${NC}"
        echo "Run: git submodule update --init --recursive"
        return 1
    fi

    echo -e "${GREEN}✓ EdAsm submodule initialized${NC}"
    return 0
}

# Ensure a ProDOS boot disk is available (download if missing)
ensure_prodos_disk() {
    if [[ -f $PRODOS_DISK ]]; then
        echo -e "${GREEN}✓ ProDOS boot disk present${NC}"
        return 0
    fi

    echo "ProDOS disk not found; downloading from $PRODOS_URL ..."
    mkdir -p "$(dirname "$PRODOS_DISK")"
    local curl_opts=("-L" "--fail")
    if [[ "${PRODOS_INSECURE:-0}" == "1" ]]; then
        curl_opts+=("-k")
    fi

    if curl "${curl_opts[@]}" "$PRODOS_URL" -o "$PRODOS_DISK"; then
        echo -e "${GREEN}✓ ProDOS boot disk downloaded to $PRODOS_DISK${NC}"
        return 0
    else
        echo -e "${RED}Error: Failed to download ProDOS disk${NC}"
        return 1
    fi
}

# Check for Apple II ROM files
check_roms() {
    echo ""
    echo "Checking for Apple II ROM files..."

    # Try to verify ROMs with MAME
    if mame -rompath "$ROM_DIR" -verifyroms "$MAME_SYSTEM" &>/dev/null || mame -verifyroms "$MAME_SYSTEM" &>/dev/null; then
        echo -e "${GREEN}✓ $MAME_SYSTEM ROM files available${NC}"
        return 0
    fi

    echo "ROM verification failed; attempting to fetch emularity BIOS ROMs..."
    fetch_roms || return 1

    if mame -rompath "$ROM_DIR" -verifyroms "$MAME_SYSTEM" &>/dev/null || mame -verifyroms "$MAME_SYSTEM" &>/dev/null; then
        echo -e "${GREEN}✓ $MAME_SYSTEM ROM files available after fetch${NC}"
        return 0
    fi

    # Check common ROM paths
    local rom_paths=(
        "$HOME/mame/roms"
        "/usr/local/share/games/mame/roms"
        "/usr/share/games/mame/roms"
    )

    local rom_files=(
        "apple2e.zip"
        "apple2gs.zip"
    )

    for rom_path in "${rom_paths[@]}"; do
        for rom_file in "${rom_files[@]}"; do
            if [[ -f "$rom_path/$rom_file" ]]; then
                echo -e "${YELLOW}⚠ ROM files found at $rom_path/$rom_file but MAME verification failed${NC}"
                echo "  This may be due to incorrect ROM versions or incomplete ROM sets."
                return 1
            fi
        done
    done

    echo -e "${RED}✗ Apple II ROM files not found${NC}"
    echo ""
    echo "MAME requires Apple II ROM/BIOS files to run emulation."
    echo "These files are copyrighted and cannot be distributed with this project."
    echo ""
    echo "To obtain ROM files, see: tests/emulator/README.md"
    echo "Or run: ./scripts/setup_emulator_deps.sh (for detailed instructions)"
    echo ""
    return 1
}

# Download Apple II ROMs from emularity-bios if missing
fetch_roms() {
    mkdir -p "$ROM_DIR"
    mkdir -p "$HOME/.mame"

    local apple2e_zip="$ROM_DIR/apple2e.zip"
    local apple2gs_zip="$ROM_DIR/apple2gs.zip"

    if [[ ! -f "$apple2e_zip" ]]; then
        curl -L https://github.com/internetarchive/emularity-bios/raw/main/apple2e.zip -o "$apple2e_zip"
    fi

    if [[ ! -f "$apple2gs_zip" ]]; then
        curl -L https://github.com/internetarchive/emularity-bios/raw/main/apple2gs.zip -o "$apple2gs_zip"
    fi

    ln -s "$ROM_DIR" "$HOME/.mame/roms" 2>/dev/null || true
}

# Check if MAME can run in the current environment
check_mame_runtime() {
    echo ""
    echo "Checking if MAME can run in this environment..."
    
    # Try a quick MAME test that doesn't require ROMs
    # Use -listxml with a short timeout to test basic functionality
    if timeout 5 mame -listxml apple2gs >/dev/null 2>&1; then
        echo -e "${GREEN}✓ MAME runtime check passed${NC}"
        return 0
    fi
    
    echo -e "${YELLOW}⚠ MAME runtime check failed${NC}"
    echo "  MAME may not be able to run in this environment."
    echo "  This is common in CI environments without display/GPU support."
    echo ""
    return 1
}

# Create test work directory
setup_work_dir() {
    echo ""
    echo "Setting up work directory: $TEST_WORK_DIR"
    mkdir -p "$TEST_WORK_DIR"
    mkdir -p "$TEST_WORK_DIR/results"
    echo -e "${GREEN}✓ Work directory ready${NC}"
}

# Create test disk with sample source files
create_test_disk() {
    echo ""
    echo "Creating test disk..."

    local test_disk="$TEST_WORK_DIR/test_disk.2mg"

    # Create 140KB ProDOS disk
    if [[ -f $test_disk ]]; then
        echo "Removing existing test disk..."
        rm "$test_disk"
    fi

    # Use disk_helper.sh for disk operations
    "$SCRIPT_DIR/disk_helper.sh" create "$test_disk" 140KB TESTDSK

    # Inject test source files
    echo "Injecting test files..."
    local test_files=(
        "$PROJECT_ROOT/tests/test_simple.src"
        "$PROJECT_ROOT/tests/test_hex_add.src"
        "$PROJECT_ROOT/tests/test_symbol_add.src"
    )

    for src_file in "${test_files[@]}"; do
        if [[ -f $src_file ]]; then
            local basename=$(basename "$src_file")
            echo "  - $basename"
            "$SCRIPT_DIR/disk_helper.sh" inject "$test_disk" "$src_file"
        fi
    done

    echo -e "${GREEN}✓ Test disk created: $test_disk${NC}"
}

# Run MAME with the specified Lua script
run_mame_test() {
    local lua_script="$1"
    local script_name=$(basename "$lua_script")
    local test_disk="$TEST_WORK_DIR/test_disk.2mg"
    local flop1_args=("-flop1" "$PRODOS_DISK")
    local flop2_args=()
    local flop3_args=("-flop3" "$EDASM_DISK")
    local flop4_args=()
    local seconds_args=()
    local timeout_cmd=(timeout "$MAME_TIMEOUT")

    if [[ -f $test_disk ]]; then
        flop4_args=("-flop4" "$test_disk")
    fi

    if [[ -n ${MAME_SECONDS:-} ]]; then
        seconds_args=("-seconds_to_run" "$MAME_SECONDS")
    fi

    echo ""
    echo "Running MAME with $script_name..."
    echo "---" | tee -a "$TEST_WORK_DIR/$script_name.log"

    # MAME command with headless options
    # Note: -nothrottle makes it run as fast as possible
    # -video none and -sound none disable graphics/audio
    set +e  # Don't exit on error, we want to check the result
    "${timeout_cmd[@]}" mame "$MAME_SYSTEM" \
        "${flop1_args[@]}" \
        "${flop3_args[@]}" \
        "${flop4_args[@]}" \
        "${seconds_args[@]}" \
        -rompath "$ROM_DIR" \
        -video "$MAME_VIDEO" \
        -sound none \
        -nomouse \
        -nothrottle \
        -autoboot_script "$lua_script" \
        2>&1 | tee "$TEST_WORK_DIR/$script_name.log"
    
    # Capture exit code of the timeout/mame command (first pipeline element)
    local exit_code=${PIPESTATUS[0]}
    echo "EXIT_CODE=$exit_code" >>"$TEST_WORK_DIR/$script_name.log"
    set -e

    echo "---"
    
    # Check if MAME crashed (segfault = 139, killed by signal = 128+signal)
    if [[ $exit_code -eq 139 ]]; then
        echo -e "${RED}✗ MAME crashed with segmentation fault${NC}" | tee -a "$TEST_WORK_DIR/$script_name.log"
        echo ""
        echo "This is a known issue in CI environments without display/GPU access."
        echo "MAME requires hardware graphics support even with -video none."
        echo ""
        echo "Possible solutions:"
        echo "  1. Run tests on a system with display/GPU support"
        echo "  2. Use Xvfb (virtual framebuffer) in CI"
        echo "  3. Use a different emulator (e.g., linapple, gsplus)"
        echo ""
        echo "For more information, see: tests/emulator/README.md"
        return 1
    elif [[ $exit_code -eq 124 ]]; then
        echo -e "${YELLOW}⚠ MAME timed out after ${MAME_TIMEOUT}s${NC}" | tee -a "$TEST_WORK_DIR/$script_name.log"
        echo "Check log for details: $TEST_WORK_DIR/$script_name.log"
        return 1
    elif [[ $exit_code -ne 0 ]]; then
        echo -e "${YELLOW}⚠ MAME exited with code $exit_code${NC}" | tee -a "$TEST_WORK_DIR/$script_name.log"
        echo "Check log for details: $TEST_WORK_DIR/$script_name.log"
        return 1
    fi
    
    echo -e "${GREEN}✓ MAME test complete${NC}" | tee -a "$TEST_WORK_DIR/$script_name.log"
    echo "Log saved to: $TEST_WORK_DIR/$script_name.log"
    return 0
}

# Extract results from test disk
extract_results() {
    echo ""
    echo "Extracting results from test disk..."

    "$SCRIPT_DIR/disk_helper.sh" list "$TEST_WORK_DIR/test_disk.2mg" >"$TEST_WORK_DIR/disk_listing.txt"
    echo "Disk contents:"
    cat "$TEST_WORK_DIR/disk_listing.txt"

    # Try to extract all files
    "$SCRIPT_DIR/disk_helper.sh" extract "$TEST_WORK_DIR/test_disk.2mg" "$TEST_WORK_DIR/results/" || true

    echo ""
    echo "Results extracted to: $TEST_WORK_DIR/results/"
    ls -lh "$TEST_WORK_DIR/results/" || echo "No files extracted"
}

# Main test execution
main() {
    local test_type="${1:-boot}"

    # Pre-flight checks
    check_dependencies || exit 1
    check_submodule || exit 1
    ensure_prodos_disk || exit 1
    check_roms || exit 1
    
    # Check if MAME can run (warn but don't fail)
    local mame_can_run=1
    check_mame_runtime || mame_can_run=0

    # Setup
    setup_work_dir

    case "$test_type" in
    boot)
        echo ""
        echo "Test type: Boot test (demonstrates MAME + ProDOS + EDASM launch)"
        
        if [[ $mame_can_run -eq 0 ]]; then
            echo -e "${YELLOW}⚠ Skipping MAME test due to runtime environment limitations${NC}"
            echo ""
            echo "=== Test Skipped ===" 
            echo ""
            echo "Note: MAME requires display/GPU support to run."
            echo "See tests/emulator/README.md for troubleshooting."
            exit 0
        fi
        
        if run_mame_test "$PROJECT_ROOT/tests/emulator/boot_test.lua"; then
            echo ""
            echo -e "${GREEN}=== Test Complete ===${NC}"
        else
            echo ""
            echo -e "${YELLOW}=== Test Failed ===${NC}"
            echo ""
            echo "The emulator test could not complete successfully."
            echo "This is expected in environments without display/GPU support."
            exit 1
        fi
        ;;

    assemble)
        echo ""
        echo "Test type: Assembly test (full workflow: load, assemble, save)"
        
        if [[ $mame_can_run -eq 0 ]]; then
            echo -e "${YELLOW}⚠ Skipping MAME test due to runtime environment limitations${NC}"
            echo ""
            echo "=== Test Skipped ===" 
            echo ""
            echo "Note: MAME requires display/GPU support to run."
            echo "See tests/emulator/README.md for troubleshooting."
            exit 0
        fi
        
        create_test_disk
        if run_mame_test "$PROJECT_ROOT/tests/emulator/assemble_test.lua"; then
            extract_results
            echo ""
            echo -e "${GREEN}=== Test Complete ===${NC}"
        else
            echo ""
            echo -e "${YELLOW}=== Test Failed ===${NC}"
            echo ""
            echo "The emulator test could not complete successfully."
            echo "This is expected in environments without display/GPU support."
            exit 1
        fi
        ;;

    *)
        echo -e "${RED}Error: Unknown test type: $test_type${NC}"
        echo "Usage: $0 [boot|assemble]"
        echo ""
        echo "Test types:"
        echo "  boot     - Basic boot test (ProDOS + EDASM launch)"
        echo "  assemble - Full assembly workflow test"
        exit 1
        ;;
    esac

    echo ""
    echo "Note: These are prototype tests. Full validation requires display/GPU support."
    echo "See tests/emulator/README.md for implementation status."
}

main "$@"
