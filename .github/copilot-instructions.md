# C-EDASM AI Guidance

- Project: C++20/ncurses port of Apple II EDASM (6502 editor/assembler); preserve original behavior while modernizing C++.
- Layout: src/core (app loop, screen), src/editor (text buffer/commands), src/assembler (two-pass assembler, symbol table, expression eval), src/files (ProDOS file type mapping). Headers mirror in include/edasm/.
- Documentation: docs/ASSEMBLER_ARCHITECTURE.md (assembler passes, symbol table), docs/COMMAND_REFERENCE.md (command semantics), docs/PORTING_PLAN.md (14-week roadmap), docs/6502_INSTRUCTION_SET.md (opcode reference). Refer to third_party/EdAsm/EDASM.SRC for source-of-truth 6502.
- Submodule: run `git submodule update --init --recursive` before work to pull EDASM.SRC.
- Build: `./scripts/configure.sh` (uses BUILD_TYPE, default Debug) or `cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug`; build via `./scripts/build.sh` or `cmake --build build`; binary at ./build/edasm_cli (currently placeholder UI).
- Tests: after configuring, `cd build && ctest`.
- Dependencies: CMake >=3.16, C++20 compiler (GCC/Clang), ncurses dev headers (e.g., libncurses5-dev on Debian/Ubuntu).
- Conventions: maintain 1:1 parity with EDASM commands/directives; use third_party/EdAsm/EDASM.SRC for correctness; favor RAII/stdlib while keeping logic close to 6502 flow; keep module boundaries (UI in core/editor, assembly logic in assembler, file metadata in files).
- File semantics: ProDOS type/extension tables live in src/files and include/edasm/files/prodos_file.hpp—preserve codes and mappings.
- Workflow: out-of-source builds in build/; work from repo root; avoid adding media/external links; only add .vscode for tasks if needed; prefer small focused PRs and add/update tests in tests/ alongside code.
