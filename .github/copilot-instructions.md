# C-EDASM AI Guidance

## Project Overview
C++20/ncurses port of Apple II EDASM (6502 editor/assembler). Goal: preserve original EDASM behavior exactly while modernizing with C++20 idioms.

## Architecture & Module Boundaries
- **src/core/**: App loop (`app.cpp`), screen wrapper (`screen.cpp`), CPU emulator (`cpu.cpp`, `bus.cpp`, `traps.cpp`)
- **src/editor/**: Text buffer, line editing commands (INSERT, DELETE, FIND, CHANGE, MOVE, COPY, JOIN, SPLIT)
- **src/assembler/**: Two-pass assembler (`assembler.cpp`), symbol table (`symbol_table.cpp`), expression evaluator (`expression.cpp`), opcode table (`opcode_table.cpp`), listing generator (`listing.cpp`), linker (`linker.cpp`)
- **src/files/**: ProDOS file type mapping (`prodos_file.cpp`)
- **include/edasm/**: Public headers mirror src/ structure; `constants.hpp` defines shared constants from COMMONEQUS.S
- **third_party/EdAsm/**: Git submodule with original 6502 source (EDASM.SRC) - **source of truth** for correctness

## Critical Workflows

### Initial Setup
```bash
# ALWAYS run first - pulls EDASM.SRC submodule (required for verification)
git submodule update --init --recursive

# Install dependencies (Ubuntu/Debian)
sudo apt-get install cmake g++ libncurses5-dev
```

### Build System
```bash
# Configure (out-of-source in build/)
./scripts/configure.sh              # Uses BUILD_TYPE env (default: Debug)
# OR: cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug

# Build
./scripts/build.sh                  # Wrapper for cmake --build build
# OR: cmake --build build

# Binaries produced:
# - ./build/edasm_cli              # Main editor/assembler UI
# - ./build/test_asm               # Standalone assembler test
# - ./build/test_asm_listing       # Assembler with listing output
# - ./build/test_linker            # Linker test
# - ./build/emulator_runner        # 65C02 emulator for EDASM.SYSTEM
```

### Testing
```bash
# Run all tests (ALWAYS run before/after changes)
cd build && ctest

# Run specific test with verbose output
cd build && ctest -V -R test_assembler_integration

# Direct test binary execution
./build/tests/test_editor
./build/tests/test_assembler_integration
./build/tests/test_emulator
```

## Project-Specific Conventions

### 1:1 EDASM Parity (CRITICAL)
- **Command syntax**: Must match original EDASM exactly (e.g., `LIST 10,20` not `LIST 10-20`)
- **Directive names**: Use EDASM names: ORG, EQU, DA, DW, DB, DFB, ASC, DCI, DS, END, LST, MSB, SBTL, ENT, EXT, REL, INCLUDE, DO/ELSE/FIN
- **Operator precedence**: EDASM-specific: `*`, `/` > `+`, `-` > `<`, `>` > `^` (AND), `|` (OR), `!` (XOR)
- **Bitwise operators**: **NON-STANDARD** - `^` = AND, `!` = XOR, `|` = OR (NOT C-style!)
- **Verify against EDASM.SRC**: When implementing features, cross-reference third_party/EdAsm/EDASM.SRC for correct behavior

### Code Style
- C++20 features: Use `std::string_view`, structured bindings, concepts where appropriate
- RAII everywhere: Smart pointers, RAII wrappers for resources (files, buffers)
- Zero-page mapping: Original 6502 zero-page variables ($60-$97) map to class members (see docs/PORTING_PLAN.md §Zero Page Variables)
- Comments: Reference original EDASM.SRC locations (e.g., `// Reference: ASM2.S DoPass1 ($7E1E)`)
- Avoid premature optimization: Correctness > performance (this is not a production assembler)

### File Semantics
ProDOS file types map to Linux extensions:
- TXT ($04) → `.src`, `.txt`
- BIN ($06) → `.bin`, `.obj`
- REL ($FE) → `.rel`
- SYS ($FF) → `.sys`
- Listing → `.lst`

Tables: `src/files/prodos_file.cpp` and `include/edasm/files/prodos_file.hpp`

## Documentation Structure (READ BEFORE CODING)

### Start Here
- **docs/VERIFICATION_QUICK_REF.md**: One-page quick reference for feature verification
- **docs/VERIFICATION_INDEX.md**: Feature-by-feature lookup with EDASM.SRC cross-references
- **docs/MISSING_FEATURES.md**: Explicit list of features NOT ported (BCD line numbers, MACLIB, etc.)

### Implementation Guides
- **docs/PORTING_PLAN.md**: 14-week roadmap, module mapping (6502 → C++), zero-page variable mapping
- **docs/ASSEMBLER_ARCHITECTURE.md**: Two-pass design, symbol table structure, addressing mode detection
- **docs/COMMAND_REFERENCE.md**: Complete command set (LOAD, SAVE, LIST, INSERT, ASM, LINK, etc.)
- **docs/6502_INSTRUCTION_SET.md**: Full 6502 opcode reference with addressing modes

## Integration Points

### Assembler Passes
- **Pass 1** (`Assembler::pass1()`): Build symbol table, track PC, mark forward references
- **Pass 2** (`Assembler::pass2()`): Generate machine code, resolve symbols, emit bytes
- **Listing** (optional): `ListingGenerator` integrates with Pass 2 for line-by-line output

### Symbol Table
- Hash-based lookup (256 buckets in original, `std::unordered_map` in C++)
- Flags: R=relative, E=entry, X=external, U=undefined
- See `SymbolTable` class in `symbol_table.cpp`

### Linker (6-phase)
1. Load REL files
2. Parse RLD (Relocation Dictionary) and ESD (External Symbol Dictionary)
3. Assign load addresses
4. Resolve external references
5. Apply relocations
6. Generate output (BIN, REL, or SYS)

### Emulator (NEW in 2026-01)
- Minimal 65C02 CPU emulator for testing EDASM.SYSTEM binary
- Trap-first discovery: memory prefilled with trap opcode ($02)
- 847 instructions executed before hitting ProDOS MLI trap
- See `docs/EMULATOR_MINIMAL_PLAN.md` for design

## Common Pitfalls

1. **Don't confuse bitwise ops**: `^` is AND in EDASM, not XOR!
2. **Always check submodule**: EDASM.SRC must be present for verification
3. **Build artifacts in build/**: Never commit `build/`, `tmp/`, `*.bin`, `*.lst`, `*.rel` (see `.gitignore`)
4. **Test discovery**: Use manual test programs in `tests/test_*.src` for validation (14 programs, all passing)
5. **Module boundaries**: Keep editor logic in `src/editor/`, assembler in `src/assembler/`, don't mix

## Test Programs
- `tests/test_simple.src`: Basic instructions (LDA, STA, RTS)
- `tests/test_addr_mode.src`: All 9 addressing modes
- `tests/test_expressions.src`: Arithmetic, bitwise, byte extraction
- `tests/test_directives.src`: ORG, EQU, DA, DW, DB, ASC, DCI, DS
- `tests/test_rel.src`: REL file with ENT/EXT directives
- `tests/test_module1.src`, `test_module2.src`: Multi-module linking

## Debugging Tips
- **Assembler issues**: Enable listing output (`Options::generate_listing = true`) to see line-by-line assembly
- **Symbol resolution**: Check `SymbolTable::dump()` output for symbol values and flags
- **Expression evaluation**: Add debug prints in `Expression::evaluate()` to trace operator stack
- **Linker problems**: Use `Linker::generate_load_map()` to see module layout and symbol addresses
