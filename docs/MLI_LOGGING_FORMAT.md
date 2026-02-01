# MLI Handler Logging Format

This document describes the new two-line logging format for ProDOS MLI (Machine Language Interface) handlers.

## Overview

All MLI handlers (except GET_TIME) now log two lines:
1. **Input line**: Call name, number, and INPUT parameters with their values
2. **Output line**: Result (success/error) and OUTPUT parameters (excluding pointers)

## Special Case: GET_TIME

GET_TIME is a special case because it has no parameter list - it writes directly to ProDOS memory locations ($BF90-$BF93). Therefore, GET_TIME logs only a single line with the call name and number:

```
GET_TIME ($82)
```

The handler still logs its internal details separately for debugging:
```
GET_TIME: wrote date/time to $BF90-$BF93
  Year (since 1900): 126
  Month: 2
  Day: 1
  Hour: 21
  Minute: 33
```

## Parameter Value Formatting

Parameters are formatted based on their type:

- **Strings (PATHNAME_PTR)**: Quoted strings, e.g., `pathname="EDASM.ASM"`
- **Buffers (BUFFER_PTR)**: Hex addresses, e.g., `io_buffer=$4000`
- **Bytes/Words**: Hex values with $ prefix, e.g., `ref_num=$01`, `request_count=$000C`
- **Date/Time (future enhancement)**: ISO 8601 format (YYYY-MM-DDTHH:MM)

## Examples

### Successful Call: SET_PREFIX

```
SET_PREFIX ($C6) pathname="/"
  Result: success
```

### Successful Call with Output: OPEN

```
OPEN ($C8) pathname="EDASM.ASM" io_buffer=$4000
  Result: success ref_num=$01
```

### Call with Multiple Parameters: READ

```
READ ($CA) ref_num=$01 data_buffer=$5000 request_count=$000C
  Result: success transfer_count=$000C
```

### Failed Call: OPEN (file not found)

```
OPEN ($C8) pathname="MISSING.TXT" io_buffer=$4000
  Result: error=$46 (File not found)
```

### Failed Call: CLOSE (invalid reference)

```
CLOSE ($CC) ref_num=$01
  Result: error=$43 (Invalid reference number)
```

## Comparison with Old Format

### Old Format (removed)
```
[PRODOS] call=$C8 (OPEN) params=$3000 params_bytes=[03 00 32 00 40 00 00 00 00 00 00 00 00 00 00 00] A=$00
```

### New Format
```
OPEN ($C8) pathname="EDASM.ASM" io_buffer=$4000
  Result: success ref_num=$01
```

The new format is:
- More human-readable
- Shows parameter names and values instead of raw hex bytes
- Clearly separates inputs from outputs
- Shows error messages in plain text
- Excludes internal implementation details (parameter list address, accumulator value)

## Implementation Notes

1. **Pointers are not logged in output**: Buffer pointers and pathname pointers are logged as inputs (showing the address), but not as outputs since they're addresses the handler writes to, not values returned.

2. **Logging occurs before error handling**: Both success and error cases log the output line before any detailed error dump or halt occurs.

3. **Trace mode required**: Logging only occurs when trace mode is enabled via `MLIHandler::set_trace(true)`.

4. **INPUT_OUTPUT parameters**: Parameters that are both input and output (like data_buffer in READ) appear only in the input line with the buffer address, and the actual data is not logged.
