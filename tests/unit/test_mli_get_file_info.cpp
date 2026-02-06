#include "../../include/edasm/emulator/bus.hpp"
#include "../../include/edasm/emulator/cpu.hpp"
#include "../../include/edasm/emulator/mli.hpp"
#include "../../include/edasm/emulator/traps.hpp"
#include <cassert>
#include <fstream>
#include <filesystem>
#include <iostream>

using namespace edasm;

// Helper function to set up MLI call structure for GET_FILE_INFO
void setup_get_file_info_call(Bus &bus, CPUState &state, const std::string &path) {
    // Set up MLI call structure
    // JSR $BF00 at $2000-$2002, return address is $2002
    state.SP = 0xFD;
    bus.write(0x01FE, 0x02); // Return address low byte
    bus.write(0x01FF, 0x20); // Return address high byte

    // GET_FILE_INFO command at $2003
    bus.write(0x2003, 0xC4); // GET_FILE_INFO command
    bus.write(0x2004, 0x00); // Parameter list pointer low
    bus.write(0x2005, 0x30); // Parameter list pointer high

    // Set up parameter list at $3000
    // param_count = 10 (1 input + 9 output)
    bus.write(0x3000, 10);

    // pathname pointer (points to $3100)
    bus.write(0x3001, 0x00);
    bus.write(0x3002, 0x31);

    // Output parameters at $3003-$3019 (we don't need to initialize these)
    // access (byte at $3003)
    // file_type (byte at $3004)
    // aux_type (word at $3005-$3006)
    // storage_type (byte at $3007)
    // blocks_used (word at $3008-$3009)
    // mod_date (word at $300A-$300B)
    // mod_time (word at $300C-$300D)
    // create_date (word at $300E-$300F)
    // create_time (word at $3010-$3011)
    // eof (3 bytes at $3012-$3014)

    // Set up pathname at $3100 (length-prefixed string)
    bus.write(0x3100, static_cast<uint8_t>(path.length()));
    for (size_t i = 0; i < path.length(); ++i) {
        bus.write(static_cast<uint16_t>(0x3101 + i), static_cast<uint8_t>(path[i]));
    }
}

// Test GET_FILE_INFO with a valid text file (.txt extension)
void test_get_file_info_text_file() {
    Bus bus;
    CPU cpu(bus);
    CPUState &state = cpu.state();

    // Create a temporary test file
    std::string test_file = "/tmp/test_get_file_info.txt";
    std::ofstream ofs(test_file);
    ofs << "Test content for text file";
    ofs.close();

    // Get actual file size
    auto file_size = std::filesystem::file_size(test_file);

    setup_get_file_info_call(bus, state, test_file);

    // Call the MLI handler
    bool result = MLIHandler::prodos_mli_trap_handler(state, bus, 0xBF00);

    // Should continue execution
    assert(result == true);

    // Should succeed (A = 0)
    assert(state.A == 0x00);

    // Carry flag should be clear (no error)
    assert(!(state.P & StatusFlags::C));

    // Zero flag should be set (A = 0)
    assert(state.P & StatusFlags::Z);

    // PC should be advanced past the MLI call structure
    assert(state.PC == 0x2006);

    // SP should be restored
    assert(state.SP == 0xFF);

    // Check output parameters
    uint8_t access = bus.read(0x3003);
    uint8_t file_type = bus.read(0x3004);
    uint8_t storage_type = bus.read(0x3007);
    uint32_t eof = bus.read(0x3012) | (bus.read(0x3013) << 8) | (bus.read(0x3014) << 16);

    assert(access == 0xC3); // Read/write access
    assert(file_type == 0x04); // TXT file type
    assert(storage_type == 0x01); // Seedling file
    assert(eof == file_size); // EOF should match file size

    // Clean up
    std::filesystem::remove(test_file);

    std::cout << "✓ test_get_file_info_text_file passed" << std::endl;
}

// Test GET_FILE_INFO with a valid binary file (.bin extension)
void test_get_file_info_bin_file() {
    Bus bus;
    CPU cpu(bus);
    CPUState &state = cpu.state();

    // Create a temporary test file
    std::string test_file = "/tmp/test_get_file_info.bin";
    std::ofstream ofs(test_file, std::ios::binary);
    ofs << "Binary data";
    ofs.close();

    auto file_size = std::filesystem::file_size(test_file);

    setup_get_file_info_call(bus, state, test_file);

    // Call the MLI handler
    bool result = MLIHandler::prodos_mli_trap_handler(state, bus, 0xBF00);

    // Should succeed
    assert(result == true);
    assert(state.A == 0x00);
    assert(!(state.P & StatusFlags::C));
    assert(state.P & StatusFlags::Z);

    // Check output parameters
    uint8_t file_type = bus.read(0x3004);
    uint8_t storage_type = bus.read(0x3007);
    uint32_t eof = bus.read(0x3012) | (bus.read(0x3013) << 8) | (bus.read(0x3014) << 16);

    assert(file_type == 0x06); // BIN file type
    assert(storage_type == 0x01); // Seedling file
    assert(eof == file_size);

    // Clean up
    std::filesystem::remove(test_file);

    std::cout << "✓ test_get_file_info_bin_file passed" << std::endl;
}

