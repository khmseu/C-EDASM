#!/usr/bin/env bash
# Setup script for emulator testing dependencies
# Installs MAME and diskm8 required for automated testing against original EDASM

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"

echo "=== EDASM Emulator Dependencies Setup ==="
echo ""

# Detect OS
if [[ "$OSTYPE" == "linux-gnu"* ]]; then
    OS="linux"
elif [[ "$OSTYPE" == "darwin"* ]]; then
    OS="macos"
else
    echo "Error: Unsupported OS: $OSTYPE"
    exit 1
fi

# Install MAME
install_mame() {
    echo "Installing MAME..."
    
    if command -v mame &> /dev/null; then
        echo "✓ MAME already installed: $(mame -version | head -1)"
        return 0
    fi
    
    if [[ "$OS" == "linux" ]]; then
        if command -v apt-get &> /dev/null; then
            echo "Installing MAME via apt-get..."
            sudo apt-get update
            sudo apt-get install -y mame
        elif command -v dnf &> /dev/null; then
            echo "Installing MAME via dnf..."
            sudo dnf install -y mame
        else
            echo "Warning: No supported package manager found. Please install MAME manually."
            echo "See: https://www.mamedev.org/"
            return 1
        fi
    elif [[ "$OS" == "macos" ]]; then
        if command -v brew &> /dev/null; then
            echo "Installing MAME via Homebrew..."
            brew install mame
        else
            echo "Error: Homebrew not found. Please install Homebrew first."
            echo "See: https://brew.sh/"
            return 1
        fi
    fi
    
    if command -v mame &> /dev/null; then
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
    
    if command -v diskm8 &> /dev/null; then
        echo "✓ diskm8 already installed: $(diskm8 version 2>&1 | head -1 || echo 'unknown version')"
        return 0
    fi
    
    # Check if Go is installed
    if ! command -v go &> /dev/null; then
        echo "Go is required to install diskm8"
        
        if [[ "$OS" == "linux" ]]; then
            if command -v apt-get &> /dev/null; then
                echo "Installing Go via apt-get..."
                sudo apt-get install -y golang-go
            elif command -v dnf &> /dev/null; then
                echo "Installing Go via dnf..."
                sudo dnf install -y golang
            else
                echo "Warning: Please install Go manually: https://go.dev/doc/install"
                return 1
            fi
        elif [[ "$OS" == "macos" ]]; then
            if command -v brew &> /dev/null; then
                echo "Installing Go via Homebrew..."
                brew install go
            else
                echo "Error: Homebrew not found. Please install Go manually."
                echo "See: https://go.dev/doc/install"
                return 1
            fi
        fi
    fi
    
    # Install diskm8 using go install
    echo "Installing diskm8 from GitHub..."
    go install github.com/paleotronic/diskm8/cmd/diskm8@latest
    
    # Check if diskm8 is in PATH
    if command -v diskm8 &> /dev/null; then
        echo "✓ diskm8 installed successfully"
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
    
    echo ""
    
    if [[ $MAME_OK -eq 0 && $DISKM8_OK -eq 0 ]]; then
        echo "All dependencies installed successfully!"
        echo ""
        echo "Next steps:"
        echo "  1. Ensure EdAsm submodule is initialized: git submodule update --init --recursive"
        echo "  2. Run emulator tests: ./scripts/run_emulator_test.sh"
        echo "  3. See tests/emulator/README.md for more details"
        return 0
    else
        echo "Some dependencies could not be installed."
        echo "Please install them manually and try again."
        return 1
    fi
}

main "$@"
