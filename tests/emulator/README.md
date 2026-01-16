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
These scripts are **improved prototypes** with keyboard injection and screen monitoring:

1. **Keyboard handling**: Implemented using direct memory writes to Apple II keyboard registers ($C000/KBD and $C010/KBDSTRB). This simulates typing at the hardware level.

2. **Screen monitoring**: Implemented screen memory reading to detect ProDOS prompt (']') and EDASM prompt. This enables state-driven waiting instead of fixed delays.

3. **Timing**: Mix of state-based waiting (for prompts) and conservative fixed delays (for file operations). Fallback timing ensures operation even if detection fails.

4. **Basic error handling**: Timeout detection with warnings, but continues operation using fallback timing.

### Remaining Work

To make these scripts production-ready:

1. **Enhanced state verification**:
   - Monitor EDASM internal state variables in memory
   - Detect error messages on screen
   - Verify file I/O completion by checking ProDOS results

2. **Improved error handling**:
   - Abort on unrecoverable errors
   - Generate detailed error reports
   - Implement retry logic for transient failures

3. **Better timing optimization**:
   - Fine-tune wait intervals based on testing
   - Add adaptive timing based on MAME speed settings
   - Implement progress indicators for long operations

### Production Implementation Requirements

The current implementation includes:

1. **Keyboard injection** (✅ Implemented):
   ```lua
   -- Direct memory write to Apple II keyboard registers
   function send_char(ch)
       local apple_code = ascii_to_apple2(ch)
       mem:write_u8(KBD_ADDR, apple_code)      -- Write to $C000
       emu.wait(10000)
       mem:read_u8(KBDSTRB_ADDR)               -- Clear $C010
       emu.wait(5000)
   end
   ```

2. **Screen/memory monitoring** (✅ Implemented):
   ```lua
   -- Check for ProDOS prompt by reading screen memory
   function check_for_prodos_prompt()
       for offset = 0x750, 0x777 do  -- Last line of screen
           if read_screen_char(offset - TEXT_PAGE1_START) == PROMPT_CHAR then
               return true
           end
       end
       return false
   end
   ```

3. **State verification** (🚧 Partial):
   - Detects ProDOS and EDASM prompts
   - Uses timeout-based waiting with fallbacks
   - TODO: Check for error messages in screen memory
   - TODO: Monitor EDASM's internal state variables
   - TODO: Verify file was written to disk

4. **Timing** (🚧 Partial):
   - Mix of event-driven waits (prompt detection) and fixed delays
   - Conservative timing ensures reliability
   - TODO: Fine-tune based on actual testing
   - TODO: Add adaptive timing for different MAME configurations

## Development Roadmap

### Phase 1: Basic Prototype ✅ COMPLETE
- [x] Demonstrate MAME Lua concept
- [x] Show automation workflow
- [x] Document approach

### Phase 2: Working Implementation ✅ MOSTLY COMPLETE
- [x] Implement keyboard injection using memory writes
- [x] Add screen memory monitoring for prompts
- [x] Add timeout detection
- [x] Test with actual EDASM.SRC.2mg (ready for testing)
- [ ] Add comprehensive error detection
- [ ] Verify file I/O operations

### Phase 3: CI Integration (Next)
- [ ] Test full workflow with real source files
- [ ] Containerize MAME + scripts for reproducibility
- [ ] Create GitHub Actions workflow integration
- [ ] Add golden output comparison
- [ ] Implement retry logic and better error handling

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
