# EDASM (C++/ncurses)

Port of the Apple II EDASM editor/assembler/tools from `markpmlim/EdAsm` to modern C++ targeting Linux with ncurses for screen handling. The goal is to stay close to the original 6502 logic while adapting storage and UI to a terminal environment.

## Status

**Phase: Core Complete, Editor Complete, Assembler Complete, Linker Complete**

- ✅ **Phase 1 Complete**: Core infrastructure with command dispatch, screen handling, and constants
- ✅ **Phase 2 (95%)**: Editor module with all major commands including INSERT mode
- ✅ **Phase 3 Complete**: Assembler tokenizer and symbol table
- ✅ **Phase 4 Complete**: Assembler Pass 2 with expression evaluation and code generation
- ✅ **Phase 5 (95%)**: Listing file generation, symbol table printing, and control directives
- ✅ **Phase 6 (95%)**: EXEC command, REL file support, ENT/EXT directives, **Linker implemented**, **INCLUDE directive support**, **Conditional assembly!**
- ✅ Comprehensive unit tests (100% passing)
- ✅ EdAsm submodule initialized from [markpmlim/EdAsm](https://github.com/markpmlim/EdAsm)
- ✅ EDASM.SRC (~19,000 lines of 6502 assembly) analyzed and documented
- ✅ Porting plan with 14-week roadmap actively being followed
- ⏳ **Phase 6 Next**: Macro support
- ⏳ **Phase 7**: Testing & Polish

## Recent Additions (2026-01-15)

### Conditional Assembly (NEW!) ✨
- **DO/IFNE directive**: Start conditional block, assemble if expression ≠ 0
- **ELSE directive**: Alternate block (assemble if DO condition was false)
- **FIN directive**: End conditional block
- **IFEQ directive**: Assemble if expression = 0
- **IFGT directive**: Assemble if expression > 0 (signed comparison)
- **IFGE directive**: Assemble if expression ≥ 0
- **IFLT directive**: Assemble if expression < 0
- **IFLE directive**: Assemble if expression ≤ 0
- Lines in false blocks are skipped during assembly
- Full expression evaluation support for conditions
- Matches EDASM.SRC behavior from ASM3.S L90B7-L9122

### INCLUDE Directive ✨
- **File inclusion**: Include external source files during assembly
- **Path resolution**: Supports relative and absolute paths, quoted filenames
- **Nesting prevention**: Prevents nested INCLUDE directives (matches original EDASM behavior)
- **Error handling**: Clear error messages for missing files and nesting violations
- **Line tracking**: Maintains proper line number tracking across included files
- **Listing integration**: Included files appear in assembly listings with line numbers

### Linker (NEW!) ✨
- **Complete 6-phase linker** for REL (relocatable) object files
- Links multiple REL files into BIN, REL, or SYS output
- **External reference resolution**: Resolves EXTERN symbols across modules
- **Code relocation**: Adjusts addresses using RLD (Relocation Dictionary)
- **Entry point management**: Handles ENT symbols from multiple modules
- **Load map generation**: Optional detailed linking report
- Successfully tested with multi-module programs

### REL File Support ✨
- **REL directive**: Enable relocatable code mode
- **ENT/ENTRY directive**: Mark symbols as entry points
- **EXT/EXTRN directive**: Declare external symbols
- **RLD Generation**: Relocation Dictionary for address fixups
- **ESD Generation**: External Symbol Dictionary for inter-module linking
- Complete REL file format (code + RLD + ESD)
- Symbol table displays proper flags (R=relative, E=entry, X=external)
- Foundation for multi-module linking

### EXEC Command ✨
- Execute commands from text files
- Commands displayed with "+" prefix during execution
- Automatic return to keyboard mode on EOF
- Nested EXEC support (closes previous file before opening new)

### Control Directives ✨
- **LST directive**: Control listing output (ON/OFF)
- **MSB directive**: Set high bit on ASCII characters (ON/OFF)
- **SBTL directive**: Subtitle for listing sections
- Apple II text mode compatibility (MSB ON for inverse/flash text)

### INSERT Mode
- Interactive line entry with line number prompts
- Empty line exits INSERT mode  
- Lines can be inserted at any position or appended at end
- Displays count of lines inserted on exit

### Listing File Generation
- Complete assembly listings with line numbers, addresses, and hex bytes
- Multi-line hex dumps for instructions/data longer than 3 bytes
- Configurable symbol table at end (2/4/6 columns)
- Sort symbols by name or value
- Proper hex formatting with $ prefix
- Flag indicators (R=relative, X=external, E=entry, U=undefined)

## Documentation

This project includes detailed documentation for the porting effort:

- **[PORTING_PLAN.md](docs/PORTING_PLAN.md)** - Complete porting roadmap with module mapping, architecture analysis, and 14-week implementation plan
- **[COMMAND_REFERENCE.md](docs/COMMAND_REFERENCE.md)** - Full command set documentation (file, editor, assembler, control commands)
- **[ASSEMBLER_ARCHITECTURE.md](docs/ASSEMBLER_ARCHITECTURE.md)** - Two-pass assembler design, symbol table structure, expression evaluation
- **[6502_INSTRUCTION_SET.md](docs/6502_INSTRUCTION_SET.md)** - Complete 6502 opcode reference with addressing modes and C++ implementation guidance

## Build & Run

```bash
# Install dependencies (Ubuntu/Debian)
sudo apt-get install cmake g++ libncurses5-dev

# Build
./scripts/configure.sh
./scripts/build.sh

# Run (currently text-mode editor with basic assembler)
./build/edasm_cli

# Run tests
cd build && ctest
```

## Project Structure

```
C-EDASM/
├── src/
│   ├── core/          # App main loop, screen wrapper
│   ├── editor/        # Text buffer, line editing
│   ├── assembler/     # Two-pass assembler, symbol table
│   └── files/         # ProDOS file type mapping
├── include/edasm/     # Public headers and constants
├── docs/              # Comprehensive documentation
├── tests/             # Unit and integration tests
└── third_party/EdAsm/ # Original EDASM.SRC (submodule)
```

## Dependencies

- CMake (>=3.16)
- C++20 compiler (GCC 10+, Clang 12+)
- ncurses development headers and library

## ProDOS File Type Mapping

The original EDASM used ProDOS file types. In the Linux port, these map to extensions:

| ProDOS Type | Code | Extensions | Description |
|-------------|------|------------|-------------|
| TXT | $04 | `.src`, `.txt` | Source code, text files |
| BIN | $06 | `.bin`, `.obj` | Binary executable |
| REL | $FE | `.rel` | Relocatable object |
| SYS | $FF | `.sys` | System file |
| - | - | `.lst` | Listing output |

## Implemented Features

### Editor Commands (Phase 2 - 95% Complete)
- **File Operations**: LOAD, SAVE, CATALOG (directory listing)
- **Text Editing**: LIST, **INSERT (interactive mode)**, DELETE (line ranges)
- **Search & Replace**: FIND, CHANGE (with pattern matching)
- **Buffer Manipulation**: MOVE, COPY, JOIN, SPLIT
- **Navigation**: Goto line, line range specifications
- **Control**: **EXEC (run commands from file)**, PREFIX (set working directory), HELP

### Assembler (Phases 3-6 - Feature Complete)
- **Two-pass assembly**: Symbol table building and code generation
- **Full 6502 instruction set**: All addressing modes supported
- **Expression evaluation**: Arithmetic (+, -, *, /), bitwise (&, |, ^), byte extraction (<, >)
- **Directives**: ORG, EQU, DA, DW, DB, DFB, ASC, DCI, DS, END
- **REL support**: REL, ENT/ENTRY, EXT/EXTRN directives for relocatable code
- **Control directives**: LST (listing control), MSB (high bit), SBTL (subtitle), **INCLUDE** (file inclusion)
- **Conditional assembly**: DO, ELSE, FIN, IFEQ, IFNE, IFGT, IFGE, IFLT, IFLE directives
- **Binary output**: Generate machine code with header
- **REL file output**: Complete REL format with RLD/ESD for linking
- **Listing generation**: Complete assembly listings with hex dump and symbol table
- **Symbol table**: Multi-column format (2/4/6 columns), sort by name or value, flag display

### Linker (Phase 6 - Complete!)
- **Multi-module linking**: Link REL files into executable binaries
- **Symbol resolution**: Resolve EXTERN references across modules
- **Code relocation**: Apply RLD entries to relocate addresses
- **Load address assignment**: Assign contiguous memory for modules
- **Multiple output formats**: BIN (binary), REL (relocatable), SYS (system)
- **Load map generation**: Detailed report of modules, symbols, and addresses
- Successfully links multi-module 6502 programs

### System Features (Phase 1 - Complete)
- Command-line interface with dispatch table
- ProDOS file type mapping (TXT, BIN, REL, SYS)
- Line-based text buffer (std::vector<string>)
- Comprehensive constants from COMMONEQUS.S
- ncurses screen wrapper

### Testing
- Comprehensive unit tests for all editor operations
- Test coverage: LineRange parsing, FIND, CHANGE, MOVE, COPY, JOIN, SPLIT
- Assembler tests with sample 6502 programs
- All tests passing (100%)

## Examples

### Assemble with Listing
```bash
# Assemble and generate listing file
./build/test_asm_listing source.src output.lst

# Assemble only (no listing)
./build/test_asm source.src
```

### Example Listing Output
```
Line# Addr  Bytes        Source
----- ----  ----------   ---------------------------
0001                     ; Simple 6502 program
0004  0800  A9 00        START   LDA #$00
0005  0802  8D 00 04             STA $0400
0007  0807  CA           LOOP    DEX
0008  0808  D0 FD                BNE LOOP

Symbol Table (by name):
============================================================
LOOP             $0807 RSTART            $0800 R
```

## Implementation Roadmap

See [PORTING_PLAN.md](docs/PORTING_PLAN.md) for the complete 14-week implementation plan:

1. ✅ **Weeks 1-2**: Core infrastructure (command parser, file I/O) - **COMPLETE**
2. ✅ **Weeks 3-4**: Editor module (text buffer, commands) - **95% COMPLETE**
3. ✅ **Weeks 5-6**: Assembler Pass 1 (tokenizer, symbol table) - **COMPLETE**
4. ✅ **Weeks 7-8**: Assembler Pass 2 (code generation, expressions) - **COMPLETE**
5. ✅ **Weeks 9-10**: Directives & listing generation - **95% COMPLETE**
6. ✅ **Weeks 11-12**: Advanced features (REL format, linker, EXEC) - **85% COMPLETE**
7. **Weeks 13-14**: Testing & polish - **IN PROGRESS**

## Original EDASM Structure

The original Apple II EDASM consists of:

- **EI** (EDASM Interpreter) - Command dispatch, module loading
- **EDITOR** - Text buffer, line editing, file commands
- **ASM** (Assembler) - Two-pass 6502 assembler with symbol table
- **LINKER** - Links relocatable object files
- **COMMONEQUS.S** - Shared constants and definitions

See submodule at `third_party/EdAsm/EDASM.SRC/` for complete 6502 source.

## Contributing

This is a faithful port aiming to preserve EDASM semantics while using modern C++ idioms. Key principles:

- Keep command syntax identical for compatibility
- Preserve directive names and behavior
- Use modern C++ (RAII, smart pointers) while maintaining logical equivalence
- Correctness and maintainability over performance
