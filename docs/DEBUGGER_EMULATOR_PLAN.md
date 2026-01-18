# Debugger + Original EDASM Hosting Plan

## Purpose

Summarize how to host the original Apple II EDASM inside our tooling for comparison tests, and outline debugger/emulator options with control surfaces for automation.

## What EDASM Expects (from docs and EDASM.SRC)

- Target: Apple IIe/II+ with language card banking; entry around $B100 and globals near $BD00-$BEFF.
- Uses ProDOS MLI for file I/O; needs Disk II controller access and writable ProDOS volume.
- Relies on soft switches ($C0xx range), monitor vectors (GETLN/RDKEY), and text screen/keyboard I/O (40-column sufficient; detects 80-column if present).
- Memory layout: text buffer ~ $0801-$9900; assembler/linker modules occupy high memory and banked regions.
- Artifacts provided: third_party/EdAsm/EDASM_SRC.2mg (bootable ProDOS image with EDASM.SYSTEM), EDASM.SRC sources for reference.

## Goals

- Boot original EDASM from the shipped .2mg inside an emulator.
- Inject repo test sources (test\_\*.src) onto a writable ProDOS disk.
- Script EDASM to assemble/link and emit BIN/REL/LST outputs.
- Extract those outputs for byte-for-byte comparison against C-EDASM results.
- Enable headless/CI-friendly runs.

## Emulator Options (automation-focused)

- **MAME (apple2e driver) — recommended first**
    - Pros: High fidelity (language card, soft switches, ProDOS), supports .2mg/.po/.dsk; headless mode (`-video none -sound none -nothrottle`); Lua automation (`-autoboot_script`) for keystroke injection (`emu.keypost`), memory pokes, breakpoints; can save state; supports CLI-only use.
    - Cons: Heavier dependency; Lua scripting learning curve; embedding via libmame adds build complexity.
- **GSPlus/KEGS**
    - Pros: Lighter than MAME; decent Apple IIgs/IIe fidelity; CLI disk attach; built-in debugger.
    - Cons: No rich scripting API; headless/CI may require patching to feed stdin or socket events; .2mg support can be spotty.
- **LinApple / LinApple-Pie**
    - Pros: Small codebase; easier to patch for deterministic runs; fast startup.
    - Cons: Lower fidelity on edge-case soft switches; typically lacks .2mg; no native scripting/headless without SDL patching.
- **Roll our own minimal emulator**
    - Pros: Full control and tight integration.
    - Cons: High effort/risk to reach ProDOS + Disk II + language card accuracy; not recommended unless other options fail.

## Proposed Path (phased)

1. **Prototype with MAME CLI + Lua**
    - Command sketch: `mame apple2e -flop1 edasm_src.2mg -video none -sound none -nothrottle -autoboot_script run.lua`.
    - Lua: post keys to boot ProDOS, launch EDASM, load a test source, assemble/link, save outputs. Capture files from the ProDOS image or via Lua memory/disk hooks.
2. **Artifact handling**
    - Prepare a writable .2mg containing test sources; mount as flop2 if needed.
    - After scripted run, extract BIN/REL/LST from the .2mg for comparison (can use ProDOS image tools or MAME Lua to dump sectors).
3. **Stabilize automation**
    - Add timeouts/prompts detection; prefer file extraction over screen scraping.
    - Keep scripts under tests/ (e.g., tests/emulator/run_edasm.lua) with reproducible seeds.
4. **Evaluate alternatives if MAME is too heavy**
    - Patch GSPlus/KEGS for stdin→keyboard and headless build; validate .2mg; or fork LinApple and add socket/stdio control plus .2mg handling.

## Open Questions / Decisions

- Do we accept MAME as an external test dependency, or do we need an embedded core (libmame) for tighter control?
- Preferred disk injection/extraction flow: prebaked .2mg vs. on-the-fly packing; which ProDOS tooling to use in CI?
- How to store golden artifacts: committed outputs vs. regenerated per CI run.
- Do we need 80-column and printer emulation, or is 40-column text-only sufficient for tests?

## Rough Effort

- MAME CLI + Lua prototype: 1–2 days to boot, script EDASM, and extract files.
- Hardening for CI (disk prep/extract tooling, retries, docs): another 1–2 days.
- Embedding libmame or patching a lighter emulator: several additional days depending on integration depth.
