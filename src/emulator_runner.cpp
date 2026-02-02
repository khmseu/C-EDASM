/**
 * @file emulator_runner.cpp
 * @brief Standalone 65C02 emulator runner for EDASM.SYSTEM
 *
 * Runs the EDASM.SYSTEM binary in emulation mode, executing the editor/
 * assembler with ProDOS MLI emulation for file I/O.
 *
 * Features:
 * - Loads EDASM.SYSTEM binary at correct address
 * - Emulates ProDOS MLI calls for file operations
 * - Maps ProDOS paths 1:1 to Linux filesystem
 * - Provides trace output for debugging
 *
 * Reference: docs/EMULATOR_MINIMAL_PLAN.md
 */

#include "edasm/constants.hpp"
#include "edasm/emulator/bus.hpp"
#include "edasm/emulator/cpu.hpp"
#include "edasm/emulator/disassembly.hpp"
#include "edasm/emulator/host_shims.hpp"
#include "edasm/emulator/traps.hpp"
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <string>
#include <vector>

using namespace edasm;

// Helper function to read input lines from a text file
std::vector<std::string> read_input_file(const std::string &filepath) {
    std::vector<std::string> lines;
    std::ifstream file(filepath);
    if (!file.is_open()) {
        std::cerr << "Error: Failed to open input file: " << filepath << std::endl;
        return lines;
    }

    std::string line;
    while (std::getline(file, line)) {
        lines.push_back(line);
    }

    file.close();
    std::cout << "Loaded " << lines.size() << " input lines from: " << filepath << std::endl;
    return lines;
}