// Test GET_FILE_INFO with a valid source file (.src extension)
void test_get_file_info_src_file() {
    Bus bus;
    CPU cpu(bus);
    CPUState &state = cpu.state();

    // Create a temporary test file
    std::string test_file = "/tmp/test_get_file_info.src";
    std::ofstream ofs(test_file);
    ofs << "    LDA #$00\n    RTS\n";
    ofs.close();

    auto file_size = std::filesystem::file_size(test_file);

    setup_get_file_info_call(bus, state, test_file);

    // Call the MLI handler
    bool result = MLIHandler::prodos_mli_trap_handler(state, bus, 0xBF00);

    // Should succeed
    assert(result == true);
    assert(state.A == 0x00);
    assert(!(state.P & StatusFlags::C));

    // Check output parameters
    uint8_t file_type = bus.read(0x3004);
    uint8_t storage_type = bus.read(0x3007);
    uint32_t eof = bus.read(0x3012) | (bus.read(0x3013) << 8) | (bus.read(0x3014) << 16);

    assert(file_type == 0x04); // TXT file type (source treated as text)
    assert(storage_type == 0x01); // Seedling file
    assert(eof == file_size);

    // Clean up
    std::filesystem::remove(test_file);

    std::cout << "✓ test_get_file_info_src_file passed" << std::endl;
}

// Test GET_FILE_INFO with a valid system file (.sys extension)
void test_get_file_info_sys_file() {
    Bus bus;
    CPU cpu(bus);
    CPUState &state = cpu.state();

    // Create a temporary test file
    std::string test_file = "/tmp/test_get_file_info.sys";
    std::ofstream ofs(test_file, std::ios::binary);
    ofs << "System file content";
    ofs.close();

    auto file_size = std::filesystem::file_size(test_file);

    setup_get_file_info_call(bus, state, test_file);

    // Call the MLI handler
    bool result = MLIHandler::prodos_mli_trap_handler(state, bus, 0xBF00);

    // Should succeed
    assert(result == true);
    assert(state.A == 0x00);
    assert(!(state.P & StatusFlags::C));

    // Check output parameters
    uint8_t file_type = bus.read(0x3004);
    uint8_t storage_type = bus.read(0x3007);
    uint32_t eof = bus.read(0x3012) | (bus.read(0x3013) << 8) | (bus.read(0x3014) << 16);

    assert(file_type == 0xFF); // SYS file type
    assert(storage_type == 0x01); // Seedling file
    assert(eof == file_size);

    // Clean up
    std::filesystem::remove(test_file);

    std::cout << "✓ test_get_file_info_sys_file passed" << std::endl;
}

// Test GET_FILE_INFO with a directory
void test_get_file_info_directory() {
    Bus bus;
    CPU cpu(bus);
    CPUState &state = cpu.state();

    // Create a temporary test directory
    std::string test_dir = "/tmp/test_get_file_info_dir";
    std::filesystem::create_directory(test_dir);

    // Create some files in the directory to test entry count
    std::ofstream(test_dir + "/file1.txt") << "content1";
    std::ofstream(test_dir + "/file2.txt") << "content2";
    std::ofstream(test_dir + "/file3.bin") << "content3";

    setup_get_file_info_call(bus, state, test_dir);

    // Call the MLI handler
    bool result = MLIHandler::prodos_mli_trap_handler(state, bus, 0xBF00);

    // Should succeed
    assert(result == true);
    assert(state.A == 0x00);
    assert(!(state.P & StatusFlags::C));
    assert(state.P & StatusFlags::Z);

    // Check output parameters
    uint8_t file_type = bus.read(0x3004);
    uint8_t storage_type = bus.read(0x3007);
    uint32_t eof = bus.read(0x3012) | (bus.read(0x3013) << 8) | (bus.read(0x3014) << 16);

    assert(file_type == 0x0F); // Directory file type
    assert(storage_type == 0x0D); // Directory storage type

    // EOF should be 512 (header) + (entry_count * 39)
    // We created 3 files
    uint32_t expected_eof = 512 + (3 * 39);
    assert(eof == expected_eof);

    // Clean up
    std::filesystem::remove_all(test_dir);

    std::cout << "✓ test_get_file_info_directory passed" << std::endl;
}

// Test GET_FILE_INFO with non-existent file
void test_get_file_info_file_not_found() {
    Bus bus;
    CPU cpu(bus);
    CPUState &state = cpu.state();

    // Use a path that doesn't exist
    std::string test_file = "/tmp/nonexistent_file_12345.txt";

    // Ensure the file doesn't exist
    std::filesystem::remove(test_file);

    setup_get_file_info_call(bus, state, test_file);

    // Call the MLI handler
    bool result = MLIHandler::prodos_mli_trap_handler(state, bus, 0xBF00);

    // Should continue execution
    assert(result == true);

    // Should return FILE_NOT_FOUND error (0x46)
    assert(state.A == 0x46);

    // Carry flag should be set (error condition)
    assert(state.P & StatusFlags::C);

    // PC should still be advanced
    assert(state.PC == 0x2006);

    std::cout << "✓ test_get_file_info_file_not_found passed" << std::endl;
}

int main() {
    std::cout << "Running MLI GET_FILE_INFO tests..." << std::endl;
    std::cout << std::endl;

    test_get_file_info_text_file();
    test_get_file_info_bin_file();
    test_get_file_info_src_file();
    test_get_file_info_sys_file();
    test_get_file_info_directory();
    test_get_file_info_file_not_found();

    std::cout << std::endl;
    std::cout << "All MLI GET_FILE_INFO tests passed!" << std::endl;
    return 0;
}
