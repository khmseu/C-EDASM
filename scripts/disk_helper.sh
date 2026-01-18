#!/usr/bin/env bash
# Disk image management helper for EDASM testing
# Uses cadius for ProDOS disk operations

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

# Check if cadius is available
check_cadius() {
    local cadius_path="/tmp/cadius/cadius"
    if [[ ! -x ${cadius_path} ]] && ! command -v cadius &>/dev/null; then
        echo -e "${YELLOW}cadius not found, building it...${NC}"
        (
            cd /tmp
            if [[ ! -d cadius ]]; then
                git clone https://github.com/mach-kernel/cadius >/dev/null 2>&1
            fi
            cd cadius && make >/dev/null 2>&1
        )
        if [[ ! -x ${cadius_path} ]] && ! command -v cadius &>/dev/null; then
            echo -e "${RED}Error: cadius not available and could not be built${NC}"
            echo "Please install cadius manually or run: ./scripts/setup_emulator_deps.sh"
            exit 1
        fi
    fi
}

# Create a new ProDOS disk image
create_disk() {
    local disk_path="${1}"
    local size="${2:-140KB}"
    local volume_name="${3:-DISK}"

    echo -e "${BLUE}Creating disk image: ${disk_path} (${size})${NC}"

    if [[ -f ${disk_path} ]]; then
        echo -e "${YELLOW}Warning: Disk already exists, removing...${NC}"
        rm "${disk_path}"
    fi

    local cadius_cmd="cadius"
    if [[ -x "/tmp/cadius/cadius" ]]; then
        cadius_cmd="/tmp/cadius/cadius"
    fi

    "${cadius_cmd}" CREATEVOLUME "${disk_path}" "${volume_name}" "${size}"
    echo -e "${GREEN}✓ Disk created${NC}"
}

# Inject a file into disk image
inject_file() {
    local disk_path="${1}"
    local file_path="${2}"
    local dest_path="${3-}"

    if [[ ! -f ${disk_path} ]]; then
        echo -e "${RED}Error: Disk not found: ${disk_path}${NC}"
        exit 1
    fi

    if [[ ! -f ${file_path} ]]; then
        echo -e "${RED}Error: File not found: ${file_path}${NC}"
        exit 1
    fi

    local filename=$(basename "${file_path}")
    echo "  Injecting: ${filename}"

    local cadius_cmd="cadius"
    if [[ -x "/tmp/cadius/cadius" ]]; then
        cadius_cmd="/tmp/cadius/cadius"
    fi

    # If no destination path provided, use root
    if [[ -z ${dest_path} ]]; then
        # Get volume name from disk
        local volume_info=$("${cadius_cmd}" CATALOG "${disk_path}" 2>/dev/null | head -1 | grep -o '/[^/]*/' || echo "/DISK/")
        dest_path="${volume_info}"
    fi

    "${cadius_cmd}" ADDFILE "${disk_path}" "${dest_path}" "${file_path}"
}

# Inject multiple files
inject_files() {
    local disk_path="${1}"
    shift
    local files=("$@")

    echo -e "${BLUE}Injecting ${#files[@]} files into: ${disk_path}${NC}"

    for file in "${files[@]}"; do
        inject_file "${disk_path}" "${file}"
    done

    echo -e "${GREEN}✓ All files injected${NC}"
}

# List disk contents
list_disk() {
    local disk_path="${1}"

    if [[ ! -f ${disk_path} ]]; then
        echo -e "${RED}Error: Disk not found: ${disk_path}${NC}"
        exit 1
    fi

    echo -e "${BLUE}Disk contents: ${disk_path}${NC}"

    local cadius_cmd="cadius"
    if [[ -x "/tmp/cadius/cadius" ]]; then
        cadius_cmd="/tmp/cadius/cadius"
    fi

    "${cadius_cmd}" CATALOG "${disk_path}"
}

# Debug disk contents and extract files
debug_disk() {
    local disk_path="${1}"
    local output_dir="${2:-./debug_extract}"

    echo -e "${BLUE}Debugging disk: ${disk_path}${NC}"
    echo "This will list contents and extract all files."

    # List contents first
    list_disk "${disk_path}"

    echo ""
    echo "=== Extracting files ==="
    extract_disk "${disk_path}" "${output_dir}"
}

