// Manual test to demonstrate the double-read requirement for language card write-enable
// This test shows that addresses $C081, $C083, $C089, $C08B require TWO successive reads
// to enable write mode, as documented in APPLE_IIE_MEMORY_MAP.md

#include "edasm/emulator/bus.hpp"
#include "edasm/emulator/host_shims.hpp"
#include <iostream>
#include <iomanip>

using namespace edasm;

void test_single_vs_double_read() {
    std::cout << "=== Language Card Double-Read Requirement Test ===" << std::endl;
    std::cout << std::endl;
    
    Bus bus;
    HostShims shims;
    shims.install_io_traps(bus);
    
    // Initialize all memory including language card banks to 0xFF (simulating ROM content)
    // Main RAM ROM area
    for (uint32_t addr = 0xD000; addr < 0x10000; ++addr) {
        bus.data()[Bus::MAIN_RAM_OFFSET + addr] = 0xFF;
    }
    // Language card Bank 1
    for (uint32_t addr = 0; addr < 0x1000; ++addr) {
        bus.data()[Bus::LC_BANK1_OFFSET + addr] = 0xFF;
    }
    // Language card Bank 2
    for (uint32_t addr = 0; addr < 0x1000; ++addr) {
        bus.data()[Bus::LC_BANK2_OFFSET + addr] = 0xFF;
    }
    // Language card fixed RAM
    for (uint32_t addr = 0; addr < 0x2000; ++addr) {
        bus.data()[Bus::LC_FIXED_RAM_OFFSET + addr] = 0xFF;
    }
    
    std::cout << "1. Testing SINGLE read of $C083 (should NOT enable write):" << std::endl;
    std::cout << "   - Reading $C083 once..." << std::endl;
    bus.read(0xC083);
    
    std::cout << "   - Attempting to write $AA to $D000..." << std::endl;
    bus.write(0xD000, 0xAA);
    
    uint8_t value = bus.read(0xD000);
    std::cout << "   - Reading back from $D000: $" << std::hex << std::uppercase 
              << std::setw(2) << std::setfill('0') << (int)value << std::endl;
    
    if (value == 0xFF) {
        std::cout << "   ✓ CORRECT: Write was NOT enabled (still contains init value $FF)" << std::endl;
    } else if (value == 0xAA) {
        std::cout << "   ✗ WRONG: Write was enabled (RAM was written)" << std::endl;
    } else {
        std::cout << "   ? UNEXPECTED: Got unexpected value $" << std::hex 
                  << (int)value << std::endl;
    }
    std::cout << std::endl;
    
    // Reset language card state by accessing $C080 (READ_RAM_NO_WRITE, no double-read required)
    std::cout << "   - Resetting LC state with $C080..." << std::endl;
    bus.read(0xC080);
    std::cout << std::endl;
    
    std::cout << "2. Testing DOUBLE read of $C083 (should enable write):" << std::endl;
    std::cout << "   - Reading $C083 twice..." << std::endl;
    bus.read(0xC083);
    bus.read(0xC083);
    
    std::cout << "   - Writing $BB to $D000..." << std::endl;
    bus.write(0xD000, 0xBB);
    
    value = bus.read(0xD000);
    std::cout << "   - Reading back from $D000: $" << std::hex << std::uppercase 
              << std::setw(2) << std::setfill('0') << (int)value << std::endl;
    
    if (value == 0xBB) {
        std::cout << "   ✓ CORRECT: Write was enabled (reading RAM = $BB)" << std::endl;
    } else {
        std::cout << "   ✗ WRONG: Write was NOT enabled (got $" << std::hex 
                  << (int)value << ")" << std::endl;
    }
    std::cout << std::endl;
    
    std::cout << "3. Testing $C081 (READ ROM, WRITE RAM with double-read):" << std::endl;
    std::cout << "   - Reading $C081 twice..." << std::endl;
    bus.read(0xC081);
    bus.read(0xC081);
    
    std::cout << "   - Writing $CC to $D100..." << std::endl;
    bus.write(0xD100, 0xCC);
    
    std::cout << "   - Reading back from $D100 (should read ROM = $FF)..." << std::endl;
    value = bus.read(0xD100);
    std::cout << "   - Value: $" << std::hex << std::uppercase 
              << std::setw(2) << std::setfill('0') << (int)value << std::endl;
    
    if (value == 0xFF) {
        std::cout << "   ✓ CORRECT: Reading ROM (not the written RAM)" << std::endl;
    } else {
        std::cout << "   ✗ WRONG: Reading RAM instead of ROM (got $" << std::hex 
                  << (int)value << ")" << std::endl;
    }
    
    std::cout << "   - Switching to $C083 to read RAM..." << std::endl;
    bus.read(0xC083);
    bus.read(0xC083);
    
    value = bus.read(0xD100);
    std::cout << "   - Reading from $D100 (should read RAM = $CC)..." << std::endl;
    std::cout << "   - Value: $" << std::hex << std::uppercase 
              << std::setw(2) << std::setfill('0') << (int)value << std::endl;
    
    if (value == 0xCC) {
        std::cout << "   ✓ CORRECT: The write to RAM was successful!" << std::endl;
    } else {
        std::cout << "   ✗ WRONG: RAM does not contain the written value (got $" << std::hex 
                  << (int)value << ")" << std::endl;
    }
    std::cout << std::endl;
}

int main() {
    test_single_vs_double_read();
    
    std::cout << "=== Test Complete ===" << std::endl;
    std::cout << std::endl;
    std::cout << "Summary:" << std::endl;
    std::cout << "- Addresses $C081, $C083, $C089, $C08B require TWO successive reads" << std::endl;
    std::cout << "  to enable write mode (per Apple IIe documentation)" << std::endl;
    std::cout << "- A single read only affects read mode and bank selection" << std::endl;
    std::cout << "- This is now correctly implemented in host_shims.cpp" << std::endl;
    
    return 0;
}
