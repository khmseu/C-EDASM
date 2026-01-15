add_executable(test_editor test_editor.cpp)
target_link_libraries(test_editor PRIVATE edasm)
target_include_directories(test_editor PRIVATE ${CMAKE_SOURCE_DIR}/include)

add_test(
  NAME test_editor
  COMMAND test_editor
)

add_test(
  NAME smoke
  COMMAND ${CMAKE_COMMAND} -E echo "tests pending"
)
