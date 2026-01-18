#!/usr/bin/env bash
# Test original EDASM against C-EDASM for compatibility validation
# Uses the original EDASM system from new_edasm.2mg

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

# Test configuration
WORK_DIR="${PROJECT_ROOT}/tmp/edasm_test"
ORIGINAL_EDASM_DISK="${PROJECT_ROOT}/third_party/artifacts/new_edasm.2mg"
TEST_SOURCE="${PROJECT_ROOT}/tests/test_simple.src"
BOOT_DISK="${WORK_DIR}/boot.2mg"
WORK_DISK="${WORK_DIR}/work.2mg" 
RESULTS_DIR="${WORK_DIR}/results"

# Check dependencies
check_dependencies() {
    echo -e "${BLUE}Checking dependencies...${NC}"
    local missing=()
    
    if ! command -v mame &>/dev/null; then
        missing+=("mame")
    fi
    
    if [[ ! -x "${PROJECT_ROOT}/third_party/cadius/cadius" ]] && ! command -v cadius &>/dev/null; then
        missing+=("cadius")
    fi
    
    if [[ ! -f "$ORIGINAL_EDASM_DISK" ]]; then
        missing+=("new_edasm.2mg disk image")
    fi
    
    if [[ ${#missing[@]} -gt 0 ]]; then
        echo -e "${RED}✗ Missing dependencies: ${missing[*]}${NC}"
        echo "Run: ./scripts/setup_emulator_deps.sh"
        exit 1
    fi
    
    echo -e "${GREEN}✓ All dependencies available${NC}"
}

# Get cadius command
get_cadius() {
    if command -v cadius &>/dev/null; then
        echo "cadius"
    else
        echo "${PROJECT_ROOT}/third_party/cadius/cadius"
    fi
}

# Setup work environment
setup_work_env() {
    echo -e "${BLUE}Setting up test environment...${NC}"
    
    # Create work directory
    mkdir -p "$WORK_DIR" "$RESULTS_DIR"
    
    # Create ProDOS boot disk (140KB)
    local cadius_cmd
    cadius_cmd="$(get_cadius)"
    
    echo "Creating ProDOS boot disk..."
    "$cadius_cmd" CREATEVOLUME "$BOOT_DISK" "BOOT" 140KB
    
    # Create work disk with EDASM and source files
    echo "Creating work disk with EDASM system..."  
    "$cadius_cmd" CREATEVOLUME "$WORK_DISK" "WORK" 140KB
    
    # Extract EDASM files from original disk
    local temp_extract="${WORK_DIR}/edasm_extract"
    mkdir -p "$temp_extract"
    
    echo "Extracting EDASM from original disk..."
    cd "$temp_extract"
    "$cadius_cmd" EXTRACTVOLUME "$ORIGINAL_EDASM_DISK" .
    cd "$PROJECT_ROOT"
    
    # Copy EDASM components to work disk
    echo "Installing EDASM on work disk..."
    find "$temp_extract" -name "EDASM.*" -type f | while read -r file; do
        echo "  Adding $(basename "$file")"
        "$cadius_cmd" ADDFILE "$WORK_DISK" "/WORK/" "$file"
    done
    
    # Copy test source file
    echo "Adding test source file..."
    "$cadius_cmd" ADDFILE "$WORK_DISK" "/WORK/" "$TEST_SOURCE"
    
    # List what we have
    echo -e "${GREEN}Work disk contents:${NC}"
    "$cadius_cmd" CATALOG "$WORK_DISK"
    
    echo -e "${GREEN}✓ Test environment ready${NC}"
}

# Create MAME configuration for automated testing
create_mame_config() {
    echo -e "${BLUE}Creating MAME test configuration...${NC}"
    
    # Create a simple Lua script for MAME automation
    cat > "${WORK_DIR}/edasm_test.lua" << 'EOF'
-- EDASM automated test script for MAME
local function edasm_test()
    print("=== EDASM Automation Test ===")
    
    -- Wait for system to boot
    emu.wait(3)
    
    -- Type commands to load and run EDASM
    -- This is a basic framework - may need adjustment for actual EDASM workflow
    manager:machine():poketext("RUN EDASM.SYSTEM\n")
    
    -- Wait and check if EDASM loads
    emu.wait(5)
    
    print("=== EDASM test framework ready ===")
    print("Manual intervention required at this point")
    print("1. Load source file: TEST_SIMPLE.SRC") 
    print("2. Assemble with EDASM")
    print("3. Save binary and listing")
    print("4. Exit and extract results")
end

-- Register the test function
emu.register_start(edasm_test)
EOF

    echo -e "${GREEN}✓ MAME configuration created${NC}"
}

# Run MAME with EDASM test
run_mame_test() {
    echo -e "${BLUE}Starting MAME with Apple IIe and EDASM...${NC}"
    echo -e "${YELLOW}Note: This will require manual interaction${NC}"
    echo ""
    echo "MAME will boot with:"  
    echo "  Slot 6: Work disk (with EDASM and test source)"
    echo "  Slot 7: Boot disk (ProDOS)"
    echo ""
    echo "Manual steps in MAME:"
    echo "  1. Boot from slot 7 (BOOT disk)"
    echo "  2. Access files on slot 6 (WORK disk)" 
    echo "  3. Run EDASM system"
    echo "  4. Load and assemble test_simple.src"
    echo "  5. Save results and exit"
    echo ""
    echo -e "${YELLOW}Press Enter when ready to start MAME...${NC}"
    read -r
    
    # Run MAME with Apple IIe
    # Using slot 6 for work disk, slot 7 for boot disk
    cd "$WORK_DIR"
    mame apple2e \
        -flop1 "$BOOT_DISK" \
        -flop2 "$WORK_DISK" \
        -autoboot_delay 2 \
        -window \
        || true
        
    echo -e "${GREEN}MAME session completed${NC}"
}

# Extract and analyze results
extract_results() {
    echo -e "${BLUE}Extracting results from work disk...${NC}"
    
    local cadius_cmd
    cadius_cmd="$(get_cadius)"
    
    # Extract everything from work disk to see what was created
    cd "$RESULTS_DIR"
    "$cadius_cmd" EXTRACTVOLUME "$WORK_DISK" .
    cd "$PROJECT_ROOT"
    
    echo -e "${GREEN}Results extracted to: $RESULTS_DIR${NC}"
    echo "Contents:"
    ls -la "$RESULTS_DIR/"
    
    # Look for common EDASM output files
    echo ""
    echo -e "${BLUE}Looking for EDASM output files...${NC}"
    find "$RESULTS_DIR" -type f \( -name "*.BIN" -o -name "*.LST" -o -name "*.OBJ" -o -name "*#*" \) | while read -r file; do
        echo -e "${GREEN}Found output file: ${file##*/}${NC}"
        echo "  Size: $(stat -c%s "$file" 2>/dev/null || stat -f%z "$file" 2>/dev/null) bytes"
        
        # If it's a binary file, show hex dump
        if [[ "$file" =~ \.(BIN|OBJ)$ ]] || [[ "$file" =~ \#[0-9A-F] ]]; then
            echo "  Hex preview:"
            hexdump -C "$file" | head -5 | sed 's/^/    /'
        fi
    done
}

# Compare with C-EDASM output  
compare_with_cedasm() {
    echo -e "${BLUE}Comparing with C-EDASM output...${NC}"
    
    # Assemble same source with C-EDASM
    local cedasm_output="${WORK_DIR}/cedasm_output.bin"
    cd "$PROJECT_ROOT/build"
    ./test_asm "$TEST_SOURCE" > "${WORK_DIR}/cedasm_result.txt" 2>&1 || true
    
    echo "C-EDASM assembly result:"
    cat "${WORK_DIR}/cedasm_result.txt"
    
    # This comparison would need the actual binary output files from both systems
    echo -e "${YELLOW}Manual comparison required between original EDASM and C-EDASM outputs${NC}"
}

# Main execution
main() {
    echo -e "${BLUE}=== Original EDASM Test Framework ===${NC}"
    echo ""
    
    check_dependencies
    setup_work_env
    create_mame_config
    
    echo ""
    echo -e "${GREEN}Test environment is ready!${NC}"
    echo ""
    echo "Next steps:"
    echo "1. Run MAME test: $0 --run-mame"
    echo "2. Extract results: $0 --extract"  
    echo "3. Compare outputs: $0 --compare"
    echo ""
    echo "Or run full test: $0 --full-test"
}

# Command line handling
case "${1:-setup}" in
    --run-mame)
        run_mame_test
        ;;
    --extract)
        extract_results
        ;;
    --compare)
        compare_with_cedasm
        ;;
    --full-test)
        check_dependencies
        setup_work_env
        create_mame_config
        run_mame_test
        extract_results
        compare_with_cedasm
        ;;
    setup|*)
        main
        ;;
esac