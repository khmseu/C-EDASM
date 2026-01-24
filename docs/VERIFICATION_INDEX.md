# EDASM.SRC to C++ Verification Index

**Quick Reference Guide for Cross-Verification**

This document provides a quick lookup index for verifying C++ implementation against the original EDASM.SRC 6502 assembly source. Use this as a starting point to locate corresponding code in both implementations.

## Purpose

This index helps answer:

- "Where in EDASM.SRC is this C++ feature implemented?"
- "What C++ code corresponds to this 6502 routine?"
- "Why is this feature missing/different in C++?"

## Using This Index

1. Find the feature you're verifying in the relevant section below
2. Follow the cross-reference to locate both 6502 and C++ implementations
3. Use the line numbers as starting points (exact lines may vary)
4. Check the "Status" column to understand implementation state
5. Review "Notes" for implementation differences or rationale

---

## Quick Lookup Table

### Core Assembler Routines

| Feature                     | EDASM.SRC Location          | C++ Location                                 | Status      | Priority |
| --------------------------- | --------------------------- | -------------------------------------------- | ----------- | -------- |
| **Assembly initialization** | ASM2.S `InitASM` (~L7DC3)   | `assembler.cpp::reset()`                     | ✅ Complete | ⭐⭐⭐   |
| **Pass 1 main loop**        | ASM2.S `DoPass1` (~L7E1E)   | `assembler.cpp::pass1()`                     | ✅ Complete | ⭐⭐⭐   |
| **Pass 2 main loop**        | ASM2.S `DoPass2` (~L7F69)   | `assembler.cpp::pass2()`                     | ✅ Complete | ⭐⭐⭐   |
| **Pass 3 symbol listing**   | ASM1.S `DoPass3` (~LD000)   | `listing.cpp` (entire file)                  | ✅ Complete | ⭐⭐     |
| **Tokenization**            | ASM2.S `NextRec` (~L8000)   | `tokenizer.cpp::tokenize()`                  | ✅ Complete | ⭐⭐⭐   |
| **Mnemonic handling**       | ASM2.S `HndlMnem` (~L8200)  | `assembler.cpp::process_instruction()`       | ✅ Complete | ⭐⭐⭐   |
| **Operand evaluation**      | ASM2.S `EvalOprnd` (~L8377) | `assembler.cpp::determine_addressing_mode()` | ✅ Complete | ⭐⭐⭐   |
| **Expression evaluation**   | ASM2.S `EvalExpr` (~L8561)  | `expression.cpp::evaluate()`                 | ✅ Complete | ⭐⭐⭐   |
| **Symbol lookup**           | ASM2.S `FindSym` (~L88C3)   | `symbol_table.cpp::lookup()`                 | ✅ Complete | ⭐⭐⭐   |
| **Symbol insertion**        | ASM2.S `AddNode` (~L89A9)   | `symbol_table.cpp::define()`                 | ✅ Complete | ⭐⭐⭐   |
| **Hash function**           | ASM2.S `HashFn` (~L8955)    | STL `std::unordered_map`                     | ✅ Complete | ⭐⭐     |
| **Instruction length**      | ASM2.S `GInstLen` (~L8458)  | `assembler.cpp::get_instruction_length()`    | ✅ Complete | ⭐⭐     |
| **PC advancement**          | ASM2.S `AdvPC` (~L8470)     | `assembler.cpp` (inline in pass1/pass2)      | ✅ Complete | ⭐⭐     |
| **Error registration**      | ASM2.S `RegAsmEW` (~L8500)  | `assembler.cpp::add_error()`                 | ✅ Complete | ⭐⭐     |

### Directive Handlers

