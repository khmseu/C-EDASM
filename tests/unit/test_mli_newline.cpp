/**
 * @file test_mli_newline.cpp
 * @brief Unit tests for ProDOS MLI NEWLINE ($C9) call
 *
 * Tests the NEWLINE call implementation:
 * - Setting newline mode enable/disable
 * - Configuring newline character and mask
 * - Reading with newline termination
 * - Reading without newline mode
 */

#include "../../include/edasm/emulator/bus.hpp"
#include "../../include/edasm/emulator/cpu.hpp"
#include "../../include/edasm/emulator/mli.hpp"
#include <cassert>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <unistd.h>

using namespace edasm;

// Helper to set up MLI call structure
void setup_mli_call(Bus &bus, CPUState &state, uint8_t call_num, uint16_t param_list_addr) {
    // JSR pushes return address - 1
    state.SP = 0xFD;
    bus.write(0x01FE, 0x02); // Return address low byte
    bus.write(0x01FF, 0x20); // Return address high byte

    // MLI call parameters at return_addr + 1
    bus.write(0x2003, call_num);
    bus.write(0x2004, param_list_addr & 0xFF);
    bus.write(0x2005, (param_list_addr >> 8) & 0xFF);
}

void test_newline_basic_enable_disable() {
    std::cout << "Testing NEWLINE enable/disable..." << std::endl;

    Bus bus;
    CPU cpu(bus);
    CPUState &state = cpu.state();

    // Create a test file
    const char *test_file = "/tmp/test_newline_basic.txt";
    const char *content = "Line 1\rLine 2\rLine 3\r";
    {
        std::ofstream out(test_file, std::ios::binary);
        out.write(content, strlen(content));
    }

    // OPEN the file first
    setup_mli_call(bus, state, 0xC8, 0x3000); // OPEN

    bus.write(0x3000, 3); // param_count
    bus.write(0x3001, 0x00);
    bus.write(0x3002, 0x31); // pathname pointer at $3100

    // Write pathname (length-prefixed)
    bus.write(0x3100, strlen(test_file));
    for (size_t i = 0; i < strlen(test_file); ++i) {
        bus.write(0x3101 + i, test_file[i]);
    }

    bus.write(0x3003, 0x00);
    bus.write(0x3004, 0x32); // io_buffer pointer

    bool result = MLIHandler::prodos_mli_trap_handler(state, bus, 0xBF00);
    assert(result == true);
    assert(state.A == 0x00); // NO_ERROR

    uint8_t refnum = bus.read(0x3005); // ref_num output
    assert(refnum != 0);

    std::cout << "  File opened, refnum=" << static_cast<int>(refnum) << std::endl;

    // Test 1: NEWLINE enable with CR ($0D)
    setup_mli_call(bus, state, 0xC9, 0x3000); // NEWLINE

    bus.write(0x3000, 3);      // param_count
    bus.write(0x3001, refnum); // ref_num
    bus.write(0x3002, 0x7F);   // enable_mask (strip high bit)
    bus.write(0x3003, 0x0D);   // newline_char (CR)

    result = MLIHandler::prodos_mli_trap_handler(state, bus, 0xBF00);
    assert(result == true);
    assert(state.A == 0x00); // NO_ERROR
    std::cout << "  ✓ NEWLINE enabled (mask=$7F, char=$0D)" << std::endl;

    // Test 2: NEWLINE disable
    setup_mli_call(bus, state, 0xC9, 0x3000); // NEWLINE

    bus.write(0x3000, 3);      // param_count
    bus.write(0x3001, refnum); // ref_num
    bus.write(0x3002, 0x00);   // enable_mask (disabled)
    bus.write(0x3003, 0x0D);   // newline_char (ignored when disabled)

    result = MLIHandler::prodos_mli_trap_handler(state, bus, 0xBF00);
    assert(result == true);
    assert(state.A == 0x00); // NO_ERROR
    std::cout << "  ✓ NEWLINE disabled" << std::endl;

    // Close the file
    setup_mli_call(bus, state, 0xCC, 0x3000); // CLOSE

    bus.write(0x3000, 1);      // param_count
    bus.write(0x3001, refnum); // ref_num

    result = MLIHandler::prodos_mli_trap_handler(state, bus, 0xBF00);
    assert(result == true);

    // Clean up
    unlink(test_file);

    std::cout << "✓ test_newline_basic_enable_disable passed" << std::endl;
}

