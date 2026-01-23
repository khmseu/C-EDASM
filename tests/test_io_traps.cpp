/**
 * Test program for I/O traps in $C000-$C7FF range
 * Verifies that read and write traps work for Apple II I/O devices
 */

#include "edasm/bus.hpp"
#include "edasm/cpu.hpp"
#include "edasm/host_shims.hpp"
#include <iomanip>
#include <iostream>
#include <sstream>

using namespace edasm;

void print_test_result(const std::string &test_name, bool passed) {
    std::cout << (passed ? "✓ " : "✗ ") << test_name << (passed ? " passed" : " FAILED")
              << std::endl;
}

// Test keyboard I/O traps
bool test_keyboard_io() {
    Bus bus;
    HostShims shims;

    // Install I/O traps
    shims.install_io_traps(bus);

    // Test 1: Read from $C000 with no input (should return 0, no key available)
    uint8_t value = bus.read(0xC000);
    if (value != 0) {
        std::cerr << "Expected 0 from $C000 with no input, got $" << std::hex
                  << static_cast<int>(value) << std::endl;
        return false;
    }

    // Test 2: Queue input and read from $C000
    shims.queue_input_line("A");
    value = bus.read(0xC000);
    if ((value & 0x7F) != 'A') {
        std::cerr << "Expected 'A' ($41) from $C000, got $" << std::hex << static_cast<int>(value)
                  << std::endl;
        return false;
    }
    if ((value & 0x80) == 0) {
        std::cerr << "Expected high bit set (key available)" << std::endl;
        return false;
    }

    // Test 3: Clear strobe by reading $C010
    bus.read(0xC010);
    value = bus.read(0xC000);
    if ((value & 0x80) != 0) {
        std::cerr << "Expected high bit clear after reading $C010" << std::endl;
        return false;
    }

    return true;
}

// Test graphics switches
bool test_graphics_switches() {
    Bus bus;
    HostShims shims;

    // Install I/O traps
    shims.install_io_traps(bus);

    // Test soft switches (read or write triggers them)
    // Just verify no crashes when accessing these addresses
    bus.read(0xC050); // TXTCLR
    bus.read(0xC051); // TXTSET
    bus.read(0xC052); // MIXCLR
    bus.read(0xC053); // MIXSET
    bus.read(0xC054); // LOWSCR
    bus.read(0xC055); // HISCR
    bus.read(0xC056); // LORES
    bus.read(0xC057); // HIRES

    // Test writes too
    bus.write(0xC050, 0); // TXTCLR
    bus.write(0xC051, 0); // TXTSET

    return true;
}

// Test speaker toggle
bool test_speaker_toggle() {
    Bus bus;
    HostShims shims;

    // Install I/O traps
    shims.install_io_traps(bus);

    // Access speaker toggle (any access to $C030 toggles speaker)
    bus.read(0xC030);
    bus.write(0xC030, 0);

    return true;
}

// Test game I/O
bool test_game_io() {
    Bus bus;
    HostShims shims;

    // Install I/O traps
    shims.install_io_traps(bus);

    // Read paddle buttons ($C061-$C063)
    uint8_t btn0 = bus.read(0xC061);
    if (btn0 & 0x80) {
        std::cerr << "Expected button 0 not pressed (high bit clear)" << std::endl;
        return false;
    }

    // Read paddle trigger ($C070)
    bus.read(0xC070);

    return true;
}

// Test text screen logging on $C000 access
bool test_text_screen_logging() {
    Bus bus;
    HostShims shims;

    shims.install_io_traps(bus);

    // Capture stdout
    std::ostringstream oss;
    std::streambuf *old_buf = std::cout.rdbuf(oss.rdbuf());

    // Write to text page 1 and trigger keyboard read trap
    bus.write(0x0400, 'A');
    bus.read(0xC000); // Should log screen snapshot

    std::cout.rdbuf(old_buf);
    const std::string first_log = oss.str();

    if (first_log.find("Text screen snapshot") == std::string::npos) {
        std::cerr << "Expected text screen snapshot log on keyboard read" << std::endl;
        return false;
    }
    if (first_log.find('A') == std::string::npos) {
        std::cerr << "Expected character 'A' in logged screen" << std::endl;
        return false;
    }

    // Ensure subsequent reads do not log when screen is unchanged
    std::ostringstream oss2;
    old_buf = std::cout.rdbuf(oss2.rdbuf());
    bus.read(0xC000);
    std::cout.rdbuf(old_buf);

    if (!oss2.str().empty()) {
        std::cerr << "Unexpected additional screen log without changes" << std::endl;
        return false;
    }

    return true;
}

