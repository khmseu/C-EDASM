# MLI Logging Implementation Summary

## Changes Made

### Removed: Old Single-Line Format
The old format showed raw hex bytes and internal implementation details:
```
[PRODOS] call=$C8 (OPEN) params=$3000 params_bytes=[03 00 32 00 40 00 00 00 00 00 00 00 00 00 00 00] A=$00
```

### Added: New Two-Line Format

**Line 1: Call name, number, and INPUT parameters**
- Shows parameter names and typed values
- Strings are quoted
- Buffers show addresses
- Other values in hex

**Line 2: Result and OUTPUT parameters (excluding pointers)**
- Success or error with descriptive message
- Output values (excluding buffer/pathname pointers)

## Implementation Details

### Code Changes
- **File**: `src/emulator/mli.cpp`
- **Added helper functions**:
  - `prodos_datetime_to_iso8601()` - Convert ProDOS date/time to ISO 8601
  - `is_datetime_param()` - Check if parameter is date/time
  - `format_param_value()` - Format parameter based on type
  - `get_error_message()` - Map error code to human-readable message
  - `log_mli_input()` - Log first line (inputs)
  - `log_mli_output()` - Log second line (outputs and result)
- **Removed**: Old `log_call_summary()` lambda
- **Modified**: `prodos_mli_trap_handler()` to call new logging functions

### Special Cases

#### GET_TIME ($82)
Logs only call name, no parameters:
```
GET_TIME ($82)
```
Reason: GET_TIME has no parameter list - it writes directly to ProDOS system memory ($BF90-$BF93).

#### Error Cases
Logging occurs before error handling, so errors are properly logged:
```
OPEN ($C8) pathname="MISSING.TXT" io_buffer=$4000
  Result: error=$46 (File not found)
```

## Test Results

All MLI-related tests pass:
- ✅ test_mli_descriptors
- ✅ test_mli_stubs
- ✅ test_mli_lookup_performance
- ✅ test_emulator

## Examples

### Successful Calls

**SET_PREFIX**
```
SET_PREFIX ($C6) pathname="/"
  Result: success
```

**OPEN** (with output parameter)
```
OPEN ($C8) pathname="EDASM.ASM" io_buffer=$4000
  Result: success ref_num=$01
```

**READ** (multiple parameters)
```
READ ($CA) ref_num=$01 data_buffer=$5000 request_count=$000C
  Result: success transfer_count=$000C
```

### Failed Calls

**OPEN** (file not found)
```
OPEN ($C8) pathname="MISSING.TXT" io_buffer=$4000
  Result: error=$46 (File not found)
```

**CLOSE** (invalid reference)
```
CLOSE ($CC) ref_num=$FF
  Result: error=$43 (Invalid reference number)
```

## Documentation

Added `docs/MLI_LOGGING_FORMAT.md` with:
- Overview of the format
- Special case explanation (GET_TIME)
- Parameter value formatting rules
- Examples of successful and failed calls
- Comparison with old format
- Implementation notes