# Extract all files using cadius
extract_disk() {
    local disk_path="${1}"
    local output_dir="${2}"

    if [[ ! -f ${disk_path} ]]; then
        echo -e "${RED}Error: Disk not found: ${disk_path}${NC}"
        exit 1
    fi

    # Check if cadius is available
    local cadius_path="/tmp/cadius/cadius"
    if [[ ! -x ${cadius_path} ]]; then
        echo -e "${YELLOW}cadius not found, building it...${NC}"
        (
            cd /tmp
            if [[ ! -d cadius ]]; then
                git clone https://github.com/mach-kernel/cadius >/dev/null 2>&1
            fi
            cd cadius && make >/dev/null 2>&1
        )
        if [[ ! -x ${cadius_path} ]]; then
            echo -e "${RED}Error: Could not build cadius${NC}"
            exit 1
        fi
    fi

    echo -e "${BLUE}Extracting files using cadius from: ${disk_path}${NC}"
    echo "  Output directory: ${output_dir}"

    mkdir -p "${output_dir}"

    # Convert to absolute path
    local abs_disk_path
    if [[ ${disk_path} == /* ]]; then
        abs_disk_path="${disk_path}"
    else
        abs_disk_path="$(realpath "${disk_path}")"
    fi

    # Change to output directory and extract
    local original_dir="${PWD}"
    cd "${output_dir}"

    echo "Extracting all files..."
    if "${cadius_path}" EXTRACTVOLUME "${abs_disk_path}" . 2>/dev/null; then
        echo -e "${GREEN}✓ Extraction completed successfully${NC}"
    else
        echo -e "${RED}✗ Extraction failed${NC}"
        cd "${original_dir}"
        return 1
    fi

    cd "${original_dir}"

    # Count and organize results
    local total_files=$(find "${output_dir}" -type f | wc -l)
    local source_files=$(find "${output_dir}" -name "*.S#*" | wc -l)

    echo ""
    echo "=== Extraction Results ==="
    echo "Total files extracted: ${total_files}"
    echo "Source files (.S): ${source_files}"
    echo ""
    echo "Directory structure:"
    tree -L 2 "${output_dir}" 2>/dev/null || ls -la "${output_dir}"

    echo ""
    echo -e "${GREEN}Note:${NC} Files have Apple II metadata suffixes (e.g., #040000)"
    echo "These contain ProDOS file type and auxiliary type information."

    return 0
}

# Create a test disk with sample source files
create_test_disk() {
    local disk_path="${1:-/tmp/edasm_test_disk.2mg}"

    echo -e "${BLUE}Creating test disk with sample sources${NC}"

    # Create disk
    create_disk "${disk_path}"

    # Find all test .src files
    local src_files=()
    while IFS= read -r -d '' file; do
        src_files+=("${file}")
    done < <(find "${PROJECT_ROOT}/tests" -name "test_*.src" -print0)

    if [[ ${#src_files[@]} -eq 0 ]]; then
        echo -e "${YELLOW}Warning: No test_*.src files found${NC}"
    else
        inject_files "${disk_path}" "${src_files[@]}"
    fi

    echo ""
    list_disk "${disk_path}"

    echo ""
    echo -e "${GREEN}✓ Test disk ready: ${disk_path}${NC}"
}

# Compare two binary files byte-by-byte
compare_binaries() {
    local file1="${1}"
    local file2="${2}"

    echo -e "${BLUE}Comparing binaries:${NC}"
    echo "  File 1: ${file1}"
    echo "  File 2: ${file2}"

    if [[ ! -f ${file1} ]]; then
        echo -e "${RED}✗ File 1 not found${NC}"
        return 1
    fi

    if [[ ! -f ${file2} ]]; then
        echo -e "${RED}✗ File 2 not found${NC}"
        return 1
    fi

    # Get file sizes
    local size1=$(stat -f%z "${file1}" 2>/dev/null || stat -c%s "${file1}" 2>/dev/null)
    local size2=$(stat -f%z "${file2}" 2>/dev/null || stat -c%s "${file2}" 2>/dev/null)

    echo "  Size 1: ${size1} bytes"
    echo "  Size 2: ${size2} bytes"

    if [[ ${size1} != "${size2}" ]]; then
        echo -e "${RED}✗ Size mismatch!${NC}"
        return 1
    fi

    # Byte-by-byte comparison
    if cmp -s "${file1}" "${file2}"; then
        echo -e "${GREEN}✓ Files are identical${NC}"
        return 0
    else
        echo -e "${RED}✗ Files differ${NC}"
        echo "Differences:"
        cmp -l "${file1}" "${file2}" | head -20
        return 1
    fi
}

# Show usage
usage() {
    cat <<EOF
EDASM Disk Image Management Helper

Usage: ${0} <command> [options]

Commands:
  create <disk> [size] [volume]  Create new ProDOS disk (default: 140KB, DISK)
  inject <disk> <file> [dest]    Inject file into disk
  inject-many <disk> <files...>  Inject multiple files
  list <disk>                    List disk contents
  extract <disk> <outdir>        Extract all files from disk
  debug <disk> [outdir]          Debug disk and extract files
  test-disk [disk]               Create test disk with sample sources
  compare <file1> <file2>        Compare two binary files

Examples:
  # Create a new disk
  ${0} create /tmp/mydisk.2mg 140KB
  
  # Inject a single file
  ${0} inject /tmp/mydisk.2mg tests/test_simple.src
  
  # Inject multiple files
  ${0} inject-many /tmp/mydisk.2mg test_*.src
  
  # List disk contents
  ${0} list /tmp/mydisk.2mg
  
  # Extract all files
  ${0} extract /tmp/mydisk.2mg /tmp/output/
  
  # Create test disk with all sample sources
  ${0} test-disk /tmp/test.2mg
  
  # Compare two binaries
  ${0} compare original.bin cedasm.bin

EOF
}

# Main command dispatcher
main() {
    if [[ $# -eq 0 ]]; then
        usage
        exit 0
    fi

    check_cadius

    local command="${1}"
    shift

    case "${command}" in
    create)
        if [[ $# -lt 1 ]]; then
            echo -e "${RED}Error: create requires disk path${NC}"
            echo "Usage: ${0} create <disk> [size]"
            exit 1
        fi
        create_disk "$@"
        ;;

    inject)
        if [[ $# -lt 2 ]]; then
            echo -e "${RED}Error: inject requires disk and file${NC}"
            echo "Usage: ${0} inject <disk> <file> [name]"
            exit 1
        fi
        inject_file "$@"
        ;;

    inject-many)
        if [[ $# -lt 2 ]]; then
            echo -e "${RED}Error: inject-many requires disk and files${NC}"
            echo "Usage: ${0} inject-many <disk> <files...>"
            exit 1
        fi
        inject_files "$@"
        ;;

    list | ls)
        if [[ $# -lt 1 ]]; then
            echo -e "${RED}Error: list requires disk path${NC}"
            echo "Usage: ${0} list <disk>"
            exit 1
        fi
        list_disk "$@"
        ;;

    extract)
        if [[ $# -lt 2 ]]; then
            echo -e "${RED}Error: extract requires disk and output dir${NC}"
            echo "Usage: ${0} extract <disk> <outdir>"
            exit 1
        fi
        extract_disk "$@"
        ;;

    extract-adv)
        if [[ $# -lt 2 ]]; then
            echo -e "${RED}Error: extract-adv requires disk and output dir${NC}"
            echo "Usage: ${0} extract-adv <disk> <outdir>"
            exit 1
        fi
        extract_disk_advanced "$@"
        ;;

    extract-correct)
        if [[ $# -lt 2 ]]; then
            echo -e "${RED}Error: extract-correct requires disk and output dir${NC}"
            echo "Usage: ${0} extract-correct <disk> <outdir>"
            exit 1
        fi
        extract_disk_correct "$@"
        ;;

    extract-targeted)
        if [[ $# -lt 2 ]]; then
            echo -e "${RED}Error: extract-targeted requires disk and output dir${NC}"
            echo "Usage: ${0} extract-targeted <disk> <outdir>"
            exit 1
        fi
        extract_disk_targeted "$@"
        ;;

    extract-final)
        if [[ $# -lt 2 ]]; then
            echo -e "${RED}Error: extract-final requires disk and output dir${NC}"
            echo "Usage: ${0} extract-final <disk> <outdir>"
            exit 1
        fi
        extract_disk_final "$@"
        ;;

    extract-robust)
        if [[ $# -lt 2 ]]; then
            echo -e "${RED}Error: extract-robust requires disk and output dir${NC}"
            echo "Usage: ${0} extract-robust <disk> <outdir>"
            exit 1
        fi
        extract_disk_robust "$@"
        ;;

    extract-cadius)
        if [[ $# -lt 2 ]]; then
            echo -e "${RED}Error: extract-cadius requires disk and output dir${NC}"
            echo "Usage: ${0} extract-cadius <disk> <outdir>"
            exit 1
        fi
        extract_disk_cadius "$@"
        ;;

    debug)
        if [[ $# -lt 1 ]]; then
            echo -e "${RED}Error: debug requires disk path${NC}"
            echo "Usage: ${0} debug <disk> [outdir]"
            exit 1
        fi
        debug_disk "$@"
        ;;

    test-disk)
        create_test_disk "$@"
        ;;

    compare)
        if [[ $# -lt 2 ]]; then
            echo -e "${RED}Error: compare requires two files${NC}"
            echo "Usage: ${0} compare <file1> <file2>"
            exit 1
        fi
        compare_binaries "$@"
        ;;

    help | --help | -h)
        usage
        ;;

    *)
        echo -e "${RED}Error: Unknown command: ${command}${NC}"
        echo ""
        usage
        exit 1
        ;;
    esac
}

main "$@"
