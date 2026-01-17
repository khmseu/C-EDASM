# Emulator Investigation - Table of Contents

Quick navigation for all emulator-related documentation.

## 📚 Documentation Overview

This investigation addresses the question: **How can we host the original Apple II EDASM to enable automated comparison testing with C-EDASM?**

## 🗂️ Documents in Reading Order

### 1. Background & Requirements
**File:** `DEBUGGER_EMULATOR_PLAN.md`  
**Size:** 4.0 KB  
**Purpose:** Original planning document outlining the need for emulator integration  
**Read if:** You want to understand the original problem statement  
**Time:** 5-10 minutes

**Key Topics:**
- What EDASM expects (Apple IIe, ProDOS, language card)
- Goals (boot original EDASM, inject tests, extract outputs)
- Initial emulator options overview
- Open questions

---

### 2. Comprehensive Research Report
**File:** `EMULATOR_INVESTIGATION_REPORT.md`  
**Size:** 18 KB  
**Purpose:** Complete analysis of all emulator options with detailed findings  
**Read if:** You need thorough understanding before making decisions  
**Time:** 20-30 minutes

**Key Topics:**
- Executive summary with recommendation (MAME)
- MAME analysis: Lua API, headless mode, fidelity, implementation path
- GSPlus analysis: Debugger capabilities, CLI automation, limitations
- LinApple analysis: Lightweight setup, headless challenges
- Custom emulator analysis: Complexity, effort, risks (NOT recommended)
- ProDOS disk tools: cadius, AppleCommander, apple2_prodos_utils
- Comparative feature matrix
- Implementation timelines (phase-by-phase)
- Resource requirements
- Final recommendation with rationale

**Appendices:**
- Open questions for decision-making
- Example command workflows
- Reference links (MAME docs, disk tools, community forums)

---

### 3. Quick Decision Reference
**File:** `EMULATOR_DECISION_MATRIX.md`  
**Size:** 8.4 KB  
**Purpose:** Fast lookup guide for choosing an emulator  
**Read if:** You need to make a quick decision or compare options  
**Time:** 10-15 minutes

**Key Topics:**
- Decision tree (Q&A format to guide choice)
- Quick comparison table (all options side-by-side)
- Use case recommendations:
  - Automated CI/CD testing → MAME
  - Interactive debugging → GSPlus
  - Quick prototyping → LinApple
  - Long-term production → MAME
- Effort estimates (detailed breakdown per option)
- Risk assessment (LOW/MEDIUM/HIGH ratings)
- Useful commands (ready-to-run examples)

**Best for:** Engineers who need to justify a choice to stakeholders

---

### 4. Implementation Guide
**File:** `tests/emulator/README.md`  
**Size:** 4.9 KB  
**Purpose:** Developer guide for the prototype automation scripts  
**Read if:** You want to understand or extend the Lua automation  
**Time:** 10-15 minutes

**Key Topics:**
- Overview of automation approach
- Script descriptions (boot_test.lua, assemble_test.lua)
- Usage examples with command-line invocations
- Current status (prototype phase)
- Production requirements:
  - Proper keyboard injection
  - Screen/memory monitoring
  - Error handling
  - Robust timing
- Development roadmap (3 phases)
- MAME Lua API resources and references
- Testing instructions
- Contributing guidelines

**Best for:** Developers implementing or improving automation scripts

---

### 5. Prototype Scripts

#### `tests/emulator/boot_test.lua`
**Size:** 1.7 KB  
**Purpose:** Basic proof-of-concept demonstrating MAME Lua automation  
**Contains:**
- ProDOS boot waiting logic
- EDASM.SYSTEM launch sequence
- MAME API usage examples (emu.wait, emu.register_start)
- Comments explaining approach

**Status:** Prototype only - demonstrates concept, not production-ready

#### `tests/emulator/assemble_test.lua`
**Size:** 4.2 KB  
**Purpose:** Complete workflow automation (load → assemble → save)  
**Contains:**
- Helper functions for keyboard simulation
- Multi-disk handling (source disk + test disk)
- Command sequencing (L, A, S commands)
- Timing control between operations
- Comments on real implementation requirements

**Status:** Prototype only - shows full workflow, needs proper keyboard API

---

## 🎯 Recommended Reading Paths

### Path A: Quick Decision (30 minutes)
For stakeholders who need to make an informed choice quickly:

1. **DEBUGGER_EMULATOR_PLAN.md** (5 min) - Understand the problem
2. **EMULATOR_DECISION_MATRIX.md** (15 min) - Review options and recommendation
3. **EMULATOR_INVESTIGATION_REPORT.md** - Executive Summary only (5 min)
4. ✅ **Decision:** Proceed with MAME (or discuss alternatives)

### Path B: Implementation Planning (60 minutes)
For developers who will implement the solution:

