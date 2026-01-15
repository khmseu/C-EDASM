# EDASM (C++/ncurses)

Port of the Apple II EDASM editor/assembler/tools from `markpmlim/EdAsm` to modern C++ targeting Linux with ncurses for screen handling. The goal is to stay close to the original 6502 logic while adapting storage and UI to a terminal environment.

## Status

- Workspace scaffolded with CMake, ncurses dependency, and placeholder modules for editor, assembler, symbol table, and ProDOS file mapping.
- Core logic still to be ported from `EDASM.SRC` 6502 assembly.

## Build & Run

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build
./build/edasm_cli
```

## Dependencies

- CMake (>=3.16)
- A C++20 compiler
- ncurses development headers and library

## ProDOS file type mapping (Linux extensions)

- Source text → `.src`
- Assembled object (BIN) → `.obj`
- System executable (SYS) → `.sys`
- Listing output → `.lst`
- Plain text → `.txt`
- Binary blob → `.bin`

## Next Steps

- Translate editor command set and buffer management from `EDASM.SRC`.
- Implement two-pass assembler, symbol resolution, and output emission.
- Add tests that exercise assembler and editor behaviors against sample inputs derived from the original sources.
