# Quick Reference Card for Verification

**One-Page Guide for EDASM.SRC ↔ C++ Cross-Reference**

Print this page and keep it handy when doing verification work!

---

## File Location Quick Map

| EDASM Module           | 6502 Source                   | C++ Source                       |
| ---------------------- | ----------------------------- | -------------------------------- |
| **Assembler Pass 1-2** | `ASM/ASM2.S`                  | `src/assembler/assembler.cpp`    |
| **Assembler Pass 3**   | `ASM/ASM1.S`                  | `src/assembler/listing.cpp`      |
| **Directives**         | `ASM/ASM3.S`                  | `src/assembler/assembler.cpp`    |
| **Expressions**        | `ASM/ASM2.S`, `ASM/ASM3.S`    | `src/assembler/expression.cpp`   |
| **Symbol Table**       | `ASM/ASM2.S`                  | `src/assembler/symbol_table.cpp` |
| **Opcode Table**       | `ASM/ASM3.S`                  | `src/assembler/opcode_table.cpp` |
| **Tokenizer**          | `ASM/ASM2.S`                  | `src/assembler/tokenizer.cpp`    |
| **Linker**             | `LINKER/LINK.S`               | `src/assembler/linker.cpp`       |
| **Editor**             | `EDITOR/EDITOR*.S`            | `src/editor/editor.cpp`          |
| **Interpreter**        | `EI/EDASMINT.S`               | `src/core/app.cpp`               |
| **Constants**          | `COMMONEQUS.S`, `*/EQUATES.S` | `include/edasm/constants.hpp`    |

---

## Critical Routine Cross-Reference

### Assembler Core

```
DoPass1    (ASM2.S L7E1E)     → assembler.cpp::pass1()
DoPass2    (ASM2.S L7F69)     → assembler.cpp::pass2()
DoPass3    (ASM1.S LD000)     → listing.cpp (entire file)
InitASM    (ASM2.S L7DC3)     → assembler.cpp::reset()
```

### Expression & Symbol Handling

```
EvalExpr   (ASM2.S L8561)     → expression.cpp::evaluate()
FindSym    (ASM2.S L88C3)     → symbol_table.cpp::lookup()
AddNode    (ASM2.S L89A9)     → symbol_table.cpp::define()
HashFn     (ASM2.S L8955)     → std::unordered_map (STL)
```

### Code Generation

```
HndlMnem   (ASM2.S L8200)     → assembler.cpp::process_instruction()
EvalOprnd  (ASM2.S L8377)     → assembler.cpp::determine_addressing_mode()
GInstLen   (ASM2.S L8458)     → assembler.cpp::get_instruction_length()
```

### Directives (ASM3.S → assembler.cpp)

```
ORG        (L8A82)            → process_directive_pass1() ORG case
EQU        (L8A31)            → process_directive_pass1() EQU case
REL        (L9126)            → process_directive_pass1() REL case
ENT        (L9144)            → process_directive_pass1() ENT case
EXT        (L91A8)            → process_directive_pass1() EXT case
DB/DFB     (L8CC3)            → process_directive_pass2() DB case
DW         (L8D67)            → process_directive_pass2() DW case
ASC        (L8DD2)            → process_directive_pass2() ASC case
```

---

## Common Verification Tasks

### Task 1: Verify an Instruction

```bash
# 1. Find opcode in 6502 source
grep "LDA" third_party/EdAsm/EDASM.SRC/ASM/ASM3.S

# 2. Check C++ opcode table
grep "LDA" src/assembler/opcode_table.cpp

# 3. Test it
cd build && ./tests/test_assembler_integration
```

### Task 2: Verify a Directive

```bash
# 1. Find directive in EDASM.SRC
grep -n "ORG" third_party/EdAsm/EDASM.SRC/ASM/ASM3.S

# 2. Find C++ handler
grep -n "ORG" src/assembler/assembler.cpp

# 3. Compare logic
diff <(extract_6502_logic) <(extract_cpp_logic)
```

