/**
 * @file test_rom_reset.cpp
 * @brief Test ROM loading and reset vector handling
 *
 * Verifies that ROM can be loaded into the $F800-$FFFF range and that
 * the reset vector is properly read after ROM loading.
 */

#include "edasm/emulator/bus.hpp"
#include "edasm/emulator/cpu.hpp"
#include <cassert>
#include <filesystem>
#include <fstream>
#include <iostream>

using namespace edasm;

void print_test_result(const std::string &test_name, bool passed) {
    std::cout << (passed ? "✓ " : "✗ ") << test_name << (passed ? " passed" : " FAILED")
              << std::endl;
}

bool test_rom_reset_vector() {
    Bus bus;
    CPU cpu(bus);

    // Create a mock ROM with specific reset behavior
    std::vector<uint8_t> rom(0x800, 0xEA); // 2KB of NOPs

    // Set reset vector to point to $FA00
    rom[0x7FC] = 0x00; // Low byte
    rom[0x7FD] = 0xFA; // High byte

    // Place a recognizable instruction at $FA00
    rom[0x200] = 0xD8; // CLD (Clear Decimal mode)
    rom[0x201] = 0x18; // CLC (Clear Carry)
    rom[0x202] = 0x60; // RTS

    // Load ROM at $F800
    if (!bus.initialize_memory(0xF800, rom)) {
        std::cerr << "Failed to load ROM" << std::endl;
        return false;
    }

    // Read reset vector - should get $FA00
    uint16_t reset_vec = bus.read_word(0xFFFC);
    if (reset_vec != 0xFA00) {
        std::cerr << "Reset vector incorrect: expected $FA00, got $" << std::hex << reset_vec
                  << std::endl;
        return false;
    }

    // Set CPU PC from reset vector
    cpu.state().PC = reset_vec;

    // Verify we can read the instructions from ROM
    if (bus.read(0xFA00) != 0xD8) {
        std::cerr << "Failed to read CLD instruction at $FA00" << std::endl;
        return false;
    }

    if (bus.read(0xFA01) != 0x18) {
        std::cerr << "Failed to read CLC instruction at $FA01" << std::endl;
        return false;
    }

    // Execute first instruction (CLD)
    cpu.step();
    if (cpu.state().PC != 0xFA01) {
        std::cerr << "PC should be at $FA01 after CLD, got $" << std::hex << cpu.state().PC
                  << std::endl;
        return false;
    }

    // Verify decimal flag is cleared
    if (cpu.state().P & 0x08) {
        std::cerr << "Decimal flag should be clear after CLD" << std::endl;
        return false;
    }

    return true;
}

bool test_actual_monitor_rom() {
    // Test with actual Apple II Monitor ROM if available
    const std::string rom_path =
        "third_party/artifacts/Apple II plus ROM Pages F8-FF - 341-0020 - Autostart Monitor.bin";

    if (!std::filesystem::exists(rom_path)) {
        std::cout << "  (Skipping: ROM file not found at " << rom_path << ")" << std::endl;
        return true; // Not a failure, just skip
    }

    Bus bus;

    // Load actual ROM
    if (!bus.load_rom_from_file(0xF800, rom_path)) {
        std::cerr << "Failed to load ROM from " << rom_path << std::endl;
        return false;
    }

    // Read reset vector - should be within ROM range
    uint16_t reset_vec = bus.read_word(0xFFFC);
    if (reset_vec < 0xF800 || reset_vec > 0xFFFF) {
        std::cerr << "Reset vector $" << std::hex << reset_vec << " is outside ROM range"
                  << std::endl;
        return false;
    }

    // Should not be trap opcode
    if (bus.read(0xFFFC) == Bus::TRAP_OPCODE || bus.read(0xFFFF) == Bus::TRAP_OPCODE) {
        std::cerr << "Reset vector area still contains trap opcodes" << std::endl;
        return false;
    }

    std::cout << "  Reset vector from ROM: $" << std::hex << reset_vec << std::endl;

    return true;
}

bool test_rom_write_protection() {
    Bus bus;

    // Load a ROM with known values
    std::vector<uint8_t> rom(0x800, 0x42);
    bus.initialize_memory(0xF800, rom);

    // Verify ROM loaded
    if (bus.read(0xF800) != 0x42) {
        std::cerr << "ROM not loaded correctly" << std::endl;
        return false;
    }

    // Try to write to ROM area - should not affect readable value
    // (writes go to write-sink at power-on state)
    bus.write(0xF800, 0x99);

    // Should still read original value
    if (bus.read(0xF800) != 0x42) {
        std::cerr << "ROM was modified by write (expected write-protection)" << std::endl;
        return false;
    }

    // Test across the entire ROM range
    bus.write(0xFFFF, 0xAA);
    if (bus.read(0xFFFF) != 0x42) {
        std::cerr << "ROM at $FFFF was modified by write" << std::endl;
        return false;
    }

    return true;
}

int main() {
    std::cout << "Testing ROM loading and reset vector handling..." << std::endl;
    std::cout << std::endl;

    bool all_passed = true;

    bool result = test_rom_reset_vector();
    print_test_result("test_rom_reset_vector", result);
    all_passed = all_passed && result;

    result = test_actual_monitor_rom();
    print_test_result("test_actual_monitor_rom", result);
    all_passed = all_passed && result;

    result = test_rom_write_protection();
    print_test_result("test_rom_write_protection", result);
    all_passed = all_passed && result;

    std::cout << std::endl;
    if (all_passed) {
        std::cout << "All tests passed! ✓" << std::endl;
        return 0;
    } else {
        std::cerr << "Some tests failed! ✗" << std::endl;
        return 1;
    }
}
