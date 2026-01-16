#!/usr/bin/env bash
# Setup script for emulator testing dependencies
# Installs MAME and diskm8 required for automated testing against original EDASM

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"

REQUIRED_GO_VERSION="1.22.3"

download_tool() {
    if command -v curl &>/dev/null; then
        curl -fsSL "$1" -o "$2"
    elif command -v wget &>/dev/null; then
        wget -q "$1" -O "$2"
    else
        echo "Error: Neither curl nor wget is available to download $1"
        return 1
    fi
}

version_ge() {
    # Returns 0 if $1 >= $2 (both semver-like, e.g., 1.22.3)
    [[ $1 == "$2" ]] && return 0
    if [[ "$(printf '%s\n' "$1" "$2" | sort -V | head -n1)" == "$2" ]]; then
        return 0
    else
        return 1
    fi
}

ensure_go_version() {
    local current=""
    if command -v go &>/dev/null; then
        current="$(go version 2>/dev/null | awk '{print $3}' | sed 's/go//')"
        if version_ge "$current" "$REQUIRED_GO_VERSION"; then
            echo "✓ Go $current already installed (meets >= $REQUIRED_GO_VERSION)"
            return 0
        else
            echo "Go $current found, need >= $REQUIRED_GO_VERSION"
        fi
    else
        echo "Go not found, installing >= $REQUIRED_GO_VERSION"
    fi

    local go_os="linux"
    local go_arch="amd64"

    if [[ $OS == "macos" ]]; then
        go_os="darwin"
    fi

    case "$(uname -m)" in
    x86_64 | amd64)
        go_arch="amd64"
        ;;
    arm64 | aarch64)
        go_arch="arm64"
        ;;
    *)
        echo "Warning: Unknown architecture $(uname -m), defaulting to amd64"
        go_arch="amd64"
        ;;
    esac

    local archive="go${REQUIRED_GO_VERSION}.${go_os}-${go_arch}.tar.gz"
    local url="https://go.dev/dl/${archive}"
    local tmpfile="/tmp/${archive}"

    echo "Downloading Go $REQUIRED_GO_VERSION..."
    download_tool "$url" "$tmpfile"

    echo "Installing Go to /usr/local/go (requires sudo)..."
    sudo rm -rf /usr/local/go
    sudo tar -C /usr/local -xzf "$tmpfile"
    rm -f "$tmpfile"

    # Ensure newly installed Go is on PATH for the rest of this script
    export PATH="/usr/local/go/bin:$PATH"
    echo "✓ Go $REQUIRED_GO_VERSION installed"
}

diskm8_version() {
    # Report the resolved binary path without launching interactive mode
    command -v diskm8 2>/dev/null
}

echo "=== EDASM Emulator Dependencies Setup ==="
echo ""

# Detect OS
if [[ $OSTYPE == "linux-gnu"* ]]; then
    OS="linux"
elif [[ $OSTYPE == "darwin"* ]]; then
    OS="macos"
else
    echo "Error: Unsupported OS: $OSTYPE"
    exit 1
fi

# Install MAME
install_mame() {
    echo "Installing MAME..."

    if command -v mame &>/dev/null; then
        echo "✓ MAME already installed: $(mame -version | head -1)"
        return 0
    fi

    if [[ $OS == "linux" ]]; then
        if command -v apt-get &>/dev/null; then
            echo "Installing MAME via apt-get..."
            sudo apt-get update
            sudo apt-get install -y mame
        elif command -v dnf &>/dev/null; then
            echo "Installing MAME via dnf..."
            sudo dnf install -y mame
        else
            echo "Warning: No supported package manager found. Please install MAME manually."
            echo "See: https://www.mamedev.org/"
            return 1
        fi
    elif [[ $OS == "macos" ]]; then
        if command -v brew &>/dev/null; then
            echo "Installing MAME via Homebrew..."
            brew install mame
        else
            echo "Error: Homebrew not found. Please install Homebrew first."
            echo "See: https://brew.sh/"
            return 1
        fi
    fi

    if command -v mame &>/dev/null; then
        echo "✓ MAME installed successfully: $(mame -version | head -1)"
    else
        echo "✗ MAME installation failed"
        return 1
    fi
}

