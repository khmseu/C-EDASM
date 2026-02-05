# MLI NEWLINE Implementation

## Overview

This document describes the implementation of the ProDOS MLI NEWLINE ($C9) call in the C-EDASM emulator.

## ProDOS NEWLINE Call ($C9)

The NEWLINE call allows programs to enable or disable newline read mode for open files. When enabled, READ operations will terminate when they encounter the specified newline character.

### Parameters

```
Offset | Type  | Name         | Description
-------|-------|--------------|----------------------------------
0      | BYTE  | param_count  | Always 3
1      | BYTE  | ref_num      | File reference number (1-15)
2      | BYTE  | enable_mask  | $00 = disabled, nonzero = enabled
3      | BYTE  | newline_char | Character to match (typically $0D)
```

### Behavior

- **Enable Mask**: When $00, newline mode is disabled. Any nonzero value enables it.
- **Character Matching**: Each byte read is ANDed with enable_mask, then compared to newline_char
- **Common Usage**: enable_mask=$7F, newline_char=$0D matches both $0D and $8D (CR with/without high bit)
- **Read Termination**: When a match is found, the READ call terminates immediately, **including the matched byte**

## Implementation Details

### File Entry Structure

Added two fields to the `FileEntry` structure in `mli.cpp`:

```cpp
struct FileEntry {
    // ... existing fields ...
    uint8_t newline_enable_mask = 0x00; // $00 = disabled, nonzero = enabled
    uint8_t newline_char = 0x0D;        // default to CR ($0D)
};
```

### NEWLINE Handler

Implemented in `MLIHandler::handle_newline()`:

1. Extracts refnum, enable_mask, and newline_char from parameters
2. Validates the refnum
3. Stores the configuration in the FileEntry
4. Returns NO_ERROR on success, INVALID_REF_NUM on failure

### READ Modification

Modified `MLIHandler::handle_read()` to check newline mode:

```cpp
if (entry->newline_enable_mask != 0x00) {
    for (uint16_t i = 0; i < actual_read; ++i) {
        uint8_t ch = buffer[i];
        bus.write(static_cast<uint16_t>(data_buffer + i), ch);
        
        // Check if this character matches the newline char (after masking)
        if ((ch & entry->newline_enable_mask) == entry->newline_char) {
            // Found newline - terminate read after this character
            actual_read = i + 1;
            newline_found = true;
            break;
        }
    }
}
```

## Testing

Comprehensive unit tests in `tests/unit/test_mli_newline.cpp`:

1. **test_newline_basic_enable_disable**: Verifies enable/disable functionality
2. **test_newline_read_termination**: Tests line-by-line reading with CR termination
3. **test_newline_mask_behavior**: Validates mask behavior (e.g., $8D matches $0D with mask $7F)
4. **test_newline_invalid_refnum**: Error handling (currently skipped due to halt-on-error design)

All tests pass successfully.

## Reference

- **ProDOS Technical Reference Manual**: Section 4.5.2 (NEWLINE $C9)
- **Implementation Files**:
  - `src/emulator/mli.cpp`: Main implementation
  - `include/edasm/emulator/mli.hpp`: Handler declaration
  - `tests/unit/test_mli_newline.cpp`: Unit tests

## Example Usage

```
; Enable newline mode with CR ($0D) as terminator
LDA #1           ; refnum
STA NEWLINE_PARMS+1
LDA #$7F         ; enable_mask (strip high bit)
STA NEWLINE_PARMS+2
LDA #$0D         ; newline_char (CR)
STA NEWLINE_PARMS+3
LDA #$C9         ; NEWLINE call
JSR $BF00        ; ProDOS MLI
DW NEWLINE_PARMS

; Now READ calls will stop at CR
LDA #1           ; refnum
STA READ_PARMS+1
LDA #<BUFFER
STA READ_PARMS+2
LDA #>BUFFER
STA READ_PARMS+3
LDA #$FF         ; request 255 bytes
STA READ_PARMS+4
LDA #$00
STA READ_PARMS+5
LDA #$CA         ; READ call
JSR $BF00
DW READ_PARMS
; Returns with line of text in BUFFER, terminated at CR
```

## Notes

- Default state: newline mode disabled (enable_mask = $00)
- Default character: $0D (CR)
- The matched newline byte **is included** in the transferred data
- File position is updated to point past the newline character
- Works with all file types (TXT, BIN, etc.)
