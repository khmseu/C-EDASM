#include "../include/edasm/emulator/bus.hpp"
#include "../include/edasm/emulator/cpu.hpp"
#include <cassert>
#include <iostream>

using namespace edasm;

void test_cpu_reset() {
    Bus bus;
    CPU cpu(bus);

    cpu.reset();

    assert(cpu.state().A == 0);
    assert(cpu.state().X == 0);
    assert(cpu.state().Y == 0);
    assert(cpu.state().SP == 0xFF);
    assert(cpu.state().PC == 0x2000); // Default entry point

    std::cout << "✓ test_cpu_reset passed" << std::endl;
}

void test_bus_memory() {
    Bus bus;

    // Test read/write
    bus.write(0x1000, 0x42);
    assert(bus.read(0x1000) == 0x42);

    // Test word operations
    bus.write_word(0x2000, 0x1234);
    assert(bus.read_word(0x2000) == 0x1234);
    assert(bus.read(0x2000) == 0x34); // Little-endian
    assert(bus.read(0x2001) == 0x12);

    std::cout << "✓ test_bus_memory passed" << std::endl;
}

void test_bus_reset() {
    Bus bus;

    // Write some data
    bus.write(0x1000, 0x42);

    // Reset should fill with trap opcode
    bus.reset();
    assert(bus.read(0x1000) == Bus::TRAP_OPCODE);
    assert(bus.read(0x0000) == Bus::TRAP_OPCODE);
    assert(bus.read(0xFFFF) == Bus::TRAP_OPCODE);

    std::cout << "✓ test_bus_reset passed" << std::endl;
}

void test_cpu_lda_immediate() {
    Bus bus;
    CPU cpu(bus);

    // Write: LDA #$42
    bus.write(0x2000, 0xA9);             // LDA immediate
    bus.write(0x2001, 0x42);             // value
    bus.write(0x2002, Bus::TRAP_OPCODE); // halt

    cpu.reset();
    cpu.step(); // Execute LDA

    assert(cpu.state().A == 0x42);
    assert(cpu.state().PC == 0x2002);

    std::cout << "✓ test_cpu_lda_immediate passed" << std::endl;
}

void test_cpu_ldx_ldy() {
    Bus bus;
    CPU cpu(bus);

    // Write: LDX #$10, LDY #$20
    bus.write(0x2000, 0xA2); // LDX immediate
    bus.write(0x2001, 0x10);
    bus.write(0x2002, 0xA0); // LDY immediate
    bus.write(0x2003, 0x20);
    bus.write(0x2004, Bus::TRAP_OPCODE);

    cpu.reset();
    cpu.step(); // LDX
    assert(cpu.state().X == 0x10);

    cpu.step(); // LDY
    assert(cpu.state().Y == 0x20);

    std::cout << "✓ test_cpu_ldx_ldy passed" << std::endl;
}

void test_cpu_flags() {
    Bus bus;
    CPU cpu(bus);

    // Test zero flag: LDA #$00
    bus.write(0x2000, 0xA9);
    bus.write(0x2001, 0x00);
    bus.write(0x2002, Bus::TRAP_OPCODE);

    cpu.reset();
    cpu.step();

    assert((cpu.state().P & StatusFlags::Z) != 0); // Zero flag set
    assert((cpu.state().P & StatusFlags::N) == 0); // Negative flag clear

    // Test negative flag: LDA #$80
    bus.write(0x2000, 0xA9);
    bus.write(0x2001, 0x80);

    cpu.reset();
    cpu.step();

    assert((cpu.state().P & StatusFlags::Z) == 0); // Zero flag clear
    assert((cpu.state().P & StatusFlags::N) != 0); // Negative flag set

    std::cout << "✓ test_cpu_flags passed" << std::endl;
}

void test_cpu_stack() {
    Bus bus;
    CPU cpu(bus);

    // Test: PHA, PLA
    bus.write(0x2000, 0xA9); // LDA #$42
    bus.write(0x2001, 0x42);
    bus.write(0x2002, 0x48); // PHA
    bus.write(0x2003, 0xA9); // LDA #$00
    bus.write(0x2004, 0x00);
    bus.write(0x2005, 0x68); // PLA
    bus.write(0x2006, Bus::TRAP_OPCODE);

    cpu.reset();
    uint8_t initial_sp = cpu.state().SP;

    cpu.step();                               // LDA #$42
    cpu.step();                               // PHA
    assert(cpu.state().SP == initial_sp - 1); // Stack pointer decremented

    cpu.step(); // LDA #$00
    assert(cpu.state().A == 0x00);

    cpu.step();                           // PLA
    assert(cpu.state().A == 0x42);        // Value restored
    assert(cpu.state().SP == initial_sp); // Stack pointer restored

    std::cout << "✓ test_cpu_stack passed" << std::endl;
}

