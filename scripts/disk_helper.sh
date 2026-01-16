#!/usr/bin/env bash
# Disk image management helper for EDASM testing
# Wraps diskm8 with convenient operations for test automation

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

# Check if diskm8 is available
check_diskm8() {
    if ! command -v diskm8 &> /dev/null; then
        GOPATH="${GOPATH:-$HOME/go}"
        if [[ -f "$GOPATH/bin/diskm8" ]]; then
            export PATH="$PATH:$GOPATH/bin"
        else
            echo -e "${RED}Error: diskm8 not found${NC}"
            echo "Run: ./scripts/setup_emulator_deps.sh"
            exit 1
        fi
    fi
}

# Create a new ProDOS disk image
create_disk() {
    local disk_path="$1"
    local size="${2:-140KB}"
    
    echo -e "${BLUE}Creating disk image: $disk_path (${size})${NC}"
    
    if [[ -f "$disk_path" ]]; then
        echo -e "${YELLOW}Warning: Disk already exists, removing...${NC}"
        rm "$disk_path"
    fi
    
    diskm8 create "$disk_path" "$size"
    echo -e "${GREEN}✓ Disk created${NC}"
}

# Inject a file into disk image
inject_file() {
    local disk_path="$1"
    local file_path="$2"
    local prodos_name="${3:-}"
    
    if [[ ! -f "$disk_path" ]]; then
        echo -e "${RED}Error: Disk not found: $disk_path${NC}"
        exit 1
    fi
    
    if [[ ! -f "$file_path" ]]; then
        echo -e "${RED}Error: File not found: $file_path${NC}"
        exit 1
    fi
    
    local filename=$(basename "$file_path")
    echo "  Injecting: $filename"
    
    if [[ -n "$prodos_name" ]]; then
        diskm8 inject "$disk_path" "$file_path" --name "$prodos_name"
    else
        diskm8 inject "$disk_path" "$file_path"
    fi
}

# Inject multiple files
inject_files() {
    local disk_path="$1"
    shift
    local files=("$@")
    
    echo -e "${BLUE}Injecting ${#files[@]} files into: $disk_path${NC}"
    
    for file in "${files[@]}"; do
        inject_file "$disk_path" "$file"
    done
    
    echo -e "${GREEN}✓ All files injected${NC}"
}

# List disk contents
list_disk() {
    local disk_path="$1"
    
    if [[ ! -f "$disk_path" ]]; then
        echo -e "${RED}Error: Disk not found: $disk_path${NC}"
        exit 1
    fi
    
    echo -e "${BLUE}Disk contents: $disk_path${NC}"
    diskm8 ls "$disk_path"
}

# Extract all files from disk
extract_disk() {
    local disk_path="$1"
    local output_dir="$2"
    
    if [[ ! -f "$disk_path" ]]; then
        echo -e "${RED}Error: Disk not found: $disk_path${NC}"
        exit 1
    fi
    
    echo -e "${BLUE}Extracting files from: $disk_path${NC}"
    echo "  Output directory: $output_dir"
    
    mkdir -p "$output_dir"
    diskm8 extract "$disk_path" "$output_dir/"
    
    echo -e "${GREEN}✓ Files extracted${NC}"
    echo "Extracted files:"
    ls -lh "$output_dir/"
}

