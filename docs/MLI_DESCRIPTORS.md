# MLI Parameter Descriptors

This document describes the parameter descriptor infrastructure for ProDOS MLI (Machine Language Interface) calls.

## Overview

The MLI parameter descriptor system provides:

1. **Type-safe parameter definitions** for all 26 ProDOS MLI calls
2. **Automatic parameter parsing** from memory into C++ types
3. **Automatic parameter writing** back to memory
4. **ProDOS error code enum** for consistent error handling
5. **Stub handlers** that return errors for unimplemented calls instead of halting

## Components

### ProDOS Error Codes (`ProDOSError` enum)

All 30 ProDOS error codes from the Apple ProDOS 8 Technical Reference Manual, section 4.8:

- `NO_ERROR` (0x00) - Success
- `BAD_CALL_NUMBER` (0x01) - Non-existent command
- `BAD_PARAM_COUNT` (0x04) - Bad parameter count
- `FILE_NOT_FOUND` (0x46) - File not found
- `DISK_FULL` (0x48) - Disk full
- And 25 more...

### Parameter Types (`MLIParamType` enum)

Six parameter types cover all MLI call parameters:

- `BYTE` - Single byte value
- `WORD` - Two-byte value (little-endian)
- `THREE_BYTE` - Three-byte value (24-bit, for EOF/position)
- `PATHNAME_PTR` - Pointer to length-prefixed pathname string
- `BUFFER_PTR` - Pointer to data buffer
- `REF_NUM` - File reference number (byte)

### Parameter Direction (`MLIParamDirection` enum)

- `INPUT` - Parameter read by MLI
- `OUTPUT` - Parameter written by MLI
- `INPUT_OUTPUT` - Parameter both read and written

### Call Descriptors

Each MLI call has a descriptor (`MLICallDescriptor`) containing:

- Call number (e.g., 0xC8 for OPEN)
- Call name (e.g., "OPEN")
- Parameter count
- Array of parameter descriptors with type, direction, and name

Example for OPEN ($C8):
```cpp
{0xC8, "OPEN", 3, {{
    IN(PATHNAME_PTR, INPUT, "pathname"),
    IN(BUFFER_PTR, INPUT, "io_buffer"),
    OUT(REF_NUM, OUTPUT, "ref_num"),
}}}
```

## API Functions

### `get_call_descriptor(uint8_t call_num)`

Look up descriptor for a call number. Returns `nullptr` if call is unknown.

```cpp
const MLICallDescriptor *desc = MLIHandler::get_call_descriptor(0xC8); // OPEN
```

### `read_input_params(bus, param_list_addr, desc)`

Read input parameters from ProDOS parameter list in memory.

Returns `std::vector<MLIParamValue>` containing parsed parameter values:
- `uint8_t` for BYTE/REF_NUM
- `uint16_t` for WORD/BUFFER_PTR
- `uint32_t` for THREE_BYTE
- `std::string` for PATHNAME_PTR

```cpp
auto values = MLIHandler::read_input_params(bus, param_list_addr, *desc);
std::string pathname = std::get<std::string>(values[0]);
uint16_t io_buffer = std::get<uint16_t>(values[1]);
```

### `write_output_params(bus, param_list_addr, desc, values)`

Write output parameters back to ProDOS parameter list in memory.

```cpp
std::vector<MLIParamValue> out_values = {
    std::string(""),  // pathname (input, skipped)
    uint16_t(0x4000), // io_buffer (input, skipped)
    uint8_t(3)        // ref_num (output, written)
};
MLIHandler::write_output_params(bus, param_list_addr, *desc, out_values);
```

### `set_error(cpu, ProDOSError)`

Set CPU registers for ProDOS error return:
- A = error code
- Carry flag set
- Zero flag cleared

```cpp
MLIHandler::set_error(state, ProDOSError::FILE_NOT_FOUND);
```

## Stub Handlers

Unimplemented MLI calls (those with descriptors but no handler code) now:

1. Log a message: `[MLI STUB] Call $C0 (CREATE) not yet implemented`
2. Return `BAD_CALL_NUMBER` error
3. Continue execution (don't halt)

Unknown calls (not in descriptor table) still halt the emulator.

## Implemented vs. Stub Calls

### Currently Implemented (12 calls)

- GET_TIME ($82)
- GET_FILE_INFO ($C4)
- SET_PREFIX ($C6)
- GET_PREFIX ($C7)
- OPEN ($C8)
- READ ($CA)
- WRITE ($CB)
- CLOSE ($CC)
- FLUSH ($CD)
- SET_MARK ($CE)
- GET_MARK ($CF)
- GET_EOF ($D1)

### Stub Handlers (14 calls)

- ALLOC_INTERRUPT ($40)
- DEALLOC_INTERRUPT ($41)
- QUIT ($65)
- READ_BLOCK ($80)
- WRITE_BLOCK ($81)
- CREATE ($C0)
- DESTROY ($C1)
- RENAME ($C2)
- SET_FILE_INFO ($C3)
- ONLINE ($C5)
- NEWLINE ($C9)
- SET_EOF ($D0)
- SET_BUF ($D2)
- GET_BUF ($D3)

## Testing

Two comprehensive test suites verify the infrastructure:

### `test_mli_descriptors`

Tests descriptor lookup, parameter parsing, and output writing:
- All 30 error codes defined correctly
- All 26 call descriptors present
- Parameter type and direction metadata correct
- Input parameter reading works for all types
- Output parameter writing works for all types

### `test_mli_stubs`

Tests stub handler behavior:
- Stub calls return BAD_CALL_NUMBER error
- Stub calls continue execution (don't halt)
- Implemented calls still work correctly
- Unknown calls still halt

## Usage Example

Future MLI handler implementations can use the descriptor infrastructure:

```cpp
// Look up descriptor
const MLICallDescriptor *desc = get_call_descriptor(call_num);
if (!desc) {
    // Unknown call - halt
    return false;
}

// Read input parameters
auto params = read_input_params(bus, param_list, *desc);
std::string pathname = std::get<std::string>(params[0]);
uint16_t io_buffer = std::get<uint16_t>(params[1]);

// Do the actual work...
uint8_t ref_num = allocate_file_handle(pathname, io_buffer);

// Write output parameters
std::vector<MLIParamValue> out_values = {
    params[0],  // pathname (input, unchanged)
    params[1],  // io_buffer (input, unchanged)
    ref_num     // ref_num (output)
};
write_output_params(bus, param_list, *desc, out_values);

set_success(cpu);
```

## References

- Apple ProDOS 8 Technical Reference Manual, Chapter 4 (MLI calls)
- Apple ProDOS 8 Technical Reference Manual, Section 4.8 (error codes)
- `include/edasm/emulator/mli.hpp` - Header with all declarations
- `src/emulator/mli.cpp` - Implementation and descriptor table
- `tests/unit/test_mli_descriptors.cpp` - Descriptor tests
- `tests/unit/test_mli_stubs.cpp` - Stub handler tests
