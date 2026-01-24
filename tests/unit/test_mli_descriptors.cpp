#include "../../include/edasm/emulator/mli.hpp"
#include "../../include/edasm/emulator/bus.hpp"
#include "../../include/edasm/emulator/cpu.hpp"
#include <cassert>
#include <iostream>
#include <iomanip>

using namespace edasm;

void test_error_code_enum() {
    // Test that error codes are correctly defined
    assert(static_cast<uint8_t>(ProDOSError::NO_ERROR) == 0x00);
    assert(static_cast<uint8_t>(ProDOSError::BAD_CALL_NUMBER) == 0x01);
    assert(static_cast<uint8_t>(ProDOSError::BAD_PARAM_COUNT) == 0x04);
    assert(static_cast<uint8_t>(ProDOSError::INTERRUPT_TABLE_FULL) == 0x25);
    assert(static_cast<uint8_t>(ProDOSError::IO_ERROR) == 0x27);
    assert(static_cast<uint8_t>(ProDOSError::NO_DEVICE) == 0x28);
    assert(static_cast<uint8_t>(ProDOSError::WRITE_PROTECTED) == 0x2B);
    assert(static_cast<uint8_t>(ProDOSError::DISK_SWITCHED) == 0x2E);
    assert(static_cast<uint8_t>(ProDOSError::INVALID_PATH_SYNTAX) == 0x40);
    assert(static_cast<uint8_t>(ProDOSError::FCB_FULL) == 0x42);
    assert(static_cast<uint8_t>(ProDOSError::INVALID_REF_NUM) == 0x43);
    assert(static_cast<uint8_t>(ProDOSError::PATH_NOT_FOUND) == 0x44);
    assert(static_cast<uint8_t>(ProDOSError::VOL_NOT_FOUND) == 0x45);
    assert(static_cast<uint8_t>(ProDOSError::FILE_NOT_FOUND) == 0x46);
    assert(static_cast<uint8_t>(ProDOSError::DUPLICATE_FILE) == 0x47);
    assert(static_cast<uint8_t>(ProDOSError::DISK_FULL) == 0x48);
    assert(static_cast<uint8_t>(ProDOSError::VOL_DIR_FULL) == 0x49);
    assert(static_cast<uint8_t>(ProDOSError::INCOMPATIBLE_FORMAT) == 0x4A);
    assert(static_cast<uint8_t>(ProDOSError::UNSUPPORTED_STORAGE) == 0x4B);
    assert(static_cast<uint8_t>(ProDOSError::END_OF_FILE) == 0x4C);
    assert(static_cast<uint8_t>(ProDOSError::POSITION_OUT_OF_RANGE) == 0x4D);
    assert(static_cast<uint8_t>(ProDOSError::ACCESS_ERROR) == 0x4E);
    assert(static_cast<uint8_t>(ProDOSError::FILE_OPEN) == 0x50);
    assert(static_cast<uint8_t>(ProDOSError::DIR_COUNT_ERROR) == 0x51);
    assert(static_cast<uint8_t>(ProDOSError::NOT_PRODOS_DISK) == 0x52);
    assert(static_cast<uint8_t>(ProDOSError::INVALID_PARAMETER) == 0x53);
    assert(static_cast<uint8_t>(ProDOSError::VCB_FULL) == 0x55);
    assert(static_cast<uint8_t>(ProDOSError::BAD_BUFFER_ADDR) == 0x56);
    assert(static_cast<uint8_t>(ProDOSError::DUPLICATE_VOLUME) == 0x57);
    assert(static_cast<uint8_t>(ProDOSError::BITMAP_IMPOSSIBLE) == 0x5A);

    std::cout << "✓ test_error_code_enum passed" << std::endl;
}

void test_set_error_with_enum() {
    Bus bus;
    CPU cpu(bus);
    CPUState &state = cpu.state();

    // Test set_error with enum
    MLIHandler::set_error(state, ProDOSError::FILE_NOT_FOUND);
    assert(state.A == 0x46);
    assert(state.P & StatusFlags::C); // Carry flag set on error
    assert(!(state.P & StatusFlags::Z)); // Zero flag clear on error

    MLIHandler::set_error(state, ProDOSError::INVALID_REF_NUM);
    assert(state.A == 0x43);
    assert(state.P & StatusFlags::C);

    std::cout << "✓ test_set_error_with_enum passed" << std::endl;
}

