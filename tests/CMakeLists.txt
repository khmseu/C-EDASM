# ============================================================================
# Integration Tests (command-line tools for assembling and linking)
# ============================================================================

# Test assembler program
add_executable(test_asm integration/test_asm.cpp)
target_link_libraries(test_asm PRIVATE edasm)
target_include_directories(test_asm PRIVATE ${CMAKE_SOURCE_DIR}/include)

# Test assembler with listing generation
add_executable(test_asm_listing integration/test_asm_listing.cpp)
target_link_libraries(test_asm_listing PRIVATE edasm)
target_include_directories(test_asm_listing PRIVATE ${CMAKE_SOURCE_DIR}/include)

# Test linker program
add_executable(test_linker integration/test_linker.cpp)
target_link_libraries(test_linker PRIVATE edasm)
target_include_directories(test_linker PRIVATE ${CMAKE_SOURCE_DIR}/include)

# Test linker debug program
add_executable(test_linker_debug integration/test_linker_debug.cpp)
target_link_libraries(test_linker_debug PRIVATE edasm)
target_include_directories(test_linker_debug PRIVATE ${CMAKE_SOURCE_DIR}/include)

# Test assembler with REL output
add_executable(test_asm_rel integration/test_asm_rel.cpp)
target_link_libraries(test_asm_rel PRIVATE edasm)
target_include_directories(test_asm_rel PRIVATE ${CMAKE_SOURCE_DIR}/include)

# Test assembler integration
add_executable(test_assembler_integration integration/test_assembler_integration.cpp)
target_link_libraries(test_assembler_integration PRIVATE edasm)
target_include_directories(test_assembler_integration PRIVATE ${CMAKE_SOURCE_DIR}/include)

add_test(
  NAME test_assembler_integration
  COMMAND test_assembler_integration
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)

# ============================================================================
# Unit Tests
# ============================================================================

add_executable(test_editor unit/test_editor.cpp)
target_link_libraries(test_editor PRIVATE edasm)
target_include_directories(test_editor PRIVATE ${CMAKE_SOURCE_DIR}/include)

add_test(
  NAME test_editor
  COMMAND test_editor
)

add_executable(test_emulator unit/test_emulator.cpp)
target_link_libraries(test_emulator PRIVATE edasm)
target_include_directories(test_emulator PRIVATE ${CMAKE_SOURCE_DIR}/include)

add_test(
  NAME test_emulator
  COMMAND test_emulator
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)

add_executable(test_io_traps unit/test_io_traps.cpp)
target_link_libraries(test_io_traps PRIVATE edasm)
target_include_directories(test_io_traps PRIVATE ${CMAKE_SOURCE_DIR}/include)

add_test(
  NAME test_io_traps
  COMMAND test_io_traps
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)

add_executable(test_mli_descriptors unit/test_mli_descriptors.cpp)
target_link_libraries(test_mli_descriptors PRIVATE edasm)
target_include_directories(test_mli_descriptors PRIVATE ${CMAKE_SOURCE_DIR}/include)

add_test(
  NAME test_mli_descriptors
  COMMAND test_mli_descriptors
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)

add_executable(test_mli_stubs unit/test_mli_stubs.cpp)
target_link_libraries(test_mli_stubs PRIVATE edasm)
target_include_directories(test_mli_stubs PRIVATE ${CMAKE_SOURCE_DIR}/include)

add_test(
  NAME test_mli_stubs
  COMMAND test_mli_stubs
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)

add_executable(test_mli_lookup_performance unit/test_mli_lookup_performance.cpp)
target_link_libraries(test_mli_lookup_performance PRIVATE edasm)
target_include_directories(test_mli_lookup_performance PRIVATE ${CMAKE_SOURCE_DIR}/include)

add_test(
  NAME test_mli_lookup_performance
  COMMAND test_mli_lookup_performance
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)

add_executable(test_mli_newline unit/test_mli_newline.cpp)
target_link_libraries(test_mli_newline PRIVATE edasm)
target_include_directories(test_mli_newline PRIVATE ${CMAKE_SOURCE_DIR}/include)

add_test(
  NAME test_mli_newline
  COMMAND test_mli_newline
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)

