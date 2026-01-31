# Porting Plan (EDASM to C++/ncurses)

## Goals

- Preserve original EDASM editor/assembler semantics while targeting Linux terminal.
- Mirror source structure from `EDASM.SRC` for traceability.
- Replace ProDOS file model with Linux paths and extensions.

## EDASM.SRC Structure Overview

The original EDASM system consists of several modules totaling ~19,000 lines of 6502 assembly:

### Core Modules

1. **EI (EDASM Interpreter)** - `EI/EDASMINT.S` (~27K)
    - Command interpreter and main loop (entry at `$B100`)
    - Module loader/unloader for Editor, Assembler, Linker
    - ProDOS interface and file management
    - Global page at `$BD00-$BEFF` for inter-module communication
    - Command parsing and dispatch
    - Handles RESET vector (`$03F2`) and Ctrl-Y vector (`$03F8`)

2. **EDITOR** - 3 files (~33K total)
    - `EDITOR1.S`: File commands (LOCK/UNLOCK/DELETE/RENAME), line printing
    - `EDITOR2.S`: Text buffer manipulation, cursor movement, search/replace
    - `EDITOR3.S`: Insert/delete operations, command execution
    - Uses Sweet16 pseudo-machine for 16-bit pointer operations
    - Text buffer at `$0801` to `$9900` (HiMem)
    - Split buffer mode support (SwapMode)

3. **ASM (Assembler)** - 3 files (~38K total)
    - `ASM1.S`: Pass 3 (symbol table printing), sorting logic
    - `ASM2.S`: Pass 1 & 2 (tokenization, code generation)
    - `ASM3.S`: Expression evaluation, opcode emission, directive handling
    - Symbol table with hash-based lookup
    - Two-pass assembly with forward reference resolution
    - Relocatable object file generation (REL type)

4. **LINKER** - `LINK.S`
    - Links multiple REL files into executable
    - Address resolution and relocation
    - Entry point handling

5. **COMMONEQUS.S** - Shared equates
    - ASCII control codes (CTRL-A through DEL)
    - ProDOS file types (TXT=$04, BIN=$06, REL=$FE, SYS=$FF)
    - Zero page locations shared across modules
    - Apple II monitor entry points
    - Sweet16 register definitions (R0-R15)
    - ProDOS MLI parameter offsets

## Source Mapping (Detailed)

### Module → C++ Class/Namespace Mapping

| Original Module      | Target Location                  | Responsibility                                      |
| -------------------- | -------------------------------- | --------------------------------------------------- |
| `EI/EDASMINT.S`      | `src/core/app.cpp`               | Main loop, command dispatch, module coordination    |
| `EDITOR/EDITOR*.S`   | `src/editor/editor.cpp`          | Text buffer, line editing, file commands            |
| `EDITOR/SWEET16.S`   | (inline C++)                     | 16-bit pointer arithmetic (use native C++ pointers) |
| `ASM/ASM*.S`         | `src/assembler/assembler.cpp`    | Two-pass assembly, tokenization                     |
| `ASM/*` symbol table | `src/assembler/symbol_table.cpp` | Symbol storage, lookup, sorting                     |
| `LINKER/LINK.S`      | `src/assembler/linker.cpp`       | REL file linking (new file)                         |
| `COMMONEQUS.S`       | `include/edasm/constants.hpp`    | Shared constants (new file)                         |
| `*/EQUATES.S`        | module-specific headers          | Module-specific constants                           |

### Zero Page Variables

The original uses Apple II zero page (`$00-$FF`) extensively. Map to C++ class members:

**Editor (`$60-$82`):**

- `Z60` (TabChar expansion flag) → `Editor::tab_expand_mode_`
- `Z7A-Z82` (text move workspace) → `Editor::move_workspace_`

**Assembler (`$60-$97`):**

