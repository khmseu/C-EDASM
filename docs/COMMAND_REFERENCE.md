# EDASM Command Reference

This document describes the command set and interpreter architecture from the original EDASM system, based on analysis of `EDASM.SRC/EI/EDASMINT.S` and editor modules.

## Command Interpreter Architecture

The EDASM Interpreter (EI) is the main control program that:

1. Loads and unloads modules (Editor, Assembler, Linker) as needed
2. Provides a command-line interface with date/time display
3. Manages file operations through ProDOS MLI
4. Coordinates between modules via global page ($BD00-$BEFF)

### Main Loop

```
1. Display date/time prompt
2. Read command line from user
3. Parse command and parameters
4. Dispatch to appropriate handler:
   - File commands (LOAD, SAVE, DELETE, etc.)
   - Editor commands (LIST, INSERT, etc.)
   - Assembler commands (ASM, LINK)
   - Control commands (BYE, EXEC, etc.)
5. Return to step 1
```

### Special Keys

- **Ctrl-Y**: Warm restart (jump to $B198)
- **Ctrl-R**: Repeat last LIST command
- **RESET**: Cold restart (via $03F2 vector)
- **`*` prefix**: Comment line (ignored)

## File Commands

### LOAD <pathname>

Load a text file into the editor buffer.

- File type must be TXT ($04)
- Replaces current buffer contents
- Sets TxtBgn and TxtEnd pointers

### SAVE <pathname>

Save editor buffer to a text file.

- Creates TXT file type
- Sets ProDOS auxiliary type
- Updates file modification time

### DELETE <pathname>

Delete a file from disk.

- Checks file lock status
- Prompts for confirmation if locked
- Temporarily unlocks if user confirms 'Y'

### RENAME <old> <new>

Rename a file.

- Both pathnames in $BD80 buffer, null-separated
- Validates both names before renaming

### LOCK <pathname>

Set file to read-only.

- Sets ProDOS access bits: $C200

### UNLOCK <pathname>

Clear read-only attribute.

- Sets ProDOS access bits: $C2C2

### CATALOG [<path>]

List directory contents.

- Shows file names, types, sizes
- If no path given, uses current prefix

### PREFIX [<path>]

Set or display current directory.

- No argument: display current prefix
- With argument: change to specified directory

### EXEC <pathname>

Execute commands from a text file.

- Opens file and reads line-by-line
- Executes each line as if typed at prompt
- Useful for batch operations and startup scripts
- File: EdAsm.AutoST (auto-executed on startup if present)

## Editor Commands

### LIST [<range>]

Display lines from buffer.

- Range formats:
    - `LIST` - all lines
    - `LIST 10` - line 10 only
    - `LIST 10,20` - lines 10 through 20
    - `LIST 10,` - line 10 to end
    - `LIST ,20` - start to line 20
- Displays with line numbers

### PRINT [<range>]

Print lines to printer or file.

- Same range format as LIST
- Honors printer slot setting
- Can redirect to file

### <number>

Go to specific line number.

- Example: `100` goes to line 100
- Used for navigation

### INSERT or I

Enter insert mode.

- Type new lines
- Empty line (just RETURN) exits insert mode
- Lines inserted at current position

### DELETE <range>

Delete lines from buffer.

- Range format same as LIST
- Example: `DELETE 50,100` removes lines 50-100

### FIND <text>

Search forward for text.

- Case-sensitive
- Starts from current line
- Wraps to beginning if not found

### CHANGE <old>/<new>

Search and replace.

- Format: `CHANGE oldtext/newtext`
- Delimiter can be any character (/ shown)
- Replaces first occurrence found
- Prompt to continue search

### MOVE <range>,<dest>

Move lines to new location.

- Example: `MOVE 10,20,100` moves lines 10-20 after line 100
- Original lines removed from old location

### COPY <range>,<dest>

Copy lines to new location.

- Example: `COPY 10,20,100` copies lines 10-20 after line 100
- Original lines remain

### JOIN <range>

Join multiple lines into one.

- Concatenates lines in range
- Removes line breaks

### SPLIT <position>

Split current line at position.

- Creates two lines from one

## Assembler Commands

### ASM [<options>]

Assemble the current buffer.

Options (position-based parameters):

1. Source type: 0=buffer (default), 1=disk file
2. Object output: 0=none, 1=BIN, 2=REL
3. List output: 0=none, 1=screen, 2=printer, 3=file
4. List symbols: 0=no, 1=by name, 2=by value
5. List format: # of columns (2,4,6)

Examples:

- `ASM` - assemble buffer, no output
- `ASM 0,1` - assemble buffer, create BIN file
- `ASM 0,2,3,1` - assemble buffer, REL output, list to file, show symbols

The assembler performs:

1. **Pass 1**: Build symbol table, track PC
2. **Pass 2**: Generate code, resolve symbols
3. **Pass 3** (optional): Sort and print symbol table

### LINK [<options>]

Link relocatable object files.

- Combines multiple REL files
- Resolves external references
- Generates executable SYS or BIN file

## Control Commands

### BYE or QUIT

Exit EDASM.

- Returns to ProDOS system

### HELP or ?

Display help information.

- Shows command summary

### Multiple commands

Separate with semicolon:

- Example: `LOAD TEST.SRC; LIST; ASM 0,1`

## Buffer Management

### Normal Mode

Single text buffer from $0801 to $9900.

### Split Buffer Mode

Two independent buffers (SwapMode = 1 or 2):

- Buffer 1 and Buffer 2
- Prompt shows `1]` or `2]` to indicate active buffer
- Switch between buffers with specific command

## File Type Extensions (Linux Port)

In the C++ port, ProDOS file types map to extensions:

| ProDOS Type | Extension  | Description             |
| ----------- | ---------- | ----------------------- |
| TXT ($04)   | .src, .txt | Source code, text files |
| BIN ($06)   | .bin, .obj | Binary executable       |
| REL ($FE)   | .rel       | Relocatable object      |
| SYS ($FF)   | .sys       | System file             |
| -           | .lst       | Listing output          |

## Command Parser Implementation Notes

From analysis of EDASMINT.S command dispatch:

1. Input read via Monitor GETLN ($FD6A) into InBuf ($0200)
2. Leading spaces skipped
3. Command name extracted (until space/delimiter)
4. Table lookup for command handler
5. Parameters parsed based on command
6. Handler called via jump table

Command table structure:

```
- Command string (null-terminated)
- Handler address (2 bytes)
```

Commands checked in order:

1. File commands (LOAD, SAVE, etc.)
2. Editor commands (LIST, INSERT, etc.)
3. Assembly commands (ASM, LINK)
4. Control commands (BYE, HELP, EXEC)

## Error Handling

Errors reported via PrtError routine at $BEFC:

- Syntax error
- File not found
- Disk full
- Bad format
- File locked
- etc.

Error codes correspond to ProDOS error codes where applicable.

## Pathname Handling

Pathnames stored in $BD80 buffer:

- Maximum 64 characters (ProDOS limit)
- Format: [/]volume/dir1/dir2/filename
- Current prefix prepended if not absolute

In C++ port:

- Use standard POSIX paths
- Support both absolute and relative
- No volume name needed