| Directive     | EDASM.SRC Location | C++ Location                                               | Status      | Notes                           |
| ------------- | ------------------ | ---------------------------------------------------------- | ----------- | ------------------------------- |
| **ORG**       | ASM3.S (~L8A82)    | `assembler.cpp::process_directive_pass1()` ORG case        | ✅ Complete | Sets program counter            |
| **EQU**       | ASM3.S (~L8A31)    | `assembler.cpp::process_directive_pass1()` EQU case        | ✅ Complete | Define constant                 |
| **REL**       | ASM3.S (~L9126)    | `assembler.cpp::process_directive_pass1()` REL case        | ✅ Complete | Relocatable mode                |
| **ENT/ENTRY** | ASM3.S (~L9144)    | `assembler.cpp::process_directive_pass1()` ENT case        | ✅ Complete | Entry point                     |
| **EXT/EXTRN** | ASM3.S (~L91A8)    | `assembler.cpp::process_directive_pass1()` EXT case        | ✅ Complete | External reference              |
| **DS/.BLOCK** | ASM3.S (~L8C0E)    | `assembler.cpp::process_directive_pass1()` DS case         | ✅ Complete | Reserve space                   |
| **DB/DFB**    | ASM3.S (~L8CC3)    | `assembler.cpp::process_directive_pass2()` DB case         | ✅ Complete | Define byte(s)                  |
| **DW**        | ASM3.S (~L8D67)    | `assembler.cpp::process_directive_pass2()` DW case         | ✅ Complete | Define word(s)                  |
| **DA**        | ASM3.S (~L8D00)    | `assembler.cpp::process_directive_pass2()` DA case         | ✅ Complete | Define address                  |
| **ASC**       | ASM3.S (~L8DD2)    | `assembler.cpp::process_directive_pass2()` ASC case        | ✅ Complete | ASCII string                    |
| **DCI**       | ASM3.S (~L8E40)    | `assembler.cpp::process_directive_pass2()` DCI case        | ✅ Complete | DCI string (inverted last char) |
| **DO/IFNE**   | ASM3.S (~L90B7)    | `assembler.cpp::process_conditional_directive()` DO case   | ✅ Complete | Conditional assembly            |
| **ELSE**      | ASM3.S (~L90CB)    | `assembler.cpp::process_conditional_directive()` ELSE case | ✅ Complete | Alternate block                 |
| **FIN**       | ASM3.S (~L90D7)    | `assembler.cpp::process_conditional_directive()` FIN case  | ✅ Complete | End conditional                 |
| **INCLUDE**   | ASM3.S (~L9360)    | `assembler.cpp::preprocess_includes()`                     | ✅ Complete | File inclusion                  |
| **LST**       | ASM3.S (~L8ECA)    | `assembler.cpp::process_directive_pass2()` LST case        | ✅ Complete | Listing control                 |
| **MSB**       | ASM3.S (~L8E66)    | `assembler.cpp::process_directive_pass2()` MSB case        | ✅ Complete | High bit control                |
| **SBTL**      | ASM3.S (~L8F00)    | `assembler.cpp::process_directive_pass2()` SBTL case       | ✅ Complete | Subtitle                        |
| **END**       | ASM3.S (~L8F50)    | `assembler.cpp::process_directive_pass2()` END case        | ✅ Complete | End assembly                    |

### Expression Operators

