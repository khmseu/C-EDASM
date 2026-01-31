# C-EDASM Documentation

## Complete Documentation for EDASM.SRC to C++ Port

This directory contains comprehensive documentation for the C-EDASM project - a modern C++20 port of the Apple II EDASM editor/assembler system.

---

## Documentation Overview

### 📋 For Verification Work

**Start here if you need to verify the C++ implementation against the original EDASM.SRC:**

1. **[VERIFICATION_QUICK_REF.md](VERIFICATION_QUICK_REF.md)** - ⭐ **START HERE!**
    - One-page quick reference card
    - File location mapping
    - Critical routine cross-references
    - Common verification tasks
    - Print this and keep it handy!

2. **[VERIFICATION_INDEX.md](VERIFICATION_INDEX.md)** - ⭐ **Detailed Lookup**
    - Comprehensive quick lookup table
    - Feature-by-feature cross-references
    - Status indicators for each component
    - Testing strategy for verification
    - Use this for detailed verification work

3. **[VERIFICATION_REPORT.md](VERIFICATION_REPORT.md)** - 📊 **Complete Analysis**
    - Full cross-reference between EDASM.SRC and C++
    - Line-by-line routine mapping
    - Data structure comparisons
    - Code quality metrics
    - Most comprehensive documentation

4. **[MISSING_FEATURES.md](MISSING_FEATURES.md)** - ❓ **What's NOT Ported**
    - Explicit list of unimplemented features
    - Rationale for each omission
    - Hardware-specific features that don't apply
    - Future roadmap priorities
    - Answers "Why isn't this in C++?"

---

### 📚 For Understanding the System

**Read these to understand EDASM architecture and design:**

1. **[PORTING_PLAN.md](PORTING_PLAN.md)** - 🗓️ **Implementation Roadmap**
    - 14-week implementation plan
    - Phase-by-phase breakdown
    - Module mapping (6502 → C++)
    - Zero page variable mapping
    - Current status and progress

2. **[ASSEMBLER_ARCHITECTURE.md](ASSEMBLER_ARCHITECTURE.md)** - 🏗️ **Assembler Design**
    - Two-pass (three-pass) assembly process
    - Symbol table structure and algorithms
    - Expression evaluation design
    - Code generation process
    - Output formats (BIN, REL, SYS, LST)

3. **[COMMAND_REFERENCE.md](COMMAND_REFERENCE.md)** - 📖 **Command Set**
    - Complete command documentation
    - File commands (LOAD, SAVE, etc.)
    - Editor commands (LIST, INSERT, etc.)
    - Assembler commands (ASM, LINK)
    - Control commands (EXEC, BYE, etc.)

4. **[6502_INSTRUCTION_SET.md](6502_INSTRUCTION_SET.md)** - 🔧 **Opcode Reference**
    - Complete 6502 instruction set
    - All 13 addressing modes
    - Opcode tables with cycles
    - C++ implementation guidance
    - Addressing mode detection algorithm

---

## Quick Start Guides

### For Verifiers

```bash
# 1. Read the quick reference
cat docs/VERIFICATION_QUICK_REF.md

# 2. Look up a feature
grep -A 5 "FeatureName" docs/VERIFICATION_INDEX.md

# 3. Verify implementation
# Follow the cross-reference to find both implementations
cat third_party/EdAsm/EDASM.SRC/ASM/ASM2.S | sed -n 'LINE1,LINE2p'
cat src/assembler/assembler.cpp

# 4. Test it
cd build && ctest -R relevant_test
```

### For Developers

```bash
# 1. Understand the roadmap
cat docs/PORTING_PLAN.md

# 2. Learn the architecture
cat docs/ASSEMBLER_ARCHITECTURE.md

# 3. Check command syntax
grep "COMMAND" docs/COMMAND_REFERENCE.md

# 4. Implement feature
# (Write code)

# 5. Update documentation
# Add entry to VERIFICATION_INDEX.md
# Update status in PORTING_PLAN.md
```

---

## Documentation Workflow

### When Adding a New Feature