void test_descriptor_lookup() {
    // Test that we can find descriptors for all implemented calls
    const MLICallDescriptor *desc;

    // System calls
    desc = MLIHandler::get_call_descriptor(0x40); // ALLOC_INTERRUPT
    assert(desc != nullptr);
    assert(desc->call_number == 0x40);
    assert(std::string(desc->name) == "ALLOC_INTERRUPT");
    assert(desc->param_count == 2);

    desc = MLIHandler::get_call_descriptor(0x82); // GET_TIME
    assert(desc != nullptr);
    assert(desc->call_number == 0x82);
    assert(std::string(desc->name) == "GET_TIME");
    assert(desc->param_count == 1);

    // Housekeeping calls
    desc = MLIHandler::get_call_descriptor(0xC0); // CREATE
    assert(desc != nullptr);
    assert(desc->call_number == 0xC0);
    assert(std::string(desc->name) == "CREATE");
    assert(desc->param_count == 7);

    desc = MLIHandler::get_call_descriptor(0xC4); // GET_FILE_INFO
    assert(desc != nullptr);
    assert(desc->call_number == 0xC4);
    assert(std::string(desc->name) == "GET_FILE_INFO");
    assert(desc->param_count == 10);

    desc = MLIHandler::get_call_descriptor(0xC6); // SET_PREFIX
    assert(desc != nullptr);
    assert(desc->call_number == 0xC6);
    assert(std::string(desc->name) == "SET_PREFIX");
    assert(desc->param_count == 1);

    // Filing calls
    desc = MLIHandler::get_call_descriptor(0xC8); // OPEN
    assert(desc != nullptr);
    assert(desc->call_number == 0xC8);
    assert(std::string(desc->name) == "OPEN");
    assert(desc->param_count == 3);

    desc = MLIHandler::get_call_descriptor(0xCA); // READ
    assert(desc != nullptr);
    assert(desc->call_number == 0xCA);
    assert(std::string(desc->name) == "READ");
    assert(desc->param_count == 4);

    desc = MLIHandler::get_call_descriptor(0xCC); // CLOSE
    assert(desc != nullptr);
    assert(desc->call_number == 0xCC);
    assert(std::string(desc->name) == "CLOSE");
    assert(desc->param_count == 1);

    desc = MLIHandler::get_call_descriptor(0xCF); // GET_MARK
    assert(desc != nullptr);
    assert(desc->call_number == 0xCF);
    assert(std::string(desc->name) == "GET_MARK");
    assert(desc->param_count == 2);

    desc = MLIHandler::get_call_descriptor(0xD1); // GET_EOF
    assert(desc != nullptr);
    assert(desc->call_number == 0xD1);
    assert(std::string(desc->name) == "GET_EOF");
    assert(desc->param_count == 2);

    // Test unknown call
    desc = MLIHandler::get_call_descriptor(0xFF);
    assert(desc == nullptr);

    std::cout << "✓ test_descriptor_lookup passed" << std::endl;
}

void test_create_descriptor_details() {
    const MLICallDescriptor *desc = MLIHandler::get_call_descriptor(0xC0); // CREATE
    assert(desc != nullptr);

    // Verify each parameter
    assert(desc->params[0].type == MLIParamType::PATHNAME_PTR);
    assert(desc->params[0].direction == MLIParamDirection::INPUT);
    assert(std::string(desc->params[0].name) == "pathname");

    assert(desc->params[1].type == MLIParamType::BYTE);
    assert(desc->params[1].direction == MLIParamDirection::INPUT);
    assert(std::string(desc->params[1].name) == "access");

    assert(desc->params[2].type == MLIParamType::BYTE);
    assert(desc->params[2].direction == MLIParamDirection::INPUT);
    assert(std::string(desc->params[2].name) == "file_type");

    assert(desc->params[3].type == MLIParamType::WORD);
    assert(desc->params[3].direction == MLIParamDirection::INPUT);
    assert(std::string(desc->params[3].name) == "aux_type");

    assert(desc->params[4].type == MLIParamType::BYTE);
    assert(desc->params[4].direction == MLIParamDirection::INPUT);
    assert(std::string(desc->params[4].name) == "storage_type");

    assert(desc->params[5].type == MLIParamType::WORD);
    assert(desc->params[5].direction == MLIParamDirection::INPUT);
    assert(std::string(desc->params[5].name) == "create_date");

    assert(desc->params[6].type == MLIParamType::WORD);
    assert(desc->params[6].direction == MLIParamDirection::INPUT);
    assert(std::string(desc->params[6].name) == "create_time");

    std::cout << "✓ test_create_descriptor_details passed" << std::endl;
}