- `BCDNbr ($60)` → `Assembler::line_number_bcd_`
- `StrtSymT/EndSymT ($63-$65)` → `SymbolTable::start_/end_ptr_`
- `PassNbr ($67)` → `Assembler::pass_number_`
- `ListingF ($68)` → `Assembler::listing_enabled_`
- `PC ($7D)` → `Assembler::program_counter_`
- `ObjPC ($7F)` → `Assembler::object_counter_`

**Global (`$0A-$75` shared):**

- `TxtBgn/TxtEnd ($0A-$0F)` → `App::text_begin_/text_end_`
- `StackP ($49)` → saved/restored by App
- `FileType ($51)` → `ProDOSFile::file_type_`
- `ExecMode ($53)` → `App::exec_mode_`
- `SwapMode ($74)` → `Editor::swap_mode_`

## Command Set (Editor/Interpreter)

Based on `EDASMINT.S` command dispatch, EDASM supports these commands:

### File Commands

- `LOAD <pathname>` - Load text file into editor buffer
- `SAVE <pathname>` - Save buffer to file (TXT type)
- `DELETE <pathname>` - Delete file
- `RENAME <old> <new>` - Rename file
- `LOCK <pathname>` - Set file read-only
- `UNLOCK <pathname>` - Clear read-only
- `CATALOG [<path>]` - List directory contents
- `PREFIX [<path>]` - Set/show current directory

### Editor Commands

- `LIST [<range>]` - Display lines
- `PRINT [<range>]` - Print to printer/file
- `<n>` - Go to line n
- `INSERT` or `I` - Enter insert mode
- `DELETE <range>` - Delete lines
- `FIND <text>` - Search forward
- `CHANGE <old>/<new>` - Search & replace
- `MOVE <range>,<dest>` - Move lines
- `COPY <range>,<dest>` - Copy lines
- `JOIN <range>` - Join lines
- `SPLIT <n>` - Split line at position

### Assembler Commands

- `ASM [<params>]` - Assemble current buffer
- `LINK [<params>]` - Link object files

### Control Commands

- `EXEC <pathname>` - Execute command file
- `BYE` or `QUIT` - Exit EDASM
- `HELP` - Show help
- Ctrl-R - Repeat last LIST
- Ctrl-Y - Warm restart
- `*<text>` - Comment (ignored)

## Assembler Architecture

### Two-Pass Assembly Process

**Pass 1** (`ASM2.S`):

1. Tokenize source lines
2. Build symbol table with forward references
3. Track program counter (PC)
4. Mark undefined symbols

**Pass 2** (`ASM2.S`/`ASM3.S`):

1. Re-tokenize with symbol values known
2. Evaluate expressions
3. Generate machine code
4. Resolve forward references
5. Output to OBJ/BIN/REL file
6. Optionally generate listing file

**Pass 3** (`ASM1.S`):

1. Sort symbol table (by name or value)
2. Print symbol table to listing
3. Format in 2/4/6 columns

### Symbol Table Structure

Each symbol node (variable length):

```text
+0: Link pointer (2 bytes) - next node in hash chain
+2: Symbol name (1-16 chars, high bit set on last char)
+n: Flag byte:
    bit 7: undefined ($80)
    bit 6: unreferenced ($40)
    bit 5: relative ($20)
    bit 4: external ($10)
    bit 3: entry ($08)
    bit 2: macro ($04)
    bit 1: no such label ($02)
    bit 0: forward referenced ($01)
+n+1: Value (2 bytes, little-endian)
```

Hash table: 256-entry array at `HeaderT` pointing to linked lists.

### Directives Supported

- `ORG <addr>` - Set origin
- `EQU <value>` - Define constant
- `DA <word>,<word>...` - Define address (16-bit)
- `DW <word>` - Define word
- `DB <byte>` - Define byte
- `DFB <byte>` - Define byte
- `ASC <text>` - ASCII string
- `DCI <text>` - DCI string (last char inverted)
- `DS <count>` - Define storage
- `REL` - Relocatable mode
- `ENT <label>` - Entry point
- `EXT <label>` - External reference
- `END` - End of assembly
- `LST ON/OFF` - Listing control
- `MSB ON/OFF` - High bit control