# Install diskm8 (Go-based ProDOS disk image tool)
install_diskm8() {
    echo ""
    echo "Installing diskm8..."

    if command -v diskm8 &>/dev/null; then
        local ver
        ver="$(diskm8_version || true)"
        echo "✓ diskm8 already installed: ${ver:-available}"
        return 0
    fi

    ensure_go_version

    # Install diskm8 using go install
    echo "Installing diskm8 from GitHub (module root)..."
    # Upstream exposes the CLI at the module root (no cmd/ subdir)
    go install github.com/paleotronic/diskm8@latest

    # Check if diskm8 is in PATH
    if command -v diskm8 &>/dev/null; then
        local ver
        ver="$(diskm8_version || true)"
        echo "✓ diskm8 installed successfully (${ver:-available})"
    else
        # Try to find it in GOPATH
        GOPATH="${GOPATH:-$HOME/go}"
        if [[ -f "$GOPATH/bin/diskm8" ]]; then
            echo "✓ diskm8 installed to $GOPATH/bin/diskm8"
            echo "Note: You may need to add $GOPATH/bin to your PATH:"
            echo "  export PATH=\"\$PATH:$GOPATH/bin\""
        else
            echo "✗ diskm8 installation failed"
            return 1
        fi
    fi
}

# Check for Apple II ROM files
check_apple2_roms() {
    echo ""
    echo "Checking for Apple II ROM files..."
    
    local rom_paths=(
        "$HOME/mame/roms"
        "/usr/local/share/games/mame/roms"
        "/usr/share/games/mame/roms"
    )
    
    local rom_found=0
    for rom_path in "${rom_paths[@]}"; do
        if [[ -f "$rom_path/apple2e.zip" ]] || [[ -f "$rom_path/apple2gs.zip" ]]; then
            rom_found=1
            echo "✓ Apple II ROM files found in: $rom_path"
            break
        fi
    done
    
    if [[ $rom_found -eq 0 ]]; then
        echo "⚠ Apple II ROM files not found"
        echo ""
        echo "IMPORTANT: MAME requires Apple II ROM/BIOS files to run emulation."
        echo "These files are copyrighted and cannot be distributed with this project."
        echo ""
        echo "To obtain ROM files:"
        echo "  1. If you own Apple II hardware, you can dump the ROMs yourself"
        echo "  2. ROM files may be available from preservation archives (check legal status)"
        echo "  3. Common sources include:"
        echo "     - Internet Archive: https://archive.org/details/Apple_2_TOSEC_2012_04_23"
        echo "     - apple2.org.za: https://mirrors.apple2.org.za/ftp.apple.asimov.net/emulators/rom_images/"
        echo ""
        echo "Required ROM sets:"
        echo "  - apple2e.zip (for Apple IIe emulation)"
        echo "  - apple2gs.zip (for Apple IIGS emulation)"
        echo ""
        echo "Install location: \$HOME/mame/roms/ (or other paths in MAME's rompath)"
        echo ""
        echo "After obtaining ROM files:"
        echo "  mkdir -p \$HOME/mame/roms"
        echo "  cp apple2e.zip \$HOME/mame/roms/"
        echo "  mame -verifyroms apple2e  # Verify installation"
        echo ""
        return 1
    fi
    
    return 0
}

# Main installation
main() {
    install_mame
    MAME_OK=$?

    install_diskm8
    DISKM8_OK=$?

    echo ""
    echo "=== Installation Summary ==="

    if [[ $MAME_OK -eq 0 ]]; then
        echo "✓ MAME: Ready"
    else
        echo "✗ MAME: Not available"
    fi

    if [[ $DISKM8_OK -eq 0 ]]; then
        echo "✓ diskm8: Ready"
    else
        echo "✗ diskm8: Not available (may need to add to PATH)"
    fi

    # Check for ROM files
    check_apple2_roms
    ROM_OK=$?

    echo ""

    if [[ $MAME_OK -eq 0 && $DISKM8_OK -eq 0 ]]; then
        echo "All software dependencies installed successfully!"
        echo ""
        if [[ $ROM_OK -ne 0 ]]; then
            echo "⚠ Apple II ROM files are still required for emulation."
            echo "See the instructions above for obtaining ROM files."
            echo ""
        fi
        echo "Next steps:"
        echo "  1. Ensure EdAsm submodule is initialized: git submodule update --init --recursive"
        if [[ $ROM_OK -eq 0 ]]; then
            echo "  2. Run emulator tests: ./scripts/run_emulator_test.sh"
        else
            echo "  2. Install Apple II ROM files (see instructions above)"
            echo "  3. Run emulator tests: ./scripts/run_emulator_test.sh"
        fi
        echo "  4. See tests/emulator/README.md for more details"
        return 0
    else
        echo "Some dependencies could not be installed."
        echo "Please install them manually and try again."
        return 1
    fi
}

main "$@"