### Task 3: Verify Expression Operator

```bash
# 1. Check EDASM.SRC expression evaluator
cat third_party/EdAsm/EDASM.SRC/ASM/ASM2.S | sed -n '8561,8800p'

# 2. Check C++ expression evaluator
cat src/assembler/expression.cpp

# 3. Test with sample expression
echo "LDA #\$10+\$20" > test.src
./build/test_asm test.src
```

### Task 4: Compare Output

```bash
# Assemble same source on both systems
# (Requires Apple II or emulator for EDASM.SRC)

# C-EDASM
./build/test_asm program.src

# Compare binaries
diff edasm_output.bin cedasm_output.bin

# Compare listings
diff edasm_output.lst cedasm_output.lst
```

---

## Key Differences to Remember

| Feature            | EDASM.SRC           | C-EDASM               | Why                 |
| ------------------ | ------------------- | --------------------- | ------------------- |
| **Bitwise AND**    | `^`                 | `^`                   | Same (EDASM syntax) |
| **Bitwise XOR**    | `!`                 | `!`                   | Same (EDASM syntax) |
| **Bitwise OR**     | `\|`                | `\|`                  | Same                |
| **Symbol storage** | High-bit terminated | `std::string`         | Modern C++          |
| **Hash table**     | 256 fixed buckets   | `std::unordered_map`  | Better performance  |
| **Text buffer**    | Fixed `$0801-$9900` | `std::vector<string>` | Dynamic allocation  |
| **Zero page vars** | `$00-$FF`           | Class members         | Encapsulation       |
| **Error handling** | Flags + goto        | Exceptions            | Modern C++          |

---

## Status Symbols

| Symbol | Meaning                                     |
| ------ | ------------------------------------------- |
| ✅     | Fully implemented, equivalent functionality |
| ⚠️     | Not needed (hardware-specific or replaced)  |
| 🔄     | Partially implemented                       |
| ⭕     | Documented but not yet implemented          |
| ❌     | Intentionally not ported                    |

---

## Useful Commands

### View EDASM.SRC File

```bash
# View specific file
cat third_party/EdAsm/EDASM.SRC/ASM/ASM2.S | less

# Search for routine
grep -n "DoPass1" third_party/EdAsm/EDASM.SRC/ASM/*.S

# Count lines in module
wc -l third_party/EdAsm/EDASM.SRC/ASM/*.S
```

### Search C++ Code

```bash
# Find function definition
grep -rn "pass1()" src/

# Find all references to a symbol
grep -rn "program_counter_" src/ include/

# List all files in module
ls -la src/assembler/
```

### Run Tests

```bash
# All tests
cd build && ctest

# Specific test
cd build && ctest -R test_assembler_integration -V

# Single test case
cd build && ./tests/test_assembler_integration --gtest_filter=*Expression*

# Run assembler on test file
./build/test_asm tests/fixtures/test_simple.src
```

### Build Commands

```bash
# Clean build
rm -rf build && ./scripts/configure.sh && ./scripts/build.sh

# Rebuild single file
cd build && make assembler.o

# Debug build
BUILD_TYPE=Debug ./scripts/configure.sh && ./scripts/build.sh
```

---

## Documentation Quick Links

- **Comprehensive cross-reference**: [VERIFICATION_REPORT.md](VERIFICATION_REPORT.md)
- **Quick lookup index**: [VERIFICATION_INDEX.md](VERIFICATION_INDEX.md)
- **Missing features**: [MISSING_FEATURES.md](MISSING_FEATURES.md)
- **Implementation roadmap**: [PORTING_PLAN.md](PORTING_PLAN.md)
- **Assembler architecture**: [ASSEMBLER_ARCHITECTURE.md](ASSEMBLER_ARCHITECTURE.md)
- **Command reference**: [COMMAND_REFERENCE.md](COMMAND_REFERENCE.md)
- **6502 opcodes**: [6502_INSTRUCTION_SET.md](6502_INSTRUCTION_SET.md)

