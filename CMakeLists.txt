cmake_minimum_required(VERSION 3.16)
project(edasm LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(Curses REQUIRED)

add_library(edasm
  src/core/app.cpp
  src/core/screen.cpp
  src/assembler/assembler.cpp
  src/assembler/symbol_table.cpp
  src/assembler/tokenizer.cpp
  src/assembler/opcode_table.cpp
  src/assembler/expression.cpp
  src/assembler/listing.cpp
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

# Test assembler program
add_executable(test_asm src/test_asm.cpp)
target_link_libraries(test_asm PRIVATE edasm)

# Test assembler with listing generation
add_executable(test_asm_listing src/test_asm_listing.cpp)
target_link_libraries(test_asm_listing PRIVATE edasm)

enable_testing()
add_subdirectory(tests)