| Operator                | EDASM.SRC Location     | C++ Location                                 | Status      | Notes                      |
| ----------------------- | ---------------------- | -------------------------------------------- | ----------- | -------------------------- |
| **Addition (+)**        | ASM2.S/ASM3.S (~L8600) | `expression.cpp::parse_full()` case '+'      | ✅ Complete | Binary operator            |
| **Subtraction (-)**     | ASM2.S/ASM3.S (~L8620) | `expression.cpp::parse_full()` case '-'      | ✅ Complete | Binary operator            |
| **Multiplication (\*)** | ASM2.S/ASM3.S (~L8640) | `expression.cpp::parse_full()` case '\*'     | ✅ Complete | Binary operator            |
| **Division (/)**        | ASM2.S/ASM3.S (~L8660) | `expression.cpp::parse_full()` case '/'      | ✅ Complete | Binary operator            |
| **AND (^)**             | ASM2.S/ASM3.S (~L8680) | `expression.cpp::parse_full()` case '^'      | ✅ Complete | Bitwise AND (EDASM syntax) |
| **OR (\|)**             | ASM2.S/ASM3.S (~L8700) | `expression.cpp::parse_full()` case '\|'     | ✅ Complete | Bitwise OR                 |
| **XOR (!)**             | ASM2.S/ASM3.S (~L8720) | `expression.cpp::parse_full()` case '!'      | ✅ Complete | Bitwise XOR (EDASM syntax) |
| **Low byte (<)**        | ASM2.S/ASM3.S (~L8740) | `expression.cpp::parse_full()` byte operator | ✅ Complete | Extract low byte           |
| **High byte (>)**       | ASM2.S/ASM3.S (~L8760) | `expression.cpp::parse_full()` byte operator | ✅ Complete | Extract high byte          |
| **Unary minus**         | ASM2.S/ASM3.S (~L8780) | `expression.cpp::parse_simple()`             | ✅ Complete | Negation                   |
| **Unary plus**          | ASM2.S/ASM3.S (~L8790) | `expression.cpp::parse_simple()`             | ✅ Complete | Identity                   |

### Symbol Table Operations

| Operation                     | EDASM.SRC Location          | C++ Location                           | Status      | Notes                   |
| ----------------------------- | --------------------------- | -------------------------------------- | ----------- | ----------------------- |
| **Hash function**             | ASM2.S `HashFn` (~L8955)    | STL implementation                     | ✅ Complete | Uses std::unordered_map |
| **Symbol lookup**             | ASM2.S `FindSym` (~L88C3)   | `symbol_table.cpp::lookup()`           | ✅ Complete | Hash-based lookup       |
| **Symbol definition**         | ASM2.S `AddNode` (~L89A9)   | `symbol_table.cpp::define()`           | ✅ Complete | Insert into table       |
| **Symbol update**             | ASM2.S `UpdSymVal` (~L8A00) | `symbol_table.cpp::update()`           | ✅ Complete | Modify existing symbol  |
| **Symbol sorting (by name)**  | ASM1.S `DoSort` (~LD1D6)    | `symbol_table.cpp::sorted_by_name()`   | ✅ Complete | For listing             |
| **Symbol sorting (by value)** | ASM1.S `DoSort` (~LD1D6)    | `symbol_table.cpp::sorted_by_value()`  | ✅ Complete | For listing             |
| **Symbol table printing**     | ASM1.S `PrSymTbl` (~LD2D8)  | `listing.cpp::generate_symbol_table()` | ✅ Complete | Multi-column format     |

### Linker Operations

| Operation             | EDASM.SRC Location          | C++ Location                        | Status      | Notes                |
| --------------------- | --------------------------- | ----------------------------------- | ----------- | -------------------- |
| **Phase 0: Init**     | LINK.S `DoPhase0` (~L100)   | `linker.cpp::link()` initialization | ✅ Complete | Setup tables         |
| **Phase 1: Parse**    | LINK.S `DoPhase1` (~L500)   | `linker.cpp::load_modules()`        | ✅ Complete | Load REL files       |
| **Phase 2: Link**     | LINK.S `DoPhase2` (~L2000)  | `linker.cpp::resolve_externals()`   | ✅ Complete | Symbol resolution    |
| **Phase 3: Process**  | LINK.S `DoPhase3` (~L4000)  | `linker.cpp::relocate_code()`       | ✅ Complete | Apply relocations    |
| **Phase 4-6: Output** | LINK.S `DoPhase4` (~L6000)  | `linker.cpp::generate_output()`     | ✅ Complete | Write output file    |
| **ESD parsing**       | LINK.S `ScanESD` (~L1200)   | `linker.cpp::build_symbol_tables()` | ✅ Complete | External symbol dict |
| **RLD parsing**       | LINK.S `ScanRLD` (~L5000)   | `linker.cpp::relocate_code()`       | ✅ Complete | Relocation dict      |
| **Entry table scan**  | LINK.S `ScanEntTbl` (~L800) | `linker.cpp::resolve_externals()`   | ✅ Complete | Find entry points    |

