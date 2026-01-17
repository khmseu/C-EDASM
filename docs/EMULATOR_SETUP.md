# Emulator Testing Setup Guide

This guide explains how to set up and use the MAME-based emulator testing infrastructure for comparing C-EDASM output with the original Apple II EDASM.

## Overview

The emulator testing system allows us to:

1. Run the original Apple II EDASM in MAME
2. Assemble test source files with the original EDASM
3. Extract the output files (BIN, REL, LST)
4. Compare byte-for-byte with C-EDASM output

## Quick Start

### 1. Install Dependencies

Run the automated setup script:

```bash
./scripts/setup_emulator_deps.sh
```

This will install:

- **MAME**: Apple II emulator
- **cadius**: ProDOS disk image management tool (C++)

### 2. Initialize Submodule

Ensure the EdAsm submodule is initialized to access the original EDASM disk image:

```bash
git submodule update --init --recursive
```

This provides:

- `third_party/EdAsm/EDASM_SRC.2mg` - Bootable ProDOS disk with EDASM.SYSTEM
- `third_party/EdAsm/EDASM.SRC/` - Original 6502 source code for reference

### 3. Run Tests

Run the boot test (demonstrates MAME + EDASM launch):

```bash
./scripts/run_emulator_test.sh boot
```

Or run the full assembly test (load, assemble, save):

```bash
./scripts/run_emulator_test.sh assemble
```

## Manual Setup (Alternative)

If the automated script doesn't work for your system, you can install manually:

### Ubuntu/Debian

```bash
# Install MAME
sudo apt-get update
sudo apt-get install -y mame

# Install build dependencies (required for cadius)
sudo apt-get install -y build-essential cmake git

# Install cadius
cd /tmp && git clone https://github.com/mach-kernel/cadius && cd cadius && make && sudo cp bin/release/cadius /usr/local/bin/
```

### macOS

```bash
# Install Homebrew if not already installed
# /bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"

# Install MAME
brew install mame

# Install build dependencies (required for cadius)
brew install cmake git

# Install cadius
cd /tmp && git clone https://github.com/mach-kernel/cadius && cd cadius && make && cp bin/release/cadius /usr/local/bin/
export PATH="$PATH:$HOME/go/bin"
```

## Directory Structure

```
C-EDASM/
├── scripts/
│   ├── setup_emulator_deps.sh   # Dependency installation
│   └── run_emulator_test.sh     # Test runner wrapper
├── tests/emulator/
│   ├── README.md                # Implementation guide
│   ├── boot_test.lua            # Prototype: Boot ProDOS + launch EDASM
│   └── assemble_test.lua        # Prototype: Full assembly workflow
└── third_party/EdAsm/
    ├── EDASM_SRC.2mg            # Original EDASM bootable disk
    └── EDASM.SRC/               # Original 6502 source code
```

## Test Workflow

### Boot Test

The boot test demonstrates basic MAME automation:

1. Boot ProDOS from EDASM_SRC.2mg
2. Wait for ProDOS prompt
3. Launch EDASM.SYSTEM
4. Verify EDASM loads

**Current Status**: Prototype - timing-based, no keyboard injection yet

### Assembly Test

The assembly test demonstrates a complete workflow:

1. Create a test disk with sample .src files
2. Boot ProDOS and launch EDASM
3. Load a source file from the test disk
4. Assemble the source
5. Save the output (BIN/REL/LST)
6. Extract output files for comparison

**Current Status**: Prototype - demonstrates concept, keyboard handling needs implementation

## Implementation Status

### ✅ Completed

- [x] Documentation and planning
- [x] Prototype Lua scripts
- [x] Setup and wrapper scripts
- [x] GitHub Actions workflow
- [x] Basic MAME integration concept

### 🚧 In Progress (Phase 2)

- [ ] Proper keyboard injection using MAME's ioport API
- [ ] Screen memory monitoring for prompts
- [ ] Robust timing and state detection
- [ ] Error handling and verification

### 📋 Planned (Phase 3+)

- [ ] Automated comparison tools
- [ ] Test harness for all sample .src files
- [ ] Golden output management
- [ ] Performance optimization for CI

## Common Issues

### MAME not found

```bash
# Check if MAME is installed
which mame

# If not installed, run setup script
./scripts/setup_emulator_deps.sh
```

### cadius not found