void test_open_descriptor_details() {
    const MLICallDescriptor *desc = MLIHandler::get_call_descriptor(0xC8); // OPEN
    assert(desc != nullptr);

    assert(desc->params[0].type == MLIParamType::PATHNAME_PTR);
    assert(desc->params[0].direction == MLIParamDirection::INPUT);
    assert(std::string(desc->params[0].name) == "pathname");

    assert(desc->params[1].type == MLIParamType::BUFFER_PTR);
    assert(desc->params[1].direction == MLIParamDirection::INPUT);
    assert(std::string(desc->params[1].name) == "io_buffer");

    assert(desc->params[2].type == MLIParamType::REF_NUM);
    assert(desc->params[2].direction == MLIParamDirection::OUTPUT);
    assert(std::string(desc->params[2].name) == "ref_num");

    std::cout << "✓ test_open_descriptor_details passed" << std::endl;
}

void test_get_file_info_descriptor_details() {
    const MLICallDescriptor *desc = MLIHandler::get_call_descriptor(0xC4); // GET_FILE_INFO
    assert(desc != nullptr);
    assert(desc->param_count == 10);

    // First param is input (pathname)
    assert(desc->params[0].type == MLIParamType::PATHNAME_PTR);
    assert(desc->params[0].direction == MLIParamDirection::INPUT);

    // Rest are output
    assert(desc->params[1].direction == MLIParamDirection::OUTPUT);
    assert(desc->params[2].direction == MLIParamDirection::OUTPUT);
    assert(desc->params[3].direction == MLIParamDirection::OUTPUT);
    assert(desc->params[4].direction == MLIParamDirection::OUTPUT);
    assert(desc->params[5].direction == MLIParamDirection::OUTPUT);
    assert(desc->params[6].direction == MLIParamDirection::OUTPUT);
    assert(desc->params[7].direction == MLIParamDirection::OUTPUT);
    assert(desc->params[8].direction == MLIParamDirection::OUTPUT);
    assert(desc->params[9].direction == MLIParamDirection::OUTPUT);

    std::cout << "✓ test_get_file_info_descriptor_details passed" << std::endl;
}

void test_read_input_params_byte_and_word() {
    Bus bus;
    
    // Create a simple parameter list for CLOSE (1 byte param)
    // Address $1000: param_count=1, ref_num=5
    bus.write(0x1000, 1);    // param_count
    bus.write(0x1001, 5);    // ref_num
    
    const MLICallDescriptor *desc = MLIHandler::get_call_descriptor(0xCC); // CLOSE
    assert(desc != nullptr);
    
    auto values = MLIHandler::read_input_params(bus, 0x1000, *desc);
    assert(values.size() == 1);
    assert(std::get<uint8_t>(values[0]) == 5);

    // Test with GET_MARK (ref_num + 3-byte output)
    bus.write(0x2000, 2);    // param_count
    bus.write(0x2001, 7);    // ref_num
    bus.write(0x2002, 0x00); // position (output, but we still need space)
    bus.write(0x2003, 0x00);
    bus.write(0x2004, 0x00);
    
    desc = MLIHandler::get_call_descriptor(0xCF); // GET_MARK
    assert(desc != nullptr);
    
    values = MLIHandler::read_input_params(bus, 0x2000, *desc);
    assert(values.size() == 2);
    assert(std::get<uint8_t>(values[0]) == 7);
    // Second value is output-only, so should be placeholder
    assert(std::get<uint8_t>(values[1]) == 0);

    std::cout << "✓ test_read_input_params_byte_and_word passed" << std::endl;
}

void test_read_input_params_pathname() {
    Bus bus;
    
    // Create a parameter list for SET_PREFIX
    // Address $1000: param_count=1, pathname_ptr=$2000
    bus.write(0x1000, 1);          // param_count
    bus.write(0x1001, 0x00);       // pathname_ptr low
    bus.write(0x1002, 0x20);       // pathname_ptr high
    
    // Write pathname at $2000: length=6, "/HELLO"
    bus.write(0x2000, 6);          // length
    bus.write(0x2001, '/');
    bus.write(0x2002, 'H');
    bus.write(0x2003, 'E');
    bus.write(0x2004, 'L');
    bus.write(0x2005, 'L');
    bus.write(0x2006, 'O');
    
    const MLICallDescriptor *desc = MLIHandler::get_call_descriptor(0xC6); // SET_PREFIX
    assert(desc != nullptr);
    
    auto values = MLIHandler::read_input_params(bus, 0x1000, *desc);
    assert(values.size() == 1);
    assert(std::get<std::string>(values[0]) == "/HELLO");

    std::cout << "✓ test_read_input_params_pathname passed" << std::endl;
}

