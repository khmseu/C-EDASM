cmake_minimum_required(VERSION 3.16)
project(edasm LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(Curses REQUIRED)

add_library(edasm
  src/core/app.cpp
  src/core/screen.cpp
  src/emulator/bus.cpp
  src/emulator/cpu.cpp
  src/emulator/traps.cpp
  src/emulator/mli.cpp
  src/emulator/host_shims.cpp
  src/assembler/assembler.cpp
  src/assembler/symbol_table.cpp
  src/assembler/tokenizer.cpp
  src/assembler/opcode_table.cpp
  src/assembler/expression.cpp
  src/assembler/listing.cpp
  src/assembler/linker.cpp
  src/editor/editor.cpp
  src/files/prodos_file.cpp
)

target_include_directories(edasm
  PUBLIC include
  PRIVATE ${CURSES_INCLUDE_DIR}
)

target_link_libraries(edasm PRIVATE ${CURSES_LIBRARIES})

target_compile_features(edasm PUBLIC cxx_std_20)

add_executable(edasm_cli src/main.cpp)
target_link_libraries(edasm_cli PRIVATE edasm)

# Emulator runner for testing original EDASM.SYSTEM
add_executable(emulator_runner src/emulator_runner.cpp)
target_link_libraries(emulator_runner PRIVATE edasm)

enable_testing()
add_subdirectory(tests)