int main(int argc, char *argv[]) {
    std::cout << "C-EDASM Minimal Emulator" << std::endl;
    std::cout << "========================" << std::endl << std::endl;

    // Parse command line
    std::string binary_path = "third_party/EdAsm/EDASM.SYSTEM";
    std::string input_file_path;
    uint16_t load_addr = 0x2000;
    uint16_t entry_point = 0x0000; // will follow hardware reset vector
    size_t max_instructions = 1000;
    bool trace = false;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--binary" && i + 1 < argc) {
            binary_path = argv[++i];
        } else if (arg == "--load" && i + 1 < argc) {
            load_addr = static_cast<uint16_t>(std::stoul(argv[++i], nullptr, 16));
        } else if (arg == "--entry" && i + 1 < argc) {
            entry_point = static_cast<uint16_t>(std::stoul(argv[++i], nullptr, 16));
        } else if (arg == "--max" && i + 1 < argc) {
            max_instructions = std::stoul(argv[++i]);
        } else if (arg == "--input-file" && i + 1 < argc) {
            input_file_path = argv[++i];
        } else if (arg == "--trace") {
            trace = true;
        } else if (arg == "--help") {
            std::cout << "Usage: " << argv[0] << " [options]" << std::endl;
            std::cout << "Options:" << std::endl;
            std::cout << "  --binary <path>      Binary file to load (default: "
                         "third_party/EdAsm/EDASM.SYSTEM)"
                      << std::endl;
            std::cout << "  --load <addr>        Load address in hex (default: 2000)" << std::endl;
            std::cout << "  --entry <addr>       Entry point in hex (default: 2000)" << std::endl;
            std::cout << "  --max <n>            Max instructions to execute (default: 1000)"
                      << std::endl;
            std::cout << "  --input-file <path>  Text file with input lines (one per line)"
                      << std::endl;
            std::cout << "  --trace              Enable instruction tracing" << std::endl;
            std::cout << "  --help               Show this help" << std::endl;
            return 0;
        }
    }

    register_default_disassembly_symbols();

    // Initialize emulator
    Bus bus;
    CPU cpu(bus);
    HostShims shims;

    // Load and queue input file if provided
    if (!input_file_path.empty()) {
        std::vector<std::string> input_lines = read_input_file(input_file_path);
        if (!input_lines.empty()) {
            shims.queue_input_lines(input_lines);
        }
    }

    std::cout << "Initializing emulator..." << std::endl;
    std::cout << "  Memory: 64KB filled with trap opcode ($02)" << std::endl;

    // Reset bus (fills with trap opcode)
    bus.reset();

    // Initialize monitor soft-entry vectors as requested
    // SOFTEV ($03F2) = 0x00, SOFTEV+1 ($03F3) = 0x20
    // PWREDUP ($03F4) = $20 XOR $A5
    bus.write(SOFTEV, 0x00);
    bus.write(static_cast<uint16_t>(SOFTEV + 1), 0x20);
    bus.write(PWREDUP, static_cast<uint8_t>(0x20 ^ 0xA5));

    // Load monitor ROM into upper 8KB
    const uint16_t rom_base = 0xF800;
    const std::string rom_rel_path =
        "third_party/artifacts/Apple II plus ROM Pages F8-FF - 341-0020 - Autostart Monitor.bin";

    std::error_code exe_ec;
    std::filesystem::path exe_dir = std::filesystem::weakly_canonical(argv[0], exe_ec);
    if (exe_ec) {
        exe_dir = std::filesystem::path(argv[0]);
    }
    exe_dir = exe_dir.parent_path();

    std::filesystem::path rom_path = exe_dir.parent_path() / rom_rel_path;

    std::cout << "  Loading monitor ROM: " << rom_path.string() << std::endl;
    if (bus.load_binary_from_file(rom_base, rom_path.string())) {
        std::cout << "  Monitor ROM mapped at $F800-$FFFF" << std::endl;
        bus.set_write_trap_range(
            rom_base, 0xFFFF, [](uint16_t, uint8_t) { return true; }, "ROM_WRITE_PROTECT");
        std::cout << "  ROM writes are trapped (read-only region)" << std::endl;
    } else {
        std::cerr << "Error: Failed to load monitor ROM from " << rom_path.string() << std::endl;
    }

    // Install host shims
    shims.install_io_traps(bus);
    std::cout << "  I/O traps installed at $C000 (KBD) and $C010 (KBDSTRB)" << std::endl;

    // Try to load binary
    std::cout << "  Loading binary: " << binary_path << std::endl;
    std::cout << "  Load address: $" << std::hex << std::uppercase << std::setw(4)
              << std::setfill('0') << load_addr << std::endl;

    if (!bus.load_binary_from_file(load_addr, binary_path)) {
        std::cerr << "Error: Failed to load binary file: " << binary_path << std::endl;
        std::cerr << "Note: EDASM.SYSTEM binary should be extracted from EDASM_SRC.2mg"
                  << std::endl;
        std::cerr << "      For now, this emulator will just demonstrate trap behavior."
                  << std::endl;
    } else {
        std::cout << "  Binary loaded successfully" << std::endl;
    }

    // Set entry point from 6502 hardware reset vector at $FFFC/$FFFD
    uint16_t reset_vec = bus.read_word(0xFFFC);
    cpu.state().PC = reset_vec;
    std::cout << "  Entry point (reset vector): $" << std::hex << std::uppercase << std::setw(4)
              << std::setfill('0') << reset_vec << std::endl;

    // Install general trap handler with ProDOS MLI handler at $BF00
    TrapManager::set_trace(trace);
    TrapManager::install_address_handler(PRODOS8, TrapManager::prodos_mli_trap_handler,
                                         "ProDOS MLI");
    TrapManager::install_address_handler(0xFE84, TrapManager::monitor_setnorm_trap_handler,
                                         "MONITOR SETNORM");
    cpu.set_trap_handler(TrapManager::general_trap_handler);
    std::cout << "  General trap handler installed with ProDOS MLI at $BF00" << std::endl;
    std::cout << "  Monitor ROM SETNORM handler installed at $FE84" << std::endl;

    std::cout << std::endl << "Starting execution..." << std::endl;
    std::cout << "Maximum instructions: " << std::dec << max_instructions << std::endl;
    if (trace) {
        std::cout << "Tracing enabled" << std::endl;
    }
    std::cout << std::endl;

    // Run emulator
    size_t count = 0;
    bool running = true;

    while (running && count < max_instructions) {
        if (trace) {
            std::cout << "[" << std::dec << count << "] "
                      << TrapManager::dump_cpu_state(cpu.state());
            std::cout << "    " << format_disassembly(bus, cpu.state().PC) << std::endl;
        }

        running = cpu.step();
        if (!running)
            std::cout << "\nEmulator stopped by cpu.step()" << std::endl;
        count++;

        // Check if HostShims requested a stop (e.g., first screen char is 'E')
        if (shims.should_stop()) {
            std::cout << "\nEmulator stopped by HostShims" << std::endl;
            running = false;
        }
    }

    std::cout << std::endl;
    std::cout << "Execution stopped after " << std::dec << count << " instructions" << std::endl;
    std::cout << "Final CPU state:" << std::endl;
    std::cout << TrapManager::dump_cpu_state(cpu.state()) << std::endl;

    // Print trap statistics
    TrapStatistics::print_statistics();

    if (running) {
        std::cout << std::endl << "Reached maximum instruction limit" << std::endl;
        return 1;
    } else {
        std::cout << std::endl << "Halted by trap handler" << std::endl;
        return 0;
    }
}
