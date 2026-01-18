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

# Add tests for the main test executables
add_test(
  NAME test_asm_simple
  COMMAND ${CMAKE_BINARY_DIR}/test_asm ${CMAKE_SOURCE_DIR}/tests/test_simple.src
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)

add_test(
  NAME test_asm_expressions
  COMMAND ${CMAKE_BINARY_DIR}/test_asm ${CMAKE_SOURCE_DIR}/tests/test_expressions.src
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)

add_test(
  NAME test_asm_directives
  COMMAND ${CMAKE_BINARY_DIR}/test_asm ${CMAKE_SOURCE_DIR}/tests/test_directives.src
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)

add_test(
  NAME test_asm_listing_simple
  COMMAND ${CMAKE_BINARY_DIR}/test_asm_listing ${CMAKE_SOURCE_DIR}/tests/test_simple.src
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)

add_test(
  NAME test_asm_rel_module1
  COMMAND ${CMAKE_BINARY_DIR}/test_asm_rel ${CMAKE_SOURCE_DIR}/tests/test_module1.src ${CMAKE_CURRENT_BINARY_DIR}/test_module1_output.rel
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)

add_test(
  NAME test_asm_rel_module2
  COMMAND ${CMAKE_BINARY_DIR}/test_asm_rel ${CMAKE_SOURCE_DIR}/tests/test_module2.src ${CMAKE_CURRENT_BINARY_DIR}/test_module2_output.rel
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)

add_test(
  NAME test_linker_modules
  COMMAND ${CMAKE_BINARY_DIR}/test_linker ${CMAKE_CURRENT_BINARY_DIR}/linked_output.bin ${CMAKE_SOURCE_DIR}/tests/test_module1.rel ${CMAKE_SOURCE_DIR}/tests/test_module2.rel
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

set_tests_properties(test_editor test_assembler_integration PROPERTIES
  LABELS "unit"
)