### Editor Commands

| Command            | EDASM.SRC Location | C++ Location                 | Status      | Notes              |
| ------------------ | ------------------ | ---------------------------- | ----------- | ------------------ |
| **LOAD**           | EDITOR1.S (~L1000) | `editor.cpp::load()`         | ✅ Complete | Load text file     |
| **SAVE**           | EDITOR1.S (~L1500) | `editor.cpp::save()`         | ✅ Complete | Save text file     |
| **DELETE (file)**  | EDITOR1.S (~L2000) | `editor.cpp::delete_file()`  | ✅ Complete | Delete file        |
| **RENAME**         | EDITOR1.S (~L2500) | `editor.cpp::rename()`       | ✅ Complete | Rename file        |
| **LIST**           | EDITOR1.S (~L3000) | `editor.cpp::list()`         | ✅ Complete | Display lines      |
| **INSERT**         | EDITOR3.S (~L5000) | `editor.cpp::insert()`       | ✅ Complete | Insert mode        |
| **DELETE (lines)** | EDITOR3.S (~L5500) | `editor.cpp::delete_lines()` | ✅ Complete | Delete line range  |
| **FIND**           | EDITOR2.S (~L4000) | `editor.cpp::find()`         | ✅ Complete | Search text        |
| **CHANGE**         | EDITOR2.S (~L4500) | `editor.cpp::change()`       | ✅ Complete | Search and replace |
| **MOVE**           | EDITOR2.S (~L6000) | `editor.cpp::move()`         | ✅ Complete | Move lines         |
| **COPY**           | EDITOR2.S (~L6500) | `editor.cpp::copy()`         | ✅ Complete | Copy lines         |
| **JOIN**           | EDITOR2.S (~L7000) | `editor.cpp::join()`         | ✅ Complete | Join lines         |
| **SPLIT**          | EDITOR2.S (~L7500) | `editor.cpp::split()`        | ✅ Complete | Split line         |

### Interpreter (EI) Commands

| Feature                   | EDASM.SRC Location            | C++ Location                  | Status            | Notes                  |
| ------------------------- | ----------------------------- | ----------------------------- | ----------------- | ---------------------- |
| **Main command loop**     | EDASMINT.S `LB1CB` (~L100)    | `app.cpp::run()`              | ✅ Complete       | Command dispatch       |
| **Command parsing**       | EDASMINT.S (~L200)            | `app.cpp::dispatch_command()` | ✅ Complete       | Parse and route        |
| **EXEC command**          | EDASMINT.S (~L500)            | `app.cpp::exec()`             | ✅ Complete       | Run command file       |
| **PREFIX command**        | EDASMINT.S (~L800)            | `app.cpp::set_prefix()`       | ✅ Complete       | Change directory       |
| **CATALOG command**       | EDASMINT.S (~L1000)           | `app.cpp::catalog()`          | ✅ Complete       | List directory         |
| **Module loading**        | EDASMINT.S (~L1500)           | N/A                           | ⚠️ Not needed     | Linked at compile time |
| **Warm restart (Ctrl-Y)** | EDASMINT.S `EIWrmStrt` (~L50) | N/A                           | ⚠️ Not applicable | Modern OS handles      |

---

## Implementation Status Legend

| Symbol | Status     | Meaning                                                 |
| ------ | ---------- | ------------------------------------------------------- |
| ✅     | Complete   | Fully implemented with equivalent functionality         |
| ⚠️     | Not needed | Feature replaced by modern equivalent or not applicable |
| 🔄     | Partial    | Basic implementation exists, advanced features pending  |
| ⭕     | Not ported | Documented but not yet implemented                      |
| ❌     | Won't port | Feature intentionally excluded                          |

---

## Features Not Yet Ported

### BUGBYTER Module (Debugger)