add_executable(test_mli_read_eof unit/test_mli_read_eof.cpp)
target_link_libraries(test_mli_read_eof PRIVATE edasm)
target_include_directories(test_mli_read_eof PRIVATE ${CMAKE_SOURCE_DIR}/include)

add_test(
  NAME test_mli_read_eof
  COMMAND test_mli_read_eof
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)

add_executable(test_mli_set_file_info unit/test_mli_set_file_info.cpp)
target_link_libraries(test_mli_set_file_info PRIVATE edasm)
target_include_directories(test_mli_set_file_info PRIVATE ${CMAKE_SOURCE_DIR}/include)

add_test(
  NAME test_mli_set_file_info
  COMMAND test_mli_set_file_info
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)

add_executable(test_mli_get_file_info unit/test_mli_get_file_info.cpp)
target_link_libraries(test_mli_get_file_info PRIVATE edasm)
target_include_directories(test_mli_get_file_info PRIVATE ${CMAKE_SOURCE_DIR}/include)

add_test(
  NAME test_mli_get_file_info
  COMMAND test_mli_get_file_info
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)

# Language card soft-switch unit test
add_executable(test_language_card unit/test_language_card.cpp)
target_link_libraries(test_language_card PRIVATE edasm)
target_include_directories(test_language_card PRIVATE ${CMAKE_SOURCE_DIR}/include)

add_test(
  NAME test_language_card
  COMMAND test_language_card
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)

# ROM loading and reset vector test
add_executable(test_rom_reset unit/test_rom_reset.cpp)
target_link_libraries(test_rom_reset PRIVATE edasm)
target_include_directories(test_rom_reset PRIVATE ${CMAKE_SOURCE_DIR}/include)

add_test(
  NAME test_rom_reset
  COMMAND test_rom_reset
  WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
)

# ============================================================================
# Integration Test Cases (using fixture files)
# ============================================================================

add_test(
  NAME test_asm_simple
  COMMAND test_asm ${CMAKE_SOURCE_DIR}/tests/fixtures/test_simple.src
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)

add_test(
  NAME test_asm_expressions
  COMMAND test_asm ${CMAKE_SOURCE_DIR}/tests/fixtures/test_expressions.src
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)

add_test(
  NAME test_asm_directives
  COMMAND test_asm ${CMAKE_SOURCE_DIR}/tests/fixtures/test_directives.src
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)

add_test(
  NAME test_asm_listing_simple
  COMMAND test_asm_listing ${CMAKE_SOURCE_DIR}/tests/fixtures/test_simple.src
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)

add_test(
  NAME test_asm_rel_module1
  COMMAND test_asm_rel ${CMAKE_SOURCE_DIR}/tests/fixtures/test_module1.src ${CMAKE_CURRENT_BINARY_DIR}/test_module1_output.rel
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)

add_test(
  NAME test_asm_rel_module2
  COMMAND test_asm_rel ${CMAKE_SOURCE_DIR}/tests/fixtures/test_module2.src ${CMAKE_CURRENT_BINARY_DIR}/test_module2_output.rel
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)

add_test(
  NAME test_linker_modules
  COMMAND test_linker ${CMAKE_CURRENT_BINARY_DIR}/linked_output.bin ${CMAKE_CURRENT_BINARY_DIR}/test_module1_output.rel ${CMAKE_CURRENT_BINARY_DIR}/test_module2_output.rel
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)

# Set test dependencies - linker test needs the .rel files from earlier tests
set_tests_properties(test_linker_modules PROPERTIES
  DEPENDS "test_asm_rel_module1;test_asm_rel_module2"
)

# Set test properties for better organization
set_tests_properties(test_asm_simple test_asm_expressions test_asm_directives PROPERTIES
  LABELS "assembler"
)

set_tests_properties(test_asm_listing_simple PROPERTIES
  LABELS "listing"
)

set_tests_properties(test_asm_rel_module1 test_asm_rel_module2 test_linker_modules PROPERTIES
  LABELS "linker"
)

set_tests_properties(test_editor test_assembler_integration test_emulator test_mli_descriptors test_mli_stubs test_mli_lookup_performance test_mli_newline test_mli_read_eof test_mli_set_file_info test_mli_get_file_info test_language_card test_io_traps test_rom_reset PROPERTIES
  LABELS "unit"
)
