# Memory Architecture

## Overview

The C-EDASM emulator uses an extended memory buffer to efficiently implement the Apple II memory system with language card support. This document describes the memory layout and access patterns.

## Memory Buffer Layout

The Bus class maintains a single contiguous memory buffer of **92KB** (extended from the standard 64KB to accommodate language card hardware):

```
Total Memory: 92KB (0x17000 bytes)

Main 6502 Address Space (64KB):
  $0000-$BFFF: Main RAM (48KB)
  $C000-$C7FF: I/O Space (2KB) - Soft-switches and hardware registers
  $C800-$CFFF: Expansion ROM (2KB)
  $D000-$DFFF: Language Card Banked Window (4KB) - maps to Bank1/Bank2
  $E000-$FFFF: Language Card Fixed Window (8KB) - maps to Fixed RAM or ROM

Extended Memory Regions (beyond $FFFF):
  $10000-$10FFF: Language Card Bank 1 (4KB)
  $11000-$11FFF: Language Card Bank 2 (4KB)
  $12000-$13FFF: Language Card Fixed RAM (8KB)
  $14000-$16FFF: Language Card ROM Image (12KB)
```

## Language Card Memory Mapping

The language card provides additional memory that can be switched into the $D000-$FFFF address range:

### Banked Region ($D000-$DFFF, 4KB)

- Can map to either Bank 1 or Bank 2
- Selected via soft-switches at $C080-$C08F
- Bank 1 stored at offset $10000 in the memory buffer
- Bank 2 stored at offset $11000 in the memory buffer

### Fixed Region ($E000-$FFFF, 8KB)

- Always uses the same 8KB bank when language card RAM is active
- Stored at offset $12000 in the memory buffer

### ROM Image ($D000-$FFFF, 12KB)

- Read when language card is in ROM mode
- Stored at offset $14000 in the memory buffer
- Initialized to zeros (simulating empty ROM)

## Soft-Switch Control ($C080-$C08F)

Language card modes are controlled by reading/writing addresses in the $C080-$C08F range:

| Address | Mode | $D000-$DFFF Read | $D000-$DFFF Write | $E000-$FFFF Read | $E000-$FFFF Write |
|---------|------|------------------|-------------------|------------------|-------------------|
| $C080/8 | RDBANK (Bank1/2) | Banked RAM | Ignored | Fixed RAM | Fixed RAM |
| $C081/9 | ROMIN (Bank1/2) | ROM | Banked RAM | ROM | Fixed RAM |
| $C082/A | RDROM | ROM | Ignored | ROM | Ignored |
| $C083/B | LCBANK (Bank1/2) | Banked RAM | Banked RAM | Fixed RAM | Fixed RAM |

- Addresses $C080-$C087: Select Bank 2
- Addresses $C088-$C08F: Select Bank 1

## Trap System

The Bus uses a **sparse trap system** to efficiently handle special memory regions:

### Traditional Approach (Previous)

```cpp
// 128KB overhead: 2 arrays of 64K function pointers
std::array<ReadTrapHandler, 64K> read_traps_;
std::array<WriteTrapHandler, 64K> write_traps_;
```

### Sparse Approach (Current)

```cpp
// ~100 bytes: Small vector of trap ranges
std::vector<ReadTrapRange> read_trap_ranges_;
std::vector<WriteTrapRange> write_trap_ranges_;
```

### Trap Ranges

Traps are registered as address ranges rather than individual addresses:

- $0400-$07FF: Text screen (write trap to detect screen updates)
- $C000-$C7FF: I/O space (read/write traps for device emulation)
- $D000-$FFFF: Language card (read/write traps for bank switching logic)

When a memory access occurs:

1. Search trap ranges to find a matching handler
2. If found, invoke handler which may:
   - Return custom value (reads)
   - Block write to main memory (writes)
   - Allow normal memory access (fall through)
3. If no trap found, access main memory buffer directly

## Memory Access Patterns

### Direct Memory Access

```cpp
uint8_t *main_memory = bus.data();  // Returns pointer to $0000-$FFFF
```

- Bypasses trap handlers
- Used for bulk operations (e.g., loading binaries)
- Only provides access to main 64KB

### Trapped Memory Access

```cpp
uint8_t value = bus.read(0xC000);   // Checks traps, may invoke handler
bus.write(0xD000, 0x42);             // Checks traps, may block write
```

- Honors trap handlers
- Used for normal CPU memory operations
- Enables device emulation and bank switching

### Language Card Extended Memory Access

```cpp
uint8_t *bank1 = bus.lc_bank1();       // Bank 1: $10000-$10FFF
uint8_t *bank2 = bus.lc_bank2();       // Bank 2: $11000-$11FFF
uint8_t *fixed = bus.lc_fixed_ram();   // Fixed RAM: $12000-$13FFF
uint8_t *rom = bus.lc_rom();           // ROM: $14000-$16FFF
```

- Direct access to extended regions
- Used by language card trap handlers
- Not accessible via normal 6502 addresses

## Implementation Benefits

### Memory Efficiency

- **Previous**: 64KB main + 28KB LC buffers + 128KB traps = **220KB**
- **Current**: 92KB unified buffer + ~100 bytes traps = **92KB**
- **Savings**: ~128KB (58% reduction)

### Correctness

- Single source of truth for all memory
- No synchronization issues between components
- Trapped and direct access always consistent

### Performance

- Trap lookup only checks relevant ranges (typically 3-5 ranges)
- No iteration over 64K addresses for range registration
- Reduced cache pressure from smaller data structures

### Maintainability

- Centralized memory management in Bus class
- Clear separation between main memory and extended regions
- Explicit trap range registration shows what addresses are special

## Testing

All memory functionality is tested:

- `test_language_card.cpp`: Language card bank switching and ROM/RAM modes
- `test_io_traps.cpp`: I/O space trap handling
- `test_emulator.cpp`: General memory operations

Tests verify:

- Bank switching works correctly
- ROM and RAM modes behave as expected
- Write protection is enforced
- Trap handlers are invoked properly
- No memory leaks or corruption

## Future Enhancements

Potential optimizations for the future:

1. **Cache last trap lookup**: Most memory accesses are sequential
2. **Sort trap ranges**: Enable binary search for faster lookup
3. **Merge overlapping ranges**: Reduce number of ranges to check
4. **Memory-mapped I/O directly in buffer**: For read-only devices

However, the current implementation prioritizes correctness and maintainability over premature optimization.
