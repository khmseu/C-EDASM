# Tests Directory

This directory contains all tests for the C-EDASM project, organized into three main subdirectories:

## Directory Structure

### `unit/`
Unit tests for individual components and modules:
- `test_editor.cpp` - Editor functionality tests
- `test_emulator.cpp` - 65C02 emulator tests
- `test_io_traps.cpp` - I/O trap mechanism tests

### `integration/`
Integration tests for command-line tools and end-to-end functionality:
- `test_asm.cpp` - Standalone assembler tool
- `test_asm_listing.cpp` - Assembler with listing generation
- `test_asm_rel.cpp` - Assembler with REL output
- `test_assembler_integration.cpp` - Comprehensive assembler tests
- `test_linker.cpp` - Linker tool
- `test_linker_debug.cpp` - Linker with debug output
- `test_link_debug.cpp` - Additional linker debug tests

### `fixtures/`
Test data files used by the tests:
- `*.src` - Assembly source files for testing
- `*.txt` - Test command files
- `*.rel` - Pre-built relocatable object files (if any)

## Running Tests

### Run All Tests
```bash
cd build && ctest
```

### Run Specific Test Categories
```bash
# Unit tests only
cd build && ctest -L unit

# Integration tests only
cd build && ctest -L assembler
cd build && ctest -L linker

# Listing tests
cd build && ctest -L listing
```

### Run Individual Tests
```bash
cd build && ctest -R test_editor
cd build && ctest -R test_assembler_integration
```

### Verbose Output
```bash
cd build && ctest -V -R test_name
```

## Test Dependencies

The `test_linker_modules` test depends on output from:
- `test_asm_rel_module1` - Generates test_module1_output.rel
- `test_asm_rel_module2` - Generates test_module2_output.rel

These dependencies are automatically handled by CMake.

## Adding New Tests

### Adding a Unit Test
1. Create `unit/test_myfeature.cpp`
2. Add to `CMakeLists.txt`:
   ```cmake
   add_executable(test_myfeature unit/test_myfeature.cpp)
   target_link_libraries(test_myfeature PRIVATE edasm)
   target_include_directories(test_myfeature PRIVATE ${CMAKE_SOURCE_DIR}/include)
   
   add_test(NAME test_myfeature COMMAND test_myfeature)
   set_tests_properties(test_myfeature PROPERTIES LABELS "unit")
   ```

### Adding an Integration Test
1. Create `integration/test_mytool.cpp`
2. Add to `CMakeLists.txt` following the pattern of existing integration tests

### Adding Test Fixtures
1. Place new `.src` or data files in `fixtures/`
2. Reference them in tests using: `${CMAKE_SOURCE_DIR}/tests/fixtures/filename.src`
