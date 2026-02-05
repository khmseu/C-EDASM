# Language Card Trap Bug Fix

## Problem Description

When running the emulator with `scripts/run_emulator_runner.sh --max 30000000 --trace`, spurious language card trap messages appeared in the log:

```
[HostShims] Language Card control read at $C088 -> HW Bank 1 (idx=0) mode=0
```

These messages appeared during ProDOS MLI GET_FILE_INFO calls around instruction 173519.

## Root Cause Analysis

The `handle_get_file_info()` function in `src/emulator/mli.cpp` performs a memory scan to locate the ProDOS MLI parameter list. The original code scanned the entire 64KB address space:

```cpp
for (uint16_t addr = 0; addr < Bus::MEMORY_SIZE - 13; ++addr) {
    if (bus.read(addr) == 10) { // GET_FILE_INFO has 10 params
        // ... validation and matching logic
    }
}
```

### Why This Caused the Trap

The Apple II memory map includes:
- **$0000-$BFFF**: Main RAM (48KB)
- **$C000-$CFFF**: I/O space (4KB) - memory-mapped I/O devices and soft switches
- **$D000-$FFFF**: Language card/ROM space (12KB)

Within I/O space, addresses $C080-$C08F control the language card bank switching mechanism. When the code called `bus.read(0xC088)`, it triggered the language card soft switch, causing:

1. Bank switching state changes
2. Debug logging output
3. Unwanted side effects from I/O operations during a simple memory search

Specifically, $C088 is the READBSR1 soft switch (Bank 1, Read RAM, Write No).

## The Fix

Modified `handle_get_file_info()` to skip I/O space during memory scans:

### 1. Skip I/O addresses in main loop
```cpp
for (uint16_t addr = 0; addr < Bus::MEMORY_SIZE - 13; ++addr) {
    // Skip I/O space to avoid triggering language card and other soft switches
    if (addr >= 0xC000 && addr < 0xD000) {
        addr = 0xCFFF; // Will be incremented to 0xD000 in next iteration
        continue;
    }
    // ... rest of logic
}
```

### 2. Skip if pathname pointer targets I/O space
```cpp
uint16_t pathname_ptr = bus.read_word(static_cast<uint16_t>(addr + 1));
// Skip if pathname_ptr points to I/O space
if (pathname_ptr >= 0xC000 && pathname_ptr < 0xD000) {
    continue;
}
```

### 3. Skip if pathname characters are in I/O space
```cpp
for (uint8_t i = 0; i < path_len; ++i) {
    uint16_t char_addr = static_cast<uint16_t>(pathname_ptr + 1 + i);
    // Skip if character address is in I/O space
    if (char_addr >= 0xC000 && char_addr < 0xD000) {
        break; // Invalid path string, skip this candidate
    }
    check_path += static_cast<char>(bus.read(char_addr));
}
```

## Verification

### Before Fix
- Language card traps at $C088 occurred during GET_FILE_INFO
- Example from log line 174589:
```
GET_FILE_INFO ($C4) pathname=ptr=$BB40 "test_simple.src"
[HostShims] Language Card control read at $C088 -> HW Bank 1 (idx=0) mode=0
  Result: success access=$C3 file_type=$04 ...
```

### After Fix
- **No $C088 traps** - spurious language card accesses eliminated
- Only legitimate language card accesses remain:
  - $C080 (READBSR2): 284 occurrences
  - $C081 (WRITEBSR2): 8 occurrences  
  - $C082 (OFFBSR2): 547 occurrences
- These are actual language card operations by EDASM.SYSTEM, not side effects of memory scanning

### Test Command
```bash
./scripts/run_emulator_runner.sh --max 30000000 --trace
grep "Language Card control read at \$C088" EDASM.TEST/emulator_runner.log
# Should return: (empty)
```

## Why This Pattern is Problematic

Scanning all of memory, including I/O space, can cause:

1. **Side effects**: I/O reads may have side effects (e.g., clearing interrupt flags, advancing hardware state)
2. **Performance**: Triggering traps for every I/O address slows execution
3. **Log pollution**: Debug traces become cluttered with spurious messages
4. **Logic errors**: Some soft switches use double-read patterns that could be accidentally triggered

## Best Practices

When scanning memory in emulated systems:

1. **Skip I/O ranges**: Avoid reading from memory-mapped I/O unless specifically needed
2. **Use targeted searches**: If the parameter location is passed via registers or a known convention, use that instead of blind scanning
3. **Document memory layout**: Keep I/O ranges clearly documented (see `include/edasm/constants.hpp`)

## Related Code

- **Bug location**: `src/emulator/mli.cpp` - `MLIHandler::handle_get_file_info()`
- **Language card handler**: `src/emulator/host_shims.cpp` - `HostShims::handle_language_control_read()`
- **Constants**: `include/edasm/constants.hpp` - defines I/O addresses like `KBD = 0xC000`

## Impact

This fix ensures the emulator behaves more like real hardware, where ProDOS would never deliberately scan I/O space when looking for parameter lists. The parameter list is always in RAM, so limiting the search to RAM is both correct and more efficient.