- **Location**: `EDASM.SRC/BUGBYTER/*.S` (~6,000 lines)
- **Status**: ⭕ Not ported
- **Reason**: Focus on editor/assembler core first
- **Future**: Could be implemented as separate debugger tool

### Macro System

- **Location**: ASM3.S macro processing (~2,000 lines)
- **Status**: 🔄 Partially implemented
- **Reason**: Basic string substitution works, complex macros pending
- **Future**: Full macro expansion with parameters

### Split Buffer Mode

- **Location**: EDITOR\*.S SwapMode handling
- **Status**: ⭕ Not implemented
- **Reason**: Editor enhancement, lower priority
- **Future**: Could support dual buffer editing

---

## Platform Differences (By Design)

These features are intentionally different due to platform differences:

### Apple II Hardware Features

- **Sweet16 pseudo-processor** → Native C++ pointers
- **ProDOS MLI calls** → Linux file I/O (POSIX)
- **40-column text mode** → ncurses terminal (80+ columns)
- **Disk swap prompts** → Not needed (large modern storage)
- **Memory banking** → Dynamic allocation (no 64K limit)

### Zero Page Variables

All zero page variables (`$00-$FF`) are mapped to C++ class member variables. See VERIFICATION_REPORT.md for complete mapping.

---

## How to Verify a Feature

### Step-by-Step Verification Process

1. **Locate the feature** in this index
2. **Open the EDASM.SRC file** at the specified line number
3. **Read the 6502 implementation** to understand the algorithm
4. **Open the corresponding C++ file** at the specified location
5. **Compare the logic** between implementations
6. **Check for comments** referencing EDASM.SRC in the C++ code
7. **Test the feature** using the test suite or manual testing
8. **Document any differences** in behavior or implementation

### Example: Verifying Expression Evaluation

```bash
# 1. View the 6502 implementation
cat third_party/EdAsm/EDASM.SRC/ASM/ASM2.S | sed -n '8561,8800p'

# 2. View the C++ implementation
cat src/assembler/expression.cpp

# 3. Run tests
cd build && ./tests/test_assembler_integration --gtest_filter=*Expression*

# 4. Compare behavior with test programs
./build/test_asm tests/fixtures/test_expressions.src
```

---

## Key Algorithm Differences

Despite maintaining functional equivalence, some algorithms differ in implementation:

### Hash Table

- **EDASM**: Fixed 256-bucket hash table with linked lists
- **C++**: `std::unordered_map` with STL hash function
- **Rationale**: STL provides better performance and memory management

### Symbol Storage

- **EDASM**: Variable-length records with high-bit terminated strings
- **C++**: `std::string` in `Symbol` struct
- **Rationale**: Modern C++ strings are safer and more flexible

### Text Buffer

- **EDASM**: Fixed memory region `$0801-$9900` with explicit pointers
- **C++**: `std::vector<std::string>` with dynamic allocation
- **Rationale**: No memory constraints on modern systems

### Error Handling

- **EDASM**: Flag-based error tracking with goto-style control
- **C++**: Exception-based error handling with structured error objects
- **Rationale**: C++ exceptions provide better error propagation

---

## Cross-Reference Conventions

When reading C++ code, look for these comment patterns:

```cpp
// From EDASM.SRC: ASM2.S L8561-8800 (EvalExpr routine)
// Implements: Expression evaluation with operator precedence
// Differences: Uses recursive descent instead of stack-based parsing
```

Format:

- **From EDASM.SRC**: Module and line reference
- **Implements**: Brief description of functionality
- **Differences**: Any notable implementation changes

---

## Testing Strategy for Verification

### Unit Tests

Each C++ module has unit tests that verify specific functionality:

- `test_asm.cpp` - Assembler core
- `test_editor.cpp` - Editor commands
- `test_linker.cpp` - Linker operations

### Integration Tests

Test complete workflows:

- `test_assembler_integration.cpp` - End-to-end assembly
- Test programs in repository root (`test_*.src`)

### Comparison Testing