void test_write_output_params_byte_and_word() {
    Bus bus;
    
    // Prepare parameter list for OPEN (pathname, io_buffer, ref_num)
    bus.write(0x1000, 3);          // param_count
    bus.write(0x1001, 0x00);       // pathname_ptr (input)
    bus.write(0x1002, 0x30);
    bus.write(0x1003, 0x00);       // io_buffer (input)
    bus.write(0x1004, 0x40);
    bus.write(0x1005, 0x00);       // ref_num (output) - will be written
    
    const MLICallDescriptor *desc = MLIHandler::get_call_descriptor(0xC8); // OPEN
    assert(desc != nullptr);
    
    std::vector<MLIParamValue> values = {
        std::string(""),  // pathname (input, not written)
        uint16_t(0x4000), // io_buffer (input, not written)
        uint8_t(3)        // ref_num (output)
    };
    
    MLIHandler::write_output_params(bus, 0x1000, *desc, values);
    
    // ref_num should be written at offset 5
    assert(bus.read(0x1005) == 3);

    std::cout << "✓ test_write_output_params_byte_and_word passed" << std::endl;
}

void test_write_output_params_three_byte() {
    Bus bus;
    
    // Prepare parameter list for GET_EOF (ref_num, eof)
    bus.write(0x1000, 2);          // param_count
    bus.write(0x1001, 5);          // ref_num (input)
    bus.write(0x1002, 0x00);       // eof byte 0 (output)
    bus.write(0x1003, 0x00);       // eof byte 1 (output)
    bus.write(0x1004, 0x00);       // eof byte 2 (output)
    
    const MLICallDescriptor *desc = MLIHandler::get_call_descriptor(0xD1); // GET_EOF
    assert(desc != nullptr);
    
    std::vector<MLIParamValue> values = {
        uint8_t(5),        // ref_num (input, not written back)
        uint32_t(0x012345) // eof (output) - 24-bit value
    };
    
    MLIHandler::write_output_params(bus, 0x1000, *desc, values);
    
    // eof should be written at offset 2 (3 bytes, little-endian)
    assert(bus.read(0x1002) == 0x45);
    assert(bus.read(0x1003) == 0x23);
    assert(bus.read(0x1004) == 0x01);

    std::cout << "✓ test_write_output_params_three_byte passed" << std::endl;
}

void test_all_call_descriptors_present() {
    // Verify all 27 calls are present
    int count = 0;
    const uint8_t expected_calls[] = {
        0x40, 0x41, 0x65, 0x80, 0x81, 0x82,
        0xC0, 0xC1, 0xC2, 0xC3, 0xC4, 0xC5, 0xC6, 0xC7, 0xC8, 0xC9,
        0xCA, 0xCB, 0xCC, 0xCD, 0xCE, 0xCF, 0xD0, 0xD1, 0xD2, 0xD3
    };
    
    for (uint8_t call_num : expected_calls) {
        const MLICallDescriptor *desc = MLIHandler::get_call_descriptor(call_num);
        if (desc != nullptr) {
            count++;
            std::cout << "  Found: $" << std::hex << std::uppercase << std::setw(2) 
                      << std::setfill('0') << static_cast<int>(call_num) 
                      << " " << desc->name << " (params=" << std::dec 
                      << static_cast<int>(desc->param_count) << ")" << std::endl;
        } else {
            std::cerr << "  MISSING: $" << std::hex << std::uppercase << std::setw(2) 
                      << std::setfill('0') << static_cast<int>(call_num) << std::endl;
        }
    }
    
    assert(count == 26); // We have 26 calls defined (QUIT is 0x65)

    std::cout << "✓ test_all_call_descriptors_present passed (found " << count << " calls)" << std::endl;
}

int main() {
    std::cout << "Running MLI descriptor tests..." << std::endl;
    std::cout << std::endl;

    test_error_code_enum();
    test_set_error_with_enum();
    test_descriptor_lookup();
    test_create_descriptor_details();
    test_open_descriptor_details();
    test_get_file_info_descriptor_details();
    test_read_input_params_byte_and_word();
    test_read_input_params_pathname();
    test_write_output_params_byte_and_word();
    test_write_output_params_three_byte();
    test_all_call_descriptors_present();

    std::cout << std::endl;
    std::cout << "All MLI descriptor tests passed!" << std::endl;
    return 0;
}
