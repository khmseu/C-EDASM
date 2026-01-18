# Documentation Map

**Visual Guide to C-EDASM Documentation**

```
                    C-EDASM Documentation
                            |
        ┌───────────────────┴───────────────────┐
        |                                       |
  VERIFICATION                           IMPLEMENTATION
  DOCUMENTS                              DOCUMENTS
        |                                       |
        |                                       |
  ┌─────┴─────┐                        ┌───────┴───────┐
  |           |                        |               |
START         DETAILED                DESIGN          REFERENCE
HERE          LOOKUP                  DOCS            GUIDES
  |           |                        |               |
  |           |                        |               |
  v           v                        v               v

┌─────────────────────────────────────────────────────────────────┐
│                                                                 │
│  📋 VERIFICATION_QUICK_REF.md  ⭐ START HERE                   │
│  │  One-page reference card                                    │
│  │  • File location mapping                                    │
│  │  • Common verification tasks                                │
│  │  • Quick command reference                                  │
│  │                                                              │
│  └──→ Print this and keep it handy!                            │
│                                                                 │
└─────────────────────────────────────────────────────────────────┘
           ↓
┌─────────────────────────────────────────────────────────────────┐
│                                                                 │
│  📋 VERIFICATION_INDEX.md                                       │
│  │  Detailed feature lookup table                              │
│  │  • All features cross-referenced                            │
│  │  • Status indicators (✅⚠️🔄⭕❌)                              │
│  │  • Testing strategies                                       │
│  │                                                              │
│  └──→ Use for detailed verification                            │
│                                                                 │
└─────────────────────────────────────────────────────────────────┘
           ↓
┌─────────────────────────────────────────────────────────────────┐
│                                                                 │
│  📊 VERIFICATION_REPORT.md                                      │
│  │  Complete analysis                                           │
│  │  • Line-by-line routine mapping                             │
│  │  • Data structure comparisons                               │
│  │  • Code quality metrics                                     │
│  │                                                              │
│  └──→ Most comprehensive verification doc                      │
│                                                                 │
└─────────────────────────────────────────────────────────────────┘
           ↓
┌─────────────────────────────────────────────────────────────────┐
│                                                                 │
│  ❓ MISSING_FEATURES.md                                         │
│  │  What's NOT in C++                                          │
│  │  • Hardware-specific omissions                              │
│  │  • Features not yet ported                                  │
│  │  • Intentional design changes                               │
│  │                                                              │
│  └──→ Answers "Why isn't this implemented?"                    │
│                                                                 │
└─────────────────────────────────────────────────────────────────┘


  IMPLEMENTATION DOCUMENTATION
  ═════════════════════════════

┌─────────────────────────────────────────────────────────────────┐
│                                                                 │
│  🗓️ PORTING_PLAN.md                                            │
│  │  14-week implementation roadmap                             │
│  │  • Phase-by-phase breakdown                                 │
│  │  • Module mapping (6502 → C++)                              │
│  │  • Current status                                           │
│  │                                                              │
│  └──→ Understand the big picture                               │
│                                                                 │
└─────────────────────────────────────────────────────────────────┘

┌─────────────────────────────────────────────────────────────────┐
│                                                                 │
│  🏗️ ASSEMBLER_ARCHITECTURE.md                                 │
│  │  Assembler design details                                   │
│  │  • Two-pass assembly process                                │
│  │  • Symbol table structure                                   │
│  │  • Expression evaluation                                    │
│  │                                                              │
│  └──→ Deep dive into assembler                                 │
│                                                                 │
└─────────────────────────────────────────────────────────────────┘

┌─────────────────────────────────────────────────────────────────┐
│                                                                 │
│  📖 COMMAND_REFERENCE.md                                        │
│  │  Complete command set                                       │
│  │  • File commands                                            │
│  │  • Editor commands                                          │
│  │  • Assembler commands                                       │
│  │                                                              │
│  └──→ Command syntax reference                                 │
│                                                                 │
└─────────────────────────────────────────────────────────────────┘

┌─────────────────────────────────────────────────────────────────┐
│                                                                 │
│  🔧 6502_INSTRUCTION_SET.md                                     │
│  │  Complete opcode reference                                  │
│  │  • All 13 addressing modes                                  │
│  │  • Opcode tables                                            │
│  │  • Implementation guidance                                  │
│  │                                                              │
│  └──→ 6502 instruction reference                               │
│                                                                 │
└─────────────────────────────────────────────────────────────────┘

┌─────────────────────────────────────────────────────────────────┐
│                                                                 │
│  🖥️ DEBUGGER_EMULATOR_PLAN.md                                  │
│  │  Emulator integration planning                              │
│  │  • Goal: Host original EDASM for comparison tests           │
│  │  • Emulator options overview                                │
│  │  • Automation strategies                                    │
│  │                                                              │
│  └──→ Background on emulator requirements                      │
│                                                                 │
└─────────────────────────────────────────────────────────────────┘

┌─────────────────────────────────────────────────────────────────┐
│                                                                 │
│  📊 EMULATOR_INVESTIGATION_REPORT.md                            │
│  │  Detailed emulator research findings                        │
│  │  • MAME (recommended): Lua automation, high fidelity        │
│  │  • GSPlus: Good debugger, moderate fidelity                 │
│  │  • LinApple: Lightweight, quick setup                       │
│  │  • Custom emulator: High effort, not recommended            │
│  │  • ProDOS disk tools: cadius, AppleCommander                 │
│  │                                                              │
│  └──→ Complete analysis with recommendations                   │
│                                                                 │
└─────────────────────────────────────────────────────────────────┘

┌─────────────────────────────────────────────────────────────────┐
│                                                                 │
│  🎯 EMULATOR_DECISION_MATRIX.md                                 │
│  │  Quick reference for emulator selection                     │
│  │  • Decision tree for choosing emulator                      │
│  │  • Effort estimates for each option                         │
│  │  • Risk assessment                                          │
│  │  • Useful commands                                          │
│  │                                                              │
│  └──→ Quick lookup for emulator decisions                      │
│                                                                 │
└─────────────────────────────────────────────────────────────────┘


  NAVIGATION GUIDE
  ════════════════

  For Verifiers:
  1. VERIFICATION_QUICK_REF.md  → Quick orientation
  2. VERIFICATION_INDEX.md      → Look up features
  3. VERIFICATION_REPORT.md     → Deep analysis
  4. MISSING_FEATURES.md        → Check omissions

  For Developers:
  1. PORTING_PLAN.md            → Understand roadmap
  2. ASSEMBLER_ARCHITECTURE.md  → Learn design
  3. COMMAND_REFERENCE.md       → Command syntax
  4. 6502_INSTRUCTION_SET.md    → Opcode reference

  For Emulator Integration:
  1. DEBUGGER_EMULATOR_PLAN.md       → Initial requirements
  2. EMULATOR_INVESTIGATION_REPORT.md → Detailed research
  3. EMULATOR_DECISION_MATRIX.md     → Quick reference
  4. tests/emulator/README.md        → Implementation guide

  For Users:
  1. README.md (main)           → Project overview
  2. COMMAND_REFERENCE.md       → How to use
  3. PORTING_PLAN.md            → What works now


  CROSS-REFERENCE WEB
  ═══════════════════

                VERIFICATION_QUICK_REF.md
                         |
         ┌───────────────┼───────────────┐
         |               |               |
         v               v               v
  VERIFICATION_    ASSEMBLER_     COMMAND_
   INDEX.md      ARCHITECTURE.md  REFERENCE.md
         |               |               |
         └───────────────┼───────────────┘
                         |
                         v
              VERIFICATION_REPORT.md
                         |
                         v
              MISSING_FEATURES.md


  DOCUMENTATION METRICS
  ════════════════════

  Total Documents:       13 (including this map)
  Total Lines:          ~5,000+
  Total Size:           ~180 KB
  Cross-References:     ~120+
  Code Samples:         ~60+
  Tables:               ~40+


  QUICK LINKS
  ═══════════

  Verification:
  • Verification Quick Start: VERIFICATION_QUICK_REF.md
  • Feature Lookup:          VERIFICATION_INDEX.md
  • Complete Analysis:       VERIFICATION_REPORT.md
  • Missing Features:        MISSING_FEATURES.md

  Implementation:
  • Implementation Plan:     PORTING_PLAN.md
  • Assembler Design:        ASSEMBLER_ARCHITECTURE.md
  • Command Reference:       COMMAND_REFERENCE.md
  • Opcode Reference:        6502_INSTRUCTION_SET.md

  Testing & Emulation:
  • Emulator Planning:       DEBUGGER_EMULATOR_PLAN.md
  • Emulator Research:       EMULATOR_INVESTIGATION_REPORT.md
  • Emulator Decision:       EMULATOR_DECISION_MATRIX.md
  • Emulator Scripts:        tests/emulator/README.md

  General:
  • Documentation Guide:     README.md (this folder)


  SOURCE CODE LOCATIONS
  ════════════════════

  EDASM.SRC (6502 Assembly):
    third_party/EdAsm/EDASM.SRC/
    ├── ASM/         (Assembler)
    ├── EDITOR/      (Editor)
    ├── EI/          (Interpreter)
    ├── LINKER/      (Linker)
    └── BUGBYTER/    (Debugger - not ported)

  C++ Implementation:
    src/
    ├── assembler/   (Assembler)
    ├── editor/      (Editor)
    ├── core/        (App loop)
    └── files/       (File I/O)

    include/edasm/   (Headers)


  SUGGESTED READING PATHS
  ═══════════════════════

  Path 1: Quick Verification
  ──────────────────────────
  1. VERIFICATION_QUICK_REF.md  (5 min)
  2. VERIFICATION_INDEX.md      (10 min)
  3. Start verifying!

  Path 2: Complete Understanding
  ──────────────────────────────
  1. README.md (main)           (10 min)
  2. VERIFICATION_QUICK_REF.md  (5 min)
  3. PORTING_PLAN.md            (15 min)
  4. ASSEMBLER_ARCHITECTURE.md  (20 min)
  5. VERIFICATION_INDEX.md      (15 min)
  6. VERIFICATION_REPORT.md     (30 min)
  7. MISSING_FEATURES.md        (15 min)
  Total: ~2 hours

  Path 3: Maintenance Developer
  ─────────────────────────────
  1. README.md (main)           (10 min)
  2. PORTING_PLAN.md            (15 min)
  3. VERIFICATION_INDEX.md      (15 min)
  4. Pick specific docs as needed


  PRINT-FRIENDLY DOCUMENTS
  ═══════════════════════

  Essential (print these):
  • VERIFICATION_QUICK_REF.md   (1 page)
  • VERIFICATION_INDEX.md       (6 pages)

  Optional (keep digital):
  • All other documents


  LAST UPDATED
  ════════════

  Date:           2026-01-16
  Version:        C-EDASM main branch
  EDASM.SRC:      Commit 05a19d8
  Documentation:  Complete verification suite + emulator investigation


────────────────────────────────────────────────────────────────────

This map helps you navigate the documentation efficiently.
Start with the Quick Ref and work your way through as needed!

────────────────────────────────────────────────────────────────────
```
