#!/usr/bin/env bash
# Wrapper script for running MAME-based EDASM tests
# This script automates the process of:
# 1. Creating a test disk with sample .src files
# 2. Running MAME with the original EDASM
# 3. Extracting results for comparison

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
TEST_WORK_DIR="${TEST_WORK_DIR:-/tmp/edasm-emulator-test}"
EDASM_DISK="$PROJECT_ROOT/third_party/EdAsm/EDASM_SRC.2mg"
MAME_SYSTEM="${MAME_SYSTEM:-apple2gs}"
MAME_SECONDS="${MAME_SECONDS:-30}"

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

    if ! command -v diskm8 &>/dev/null; then
        # Check if diskm8 is in GOPATH
        GOPATH="${GOPATH:-$HOME/go}"
        if [[ ! -f "$GOPATH/bin/diskm8" ]]; then
            missing+=("diskm8")
        else
            export PATH="$PATH:$GOPATH/bin"
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

# Check for Apple II ROM files
check_roms() {
    echo ""
    echo "Checking for Apple II ROM files..."
    
    # Try to verify ROMs with MAME
    if mame -verifyroms "$MAME_SYSTEM" &>/dev/null; then
        echo -e "${GREEN}✓ $MAME_SYSTEM ROM files available${NC}"
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

    diskm8 create "$test_disk" 140KB

    # Inject test source files
    echo "Injecting test files..."
    local test_files=(
        "$PROJECT_ROOT/test_simple.src"
        "$PROJECT_ROOT/test_hex_add.src"
        "$PROJECT_ROOT/test_symbol_add.src"
    )

    for src_file in "${test_files[@]}"; do
        if [[ -f $src_file ]]; then
            local basename=$(basename "$src_file")
            echo "  - $basename"
            diskm8 inject "$test_disk" "$src_file"
        fi
    done

    echo -e "${GREEN}✓ Test disk created: $test_disk${NC}"
}

# Run MAME with the specified Lua script
run_mame_test() {
    local lua_script="$1"
    local script_name=$(basename "$lua_script")
    local test_disk="$TEST_WORK_DIR/test_disk.2mg"
    local flop2_args=()
    local flop3_args=("-flop3" "$EDASM_DISK")
    local flop4_args=()
    local seconds_args=()

    if [[ -f $test_disk ]]; then
        flop4_args=("-flop4" "$test_disk")
    fi

    if [[ -n $MAME_SECONDS ]]; then
        seconds_args=("-seconds_to_run" "$MAME_SECONDS")
    fi

    echo ""
    echo "Running MAME with $script_name..."
    echo "---"

    # MAME command with headless options
    # Note: -nothrottle makes it run as fast as possible
    # -video none and -sound none disable graphics/audio
    mame "$MAME_SYSTEM" \
        "${flop3_args[@]}" \
        "${flop4_args[@]}" \
        "${seconds_args[@]}" \
        -video none \
        -sound none \
        -nothrottle \
        -autoboot_script "$lua_script" \
        2>&1 | tee "$TEST_WORK_DIR/$script_name.log"

    echo "---"
    echo -e "${GREEN}✓ MAME test complete${NC}"
    echo "Log saved to: $TEST_WORK_DIR/$script_name.log"
}

# Extract results from test disk
extract_results() {
    echo ""
    echo "Extracting results from test disk..."

    diskm8 ls "$TEST_WORK_DIR/test_disk.2mg" >"$TEST_WORK_DIR/disk_listing.txt"
    echo "Disk contents:"
    cat "$TEST_WORK_DIR/disk_listing.txt"

    # Try to extract all files
    diskm8 extract "$TEST_WORK_DIR/test_disk.2mg" "$TEST_WORK_DIR/results/" || true

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
    check_roms || exit 1

    # Setup
    setup_work_dir

    case "$test_type" in
    boot)
        echo ""
        echo "Test type: Boot test (demonstrates MAME + ProDOS + EDASM launch)"
        run_mame_test "$PROJECT_ROOT/tests/emulator/boot_test.lua"
        ;;

    assemble)
        echo ""
        echo "Test type: Assembly test (full workflow: load, assemble, save)"
        create_test_disk
        run_mame_test "$PROJECT_ROOT/tests/emulator/assemble_test.lua"
        extract_results
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
    echo -e "${GREEN}=== Test Complete ===${NC}"
    echo ""
    echo "Note: These are prototype tests. Keyboard injection is not yet functional."
    echo "See tests/emulator/README.md for implementation status."
}

main "$@"