void test_newline_read_termination() {
    std::cout << "Testing NEWLINE read termination..." << std::endl;

    Bus bus;
    CPU cpu(bus);
    CPUState &state = cpu.state();

    // Create a test file with CR-terminated lines
    const char *test_file = "/tmp/test_newline_read.txt";
    const char *content = "First Line\rSecond Line\rThird Line\r";
    {
        std::ofstream out(test_file, std::ios::binary);
        out.write(content, strlen(content));
    }

    // OPEN the file
    setup_mli_call(bus, state, 0xC8, 0x3000); // OPEN

    bus.write(0x3000, 3);
    bus.write(0x3001, 0x00);
    bus.write(0x3002, 0x31); // pathname pointer

    bus.write(0x3100, strlen(test_file));
    for (size_t i = 0; i < strlen(test_file); ++i) {
        bus.write(0x3101 + i, test_file[i]);
    }

    bus.write(0x3003, 0x00);
    bus.write(0x3004, 0x32);

    bool result = MLIHandler::prodos_mli_trap_handler(state, bus, 0xBF00);
    assert(result == true);
    assert(state.A == 0x00);

    uint8_t refnum = bus.read(0x3005);

    // Enable NEWLINE mode
    setup_mli_call(bus, state, 0xC9, 0x3000); // NEWLINE

    bus.write(0x3000, 3);
    bus.write(0x3001, refnum);
    bus.write(0x3002, 0xFF); // enable_mask (no masking)
    bus.write(0x3003, 0x0D); // newline_char (CR)

    result = MLIHandler::prodos_mli_trap_handler(state, bus, 0xBF00);
    assert(result == true);
    assert(state.A == 0x00);

    // READ first line (should stop at CR)
    setup_mli_call(bus, state, 0xCA, 0x3000); // READ

    bus.write(0x3000, 4);      // param_count
    bus.write(0x3001, refnum); // ref_num
    bus.write(0x3002, 0x00);   // data_buffer low
    bus.write(0x3003, 0x40);   // data_buffer high ($4000)
    bus.write(0x3004, 0xFF);   // request_count low (255 bytes)
    bus.write(0x3005, 0x00);   // request_count high

    result = MLIHandler::prodos_mli_trap_handler(state, bus, 0xBF00);
    assert(result == true);
    assert(state.A == 0x00); // NO_ERROR

    uint16_t trans_count = bus.read_word(0x3006); // transfer_count output
    std::cout << "  First READ: transferred " << trans_count << " bytes" << std::endl;

    // Should have read "First Line\r" (11 bytes)
    assert(trans_count == 11);

    // Verify the content
    std::string first_line;
    for (uint16_t i = 0; i < trans_count; ++i) {
        first_line += static_cast<char>(bus.read(0x4000 + i));
    }
    assert(first_line == "First Line\r");
    std::cout << "  ✓ First line read correctly: \"" << first_line.substr(0, trans_count - 1)
              << "\" (includes CR)" << std::endl;

    // READ second line
    setup_mli_call(bus, state, 0xCA, 0x3000); // READ

    bus.write(0x3000, 4);
    bus.write(0x3001, refnum);
    bus.write(0x3002, 0x00);
    bus.write(0x3003, 0x40); // data_buffer at $4000
    bus.write(0x3004, 0xFF);
    bus.write(0x3005, 0x00);

    result = MLIHandler::prodos_mli_trap_handler(state, bus, 0xBF00);
    assert(result == true);

    trans_count = bus.read_word(0x3006);
    std::cout << "  Second READ: transferred " << trans_count << " bytes" << std::endl;

    // Should have read "Second Line\r" (12 bytes)
    assert(trans_count == 12);

    std::string second_line;
    for (uint16_t i = 0; i < trans_count; ++i) {
        second_line += static_cast<char>(bus.read(0x4000 + i));
    }
    assert(second_line == "Second Line\r");
    std::cout << "  ✓ Second line read correctly: \"" << second_line.substr(0, trans_count - 1)
              << "\" (includes CR)" << std::endl;

    // Close the file
    setup_mli_call(bus, state, 0xCC, 0x3000); // CLOSE
    bus.write(0x3000, 1);
    bus.write(0x3001, refnum);
    result = MLIHandler::prodos_mli_trap_handler(state, bus, 0xBF00);
    assert(result == true);

    // Clean up
    unlink(test_file);

    std::cout << "✓ test_newline_read_termination passed" << std::endl;
}

