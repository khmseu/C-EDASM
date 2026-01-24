# Test assembler program
add_executable(test_asm test_asm.cpp)
target_link_libraries(test_asm PRIVATE edasm)
target_include_directories(test_asm PRIVATE ${CMAKE_SOURCE_DIR}/include)

# Test assembler with listing generation
add_executable(test_asm_listing test_asm_listing.cpp)
target_link_libraries(test_asm_listing PRIVATE edasm)
target_include_directories(test_asm_listing PRIVATE ${CMAKE_SOURCE_DIR}/include)

# Test linker program
add_executable(test_linker test_linker.cpp)
target_link_libraries(test_linker PRIVATE edasm)
target_include_directories(test_linker PRIVATE ${CMAKE_SOURCE_DIR}/include)

# Test assembler with REL output
add_executable(test_asm_rel test_asm_rel.cpp)
target_link_libraries(test_asm_rel PRIVATE edasm)
target_include_directories(test_asm_rel PRIVATE ${CMAKE_SOURCE_DIR}/include)

add_executable(test_editor test_editor.cpp)
target_link_libraries(test_editor PRIVATE edasm)
target_include_directories(test_editor PRIVATE ${CMAKE_SOURCE_DIR}/include)

add_test(
  NAME test_editor
  COMMAND test_editor
)

add_executable(test_assembler_integration test_assembler_integration.cpp)
target_link_libraries(test_assembler_integration PRIVATE edasm)
target_include_directories(test_assembler_integration PRIVATE ${CMAKE_SOURCE_DIR}/include)

add_test(
  NAME test_assembler_integration
  COMMAND test_assembler_integration
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)

add_executable(test_emulator test_emulator.cpp)
target_link_libraries(test_emulator PRIVATE edasm)
target_include_directories(test_emulator PRIVATE ${CMAKE_SOURCE_DIR}/include)

add_test(
  NAME test_emulator
  COMMAND test_emulator
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)

add_executable(test_io_traps test_io_traps.cpp)
target_link_libraries(test_io_traps PRIVATE edasm)
target_include_directories(test_io_traps PRIVATE ${CMAKE_SOURCE_DIR}/include)

add_test(
  NAME test_io_traps
  COMMAND test_io_traps
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)

# Add tests for the main test executables
add_test(
  NAME test_asm_simple
  COMMAND test_asm ${CMAKE_SOURCE_DIR}/tests/test_simple.src
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)

add_test(
  NAME test_asm_expressions
  COMMAND test_asm ${CMAKE_SOURCE_DIR}/tests/test_expressions.src
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)

add_test(
  NAME test_asm_directives
  COMMAND test_asm ${CMAKE_SOURCE_DIR}/tests/test_directives.src
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)

add_test(
  NAME test_asm_listing_simple
  COMMAND test_asm_listing ${CMAKE_SOURCE_DIR}/tests/test_simple.src
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)

add_test(
  NAME test_asm_rel_module1
  COMMAND test_asm_rel ${CMAKE_SOURCE_DIR}/tests/test_module1.src ${CMAKE_CURRENT_BINARY_DIR}/test_module1_output.rel
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)

add_test(
  NAME test_asm_rel_module2
  COMMAND test_asm_rel ${CMAKE_SOURCE_DIR}/tests/test_module2.src ${CMAKE_CURRENT_BINARY_DIR}/test_module2_output.rel
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)

add_test(
  NAME test_linker_modules
  COMMAND test_linker ${CMAKE_CURRENT_BINARY_DIR}/linked_output.bin ${CMAKE_SOURCE_DIR}/tests/test_module1.rel ${CMAKE_SOURCE_DIR}/tests/test_module2.rel
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
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

set_tests_properties(test_editor test_assembler_integration test_emulator PROPERTIES
  LABELS "unit"
)
