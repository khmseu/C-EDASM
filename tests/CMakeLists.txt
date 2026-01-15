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