1. **Before coding**: Check [PORTING_PLAN.md](PORTING_PLAN.md) for module structure
2. **While coding**: Reference [ASSEMBLER_ARCHITECTURE.md](ASSEMBLER_ARCHITECTURE.md) or [COMMAND_REFERENCE.md](COMMAND_REFERENCE.md)
3. **Check for 6502 reference**: Use [VERIFICATION_INDEX.md](VERIFICATION_INDEX.md) to find EDASM.SRC location
4. **Add cross-reference**: Add comments in C++ code citing EDASM.SRC
5. **After coding**: Update [VERIFICATION_INDEX.md](VERIFICATION_INDEX.md) with new mapping
6. **If missing feature**: Consider if it should be in [MISSING_FEATURES.md](MISSING_FEATURES.md)

### When Fixing a Bug

1. **Find the feature**: Use [VERIFICATION_INDEX.md](VERIFICATION_INDEX.md)
2. **Check original**: Compare with EDASM.SRC using cross-reference
3. **Determine if intentional**: Check [MISSING_FEATURES.md](MISSING_FEATURES.md)
4. **Fix or document**: Either fix the bug or document why it's different
5. **Add test**: Ensure regression test exists

### When Reviewing Code

1. **Check cross-references**: Ensure C++ code cites EDASM.SRC locations
2. **Verify in index**: Check [VERIFICATION_INDEX.md](VERIFICATION_INDEX.md) is up to date
3. **Test coverage**: Verify tests exist for the feature
4. **Documentation**: Ensure all docs are updated

---

## Documentation Standards

### Cross-Reference Format

In C++ code, use this format:

```cpp
// From EDASM.SRC: ASM2.S L8561-8800 (EvalExpr routine)
// Implements: Expression evaluation with operator precedence
// Differences: Uses recursive descent instead of stack-based parsing
```

### Status Indicators

| Symbol | Meaning                                     |
| ------ | ------------------------------------------- |
| ✅     | Fully implemented, equivalent functionality |
| ⚠️     | Not needed (hardware-specific or replaced)  |
| 🔄     | Partially implemented                       |
| ⭕     | Documented but not yet implemented          |
| ❌     | Intentionally not ported                    |

### Priority Levels

| Symbol | Priority                         |
| ------ | -------------------------------- |
| ⭐⭐⭐ | Highest - Critical core feature  |
| ⭐⭐   | High - Important feature         |
| ⭐     | Medium - Useful but not critical |
| (none) | Low - Optional or cosmetic       |

---

## File Dependency Graph

```text
VERIFICATION_QUICK_REF.md (One-page reference)
    ↓
VERIFICATION_INDEX.md (Detailed lookup)
    ↓
VERIFICATION_REPORT.md (Complete analysis)
    ↓
MISSING_FEATURES.md (What's not ported)

PORTING_PLAN.md (Roadmap)
    ↓
ASSEMBLER_ARCHITECTURE.md (Design)
COMMAND_REFERENCE.md (Commands)
6502_INSTRUCTION_SET.md (Opcodes)
```

**Suggested reading order**:

1. VERIFICATION_QUICK_REF.md - Get oriented
2. PORTING_PLAN.md - Understand the big picture
3. VERIFICATION_INDEX.md - Look up specific features
4. ASSEMBLER_ARCHITECTURE.md - Deep dive into design
5. VERIFICATION_REPORT.md - Complete cross-reference
6. MISSING_FEATURES.md - Understand what's different

---

## Statistics

| Document                  | Lines      | Focus            | Audience               |
| ------------------------- | ---------- | ---------------- | ---------------------- |
| VERIFICATION_QUICK_REF.md | ~300       | Quick lookup     | Verifiers              |
| VERIFICATION_INDEX.md     | ~600       | Feature index    | Verifiers              |
| VERIFICATION_REPORT.md    | ~330       | Complete mapping | Verifiers, maintainers |
| MISSING_FEATURES.md       | ~500       | Differences      | Verifiers, users       |
| PORTING_PLAN.md           | ~350       | Roadmap          | Developers             |
| ASSEMBLER_ARCHITECTURE.md | ~450       | Design details   | Developers             |
| COMMAND_REFERENCE.md      | ~260       | Command syntax   | Users, developers      |
| 6502_INSTRUCTION_SET.md   | ~320       | Opcode reference | Developers             |
| **Total**                 | **~3,110** |                  |                        |