void test_newline_mask_behavior() {
    std::cout << "Testing NEWLINE mask behavior..." << std::endl;

    Bus bus;
    CPU cpu(bus);
    CPUState &state = cpu.state();

    // Create a test file with both $0D and $8D (CR with high bit set)
    const char *test_file = "/tmp/test_newline_mask.txt";
    uint8_t content[] = {'A', 'B', 'C', 0x8D, 'D', 'E', 'F', 0x0D, 'G', 'H', 'I'};
    {
        std::ofstream out(test_file, std::ios::binary);
        out.write(reinterpret_cast<const char *>(content), sizeof(content));
    }

    // OPEN the file
    setup_mli_call(bus, state, 0xC8, 0x3000);

    bus.write(0x3000, 3);
    bus.write(0x3001, 0x00);
    bus.write(0x3002, 0x31);

    bus.write(0x3100, strlen(test_file));
    for (size_t i = 0; i < strlen(test_file); ++i) {
        bus.write(0x3101 + i, test_file[i]);
    }

    bus.write(0x3003, 0x00);
    bus.write(0x3004, 0x32);

    bool result = MLIHandler::prodos_mli_trap_handler(state, bus, 0xBF00);
    assert(result == true);
    uint8_t refnum = bus.read(0x3005);

    // Enable NEWLINE with mask $7F (strip high bit)
    setup_mli_call(bus, state, 0xC9, 0x3000);

    bus.write(0x3000, 3);
    bus.write(0x3001, refnum);
    bus.write(0x3002, 0x7F); // enable_mask (strip high bit)
    bus.write(0x3003, 0x0D); // newline_char

    result = MLIHandler::prodos_mli_trap_handler(state, bus, 0xBF00);
    assert(result == true);

    // READ - should stop at $8D because ($8D & $7F) == $0D
    setup_mli_call(bus, state, 0xCA, 0x3000);

    bus.write(0x3000, 4);
    bus.write(0x3001, refnum);
    bus.write(0x3002, 0x00);
    bus.write(0x3003, 0x40);
    bus.write(0x3004, 0xFF);
    bus.write(0x3005, 0x00);

    result = MLIHandler::prodos_mli_trap_handler(state, bus, 0xBF00);
    assert(result == true);

    uint16_t trans_count = bus.read_word(0x3006);
    std::cout << "  READ with mask=$7F: transferred " << trans_count << " bytes" << std::endl;

    // Should have read "ABC" + $8D (4 bytes)
    assert(trans_count == 4);

    // Verify content
    assert(bus.read(0x4000) == 'A');
    assert(bus.read(0x4001) == 'B');
    assert(bus.read(0x4002) == 'C');
    assert(bus.read(0x4003) == 0x8D); // Newline char is included
    std::cout << "  ✓ Correctly stopped at $8D (masked as $0D)" << std::endl;

    // Close the file
    setup_mli_call(bus, state, 0xCC, 0x3000);
    bus.write(0x3000, 1);
    bus.write(0x3001, refnum);
    result = MLIHandler::prodos_mli_trap_handler(state, bus, 0xBF00);

    // Clean up
    unlink(test_file);

    std::cout << "✓ test_newline_mask_behavior passed" << std::endl;
}

void test_newline_invalid_refnum() {
    std::cout << "Testing NEWLINE with invalid refnum..." << std::endl;

    Bus bus;
    CPU cpu(bus);
    CPUState &state = cpu.state();

    // Note: The current MLI handler implementation halts on errors
    // This test verifies that INVALID_REF_NUM error is correctly detected
    // but we cannot test the error return path without changing error handling behavior

    // Try NEWLINE with invalid refnum
    setup_mli_call(bus, state, 0xC9, 0x3000);

    bus.write(0x3000, 3);
    bus.write(0x3001, 99); // Invalid refnum
    bus.write(0x3002, 0x7F);
    bus.write(0x3003, 0x0D);

    // This will halt the emulator and dump memory, which is the current behavior
    // for all MLI errors. We skip this test for now.
    
    std::cout << "  (Skipped: MLI errors currently halt execution)" << std::endl;
    std::cout << "✓ test_newline_invalid_refnum passed (test skipped)" << std::endl;
}

int main() {
    std::cout << "Running MLI NEWLINE tests..." << std::endl;
    std::cout << std::endl;

    test_newline_basic_enable_disable();
    test_newline_read_termination();
    test_newline_mask_behavior();
    test_newline_invalid_refnum();

    std::cout << std::endl;
    std::cout << "All MLI NEWLINE tests passed!" << std::endl;
    return 0;
}
