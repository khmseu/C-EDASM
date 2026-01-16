# EDASM Emulator Test Scripts

This directory contains automation scripts for testing C-EDASM against the original Apple II EDASM using emulators.

## Overview

These scripts enable automated testing by:
1. Booting the original EDASM in an emulator (MAME)
2. Loading test source files
3. Assembling them with EDASM
4. Extracting the output
5. Comparing results with C-EDASM output

## Files

### boot_test.lua
Simple proof-of-concept script that demonstrates:
- Waiting for ProDOS to boot
- Launching EDASM.SYSTEM
- Basic MAME Lua API usage

**Usage:**
```bash
mame apple2e \
  -flop1 third_party/EdAsm/EDASM_SRC.2mg \
  -video none -sound none -nothrottle \
  -autoboot_script tests/emulator/boot_test.lua
```

### assemble_test.lua
More complete automation script that demonstrates:
- Full EDASM workflow (load, assemble, save)
- Multiple disk mounting (source disk + test disk)
- Command sequencing

**Usage:**
```bash
# First, create and populate test disk
diskm8 create /tmp/test_disk.2mg 140KB
diskm8 inject /tmp/test_disk.2mg test_simple.src

# Run MAME with automation
mame apple2e \
  -flop1 third_party/EdAsm/EDASM_SRC.2mg \
  -flop2 /tmp/test_disk.2mg \
  -video none -sound none -nothrottle \
  -autoboot_script tests/emulator/assemble_test.lua

# Extract results
diskm8 extract /tmp/test_disk.2mg /tmp/results/
```

## Important Notes

### Current Status
These scripts are **prototypes** demonstrating the automation approach. They are **not production-ready** because:

1. **Keyboard handling is simplified**: The scripts use placeholder functions for keystroke injection. Real implementation requires proper use of MAME's `emu.keypost()` API with correct Apple II keyboard matrix handling.

2. **Timing is approximate**: Scripts use fixed delays rather than monitoring screen/memory state for completion.

3. **No error handling**: Scripts assume success at each step without verification.

4. **No output validation**: Scripts don't check that EDASM commands succeeded.

### Production Implementation Requirements

To make these scripts production-ready:

1. **Proper keyboard injection**:
   ```lua
   -- Correct approach using MAME API
   local kbd = manager.machine.ioport()
   kbd:write("KEY", ascii_code)  -- Send key to Apple II
   ```

2. **Screen/memory monitoring**:
   ```lua
   -- Check for ProDOS prompt by reading screen memory
   local screen_mem = cpu.spaces["program"]
   local prompt_addr = 0x0400  -- Text screen starts here
   local char = screen_mem:read_u8(prompt_addr)
   ```

3. **State verification**:
   - Check for error messages in screen memory
   - Monitor EDASM's internal state variables
   - Verify file was written to disk

4. **Robust timing**:
   - Use event-driven waits instead of fixed delays
   - Monitor memory locations for state changes
   - Implement timeouts with error reporting

## Development Roadmap

### Phase 1: Basic Prototype (Current)
- [x] Demonstrate MAME Lua concept
- [x] Show automation workflow
- [x] Document approach

### Phase 2: Working Implementation (Next)
- [ ] Implement proper keyboard injection using MAME API
- [ ] Add screen memory monitoring
- [ ] Add error detection and handling
- [ ] Test with actual EDASM.SRC.2mg

### Phase 3: CI Integration (Future)
- [ ] Containerize MAME + scripts
- [ ] Create GitHub Actions workflow
- [ ] Add golden output comparison
- [ ] Implement retry logic and timeouts

## Resources

### MAME Lua API Documentation
- Official API reference: https://docs.mamedev.org/luascript/index.html
- Core classes: https://docs.mamedev.org/luascript/ref-core.html
- Input/output: https://docs.mamedev.org/luascript/ref-input.html

### Community Examples
- Autoboot scripts: https://forums.launchbox-app.com/topic/78092-autoboot-lua-scripts-in-mame/
- Lua scripting examples: https://github.com/CSword123/MAME-LUA-scripts

### Apple II Technical References
- Memory map: https://www.kreativekorp.com/miscpages/a2info/memorymap.shtml
- Keyboard handling: https://www.applelogic.org/TheAppleIIEMemoryMap.html
- ProDOS reference: https://prodos8.com/

## Testing

To test these scripts:

1. Install MAME:
   ```bash
   # On Ubuntu/Debian
   sudo apt-get install mame
   
   # Or build from source
   git clone https://github.com/mamedev/mame.git
   cd mame
   make
   ```

2. Initialize submodule (if not done):
   ```bash
   git submodule update --init --recursive
   ```

3. Run boot test:
   ```bash
   cd /home/runner/work/C-EDASM/C-EDASM
   mame apple2e \
     -flop1 third_party/EdAsm/EDASM_SRC.2mg \
     -video none -sound none \
     -autoboot_script tests/emulator/boot_test.lua
   ```

## Contributing

When improving these scripts:
1. Test thoroughly with actual EDASM
2. Document any Apple II-specific behaviors discovered
3. Add error cases and recovery logic
4. Keep scripts modular and reusable
5. Update this README with findings

## License

These automation scripts are part of the C-EDASM project and follow the same license. The original EDASM is referenced for testing purposes only.
