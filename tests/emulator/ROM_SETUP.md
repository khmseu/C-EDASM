# Apple II ROM Setup for MAME

This document provides detailed instructions for setting up Apple II ROM files required for MAME emulation.

## Overview

MAME (Multiple Arcade Machine Emulator) requires Apple II ROM/BIOS files to emulate Apple II computers. These ROM files contain the firmware that was built into the original hardware.

## Legal Considerations

**Important**: Apple II ROM files are copyrighted by Apple Inc. and are not public domain.

You should only use ROM files if:
1. You own the original Apple II hardware and dump the ROMs yourself
2. You obtain them from sources that have proper authorization
3. You use them for personal, non-commercial purposes

## Required ROM Files

For C-EDASM emulator testing, you need one of these ROM sets:

### Apple IIe (Recommended)
- Filename: `apple2e.zip`
- Contains: 342-0133-a.chr, 342-0135-b.64, 342-0134-a.64, 342-0132-c.e12

### Apple IIGS (Alternative)
- Filename: `apple2gs.zip`
- Contains: 341s0632-2.bin, apple2gs.chr, 341-0728, 341-0748

## Where to Obtain ROM Files

### Option 1: From Your Own Hardware
If you own Apple II hardware, you can dump the ROMs using:
- EPROM programmer
- Apple II ROM extraction tools

### Option 2: Preservation Archives

**Note**: These sources host ROM files for preservation purposes. Legal status varies by jurisdiction.

1. **Internet Archive - TOSEC Collection**
   - URL: https://archive.org/details/Apple_2_TOSEC_2012_04_23
   - Contains comprehensive Apple II ROM collections
   - Download individual ROM sets or the complete archive

2. **apple2.org.za Mirror**
   - URL: https://mirrors.apple2.org.za/ftp.apple.asimov.net/emulators/rom_images/
   - Maintained archive of Apple II emulator resources
   - Individual ROM files available

3. **GitHub Repositories**
   - Some GitHub repos host ROM files (search for "apple2e rom")
   - Example: apple2abandonware/abandonware
   - Use at your own discretion

## Installation Instructions

Once you have obtained the ROM files legally:

### Step 1: Create ROM Directory

```bash
mkdir -p $HOME/mame/roms
```

### Step 2: Copy ROM Files

```bash
# Copy the ROM zip file (do not extract)
cp apple2e.zip $HOME/mame/roms/
```

**Important**: ROM files must be in ZIP format. Do not extract the contents.

### Step 3: Verify Installation

```bash
# Verify the ROM set is complete and correct
mame -verifyroms apple2e
```

Expected output if successful:
```
romset apple2e is good
```

If verification fails:
```
romset apple2e is bad
apple2e       : 342-0133-a.chr (2048 bytes) - NOT FOUND
```

### Step 4: Test with MAME

```bash
# Quick test (will boot to ProDOS prompt if disk image is provided)
mame apple2e -flop1 /path/to/disk.dsk
```

## Alternative ROM Paths

MAME checks multiple directories for ROMs (in order):
1. `$HOME/mame/roms`
2. `/usr/local/share/games/mame/roms`
3. `/usr/share/games/mame/roms`

You can place ROM files in any of these locations.

## Troubleshooting

### "romset not found" Error

This means MAME cannot find the ROM files in any of the expected directories.

Solution:
```bash
# Check where MAME looks for ROMs
mame -showconfig | grep rompath

# Ensure ROM file is in one of those directories
ls -la $HOME/mame/roms/apple2e.zip
```

### "romset is bad" Error

This means the ROM files are found but incomplete or incorrect.

Common causes:
- ROM files were extracted (they must stay zipped)
- ROM files are from wrong version
- ROM files are corrupted
- Missing ROM files in the set

Solution:
```bash
# Re-download the complete ROM set
# Ensure it's in ZIP format
# Verify checksums match MAME's requirements
```

### Which System to Use?

For C-EDASM testing:
- **apple2e**: Simpler, faster, widely available ROMs
- **apple2gs**: More advanced but requires more ROM files

We recommend starting with **apple2e**.

## Using with C-EDASM

After ROM installation:

```bash
# Run automated setup
./scripts/setup_emulator_deps.sh

# Test emulator
./scripts/run_emulator_test.sh boot

# Full assembly test
./scripts/run_emulator_test.sh assemble
```

## References

- MAME Documentation: https://docs.mamedev.org/
- MAME Apple II Driver: https://wiki.mamedev.org/index.php/Driver:Apple_II
- Apple II Documentation Project: http://www.apple2.org/

## Support

If you encounter issues:
1. Check the Troubleshooting section above
2. Verify ROM files with `mame -verifyroms apple2e`
3. See tests/emulator/README.md for additional help
4. Check MAME documentation for your specific version

## Disclaimer

This documentation is provided for educational and preservation purposes. Users are responsible for ensuring they comply with all applicable copyright laws in their jurisdiction.