1. **DEBUGGER_EMULATOR_PLAN.md** (5 min) - Background
2. **EMULATOR_INVESTIGATION_REPORT.md** (25 min) - Full analysis
3. **tests/emulator/README.md** (10 min) - Implementation guide
4. **Skim prototype scripts** (10 min) - Understand current state
5. **MAME Lua API docs** (10 min) - External reference
6. ✅ **Output:** Implementation plan and timeline

### Path C: Complete Understanding (90 minutes)
For technical leads who need comprehensive knowledge:

1. **DEBUGGER_EMULATOR_PLAN.md** (10 min) - Original requirements
2. **EMULATOR_INVESTIGATION_REPORT.md** (30 min) - Full report
3. **EMULATOR_DECISION_MATRIX.md** (15 min) - Decision framework
4. **tests/emulator/README.md** (15 min) - Implementation details
5. **Review all prototype scripts** (20 min) - Code review
6. ✅ **Output:** Deep expertise and ability to answer questions

### Path D: Quick Reference (5 minutes)
For engineers who just need a refresher:

1. **EMULATOR_DECISION_MATRIX.md** - Decision tree section
2. ✅ **Answer:** Which emulator for which use case

---

## 📊 Document Relationships

```
DEBUGGER_EMULATOR_PLAN.md
         |
         | (identifies need)
         v
EMULATOR_INVESTIGATION_REPORT.md ←→ EMULATOR_DECISION_MATRIX.md
         |                                    |
         | (informs)                         | (summarizes)
         v                                    v
tests/emulator/README.md              [Quick decisions]
         |
         | (documents)
         v
boot_test.lua & assemble_test.lua
         |
         | (demonstrates)
         v
   [Proof of concept]
```

---

## 🔍 Search Guide

### Looking for...
- **"Which emulator should I use?"** → EMULATOR_DECISION_MATRIX.md
- **"Why MAME?"** → EMULATOR_INVESTIGATION_REPORT.md (Executive Summary)
- **"How does MAME Lua work?"** → tests/emulator/README.md
- **"What are the alternatives?"** → EMULATOR_INVESTIGATION_REPORT.md (sections on GSPlus, LinApple)
- **"How long will it take?"** → EMULATOR_DECISION_MATRIX.md (Effort Estimates)
- **"What about ProDOS tools?"** → EMULATOR_INVESTIGATION_REPORT.md (ProDOS Disk Image Tooling section)
- **"Original requirements?"** → DEBUGGER_EMULATOR_PLAN.md
- **"Sample code?"** → tests/emulator/*.lua
- **"Risk assessment?"** → EMULATOR_DECISION_MATRIX.md (Risk Assessment section)
- **"Commands to run?"** → EMULATOR_DECISION_MATRIX.md (Useful Commands section)

---

## 📈 Statistics

| Document | Size | Read Time | Content Type |
|----------|------|-----------|--------------|
| DEBUGGER_EMULATOR_PLAN.md | 4 KB | 5-10 min | Planning |
| EMULATOR_INVESTIGATION_REPORT.md | 18 KB | 20-30 min | Analysis |
| EMULATOR_DECISION_MATRIX.md | 8 KB | 10-15 min | Reference |
| tests/emulator/README.md | 5 KB | 10-15 min | Guide |
| boot_test.lua | 2 KB | 5 min | Code |
| assemble_test.lua | 4 KB | 10 min | Code |
| **Total** | **~41 KB** | **60-85 min** | Mixed |

---

## ✅ Investigation Status

**Status:** ✅ **COMPLETE**  
**Date:** January 16, 2026  
**Result:** Clear recommendation (MAME) with comprehensive analysis  
**Next Phase:** Decision and implementation planning

---

## 🔗 External Resources

Key external links referenced in the investigation:

### MAME
- Official Lua API: https://docs.mamedev.org/luascript/index.html
- Autoboot examples: https://forums.launchbox-app.com/topic/78092-autoboot-lua-scripts-in-mame/
- Lua scripting guide: https://www.mattgreer.dev/blog/mame-lua-for-better-retro-dev/

### Disk Tools
- cadius: https://github.com/mach-kernel/cadius
- AppleCommander: https://applecommander.github.io/
- apple2_prodos_utils: https://github.com/Michaelangel007/apple2_prodos_utils

### Alternative Emulators
- GSPlus: https://github.com/digarok/gsplus
- LinApple: https://github.com/linappleii/linapple
- KEGS: https://kegs.sourceforge.net/

### Apple II References
- ProDOS 8: https://prodos8.com/
- Apple II Memory Map: https://www.kreativekorp.com/miscpages/a2info/memorymap.shtml

---

## 📝 Notes

- All documents are in Markdown format
- Lua scripts are in the `tests/emulator/` directory
- Investigation was conducted in January 2026
- Research included official documentation, community forums, and GitHub repositories
- Recommendation is based on automation needs for CI/CD integration
- Prototype scripts are NOT production-ready (clearly documented)

---

*Use this table of contents as your navigation guide through the emulator investigation documentation.*
