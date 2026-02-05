# GET_FILE_INFO EOF Parameter Fix

## Issue Summary

The GET_FILE_INFO ProDOS MLI handler contained a **45-line memory scanning workaround** that:
1. Scanned all 64KB of memory looking for the parameter list
2. Triggered language card soft switches at $C088
3. Had an offset calculation bug that would corrupt mod_date

## Why the Memory Scan Was Wrong

The ProDOS MLI calling convention is:
```asm
    JSR $BF00              ; Call MLI
    .BYTE command_number   ; At call_site
    .WORD param_list_ptr   ; At call_site+1,+2
    ; execution continues here
```

The MLI trap handler (at line 1732 in mli.cpp) **already extracts** the parameter list pointer:
```cpp
uint16_t param_list = static_cast<uint16_t>((param_hi << 8) | param_lo);
```

This pointer is passed to `read_input_params()` and `write_output_params()`. The memory scan was completely unnecessary!

## The Real Problem

The GET_FILE_INFO descriptor was missing the EOF parameter:
- ProDOS spec: GET_FILE_INFO has **11 parameters** (including EOF)
- Original code: Descriptor had only **10 parameters** (missing EOF)
- Workaround: Scanned memory to find param_list, then manually wrote EOF

## The Proper Solution

**Add EOF to the descriptor as the 11th parameter:**

```cpp
{0xC4,
 "GET_FILE_INFO",
 11,  // Changed from 10
 {{
     IN(PATHNAME_PTR, INPUT, "pathname"),
     OUT(BYTE, OUTPUT, "access"),
     OUT(BYTE, OUTPUT, "file_type"),
     OUT(WORD, OUTPUT, "aux_type"),
     OUT(BYTE, OUTPUT, "storage_type"),
     OUT(WORD, OUTPUT, "blocks_used"),
     OUT(WORD, OUTPUT, "mod_date"),
     OUT(WORD, OUTPUT, "mod_time"),
     OUT(WORD, OUTPUT, "create_date"),
     OUT(WORD, OUTPUT, "create_time"),
     OUT(THREE_BYTE, OUTPUT, "eof"),  // NEW
 }},
 &MLIHandler::handle_get_file_info},
```

**Return EOF via outputs like all other handlers:**

```cpp
outputs.push_back(size32); // eof (3 bytes)
```

The existing `write_output_params()` function already has full support for THREE_BYTE parameters and will write them to the correct memory location using the `param_list` pointer.

## Why This Works

1. **MLI trap handler** extracts param_list from calling convention
2. **read_input_params()** reads inputs using param_list
3. **Handler** returns outputs via std::vector
4. **write_output_params()** writes outputs to memory using param_list
5. **No memory scanning needed!**

## Additional Bug Fixed

The memory scan had an offset calculation error:
```cpp
uint16_t eof_offset = param_list + 10;  // WRONG!
```

This calculates offset as bytes 10-12, but the actual EOF location is:
- Byte 0: param_count
- Bytes 1-2: pathname_ptr (WORD)
- Byte 3: access (BYTE)
- Byte 4: file_type (BYTE)
- Bytes 5-6: aux_type (WORD)
- Byte 7: storage_type (BYTE)
- Bytes 8-9: blocks_used (WORD)
- Bytes 10-11: mod_date (WORD) ← scan was overwriting this!
- Bytes 12-13: mod_time (WORD)
- Bytes 14-15: create_date (WORD)
- Bytes 16-17: create_time (WORD)
- Bytes 18-20: EOF (THREE_BYTE) ← actual location

The scan would have corrupted mod_date!

## Verification

**Before fix:**
- Memory scan triggered language card trap at $C088
- 45 lines of unnecessary code
- Potential data corruption bug

**After fix:**
- No memory scanning
- No language card traps
- Correct EOF values returned: `eof=$001FFF`, `eof=$000039`, `eof=$000136`
- Code reduced by 43 lines

## Lesson Learned

When a handler needs to write output parameters:
1. ✅ **DO**: Add the parameter to the descriptor and return via outputs
2. ❌ **DON'T**: Scan memory to find param_list yourself
3. ✅ **TRUST**: The MLI infrastructure already has param_list and handles it correctly

The framework is designed to handle all parameter types (BYTE, WORD, THREE_BYTE) correctly. Use it!