# Create a test disk with sample source files
create_test_disk() {
    local disk_path="${1:-/tmp/edasm_test_disk.2mg}"
    
    echo -e "${BLUE}Creating test disk with sample sources${NC}"
    
    # Create disk
    create_disk "$disk_path"
    
    # Find all test .src files
    local src_files=()
    while IFS= read -r -d '' file; do
        src_files+=("$file")
    done < <(find "$PROJECT_ROOT" -maxdepth 1 -name "test_*.src" -print0)
    
    if [[ ${#src_files[@]} -eq 0 ]]; then
        echo -e "${YELLOW}Warning: No test_*.src files found${NC}"
    else
        inject_files "$disk_path" "${src_files[@]}"
    fi
    
    echo ""
    list_disk "$disk_path"
    
    echo ""
    echo -e "${GREEN}✓ Test disk ready: $disk_path${NC}"
}

# Compare two binary files byte-by-byte
compare_binaries() {
    local file1="$1"
    local file2="$2"
    
    echo -e "${BLUE}Comparing binaries:${NC}"
    echo "  File 1: $file1"
    echo "  File 2: $file2"
    
    if [[ ! -f "$file1" ]]; then
        echo -e "${RED}✗ File 1 not found${NC}"
        return 1
    fi
    
    if [[ ! -f "$file2" ]]; then
        echo -e "${RED}✗ File 2 not found${NC}"
        return 1
    fi
    
    # Get file sizes
    local size1=$(stat -f%z "$file1" 2>/dev/null || stat -c%s "$file1" 2>/dev/null)
    local size2=$(stat -f%z "$file2" 2>/dev/null || stat -c%s "$file2" 2>/dev/null)
    
    echo "  Size 1: $size1 bytes"
    echo "  Size 2: $size2 bytes"
    
    if [[ "$size1" != "$size2" ]]; then
        echo -e "${RED}✗ Size mismatch!${NC}"
        return 1
    fi
    
    # Byte-by-byte comparison
    if cmp -s "$file1" "$file2"; then
        echo -e "${GREEN}✓ Files are identical${NC}"
        return 0
    else
        echo -e "${RED}✗ Files differ${NC}"
        echo "Differences:"
        cmp -l "$file1" "$file2" | head -20
        return 1
    fi
}

# Show usage
usage() {
    cat << EOF
EDASM Disk Image Management Helper

Usage: $0 <command> [options]

Commands:
  create <disk> [size]           Create new ProDOS disk (default: 140KB)
  inject <disk> <file> [name]    Inject file into disk
  inject-many <disk> <files...>  Inject multiple files
  list <disk>                    List disk contents
  extract <disk> <outdir>        Extract all files from disk
  test-disk [disk]               Create test disk with sample sources
  compare <file1> <file2>        Compare two binary files

Examples:
  # Create a new disk
  $0 create /tmp/mydisk.2mg 140KB
  
  # Inject a single file
  $0 inject /tmp/mydisk.2mg test_simple.src
  
  # Inject multiple files
  $0 inject-many /tmp/mydisk.2mg test_*.src
  
  # List disk contents
  $0 list /tmp/mydisk.2mg
  
  # Extract all files
  $0 extract /tmp/mydisk.2mg /tmp/output/
  
  # Create test disk with all sample sources
  $0 test-disk /tmp/test.2mg
  
  # Compare two binaries
  $0 compare original.bin cedasm.bin

EOF
}

# Main command dispatcher
main() {
    if [[ $# -eq 0 ]]; then
        usage
        exit 0
    fi
    
    check_diskm8
    
    local command="$1"
    shift
    
    case "$command" in
        create)
            if [[ $# -lt 1 ]]; then
                echo -e "${RED}Error: create requires disk path${NC}"
                echo "Usage: $0 create <disk> [size]"
                exit 1
            fi
            create_disk "$@"
            ;;
        
        inject)
            if [[ $# -lt 2 ]]; then
                echo -e "${RED}Error: inject requires disk and file${NC}"
                echo "Usage: $0 inject <disk> <file> [name]"
                exit 1
            fi
            inject_file "$@"
            ;;
        
        inject-many)
            if [[ $# -lt 2 ]]; then
                echo -e "${RED}Error: inject-many requires disk and files${NC}"
                echo "Usage: $0 inject-many <disk> <files...>"
                exit 1
            fi
            inject_files "$@"
            ;;
        
        list|ls)
            if [[ $# -lt 1 ]]; then
                echo -e "${RED}Error: list requires disk path${NC}"
                echo "Usage: $0 list <disk>"
                exit 1
            fi
            list_disk "$@"
            ;;
        
        extract)
            if [[ $# -lt 2 ]]; then
                echo -e "${RED}Error: extract requires disk and output dir${NC}"
                echo "Usage: $0 extract <disk> <outdir>"
                exit 1
            fi
            extract_disk "$@"
            ;;
        
        test-disk)
            create_test_disk "$@"
            ;;
        
        compare)
            if [[ $# -lt 2 ]]; then
                echo -e "${RED}Error: compare requires two files${NC}"
                echo "Usage: $0 compare <file1> <file2>"
                exit 1
            fi
            compare_binaries "$@"
            ;;
        
        help|--help|-h)
            usage
            ;;
        
        *)
            echo -e "${RED}Error: Unknown command: $command${NC}"
            echo ""
            usage
            exit 1
            ;;
    esac
}

main "$@"
