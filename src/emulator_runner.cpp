#include "edasm/bus.hpp"
#include "edasm/cpu.hpp"
#include "edasm/host_shims.hpp"
#include "edasm/traps.hpp"
#include <algorithm>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>

using namespace edasm;

namespace {

// Minimal 6502 instruction length table: 1-byte implied/accumulator, 3-byte absolute/JMP, else
// 2-byte
size_t guess_instruction_length(uint8_t opcode) {
    switch (opcode) {
    // Implied/accumulator (1 byte)
    case 0x00: // BRK
    case 0x08: // PHP
    case 0x0A: // ASL A
    case 0x18: // CLC
    case 0x28: // PLP
    case 0x2A: // ROL A
    case 0x38: // SEC
    case 0x40: // RTI
    case 0x48: // PHA
    case 0x4A: // LSR A
    case 0x58: // CLI
    case 0x60: // RTS
    case 0x68: // PLA
    case 0x6A: // ROR A
    case 0x78: // SEI
    case 0x88: // DEY
    case 0x8A: // TXA
    case 0x98: // TYA
    case 0x9A: // TXS
    case 0xA8: // TAY
    case 0xAA: // TAX
    case 0xB8: // CLV
    case 0xBA: // TSX
    case 0xC8: // INY
    case 0xCA: // DEX
    case 0xD8: // CLD
    case 0xE8: // INX
    case 0xEA: // NOP
    case 0xF8: // SED
        return 1;

    // Absolute/absolute indexed/JMP (3 bytes)
    case 0x0D:
    case 0x0E:
    case 0x1D:
    case 0x1E:
    case 0x20: // JSR
    case 0x2C:
    case 0x2D:
    case 0x2E:
    case 0x3D:
    case 0x3E:
    case 0x4C: // JMP abs
    case 0x4D:
    case 0x4E:
    case 0x5D:
    case 0x5E:
    case 0x6C: // JMP (abs)
    case 0x6D:
    case 0x6E:
    case 0x7D:
    case 0x7E:
    case 0x8C:
    case 0x8D:
    case 0x8E:
    case 0x9D:
    case 0xAC:
    case 0xAD:
    case 0xAE:
    case 0xBC:
    case 0xBD:
    case 0xBE:
    case 0xCC:
    case 0xCD:
    case 0xCE:
    case 0xEC:
    case 0xED:
    case 0xEE:
    case 0xFD:
    case 0xFE:
        return 3;

    default:
        return 2; // Immediate/zero page/branch, and unknowns fallback to 2-byte view
    }
}

std::string format_disassembly(const Bus &bus, uint16_t pc) {
    const uint8_t *mem = bus.data();
    uint8_t opcode = mem[pc];
    size_t length = guess_instruction_length(opcode);

    std::ostringstream oss;
    oss << "PC=$" << std::hex << std::uppercase << std::setw(4) << std::setfill('0') << pc
        << "  OP=" << std::setw(2) << static_cast<int>(opcode) << "  bytes:";

    // Clamp to a short preview window to keep logs compact
    size_t preview_len = std::min<size_t>(length, 3);
    for (size_t i = 0; i < preview_len; ++i) {
        oss << ' ' << std::setw(2) << static_cast<int>(mem[pc + static_cast<uint16_t>(i)]);
    }

    oss << "  (len~" << length << ")";
    return oss.str();
}

} // namespace

int main(int argc, char *argv[]) {
    std::cout << "C-EDASM Minimal Emulator" << std::endl;
    std::cout << "========================" << std::endl << std::endl;

    // Parse command line
    std::string binary_path = "third_party/EdAsm/EDASM.SYSTEM";
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
        } else if (arg == "--trace") {
            trace = true;
        } else if (arg == "--help") {
            std::cout << "Usage: " << argv[0] << " [options]" << std::endl;
            std::cout << "Options:" << std::endl;
            std::cout << "  --binary <path>   Binary file to load (default: "
                         "third_party/EdAsm/EDASM.SYSTEM)"
                      << std::endl;
            std::cout << "  --load <addr>     Load address in hex (default: 2000)" << std::endl;
            std::cout << "  --entry <addr>    Entry point in hex (default: 2000)" << std::endl;
            std::cout << "  --max <n>         Max instructions to execute (default: 1000)"
                      << std::endl;
            std::cout << "  --trace           Enable instruction tracing" << std::endl;
            std::cout << "  --help            Show this help" << std::endl;
            return 0;
        }
    }

    // Initialize emulator
    Bus bus;
    CPU cpu(bus);
    HostShims shims;

    std::cout << "Initializing emulator..." << std::endl;
    std::cout << "  Memory: 64KB filled with trap opcode ($02)" << std::endl;

    // Reset bus (fills with trap opcode)
    bus.reset();

    // Initialize monitor soft-entry vectors as requested
    // SOFTEV ($03F2) = 0x00, SOFTEV+1 ($03F3) = 0x20
    // PWREDUP ($03F4) = $20 XOR $A5
    bus.write(0x03F2, 0x00);
    bus.write(0x03F3, 0x20);
    bus.write(0x03F4, static_cast<uint8_t>(0x20 ^ 0xA5));

    // Load monitor ROM into upper 8KB
    const uint16_t rom_base = 0xF800;
    const std::string rom_path =
        "third_party/artifacts/Apple II plus ROM Pages F8-FF - 341-0020 - Autostart Monitor.bin";
    std::cout << "  Loading monitor ROM: " << rom_path << std::endl;
    if (bus.load_binary_from_file(rom_base, rom_path)) {
        std::cout << "  Monitor ROM mapped at $F800-$FFFF" << std::endl;
        bus.set_write_trap_range(rom_base, 0xFFFF, [](uint16_t, uint8_t) { return true; });
        std::cout << "  ROM writes are trapped (read-only region)" << std::endl;
    } else {
        std::cerr << "Error: Failed to load monitor ROM from " << rom_path << std::endl;
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
    TrapManager::install_address_handler(0xBF00, TrapManager::prodos_mli_trap_handler);
    TrapManager::install_address_handler(0xFE84, TrapManager::monitor_setnorm_trap_handler);
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
                      << TrapManager::dump_cpu_state(cpu.state()) << std::endl;
            std::cout << "    " << format_disassembly(bus, cpu.state().PC) << std::endl;
        }

        running = cpu.step();
        count++;

        // Check if HostShims requested a stop (e.g., first screen char is 'E')
        if (shims.should_stop()) {
            std::cout << "\nEmulator stopped by HostShims (first screen char is 'E')" << std::endl;
            running = false;
        }
    }

    std::cout << std::endl;
    std::cout << "Execution stopped after " << std::dec << count << " instructions" << std::endl;
    std::cout << "Final CPU state:" << std::endl;
    std::cout << TrapManager::dump_cpu_state(cpu.state()) << std::endl;

    if (running) {
        std::cout << std::endl << "Reached maximum instruction limit" << std::endl;
        return 1;
    } else {
        std::cout << std::endl << "Halted by trap handler" << std::endl;
        return 0;
    }
}