// Test stop on 'E' character at first screen position
bool test_stop_on_e_character() {
    Bus bus;
    HostShims shims;

    shims.install_io_traps(bus);

    // Capture stdout
    std::ostringstream oss;
    std::streambuf *old_buf = std::cout.rdbuf(oss.rdbuf());

    // Write 'A' to first position - should not stop
    bus.write(0x0400, 'A');
    std::cout.rdbuf(old_buf);

    if (shims.should_stop()) {
        std::cerr << "Unexpected stop after writing 'A' to first screen position" << std::endl;
        return false;
    }

    // Now write 'E' to first position - should log and stop
    oss.str("");
    oss.clear();
    old_buf = std::cout.rdbuf(oss.rdbuf());
    bus.write(0x0400, 'E');
    std::cout.rdbuf(old_buf);

    const std::string output = oss.str();

    if (!shims.should_stop()) {
        std::cerr << "Expected stop after writing 'E' to first screen position" << std::endl;
        return false;
    }

    if (output.find("First screen character set to 'E'") == std::string::npos) {
        std::cerr << "Expected message about 'E' character in output" << std::endl;
        return false;
    }

    if (output.find("Text screen snapshot") == std::string::npos) {
        std::cerr << "Expected screen log after 'E' written" << std::endl;
        return false;
    }

    return true;
}

// Test full I/O range coverage
bool test_full_io_range() {
    Bus bus;
    HostShims shims;

    // Install I/O traps
    shims.install_io_traps(bus);

    // Test that we can read/write across entire $C000-$C7FF range
    // without crashes
    for (uint16_t addr = 0xC000; addr <= 0xC0FF; addr++) {
        bus.read(addr);
        bus.write(addr, 0xFF);
    }

    // Test slot I/O areas ($C100-$C7FF)
    for (uint16_t addr = 0xC100; addr <= 0xC7FF; addr += 0x10) {
        bus.read(addr);
        bus.write(addr, 0xFF);
    }

    return true;
}

// Test that traps are properly installed
bool test_trap_installation() {
    Bus bus;
    HostShims shims;

    // Before installing traps, reads should return trap opcode ($02)
    uint8_t value = bus.read(0xC000);
    if (value != 0x02) {
        std::cerr << "Expected trap opcode ($02) before installing traps, got $" << std::hex
                  << static_cast<int>(value) << std::endl;
        return false;
    }

    // After installing traps, reads should return trap handler values
    shims.install_io_traps(bus);
    value = bus.read(0xC000);
    if (value == 0x02) {
        std::cerr << "Expected trap handler to be called, but got trap opcode" << std::endl;
        return false;
    }

    return true;
}

int main() {
    std::cout << "Testing I/O traps for $C000-$C7FF range..." << std::endl;
    std::cout << std::endl;

    bool all_passed = true;

    bool result = test_trap_installation();
    print_test_result("test_trap_installation", result);
    all_passed = all_passed && result;

    result = test_keyboard_io();
    print_test_result("test_keyboard_io", result);
    all_passed = all_passed && result;

    result = test_graphics_switches();
    print_test_result("test_graphics_switches", result);
    all_passed = all_passed && result;

    result = test_speaker_toggle();
    print_test_result("test_speaker_toggle", result);
    all_passed = all_passed && result;

    result = test_game_io();
    print_test_result("test_game_io", result);
    all_passed = all_passed && result;

    result = test_text_screen_logging();
    print_test_result("test_text_screen_logging", result);
    all_passed = all_passed && result;

    result = test_full_io_range();
    print_test_result("test_full_io_range", result);
    all_passed = all_passed && result;

    result = test_stop_on_e_character();
    print_test_result("test_stop_on_e_character", result);
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