### 6502 Opcodes

Full 6502 instruction set with all addressing modes:

- Implied, Immediate, Absolute, Zero Page
- Indexed (X, Y), Indirect, Indirect Indexed

## Platform Adaptations

### Screen System

- **Original**: Direct Apple II video memory writes (`$0400-$07FF`), 40-column text
- **Port**: ncurses `Screen` class with:
  - `clear()`, `refresh()`, `write_line(row, text)`
  - 80-column support
  - Handle terminal resize events

### Keyboard Input

- **Original**: Apple II keyboard (`$C000`), with strobe (`$C010`)
- **Port**: ncurses `getch()` with mapping:
  - Ctrl-A to Ctrl-Z → command keys
  - ESC → ESCAPE
  - Backspace/Delete → line editing
  - Arrow keys (not in original) → cursor movement enhancement

### File System

- **Original**: ProDOS with volume slots, type/aux type, 15-char names
- **Port**: POSIX paths with extension mapping:
  - TXT ($04) → `.src`, `.txt`
  - BIN ($06) → `.bin`, `.obj`
  - REL ($FE) → `.rel`
  - SYS ($FF) → `.sys`
  - Maintain load address in file header (4 bytes: addr, len)

### Memory Layout

- **Original**: Apple II 64K with language card banking
  - `$0800-$9900`: Text buffer (37K)
  - `$D000-$FFFF` (LC Bank 2): Part of assembler
  - `$7800-$9EFF`: Main assembler code
- **Port**: Dynamic allocation
  - `std::vector<std::string>` for text buffer
  - `std::unordered_map` for symbol table
  - No memory constraints

## Incremental Implementation Steps

### Phase 1: Core Infrastructure (Week 1-2)

- [x] Project scaffolding with CMake
- [x] ncurses Screen wrapper
- [x] Create `include/edasm/constants.hpp` from `COMMONEQUS.S`
- [x] Implement `App` main loop and command dispatcher
- [x] Basic command parser (tokenize on space/comma)
- [x] File type mapping in `ProDOSFile` class

### Phase 2: Editor Module (Week 3-4)

- [x] Implement text buffer as `std::vector<std::string>`
- [x] Line-based commands: LIST, INSERT, DELETE
- [x] File I/O: LOAD, SAVE
- [x] Search: FIND, CHANGE
- [x] Buffer manipulation: MOVE, COPY, JOIN, SPLIT
- [x] CATALOG command (directory listing)
- [x] RENAME, LOCK, UNLOCK, DELETE file commands (from EDITOR1.S)
- [ ] Split buffer mode (two buffers)
- [x] **INSERT mode (interactive line entry from EDITOR3.S)** ✨ NEW!

### Phase 3: Assembler Pass 1 (Week 5-6)

- [x] Tokenizer (split labels, mnemonics, operands)
- [x] Symbol table with hash map
- [x] Label definition tracking
- [x] Forward reference marking
- [x] Expression parser (no evaluation yet)
- [x] Program counter tracking

### Phase 4: Assembler Pass 2 (Week 7-8)

- [x] Opcode table for 6502 instructions
- [x] Addressing mode detection
- [x] Expression evaluator (basic literals and symbols)
- [x] **Expression operators (NEW!)**: +, -, \*, /, &, |, ^ (XOR)
- [x] **Byte extraction operators (NEW!)**: < (low byte), > (high byte)
- [x] **Unary operators (NEW!)**: +, -
- [x] Code generation (emit bytes)
- [x] Binary output (basic with header)
- [x] Enhanced directive handling (DB/DW with multiple comma-separated values)
- [x] Error reporting improvements (line context, better messages)

