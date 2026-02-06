/**
 * @file test_mli_read_eof.cpp
 * @brief Unit tests for ProDOS MLI READ ($CA) call EOF handling
 *
 * Tests the READ call implementation edge cases near EOF:
 * - Reading when mark == EOF (should return EOF error with 0 bytes)
 * - Reading when mark > EOF (should return EOF error with 0 bytes)
 * - Reading when mark < EOF but request_count extends past EOF (partial read, no error)
 * - Reading when mark < EOF and request_count within bounds (normal read)
 *
 * Per ProDOS 8 Technical Reference Manual:
 * "If the end of file is encountered before request_count bytes have been
 * read, then trans_count is set to the number of bytes transferred. The
 * end of file error ($4C) is returned if and only if zero bytes were
 * transferred (trans_count = 0)."
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

uint8_t open_test_file(Bus &bus, CPUState &state, const char *test_file) {
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

    return bus.read(0x3005); // ref_num output
}

void close_file(Bus &bus, CPUState &state, uint8_t refnum) {
    setup_mli_call(bus, state, 0xCC, 0x3000); // CLOSE
    bus.write(0x3000, 1);      // param_count
    bus.write(0x3001, refnum); // ref_num
    bool result = MLIHandler::prodos_mli_trap_handler(state, bus, 0xBF00);
    assert(result == true);
}

void test_read_at_eof() {
    std::cout << "Testing READ at EOF (mark == file_size)..." << std::endl;

    Bus bus;
    CPU cpu(bus);
    CPUState &state = cpu.state();

    // Create a test file with 10 bytes
    const char *test_file = "/tmp/test_read_at_eof.bin";
    const char *content = "0123456789"; // 10 bytes
    {
        std::ofstream out(test_file, std::ios::binary);
        out.write(content, 10);
    }

    uint8_t refnum = open_test_file(bus, state, test_file);
    std::cout << "  File opened, refnum=" << static_cast<int>(refnum) << std::endl;

    // SET_MARK to EOF (position 10)
    setup_mli_call(bus, state, 0xCE, 0x3000); // SET_MARK
    bus.write(0x3000, 2);      // param_count
    bus.write(0x3001, refnum); // ref_num
    bus.write(0x3002, 10);     // position low byte (10 = EOF)
    bus.write(0x3003, 0);      // position high byte
    bus.write(0x3004, 0);      // position high byte
    bool result = MLIHandler::prodos_mli_trap_handler(state, bus, 0xBF00);
    assert(result == true);
    std::cout << "  Mark set to EOF (10)" << std::endl;

    // READ at EOF - should return EOF error with trans_count = 0
    setup_mli_call(bus, state, 0xCA, 0x3000); // READ
    bus.write(0x3000, 4);      // param_count
    bus.write(0x3001, refnum); // ref_num
    bus.write(0x3002, 0x00);   // data_buffer low
    bus.write(0x3003, 0x40);   // data_buffer high ($4000)
    bus.write(0x3004, 5);      // request_count low (5 bytes)
    bus.write(0x3005, 0);      // request_count high

    result = MLIHandler::prodos_mli_trap_handler(state, bus, 0xBF00);
    assert(result == true);
    
    uint8_t error = state.A;
    uint16_t trans_count = bus.read_word(0x3006);

    std::cout << "  Error code: $" << std::hex << std::uppercase << std::setw(2)
              << std::setfill('0') << static_cast<int>(error) << std::endl;
    std::cout << "  Transfer count: " << std::dec << trans_count << std::endl;

    assert(error == 0x4C);      // END_OF_FILE
    assert(trans_count == 0);   // No bytes transferred
    std::cout << "  ✓ Correctly returned EOF error with trans_count=0" << std::endl;

    close_file(bus, state, refnum);
    unlink(test_file);

    std::cout << "✓ test_read_at_eof passed" << std::endl;
}

void test_read_beyond_eof() {
    std::cout << "Testing READ beyond EOF (mark > file_size)..." << std::endl;

    Bus bus;
    CPU cpu(bus);
    CPUState &state = cpu.state();

    // Create a test file with 10 bytes
    const char *test_file = "/tmp/test_read_beyond_eof.bin";
    const char *content = "ABCDEFGHIJ"; // 10 bytes
    {
        std::ofstream out(test_file, std::ios::binary);
        out.write(content, 10);
    }

    uint8_t refnum = open_test_file(bus, state, test_file);

    // SET_MARK beyond EOF (position 15, file is only 10 bytes)
    setup_mli_call(bus, state, 0xCE, 0x3000); // SET_MARK
    bus.write(0x3000, 2);      // param_count
    bus.write(0x3001, refnum); // ref_num
    bus.write(0x3002, 15);     // position low byte (15 > 10)
    bus.write(0x3003, 0);      // position high byte
    bus.write(0x3004, 0);      // position high byte
    bool result = MLIHandler::prodos_mli_trap_handler(state, bus, 0xBF00);
    assert(result == true);
    std::cout << "  Mark set beyond EOF (15 > 10)" << std::endl;

    // READ beyond EOF - should return EOF error with trans_count = 0
    setup_mli_call(bus, state, 0xCA, 0x3000); // READ
    bus.write(0x3000, 4);
    bus.write(0x3001, refnum);
    bus.write(0x3002, 0x00);
    bus.write(0x3003, 0x40);
    bus.write(0x3004, 5);
    bus.write(0x3005, 0);

    result = MLIHandler::prodos_mli_trap_handler(state, bus, 0xBF00);
    assert(result == true);
    
    uint8_t error = state.A;
    uint16_t trans_count = bus.read_word(0x3006);

    std::cout << "  Error code: $" << std::hex << std::uppercase << std::setw(2)
              << std::setfill('0') << static_cast<int>(error) << std::endl;
    std::cout << "  Transfer count: " << std::dec << trans_count << std::endl;

    assert(error == 0x4C);      // END_OF_FILE
    assert(trans_count == 0);   // No bytes transferred
    std::cout << "  ✓ Correctly returned EOF error with trans_count=0" << std::endl;

    close_file(bus, state, refnum);
    unlink(test_file);

    std::cout << "✓ test_read_beyond_eof passed" << std::endl;
}

void test_read_partial_at_eof() {
    std::cout << "Testing partial READ near EOF..." << std::endl;

    Bus bus;
    CPU cpu(bus);
    CPUState &state = cpu.state();

    // Create a test file with 10 bytes
    const char *test_file = "/tmp/test_read_partial.bin";
    const char *content = "0123456789"; // 10 bytes
    {
        std::ofstream out(test_file, std::ios::binary);
        out.write(content, 10);
    }

    uint8_t refnum = open_test_file(bus, state, test_file);

    // SET_MARK to position 7 (3 bytes before EOF)
    setup_mli_call(bus, state, 0xCE, 0x3000); // SET_MARK
    bus.write(0x3000, 2);
    bus.write(0x3001, refnum);
    bus.write(0x3002, 7);
    bus.write(0x3003, 0);
    bus.write(0x3004, 0);
    bool result = MLIHandler::prodos_mli_trap_handler(state, bus, 0xBF00);
    assert(result == true);
    std::cout << "  Mark set to 7 (3 bytes before EOF)" << std::endl;

    // READ requesting 5 bytes, but only 3 available
    setup_mli_call(bus, state, 0xCA, 0x3000); // READ
    bus.write(0x3000, 4);
    bus.write(0x3001, refnum);
    bus.write(0x3002, 0x00);
    bus.write(0x3003, 0x40);
    bus.write(0x3004, 5);  // Request 5 bytes
    bus.write(0x3005, 0);

    result = MLIHandler::prodos_mli_trap_handler(state, bus, 0xBF00);
    assert(result == true);
    
    uint8_t error = state.A;
    uint16_t trans_count = bus.read_word(0x3006);

    std::cout << "  Error code: $" << std::hex << std::uppercase << std::setw(2)
              << std::setfill('0') << static_cast<int>(error) << std::endl;
    std::cout << "  Transfer count: " << std::dec << trans_count << std::endl;

    // Should return NO_ERROR with partial data (3 bytes)
    assert(error == 0x00);       // NO_ERROR
    assert(trans_count == 3);    // Only 3 bytes transferred
    
    // Verify the data read
    assert(bus.read(0x4000) == '7');
    assert(bus.read(0x4001) == '8');
    assert(bus.read(0x4002) == '9');
    std::cout << "  ✓ Correctly returned NO_ERROR with trans_count=3 (partial read)" << std::endl;
    std::cout << "  ✓ Data verified: '789'" << std::endl;

    close_file(bus, state, refnum);
    unlink(test_file);

    std::cout << "✓ test_read_partial_at_eof passed" << std::endl;
}

void test_read_exact_remaining_bytes() {
    std::cout << "Testing READ of exact remaining bytes to EOF..." << std::endl;

    Bus bus;
    CPU cpu(bus);
    CPUState &state = cpu.state();

    const char *test_file = "/tmp/test_read_exact.bin";
    const char *content = "ABCDEFGHIJ"; // 10 bytes
    {
        std::ofstream out(test_file, std::ios::binary);
        out.write(content, 10);
    }

    uint8_t refnum = open_test_file(bus, state, test_file);

    // SET_MARK to position 6
    setup_mli_call(bus, state, 0xCE, 0x3000); // SET_MARK
    bus.write(0x3000, 2);
    bus.write(0x3001, refnum);
    bus.write(0x3002, 6);
    bus.write(0x3003, 0);
    bus.write(0x3004, 0);
    bool result = MLIHandler::prodos_mli_trap_handler(state, bus, 0xBF00);
    assert(result == true);

    // READ exactly 4 bytes (to reach EOF exactly)
    setup_mli_call(bus, state, 0xCA, 0x3000); // READ
    bus.write(0x3000, 4);
    bus.write(0x3001, refnum);
    bus.write(0x3002, 0x00);
    bus.write(0x3003, 0x40);
    bus.write(0x3004, 4);  // Request 4 bytes (exactly to EOF)
    bus.write(0x3005, 0);

    result = MLIHandler::prodos_mli_trap_handler(state, bus, 0xBF00);
    assert(result == true);
    
    uint8_t error = state.A;
    uint16_t trans_count = bus.read_word(0x3006);

    // Should return NO_ERROR with all 4 bytes read
    assert(error == 0x00);
    assert(trans_count == 4);
    
    // Verify data
    assert(bus.read(0x4000) == 'G');
    assert(bus.read(0x4001) == 'H');
    assert(bus.read(0x4002) == 'I');
    assert(bus.read(0x4003) == 'J');
    std::cout << "  ✓ NO_ERROR with trans_count=4 (exact read to EOF)" << std::endl;

    // Now try to read again - should get EOF error
    setup_mli_call(bus, state, 0xCA, 0x3000); // READ
    bus.write(0x3000, 4);
    bus.write(0x3001, refnum);
    bus.write(0x3002, 0x00);
    bus.write(0x3003, 0x40);
    bus.write(0x3004, 1);  // Request 1 byte
    bus.write(0x3005, 0);

    result = MLIHandler::prodos_mli_trap_handler(state, bus, 0xBF00);
    assert(result == true);
    
    error = state.A;
    trans_count = bus.read_word(0x3006);

    assert(error == 0x4C);      // END_OF_FILE
    assert(trans_count == 0);
    std::cout << "  ✓ Subsequent read at EOF returns END_OF_FILE with trans_count=0" << std::endl;

    close_file(bus, state, refnum);
    unlink(test_file);

    std::cout << "✓ test_read_exact_remaining_bytes passed" << std::endl;
}

int main() {
    std::cout << "Running MLI READ EOF handling tests..." << std::endl;
    std::cout << std::endl;

    test_read_at_eof();
    test_read_beyond_eof();
    test_read_partial_at_eof();
    test_read_exact_remaining_bytes();

    std::cout << std::endl;
    std::cout << "All MLI READ EOF handling tests passed!" << std::endl;
    return 0;
}
