# EDASM (C++/ncurses)

Port of the Apple II EDASM editor/assembler/tools from `markpmlim/EdAsm` to modern C++ targeting Linux with ncurses for screen handling. The goal is to stay close to the original 6502 logic while adapting storage and UI to a terminal environment.

## Status

**Phase: Planning & Documentation Complete**

- ✅ Workspace scaffolded with CMake, ncurses dependency, and module structure
- ✅ EdAsm submodule initialized from [markpmlim/EdAsm](https://github.com/markpmlim/EdAsm)
- ✅ EDASM.SRC (~19,000 lines of 6502 assembly) analyzed and documented
- ✅ Comprehensive porting plan created with 14-week implementation roadmap
- ⏳ Core logic to be ported from `EDASM.SRC` 6502 assembly

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
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build

# Run (currently placeholder)
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

## Implementation Roadmap

See [PORTING_PLAN.md](docs/PORTING_PLAN.md) for the complete 14-week implementation plan:

1. **Weeks 1-2**: Core infrastructure (command parser, file I/O)
2. **Weeks 3-4**: Editor module (text buffer, commands)
3. **Weeks 5-6**: Assembler Pass 1 (tokenizer, symbol table)
4. **Weeks 7-8**: Assembler Pass 2 (code generation, expressions)
5. **Weeks 9-10**: Directives & listing generation
6. **Weeks 11-12**: Advanced features (REL format, linker)
7. **Weeks 13-14**: Testing & polish

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
