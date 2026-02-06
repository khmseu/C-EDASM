#include "../../include/edasm/emulator/bus.hpp"
#include "../../include/edasm/emulator/cpu.hpp"
#include "../../include/edasm/emulator/mli.hpp"
#include "../../include/edasm/emulator/traps.hpp"
#include <cassert>
#include <fstream>
#include <filesystem>
#include <iomanip>
#include <iostream>

using namespace edasm;

// Test SET_FILE_INFO with a valid file
void test_set_file_info_valid_file() {
    Bus bus;
    CPU cpu(bus);
    CPUState &state = cpu.state();

    // Create a temporary test file
    std::string test_file = "/tmp/test_set_file_info.txt";
    std::ofstream ofs(test_file);
    ofs << "Test content";
    ofs.close();

    // Set up MLI call structure
    // JSR $BF00 at $2000-$2002, return address is $2002
    state.SP = 0xFD;
    bus.write(0x01FE, 0x02); // Return address low byte
    bus.write(0x01FF, 0x20); // Return address high byte

    // SET_FILE_INFO command at $2003
    bus.write(0x2003, 0xC3); // SET_FILE_INFO command
    bus.write(0x2004, 0x00); // Parameter list pointer low
    bus.write(0x2005, 0x30); // Parameter list pointer high

    // Set up parameter list at $3000
    // param_count = 7
    bus.write(0x3000, 7);

    // pathname pointer (points to $3100)
    bus.write(0x3001, 0x00);
    bus.write(0x3002, 0x31);

    // Set up pathname at $3100 (length-prefixed string)
    std::string path = test_file;
    bus.write(0x3100, static_cast<uint8_t>(path.length()));
    for (size_t i = 0; i < path.length(); ++i) {
        bus.write(static_cast<uint16_t>(0x3101 + i), static_cast<uint8_t>(path[i]));
    }

    // access (byte at $3003)
    bus.write(0x3003, 0xC3); // Read/write access

    // file_type (byte at $3004)
    bus.write(0x3004, 0x04); // TEXT file type

    // aux_type (word at $3005-$3006)
    bus.write(0x3005, 0x00);
    bus.write(0x3006, 0x00);

    // reserved1 (byte at $3007)
    bus.write(0x3007, 0x00);

    // mod_date (word at $3008-$3009)
    bus.write(0x3008, 0x00);
    bus.write(0x3009, 0x00);

    // mod_time (word at $300A-$300B)
    bus.write(0x300A, 0x00);
    bus.write(0x300B, 0x00);

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

    // Clean up
    std::filesystem::remove(test_file);

    std::cout << "✓ test_set_file_info_valid_file passed" << std::endl;
}

// Test SET_FILE_INFO with non-existent file
void test_set_file_info_file_not_found() {
    Bus bus;
    CPU cpu(bus);
    CPUState &state = cpu.state();

    // Set up MLI call structure
    state.SP = 0xFD;
    bus.write(0x01FE, 0x02);
    bus.write(0x01FF, 0x20);

    bus.write(0x2003, 0xC3); // SET_FILE_INFO command
    bus.write(0x2004, 0x00);
    bus.write(0x2005, 0x30);

    // Parameter list at $3000
    bus.write(0x3000, 7); // param_count

    // pathname pointer (points to $3100)
    bus.write(0x3001, 0x00);
    bus.write(0x3002, 0x31);

    // Set up pathname for non-existent file
    std::string path = "/tmp/nonexistent_file_12345.txt";
    bus.write(0x3100, static_cast<uint8_t>(path.length()));
    for (size_t i = 0; i < path.length(); ++i) {
        bus.write(static_cast<uint16_t>(0x3101 + i), static_cast<uint8_t>(path[i]));
    }

    // Other parameters (values don't matter for this test)
    bus.write(0x3003, 0xC3); // access
    bus.write(0x3004, 0x04); // file_type
    bus.write(0x3005, 0x00); // aux_type low
    bus.write(0x3006, 0x00); // aux_type high
    bus.write(0x3007, 0x00); // reserved1
    bus.write(0x3008, 0x00); // mod_date low
    bus.write(0x3009, 0x00); // mod_date high
    bus.write(0x300A, 0x00); // mod_time low
    bus.write(0x300B, 0x00); // mod_time high

    // Call the MLI handler
    bool result = MLIHandler::prodos_mli_trap_handler(state, bus, 0xBF00);

    // Should continue execution
    assert(result == true);

    // Should return FILE_NOT_FOUND error (0x46)
    assert(state.A == 0x46);

    // Carry flag should be set (error condition)
    assert(state.P & StatusFlags::C);

    std::cout << "✓ test_set_file_info_file_not_found passed" << std::endl;
}

// Test SET_FILE_INFO with different file attributes
void test_set_file_info_different_attributes() {
    Bus bus;
    CPU cpu(bus);
    CPUState &state = cpu.state();

    // Create a temporary test file
    std::string test_file = "/tmp/test_set_file_info_attrs.bin";
    std::ofstream ofs(test_file, std::ios::binary);
    ofs << "Binary data";
    ofs.close();

    // Set up MLI call structure
    state.SP = 0xFD;
    bus.write(0x01FE, 0x02);
    bus.write(0x01FF, 0x20);

    bus.write(0x2003, 0xC3); // SET_FILE_INFO command
    bus.write(0x2004, 0x00);
    bus.write(0x2005, 0x30);

    // Parameter list
    bus.write(0x3000, 7); // param_count

    // pathname pointer
    bus.write(0x3001, 0x00);
    bus.write(0x3002, 0x31);

    // Set up pathname
    std::string path = test_file;
    bus.write(0x3100, static_cast<uint8_t>(path.length()));
    for (size_t i = 0; i < path.length(); ++i) {
        bus.write(static_cast<uint16_t>(0x3101 + i), static_cast<uint8_t>(path[i]));
    }

    // Set different file attributes
    bus.write(0x3003, 0xE3); // access (full access)
    bus.write(0x3004, 0x06); // file_type (BIN)
    bus.write(0x3005, 0x00); // aux_type = $2000 (load address)
    bus.write(0x3006, 0x20);
    bus.write(0x3007, 0x00); // reserved1
    bus.write(0x3008, 0x21); // mod_date (some date)
    bus.write(0x3009, 0xA5);
    bus.write(0x300A, 0x15); // mod_time (some time)
    bus.write(0x300B, 0x0C);

    // Call the MLI handler
    bool result = MLIHandler::prodos_mli_trap_handler(state, bus, 0xBF00);

    // Should succeed
    assert(result == true);
    assert(state.A == 0x00);
    assert(!(state.P & StatusFlags::C));

    // Clean up
    std::filesystem::remove(test_file);

    std::cout << "✓ test_set_file_info_different_attributes passed" << std::endl;
}

int main() {
    std::cout << "Running MLI SET_FILE_INFO tests..." << std::endl;
    std::cout << std::endl;

    test_set_file_info_valid_file();
    test_set_file_info_file_not_found();
    test_set_file_info_different_attributes();

    std::cout << std::endl;
    std::cout << "All MLI SET_FILE_INFO tests passed!" << std::endl;
    return 0;
}
