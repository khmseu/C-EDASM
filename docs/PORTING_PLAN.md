# Porting Plan (EDASM to C++/ncurses)

## Goals

- Preserve original EDASM editor/assembler semantics while targeting Linux terminal.
- Mirror source structure from `EDASM.SRC` for traceability.
- Replace ProDOS file model with Linux paths and extensions.

## Source Mapping (initial)

- `EDITOR/` → screen/buffer management, command loop → `src/editor`, `src/core/screen`.
- `ASM/` → assembler passes, symbol handling → `src/assembler`.
- `LINKER/` → output handling; may integrate into assembler for now.
- `COMMONEQUS.S` → shared constants → `include/edasm` headers.

## Platform Adaptations

- Screen: ncurses wrapper (`Screen`) replaces direct Apple II video/monitor calls.
- Keyboard: ncurses `getch` with mapping to EDASM commands.
- Files: map ProDOS types to extensions (`.src`, `.obj`, `.sys`, `.lst`, `.txt`, `.bin`).
- Paths: absolute/relative POSIX paths; no volume slots.

## Incremental Steps

1. Extract EDASM command set and buffers from `EDITOR/` and mirror data structures in C++.
2. Implement two-pass assembler: tokenization, symbol table, expression eval, opcode emission.
3. Add file I/O (load/save source, write object/listing) using extension mapping.
4. Recreate toolchain utilities (linker, Sweet16 support) as separate modules.
5. Add tests derived from original sample sources.

## Notes

- Preserve numeric formats (hex/decimal) and directive names as in original sources.
- Keep data layout close to original to ease comparison while using modern safety where trivial.