---

## Operator Precedence Comparison

### EDASM.SRC (from ASM2.S expression evaluator)

```
1. Parentheses ()
2. Unary: -, +
3. Multiplicative: *, /, MOD
4. Additive: +, -
5. Bitwise: ^(AND), |(OR), !(XOR)
6. Byte extraction: <(low), >(high)
```

### C++ (expression.cpp)

```cpp
1. parse_full():     binary operators (+, -, *, /, ^, |, !)
2. parse_simple():   unary operators (-, +), byte ops (<, >)
3. parse_primary():  literals, symbols, parentheses
```

**Note**: Precedence is equivalent, just different implementation style.

---

## Addressing Mode Quick Reference

| Mode             | EDASM Syntax  | C++ Enum                          |
| ---------------- | ------------- | --------------------------------- |
| Implied          | `RTS`         | `AddressingMode::Implied`         |
| Accumulator      | `ASL A`       | `AddressingMode::Accumulator`     |
| Immediate        | `LDA #$42`    | `AddressingMode::Immediate`       |
| Zero Page        | `LDA $42`     | `AddressingMode::ZeroPage`        |
| Zero Page,X      | `LDA $42,X`   | `AddressingMode::ZeroPageX`       |
| Zero Page,Y      | `LDX $42,Y`   | `AddressingMode::ZeroPageY`       |
| Absolute         | `LDA $1234`   | `AddressingMode::Absolute`        |
| Absolute,X       | `LDA $1234,X` | `AddressingMode::AbsoluteX`       |
| Absolute,Y       | `LDA $1234,Y` | `AddressingMode::AbsoluteY`       |
| Indirect         | `JMP ($1234)` | `AddressingMode::Indirect`        |
| Indexed Indirect | `LDA ($42,X)` | `AddressingMode::IndexedIndirect` |
| Indirect Indexed | `LDA ($42),Y` | `AddressingMode::IndirectIndexed` |
| Relative         | `BNE label`   | `AddressingMode::Relative`        |

---

## Symbol Flags Comparison

| Flag         | EDASM.SRC Bit | C++ Flag           | Meaning                           |
| ------------ | ------------- | ------------------ | --------------------------------- |
| Undefined    | bit 7 ($80)   | `FLAG_UNDEFINED`   | Symbol not defined                |
| Unreferenced | bit 6 ($40)   | `SYM_UNREFERENCED` | Tracked in C++ (as of 2026-01-16) |
| Relative     | bit 5 ($20)   | `FLAG_RELATIVE`    | REL mode symbol                   |
| External     | bit 4 ($10)   | `FLAG_EXTERNAL`    | EXT declaration                   |
| Entry        | bit 3 ($08)   | `FLAG_ENTRY`       | ENT declaration                   |
| Macro        | bit 2 ($04)   | N/A                | Macros not implemented            |
| Forward ref  | bit 0 ($01)   | Tracked separately | Forward reference                 |

---

## Verification Checklist Template

When verifying a feature:

- [ ] Located EDASM.SRC implementation (file and line numbers)
- [ ] Located C++ implementation (file and function)
- [ ] Compared algorithm/logic
- [ ] Identified any intentional differences
- [ ] Checked for existing unit tests
- [ ] Added new tests if needed
- [ ] Updated VERIFICATION_REPORT.md if significant
- [ ] Tested with sample input
- [ ] Documented any discrepancies

---

## Emergency Contacts

- **EDASM.SRC Source**: third_party/EdAsm/EDASM.SRC/
- **C++ Source**: src/, include/edasm/
- **Tests**: tests/
- **Documentation**: docs/
- **Build Scripts**: scripts/

---

**Print Date**: 2026-01-16  
**Version**: C-EDASM main branch  
**EDASM.SRC**: Commit 05a19d8

---

_Keep this card handy! Pin it to your wall or keep it on your second monitor._
