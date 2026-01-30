#include "../../include/edasm/emulator/bus.hpp"
#include "../../include/edasm/emulator/cpu.hpp"
#include "../../include/edasm/emulator/mli.hpp"
#include "../../include/edasm/emulator/traps.hpp"
#include <cassert>
#include <iomanip>
#include <iostream>

using namespace edasm;

// Test that stub handlers return an error instead of halting
void test_stub_handler_create() {
    Bus bus;
    CPU cpu(bus);
    CPUState &state = cpu.state();

    // Set up a CREATE call at $BF00 (trap handler address)
    // MLI call structure:
    //   JSR $BF00         (at $2000-$2002)
    //   .BYTE command_number (at $2003)
    //   .WORD parameter_list_pointer (at $2004-$2005)

    // JSR pushes return address - 1, so it pushes $2002
    state.SP = 0xFD;
    bus.write(0x01FE, 0x02); // Return address low byte (last byte of JSR)
    bus.write(0x01FF, 0x20); // Return address high byte

    // Set up MLI call parameters at $2003 (ret_addr + 1)
    bus.write(0x2003, 0xC0); // CREATE command
    bus.write(0x2004, 0x00); // Parameter list pointer low
    bus.write(0x2005, 0x30); // Parameter list pointer high

    // Set up parameter list at $3000
    bus.write(0x3000, 7);    // param_count
    bus.write(0x3001, 0x00); // pathname pointer low
    bus.write(0x3002, 0x31); // pathname pointer high
    // ... rest of parameters not needed for stub test

    // Call the MLI handler
    bool result = MLIHandler::prodos_mli_trap_handler(state, bus, 0xBF00);

    // Should return true (continue execution)
    assert(result == true);

    // Should set error code (BAD_CALL_NUMBER = 0x01)
    assert(state.A == 0x01);

    // Carry flag should be set (error condition)
    assert(state.P & StatusFlags::C);

    // PC should be advanced past the MLI call structure (ret_addr + 1 + 3)
    assert(state.PC == 0x2006);

    // SP should be restored (popped 2 bytes)
    assert(state.SP == 0xFF);

    std::cout << "✓ test_stub_handler_create passed" << std::endl;
}

void test_stub_handler_destroy() {
    Bus bus;
    CPU cpu(bus);
    CPUState &state = cpu.state();

    state.SP = 0xFD;
    bus.write(0x01FE, 0x02); // Return address low (last byte of JSR)
    bus.write(0x01FF, 0x20); // Return address high

    bus.write(0x2003, 0xC1); // DESTROY command
    bus.write(0x2004, 0x00);
    bus.write(0x2005, 0x30);

    bus.write(0x3000, 1); // param_count
    bus.write(0x3001, 0x00);
    bus.write(0x3002, 0x31);

    bool result = MLIHandler::prodos_mli_trap_handler(state, bus, 0xBF00);

    assert(result == true);
    assert(state.A == 0x01);
    assert(state.P & StatusFlags::C);

    std::cout << "✓ test_stub_handler_destroy passed" << std::endl;
}

void test_stub_handler_alloc_interrupt() {
    Bus bus;
    CPU cpu(bus);
    CPUState &state = cpu.state();

    state.SP = 0xFD;
    bus.write(0x01FE, 0x02);
    bus.write(0x01FF, 0x20);

    bus.write(0x2003, 0x40); // ALLOC_INTERRUPT command
    bus.write(0x2004, 0x00);
    bus.write(0x2005, 0x30);

    bus.write(0x3000, 2);    // param_count
    bus.write(0x3001, 0x01); // int_num
    bus.write(0x3002, 0x00); // int_code low
    bus.write(0x3003, 0x40); // int_code high

    bool result = MLIHandler::prodos_mli_trap_handler(state, bus, 0xBF00);

    assert(result == true);
    assert(state.A == 0x01);
    assert(state.P & StatusFlags::C);

    std::cout << "✓ test_stub_handler_alloc_interrupt passed" << std::endl;
}

void test_stub_handler_quit() {
    Bus bus;
    CPU cpu(bus);
    CPUState &state = cpu.state();

    state.SP = 0xFD;
    bus.write(0x01FE, 0x02);
    bus.write(0x01FF, 0x20);

    bus.write(0x2003, 0x65); // QUIT command
    bus.write(0x2004, 0x00);
    bus.write(0x2005, 0x30);

    bus.write(0x3000, 4); // param_count
    bus.write(0x3001, 0x00);
    bus.write(0x3002, 0x00);
    bus.write(0x3003, 0x00);
    bus.write(0x3004, 0x00);
    bus.write(0x3005, 0x00);
    bus.write(0x3006, 0x00);

    bool result = MLIHandler::prodos_mli_trap_handler(state, bus, 0xBF00);

    assert(result == true);
    assert(state.A == 0x01);
    assert(state.P & StatusFlags::C);

    std::cout << "✓ test_stub_handler_quit passed" << std::endl;
}

void test_implemented_handler_still_works() {
    Bus bus;
    CPU cpu(bus);
    CPUState &state = cpu.state();

    // Test that an implemented call (GET_TIME) still works
    state.SP = 0xFD;
    bus.write(0x01FE, 0x02);
    bus.write(0x01FF, 0x20);

    bus.write(0x2003, 0x82); // GET_TIME command
    bus.write(0x2004, 0x00);
    bus.write(0x2005, 0x30);

    bus.write(0x3000, 1);    // param_count
    bus.write(0x3001, 0x00); // date_time_ptr low
    bus.write(0x3002, 0xBF); // date_time_ptr high (ProDOS date/time location)

    bool result = MLIHandler::prodos_mli_trap_handler(state, bus, 0xBF00);

    // Should succeed
    assert(result == true);

    // Should have no error (A = 0)
    assert(state.A == 0x00);

    // Carry flag should be clear (no error)
    assert(!(state.P & StatusFlags::C));

    // Zero flag should be set (A = 0)
    assert(state.P & StatusFlags::Z);

    std::cout << "✓ test_implemented_handler_still_works passed" << std::endl;
}

void test_unknown_call_number_halts() {
    Bus bus;
    CPU cpu(bus);
    CPUState &state = cpu.state();

    // Test that an unknown call number (0xFF) still halts
    state.SP = 0xFD;
    bus.write(0x01FE, 0x02);
    bus.write(0x01FF, 0x20);

    bus.write(0x2003, 0xFF); // Unknown command
    bus.write(0x2004, 0x00);
    bus.write(0x2005, 0x30);

    bus.write(0x3000, 0); // param_count

    bool result = MLIHandler::prodos_mli_trap_handler(state, bus, 0xBF00);

    // Should halt (return false)
    assert(result == false);

    std::cout << "✓ test_unknown_call_number_halts passed" << std::endl;
}

int main() {
    std::cout << "Running MLI stub handler tests..." << std::endl;
    std::cout << std::endl;

    test_stub_handler_create();
    test_stub_handler_destroy();
    test_stub_handler_alloc_interrupt();
    test_stub_handler_quit();
    test_implemented_handler_still_works();
    test_unknown_call_number_halts();

    std::cout << std::endl;
    std::cout << "All MLI stub handler tests passed!" << std::endl;
    return 0;
}