void test_cpu_branches() {
    Bus bus;
    CPU cpu(bus);

    // Test BEQ (taken)
    bus.write(0x2000, 0xA9); // LDA #$00 (sets Z flag)
    bus.write(0x2001, 0x00);
    bus.write(0x2002, 0xF0);             // BEQ
    bus.write(0x2003, 0x02);             // Skip 2 bytes forward
    bus.write(0x2004, 0xEA);             // NOP (skipped)
    bus.write(0x2005, 0xEA);             // NOP (skipped)
    bus.write(0x2006, Bus::TRAP_OPCODE); // Target

    cpu.reset();
    cpu.step(); // LDA
    cpu.step(); // BEQ

    assert(cpu.state().PC == 0x2006); // Branch taken

    std::cout << "✓ test_cpu_branches passed" << std::endl;
}

void test_cpu_jsr_rts() {
    Bus bus;
    CPU cpu(bus);

    // Test JSR/RTS
    bus.write(0x2000, 0x20); // JSR $2010
    bus.write(0x2001, 0x10);
    bus.write(0x2002, 0x20);
    bus.write(0x2003, Bus::TRAP_OPCODE);

    bus.write(0x2010, 0x60); // RTS at subroutine

    cpu.reset();
    cpu.step(); // JSR
    assert(cpu.state().PC == 0x2010);

    cpu.step(); // RTS
    assert(cpu.state().PC == 0x2003);

    std::cout << "✓ test_cpu_jsr_rts passed" << std::endl;
}

void test_rom_loading_at_reset() {
    Bus bus;

    // Create a mock ROM with a reset vector pointing to 0xF800
    std::vector<uint8_t> rom_data(0x800, 0xEA); // 2KB of NOPs

    // Set reset vector at $FFFC-$FFFD to point to $F800
    rom_data[0x07FC] = 0x00; // Low byte of $F800
    rom_data[0x07FD] = 0xF8; // High byte of $F800

    // Put a special marker at $F800 (start of ROM)
    rom_data[0x0000] = 0xA9; // LDA #$42
    rom_data[0x0001] = 0x42;

    // Load ROM at $F800-$FFFF
    bool loaded = bus.initialize_memory(0xF800, rom_data);
    assert(loaded);

    // Verify that ROM data was written to physical main RAM, not write-sink
    // At power-on state, reads from $F800-$FFFF go to main RAM
    assert(bus.read(0xF800) == 0xA9); // LDA opcode
    assert(bus.read(0xF801) == 0x42); // Immediate value

    // Verify reset vector is correct
    uint16_t reset_vec = bus.read_word(0xFFFC);
    assert(reset_vec == 0xF800);

    // Verify the bytes are not TRAP_OPCODE
    assert(bus.read(0xFFFC) != Bus::TRAP_OPCODE);
    assert(bus.read(0xFFFF) != Bus::TRAP_OPCODE);

    std::cout << "✓ test_rom_loading_at_reset passed" << std::endl;
}

void test_rom_write_protected() {
    Bus bus;

    // Load ROM with a known value
    std::vector<uint8_t> rom_data(0x800, 0xEA); // 2KB of NOPs
    bus.initialize_memory(0xF800, rom_data);

    // Verify ROM loaded correctly
    assert(bus.read(0xF800) == 0xEA);

    // At power-on state, writes to $F800-$FFFF should go to write-sink
    // (not affect the main RAM that we read from)
    bus.write(0xF800, 0x42);

    // Read should still return original ROM value
    assert(bus.read(0xF800) == 0xEA);

    std::cout << "✓ test_rom_write_protected passed" << std::endl;
}

int main() {
    std::cout << "Running CPU/Bus unit tests..." << std::endl << std::endl;

    try {
        test_cpu_reset();
        test_bus_memory();
        test_bus_reset();
        test_cpu_lda_immediate();
        test_cpu_ldx_ldy();
        test_cpu_flags();
        test_cpu_stack();
        test_cpu_branches();
        test_cpu_jsr_rts();
        test_rom_loading_at_reset();
        test_rom_write_protected();

        std::cout << std::endl << "All tests passed! ✓" << std::endl;
        return 0;
    } catch (const std::exception &e) {
        std::cerr << "Test failed with exception: " << e.what() << std::endl;
        return 1;
    }
}
