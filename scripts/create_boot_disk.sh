#!/usr/bin/env bash
# Create a minimal ProDOS boot disk with essential system files
# Based on prodos.po but with only core boot files

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

# Configuration
SOURCE_DISK="${1:-/home/kai/mame/roms/apple2/prodos.po}"
TARGET_DISK="${2:-${PROJECT_ROOT}/tmp/minimal_boot.po}"
TEMP_DIR="${PROJECT_ROOT}/tmp/boot_creation"

# Get cadius command
get_cadius() {
    if command -v cadius &>/dev/null; then
        echo "cadius"
    else
        echo "${PROJECT_ROOT}/third_party/cadius/cadius"
    fi
}

# Check if source disk exists
check_source_disk() {
    if [[ ! -f "$SOURCE_DISK" ]]; then
        echo -e "${RED}Error: Source ProDOS disk not found: $SOURCE_DISK${NC}"
        echo "Please provide the path to prodos.po as first argument"
        echo "Example: $0 /path/to/prodos.po /path/to/output.po"
        exit 1
    fi
    echo -e "${GREEN}✓ Source disk found: $SOURCE_DISK${NC}"
}

# Create working directory
setup_workspace() {
    echo -e "${BLUE}Setting up workspace...${NC}"
    mkdir -p "$TEMP_DIR"
    rm -rf "${TEMP_DIR:?}"/*
}

# Extract files from source disk
extract_source_files() {
    echo -e "${BLUE}Extracting files from source ProDOS disk...${NC}"
    
    local cadius_cmd
    cadius_cmd="$(get_cadius)"
    
    # Convert source disk to absolute path
    local abs_source_disk
    if [[ "$SOURCE_DISK" == /* ]]; then
        abs_source_disk="$SOURCE_DISK"
    else
        abs_source_disk="$(realpath "$SOURCE_DISK")"
    fi
    
    cd "$TEMP_DIR"
    "$cadius_cmd" EXTRACTVOLUME "$abs_source_disk" .
    cd "$PROJECT_ROOT"
    
    echo -e "${GREEN}✓ Source files extracted${NC}"
    
    # Show what we extracted
    echo "Extracted files:"
    find "$TEMP_DIR" -type f | head -10
}

# Create new minimal boot disk
create_boot_disk() {
    echo -e "${BLUE}Creating new minimal boot disk...${NC}"
    
    local cadius_cmd
    cadius_cmd="$(get_cadius)"
    
    # Convert target disk to absolute path
    local abs_target_disk
    if [[ "$TARGET_DISK" == /* ]]; then
        abs_target_disk="$TARGET_DISK"
    else
        abs_target_disk="$(realpath -m "$TARGET_DISK")"
    fi
    TARGET_DISK="$abs_target_disk"
    
    # Remove target if it exists
    [[ -f "$TARGET_DISK" ]] && rm "$TARGET_DISK"
    
    # Create new ProDOS volume (140KB, same as original)
    "$cadius_cmd" CREATEVOLUME "$TARGET_DISK" "BOOT" 140KB
    
    echo -e "${GREEN}✓ New boot disk created${NC}"
}

# Copy essential files in specified order
copy_essential_files() {
    echo -e "${BLUE}Copying essential system files...${NC}"
    
    local cadius_cmd
    cadius_cmd="$(get_cadius)"
    
    # Files to copy in order
    local files_to_copy=(
        "PRODOS"
        "BITSY.BOOT" 
        "QUIT.SYSTEM"
        "BASIC.SYSTEM"
    )
    
    # Find and copy each file
    for file in "${files_to_copy[@]}"; do
        echo "  Searching for $file..."
        
        # Find the file (case insensitive search)
        local found_file
        found_file=$(find "$TEMP_DIR" -type f -iname "*${file}*" | head -1)
        
        if [[ -n "$found_file" ]]; then
            echo "    Found: $(basename "$found_file")"
            echo "    Copying to boot disk..."
            
            # Add file to root of boot disk
            if "$cadius_cmd" ADDFILE "$TARGET_DISK" "/BOOT/" "$found_file"; then
                echo -e "    ${GREEN}✓ $file copied successfully${NC}"
            else
                echo -e "    ${YELLOW}⚠ Failed to copy $file, continuing...${NC}"
            fi
        else
            echo -e "    ${YELLOW}⚠ $file not found in source disk${NC}"
        fi
        echo
    done
}

# Verify the new boot disk
verify_boot_disk() {
    echo -e "${BLUE}Verifying new boot disk...${NC}"
    
    local cadius_cmd
    cadius_cmd="$(get_cadius)"
    
    echo "Boot disk contents:"
    "$cadius_cmd" CATALOG "$TARGET_DISK"
    
    echo ""
    echo -e "${GREEN}✓ Minimal boot disk created successfully${NC}"
    echo "Location: $TARGET_DISK"
}

# Show usage
usage() {
    cat <<EOF
Create Minimal ProDOS Boot Disk

Usage: $0 [source_prodos.po] [target_boot.po]

Arguments:
  source_prodos.po  Path to source ProDOS disk (default: /home/kai/mame/roms/apple2/prodos.po)
  target_boot.po    Path for new minimal boot disk (default: tmp/minimal_boot.po)

This script creates a minimal ProDOS boot disk with only essential files:
  - PRODOS (system loader)
  - BITSY.BOOT (boot loader)  
  - QUIT.SYSTEM (quit utility)
  - BASIC.SYSTEM (BASIC interpreter)

Example:
  $0
  $0 /path/to/prodos.po /path/to/myboot.po

EOF
}

# Main execution
main() {
    echo -e "${BLUE}=== ProDOS Minimal Boot Disk Creator ===${NC}"
    echo ""
    
    if [[ "${1:-}" == "--help" ]] || [[ "${1:-}" == "-h" ]]; then
        usage
        exit 0
    fi
    
    check_source_disk
    setup_workspace
    extract_source_files
    create_boot_disk
    copy_essential_files
    verify_boot_disk
    
    echo ""
    echo -e "${GREEN}=== Boot disk creation complete! ===${NC}"
    echo "You can now use this disk to boot Apple II systems in MAME:"
    echo "  mame apple2e -flop1 $TARGET_DISK -window"
}

# Cleanup on exit
cleanup() {
    if [[ -d "$TEMP_DIR" ]]; then
        rm -rf "$TEMP_DIR"
    fi
}
trap cleanup EXIT

# Run main function
main "$@"