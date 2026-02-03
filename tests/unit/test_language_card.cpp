#include "edasm/emulator/bus.hpp"
#include "edasm/emulator/host_shims.hpp"
#include <iostream>

using namespace edasm;

void print_test_result(const std::string &test_name, bool passed) {
    std::cout << (passed ? "✓ " : "✗ ") << test_name << (passed ? " passed" : " FAILED")
              << std::endl;
}

bool test_lc_basic_write_read() {
    Bus bus;
    HostShims shims;
    shims.install_io_traps(bus);

    // Initialize ROM area in main RAM to 0x00 (simulating empty ROM)
    // This is what happens after ROM is loaded in a real system
    // We need to write directly to physical memory because at power-on, 
    // writes to $D000-$FFFF are directed to write-sink (ROM is read-only)
    auto ranges = bus.translate_read_range(0xD000, 0x3000);
    uint8_t *mem = bus.physical_memory();
    for (const auto &range : ranges) {
        std::fill_n(mem + range.physical_offset, range.length, 0x00);
    }

    // Activate bank2 LCBANK2 (read/write RAM) -> address C083
    // NOTE: Requires TWO successive reads to enable write (per Apple IIe spec)
    bus.read(0xC083);
    bus.read(0xC083); // Second read required to enable write

    // Write value to $D000 and read back
    bus.write(0xD000, 0x55);
    uint8_t v = bus.read(0xD000);
    if (v != 0x55) {
        std::cerr << "Expected 0x55 from $D000 after LCBANK2 write, got $" << std::hex
                  << static_cast<int>(v) << std::endl;
        return false;
    }

    // Write to E000 (fixed ram) and read back
    bus.write(0xE000, 0x11);
    v = bus.read(0xE000);
    if (v != 0x11) {
        std::cerr << "Expected 0x11 from $E000 after write, got $" << std::hex
                  << static_cast<int>(v) << std::endl;
        return false;
    }

    // Switch to RDBANK2 (read RAM only, writes ignored for D000..DFFF)
    bus.read(0xC080);
    bus.write(0xD000, 0x66);
    v = bus.read(0xD000);
    if (v != 0x55) {
        std::cerr << "Expected $55 unchanged after write in RDBANK2 (write ignored), got $"
                  << std::hex << static_cast<int>(v) << std::endl;
        return false;
    }

    // Back to LCBANK2 and write new value
    // NOTE: Requires TWO successive reads to enable write
    bus.read(0xC083);
    bus.read(0xC083); // Second read required
    bus.write(0xD000, 0x77);
    v = bus.read(0xD000);
    if (v != 0x77) {
        std::cerr << "Expected $77 after LCBANK2 write, got $" << std::hex << static_cast<int>(v)
                  << std::endl;
        return false;
    }

    // Test ROMIN2: reads should return ROM image (initially 0) across D000..FFFF, but writes update
    // underlying RAM
    // NOTE: Requires TWO successive reads to enable write
    bus.read(0xC081);
    bus.read(0xC081);        // Second read required
    bus.write(0xD000, 0x88); // should write to underlying banked RAM
    bus.write(0xE000, 0x99); // should write to underlying fixed RAM

    // While in ROMIN2, reads return ROM image (initially 0)
    v = bus.read(0xD000);
    if (v != 0x00) {
        std::cerr << "Expected ROM (0x00) while in ROMIN2 read mode (D000), got $" << std::hex
                  << static_cast<int>(v) << std::endl;
        return false;
    }
    v = bus.read(0xE000);
    if (v != 0x00) {
        std::cerr << "Expected ROM (0x00) while in ROMIN2 read mode (E000), got $" << std::hex
                  << static_cast<int>(v) << std::endl;
        return false;
    }

    // Switch back to LCBANK2 to read the RAM region and verify the writes happened
    // NOTE: Requires TWO successive reads to enable write
    bus.read(0xC083);
    bus.read(0xC083); // Second read required
    v = bus.read(0xD000);
    if (v != 0x88) {
        std::cerr << "Expected RAM 0x88 after returning from ROMIN2, got $" << std::hex
                  << static_cast<int>(v) << std::endl;
        return false;
    }
    v = bus.read(0xE000);
    if (v != 0x99) {
        std::cerr << "Expected RAM 0x99 in fixed region after returning from ROMIN2, got $"
                  << std::hex << static_cast<int>(v) << std::endl;
        return false;
    }

    return true;
}

int main() {
    std::cout << "Testing Language Card soft-switch handlers..." << std::endl;

    bool all_passed = true;

    bool result = test_lc_basic_write_read();
    print_test_result("test_lc_basic_write_read", result);
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