### Phase 5: Directives & Listing (Week 9-10)

- [x] Directive handlers (ORG, EQU, DA, DB, ASC, DCI, DS, END)
- [x] **Listing file generation** ✨ NEW!
- [x] **Line number formatting (decimal)** ✨ NEW!
- [x] **Code/data hex dump in listing** ✨ NEW!
- [x] **Symbol table printing (Pass 3)** ✨ NEW!
- [x] **Multi-column symbol table (2/4/6 columns)** ✨ NEW!
- [x] **LST directive (listing control ON/OFF)** ✨ NEW!
- [x] **MSB directive (high bit control for ASCII)** ✨ NEW!
- [x] **SBTL directive (listing subtitle)** ✨ NEW!
- [ ] BCD line number format option

### Phase 6: Advanced Features (Week 11-12)

- [x] **EXEC command (read commands from file)** ✨ NEW!
- [x] **REL file format support (relocatable object)** ✨ NEW!
- [x] **ENT/EXT directive support for REL files)** ✨ NEW!
- [x] **REL file generation (RLD + ESD records)** ✨ NEW!
- [x] **Linker implementation (merge REL files)** ✨ NEW!
  - [x] Parse REL file format
  - [x] Build ENTRY and EXTERN symbol tables
  - [x] Resolve external references
  - [x] Relocate code segments
  - [x] Generate BIN/REL/SYS output
  - [x] Load map generation
- [x] **INCLUDE directive support** ✨ NEW!
  - [x] File inclusion during assembly
  - [x] Path resolution (relative/absolute)
  - [x] Nesting prevention (INCLUDE/CHN NESTING error)
  - [x] Error handling (file not found, invalid from include)
  - [x] Line number tracking across includes
- [x] **Conditional assembly (DO/ELSE/FIN/IF\* directives)** ✨ NEW!
  - [x] DO directive (start conditional block)
  - [x] ELSE directive (alternate block)
  - [x] FIN directive (end conditional block)
  - [x] IFEQ directive (if equal to zero)
  - [x] IFNE directive (if not equal to zero)
  - [x] IFGT directive (if greater than zero)
  - [x] IFGE directive (if greater or equal to zero)
  - [x] IFLT directive (if less than zero)
  - [x] IFLE directive (if less or equal to zero)
  - [x] Line skipping when condition is false
  - [x] Expression evaluation for conditions
- [ ] Macro support (if needed)

### Phase 7: Testing & Polish (Week 13-14)

- [x] Test with sample 6502 programs (all 13+ test programs pass)
- [x] Create comprehensive test validating all features together
- [x] Fix critical bugs (DB/DFB/DW/DA value counting in Pass 1)
- [x] Add validation (line range checking, symbol name length)
- [x] Handle edge cases and error conditions (division by zero, undefined symbols)
- [x] Documentation update (README and PORTING_PLAN status)
- [ ] Compare output with original EDASM (would require Apple II or emulator)
- [ ] Performance optimization (deferred - correctness achieved)

## Testing Strategy

1. **Unit tests** for each module:
    - Tokenizer, expression evaluator, symbol table
    - Individual commands (LOAD/SAVE/LIST)
    - Opcode encoding for all addressing modes

2. **Integration tests** with sample sources:
    - Simple programs (hello world, loops)
    - Forward references
    - Complex expressions
    - All directives
    - Multi-file projects (linker)

3. **Comparison tests** against original:
    - Assemble same source on Apple II and C-EDASM
    - Compare binary output byte-by-byte
    - Compare symbol table listings

## Notes

- Preserve numeric formats (hex `$` prefix, decimal, binary `%` prefix) as in original.
- Keep directive names and syntax identical for compatibility.
- Use modern C++ idioms (RAII, smart pointers) while maintaining logical equivalence.
- Error messages should be helpful but can differ from terse Apple II originals.
- Performance is secondary to correctness and maintainability.
