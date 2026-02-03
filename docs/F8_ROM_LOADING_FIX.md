# F8 ROM Loading Fix - Summary

## Problem

The F8 ROM ($F800-$FFFF) was not being loaded correctly into memory. The reset vector at $FFFC-$FFFD still contained CALL_TRAP bytes (0x02) instead of the actual ROM reset vector.

## Root Cause

The issue was in the `Bus::load_binary()` function:

1. At power-on, `reset_bank_mappings()` configures banks 26-31 (0xD000-0xFFFF) as:
   - **Read**: from main RAM (where ROM should be)
   - **Write**: to WRITE_SINK (write-protected, simulating ROM)

2. `Bus::load_binary()` was using `translate_write_range()` which respects bank mappings

3. When loading ROM at $F800-$FFFF, writes were routed to WRITE_SINK instead of main RAM

4. The ROM data was effectively discarded, and main RAM still contained trap opcodes

5. Reading the reset vector returned TRAP_OPCODE bytes (0x02 0x02), not the actual ROM vector

## Solution

Modified `Bus::load_binary()` to write directly to physical main RAM:

```cpp
bool Bus::load_binary(uint16_t addr, const std::vector<uint8_t> &data) {
    // ... validation ...
    
    // Load binary data directly to physical main RAM, bypassing bank mappings
    // This is essential for loading ROM images at reset
    for (size_t i = 0; i < data.size(); ++i) {
        uint16_t target_addr = static_cast<uint16_t>(addr + i);
        uint32_t physical_offset = MAIN_RAM_OFFSET + target_addr;
        memory_[physical_offset] = data[i];
    }
    
    return true;
}
```

This ensures ROM data is written to main RAM regardless of bank mapping state, while reads and writes through the normal `read()`/`write()` functions continue to respect bank mappings (ROM remains read-only).

## Verification

### Before Fix

```text
Entry point (reset vector): $0202  # TRAP_OPCODE bytes
```

### After Fix

```text
Entry point (reset vector): $FA62  # Actual ROM reset vector
```

### ROM File Verification

```bash
$ xxd -s 0x7FC -l 4 "Apple II plus ROM Pages F8-FF - 341-0020 - Autostart Monitor.bin"
000007fc: 62fa 40fa                                b.@.
```

Reset vector bytes: 0x62 0xFA → $FA62 (little-endian)

## Tests Added

1. **test_rom_loading_at_reset()** (test_emulator.cpp)
   - Verifies ROM data loads to main RAM
   - Checks reset vector is readable
   - Confirms bytes are not TRAP_OPCODE

2. **test_rom_write_protected()** (test_emulator.cpp)
   - Verifies writes to ROM area go to write-sink
   - Confirms ROM appears read-only after loading

3. **test_rom_reset.cpp** (comprehensive integration test)
   - test_rom_reset_vector: Mock ROM with custom reset vector
   - test_actual_monitor_rom: Tests with real Apple II ROM
   - test_rom_write_protection: Comprehensive write protection test

## Impact

- ROM now loads correctly at power-on state
- Reset vector properly points to ROM code
- ROM remains write-protected (writes still go to write-sink)
- Language card tests simplified (no longer need workaround)
- All 16 tests pass

## Files Modified

- `src/emulator/bus.cpp` - Fixed `load_binary()` function
- `tests/unit/test_emulator.cpp` - Added ROM loading tests
- `tests/unit/test_language_card.cpp` - Simplified test (removed workaround)
- `tests/unit/test_rom_reset.cpp` - New comprehensive test file
- `tests/CMakeLists.txt` - Added test_rom_reset target

## References

- Apple II Technical Reference Manual - Reset vector at $FFFC-$FFFD
- Language card bank switching documentation
- Power-on memory state configuration