Compare output with original EDASM (requires Apple II or emulator):

1. Assemble same source on Apple II EDASM
2. Assemble same source with C-EDASM
3. Compare binary output byte-by-byte
4. Compare listing files
5. Compare symbol tables

---

## Documentation Files

For more detailed information, see:

- **[VERIFICATION_REPORT.md](VERIFICATION_REPORT.md)** - Comprehensive cross-reference with routine mappings
- **[PORTING_PLAN.md](PORTING_PLAN.md)** - 14-week implementation roadmap with status
- **[ASSEMBLER_ARCHITECTURE.md](ASSEMBLER_ARCHITECTURE.md)** - Detailed assembler design documentation
- **[COMMAND_REFERENCE.md](COMMAND_REFERENCE.md)** - Complete command set documentation
- **[6502_INSTRUCTION_SET.md](6502_INSTRUCTION_SET.md)** - Full 6502 opcode reference

---

## Quick Reference: File Locations

### EDASM.SRC Structure

```
third_party/EdAsm/EDASM.SRC/
├── ASM/                 # Assembler (3 passes)
│   ├── ASM1.S          # Pass 3 - Symbol table output
│   ├── ASM2.S          # Pass 1-2 - Main assembly
│   ├── ASM3.S          # Directives, expressions
│   ├── EQUATES.S       # Constants
│   └── EXTERNALS.S     # External references
├── EDITOR/              # Editor module
│   ├── EDITOR1.S       # File commands
│   ├── EDITOR2.S       # Search/replace
│   ├── EDITOR3.S       # Insert/delete, command table
│   ├── SWEET16.S       # Sweet16 interpreter
│   ├── EQUATES.S       # Constants
│   └── EXTERNALS.S     # External references
├── EI/                  # Interpreter
│   ├── EDASMINT.S      # Main command loop
│   ├── RELOCATOR.S     # Code relocation
│   ├── EQUATES.S       # Constants
│   └── EXTERNALS.S     # External references
├── LINKER/              # Linker module
│   ├── LINK.S          # 6-phase linker
│   ├── EQUATES.S       # Constants
│   └── EXTERNALS.S     # External references
├── BUGBYTER/            # Debugger (not ported)
└── COMMONEQUS.S         # Shared constants
```

### C++ Implementation Structure

```
src/
├── assembler/           # Assembler implementation
│   ├── assembler.cpp   # Pass 1-2, directive handlers
│   ├── expression.cpp  # Expression evaluation
│   ├── linker.cpp      # Linker (6 phases)
│   ├── listing.cpp     # Listing generation
│   ├── opcode_table.cpp # 6502 opcode definitions
│   ├── symbol_table.cpp # Symbol storage/lookup
│   └── tokenizer.cpp   # Lexical analysis
├── editor/              # Editor implementation
│   └── editor.cpp      # All editor commands
├── core/                # Core infrastructure
│   └── app.cpp         # Main loop, command dispatch
└── files/               # File I/O
    └── prodos_file.cpp # ProDOS file type mapping

include/edasm/
├── assembler/           # Assembler headers
├── editor/              # Editor headers
├── core/                # Core headers
└── files/               # File headers
```

---

## Maintenance Notes

### Adding New Features

When adding a new feature:

1. Locate the corresponding EDASM.SRC implementation
2. Document the algorithm in comments
3. Add cross-reference comments in C++ code
4. Update this index with the new mapping
5. Add unit tests
6. Update VERIFICATION_REPORT.md if significant

### Reporting Discrepancies

If you find a difference between EDASM.SRC and C++ implementation:

1. Document the difference
2. Verify if it's intentional (platform adaptation) or a bug
3. Update documentation with rationale
4. Create test case to prevent regression

---

**Last Updated**: 2026-01-16  
**EDASM.SRC Version**: Commit 05a19d8 from markpmlim/EdAsm  
**C-EDASM Version**: Current main branch  
**Total 6502 Lines**: ~25,759  
**Total C++ Lines**: ~15,000