```bash
# Check if cadius is installed
which cadius

# If present but not in PATH, add to PATH
export PATH="$PATH:$HOME/go/bin"

# If not installed, run setup script
./scripts/setup_emulator_deps.sh
```

### EdAsm submodule not initialized

```bash
# Initialize the submodule
git submodule update --init --recursive

# Verify EDASM disk is present
ls -la third_party/EdAsm/EDASM_SRC.2mg
```

### MAME keyboard not working

This is expected in the current prototype phase. The Lua scripts use placeholder functions for keyboard injection. See `tests/emulator/README.md` for implementation requirements.

## MAME Usage

### Basic MAME Commands

Run MAME with EDASM disk:

```bash
mame apple2e -flop1 third_party/EdAsm/EDASM_SRC.2mg
```

Run headless (no video/sound):

```bash
mame apple2e \
  -flop1 third_party/EdAsm/EDASM_SRC.2mg \
  -video none -sound none -nothrottle
```

Run with automation script:

```bash
mame apple2e \
  -flop1 third_party/EdAsm/EDASM_SRC.2mg \
  -video none -sound none -nothrottle \
  -autoboot_script tests/emulator/boot_test.lua
```

Run with multiple disks:

```bash
mame apple2e \
  -flop1 third_party/EdAsm/EDASM_SRC.2mg \
  -flop2 /tmp/test_disk.2mg \
  -video none -sound none -nothrottle \
  -autoboot_script tests/emulator/assemble_test.lua
```

### cadius Usage

Create a ProDOS disk image:

```bash
cadius CREATEVOLUME mydisk.2mg MYDISK 140KB
```

List disk contents:

```bash
cadius CATALOG mydisk.2mg
```

Inject a file into disk:

```bash
cadius ADDFILE mydisk.2mg /MYDISK/ myfile.src
```

Extract all files from disk:

```bash
diskm8 extract mydisk.2mg /output/directory/
```

Extract a specific file:

```bash
cadius EXTRACTFILE mydisk.2mg /MYDISK/FILENAME /output/directory/
```

## CI/CD Integration

The GitHub Actions workflow (`.github/workflows/emulator-tests.yml`) automatically:

1. Installs MAME and cadius
2. Initializes the EdAsm submodule
3. Builds C-EDASM
4. Runs emulator tests
5. Uploads test logs as artifacts

**Note**: Emulator tests currently run with `continue-on-error: true` since keyboard injection is not yet implemented.

## Next Steps

To make the emulator tests production-ready:

1. **Implement proper keyboard injection** (Phase 2)
   - Use MAME's `ioport` API for keyboard input
   - Handle Apple II keyboard matrix correctly
   - See `tests/emulator/README.md` for details

2. **Add state monitoring** (Phase 2)
   - Read screen memory to detect prompts
   - Monitor EDASM internal state
   - Implement event-driven waiting

3. **Create comparison tools** (Phase 3)
   - Automate byte-for-byte comparison
   - Generate comparison reports
   - Handle differences gracefully

4. **Optimize for CI** (Phase 4)
   - Cache MAME and disk images
   - Parallelize tests
   - Reduce execution time

## Resources

### Documentation

- [DEBUGGER_EMULATOR_PLAN.md](../docs/DEBUGGER_EMULATOR_PLAN.md) - Original planning document
- [EMULATOR_INVESTIGATION_REPORT.md](../docs/EMULATOR_INVESTIGATION_REPORT.md) - Detailed analysis
- [EMULATOR_DECISION_MATRIX.md](../docs/EMULATOR_DECISION_MATRIX.md) - Decision framework
- [tests/emulator/README.md](../tests/emulator/README.md) - Implementation guide

### External Links

- [MAME Documentation](https://docs.mamedev.org/)
- [MAME Lua Scripting](https://docs.mamedev.org/luascript/index.html)
- [cadius GitHub](https://github.com/mach-kernel/cadius)
- [Apple II Technical Notes](https://www.kreativekorp.com/miscpages/a2info/)

## Contributing

When improving the emulator testing infrastructure:

1. Test thoroughly with actual hardware/emulator
2. Document Apple II-specific behaviors
3. Keep scripts modular and reusable
4. Update this documentation
5. Add test cases for new features

## License

The emulator testing infrastructure is part of C-EDASM and follows the same license. The original EDASM is referenced for testing purposes only.
