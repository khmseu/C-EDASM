# Project Guidelines

## Code Style
- C++20; prefer `std::string_view`, structured bindings, and RAII/smart pointers as shown in [README.md](README.md) and [include/edasm](include/edasm/).
- Public headers mirror src structure; keep namespaces under `edasm`. See [README.md](README.md).
- Add EDASM.SRC references in comments; see guidance in [docs/README.md](docs/README.md).

## Architecture
- Module boundaries: core, editor, assembler, files, emulator. Keep logic in its module; see [README.md](README.md) and [docs/PORTING_PLAN.md](docs/PORTING_PLAN.md).
- Assembler is two-pass with optional listing; see [docs/ASSEMBLER_ARCHITECTURE.md](docs/ASSEMBLER_ARCHITECTURE.md).
- Emulator runner expects ProDOS paths to map 1:1 to Linux paths; see [src/emulator_runner.cpp](src/emulator_runner.cpp) and [docs/Apple ProDOS 8 Technical Reference Manual.txt](docs/Apple%20ProDOS%208%20Technical%20Reference%20Manual.txt).

## Build and Test
- Initialize EDASM.SRC: `git submodule update --init --recursive`.
- Configure/build: `./scripts/configure.sh` then `./scripts/build.sh`.
- Tests: `cd build && ctest`; category filters and structure in [tests/README.md](tests/README.md).
- Emulator smoke run: `./scripts/run_emulator_runner.sh` (log at [EDASM.TEST/emulator_runner.log](EDASM.TEST/emulator_runner.log)).

## Project Conventions
- EDASM parity is strict: command syntax, directive names, and operator rules must match EDASM. See [README.md](README.md) and [docs/COMMAND_REFERENCE.md](docs/COMMAND_REFERENCE.md).
- Bitwise ops are non-standard: `^` AND, `!` XOR, `|` OR; EDASM operator precedence differs. See [README.md](README.md).
- EDASM.SRC is the source of truth in [third_party/EdAsm/EDASM.SRC](third_party/EdAsm/EDASM.SRC).
- ProDOS file type mapping lives in [src/files/prodos_file.cpp](src/files/prodos_file.cpp) and [include/edasm/files/prodos_file.hpp](include/edasm/files/prodos_file.hpp).

## Integration Points
- Symbol table and linker behavior are centralized; see [docs/ASSEMBLER_ARCHITECTURE.md](docs/ASSEMBLER_ARCHITECTURE.md) and [src/assembler/linker.cpp](src/assembler/linker.cpp).
- Test fixtures are in [tests/fixtures](tests/fixtures).

## Security
- Path handling must follow ProDOS semantics; consult [docs/Apple ProDOS 8 Technical Reference Manual.txt](docs/Apple%20ProDOS%208%20Technical%20Reference%20Manual.txt).
- INCLUDE disallows nesting; verify logic in [src/assembler/assembler.cpp](src/assembler/assembler.cpp) when changing include handling.