---

## External Resources

### Original EDASM.SRC

- **Location**: `third_party/EdAsm/EDASM.SRC/`
- **Repository**: [markpmlim/EdAsm](https://github.com/markpmlim/EdAsm)
- **Commit**: 05a19d8693a3b511ebdf97a8e8adb3eef886e698
- **Total Lines**: ~25,759 (6502 assembly)

### C++ Implementation

- **Location**: `src/`, `include/edasm/`
- **Total Lines**: ~15,000 (C++20)
- **Tests**: `tests/`
- **Test Coverage**: Comprehensive unit and integration tests

---

## FAQ

### Q: Which document should I read first?

**A:** Start with [VERIFICATION_QUICK_REF.md](VERIFICATION_QUICK_REF.md) - it's a one-page overview designed to get you oriented quickly.

### Q: How do I find where a 6502 routine is implemented in C++?

**A:** Use [VERIFICATION_INDEX.md](VERIFICATION_INDEX.md) - it has a quick lookup table with cross-references.

### Q: Why isn't feature X implemented?

**A:** Check [MISSING_FEATURES.md](MISSING_FEATURES.md) - it documents all unimplemented features and why.

### Q: How do I verify a specific feature?

**A:** Follow the verification workflow in [VERIFICATION_QUICK_REF.md](VERIFICATION_QUICK_REF.md), Task 1-4.

### Q: Where's the implementation roadmap?

**A:** See [PORTING_PLAN.md](PORTING_PLAN.md) for the complete 14-week plan with status.

### Q: How does the assembler work?

**A:** Read [ASSEMBLER_ARCHITECTURE.md](ASSEMBLER_ARCHITECTURE.md) for detailed design documentation.

### Q: What commands are supported?

**A:** See [COMMAND_REFERENCE.md](COMMAND_REFERENCE.md) for complete command documentation.

### Q: Where's the 6502 instruction reference?

**A:** See [6502_INSTRUCTION_SET.md](6502_INSTRUCTION_SET.md) for all opcodes and addressing modes.

---

## Contributing to Documentation

### Adding New Documentation

1. Follow the format of existing documents
2. Use consistent status symbols (✅, ⚠️, 🔄, ⭕, ❌)
3. Add cross-references to other documents
4. Update this README.md to include the new document
5. Keep VERIFICATION_INDEX.md in sync

### Updating Existing Documentation

1. Maintain consistent formatting
2. Update "Last Updated" date at bottom
3. Cross-check related documents for consistency
4. Update statistics in this README if needed

### Documentation Review Checklist

- [ ] Formatting consistent with other docs
- [ ] Cross-references accurate
- [ ] Status symbols used correctly
- [ ] Examples are clear and correct
- [ ] README.md updated if needed
- [ ] Grammar and spelling checked

---

## Document History

### 2026-01-16 - Verification Documentation Suite

- Added VERIFICATION_QUICK_REF.md (one-page reference)
- Added VERIFICATION_INDEX.md (detailed lookup)
- Added MISSING_FEATURES.md (unimplemented features)
- Created this README.md
- Updated VERIFICATION_REPORT.md with new references

### Earlier - Core Documentation

- Created PORTING_PLAN.md (implementation roadmap)
- Created ASSEMBLER_ARCHITECTURE.md (assembler design)
- Created COMMAND_REFERENCE.md (command documentation)
- Created 6502_INSTRUCTION_SET.md (opcode reference)
- Created VERIFICATION_REPORT.md (cross-reference)

---

## License

Documentation is part of the C-EDASM project. See LICENSE in repository root.

---

**Last Updated**: 2026-01-16  
**Document Count**: 8 core documents + 1 README  
**Total Documentation Lines**: ~3,110 lines  
**Coverage**: Comprehensive verification and implementation guidance
