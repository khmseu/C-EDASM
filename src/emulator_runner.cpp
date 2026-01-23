#include "edasm/bus.hpp"
#include "edasm/cpu.hpp"
#include "edasm/host_shims.hpp"
#include "edasm/traps.hpp"
#include <algorithm>
#include <filesystem>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>

using namespace edasm;

namespace {

// 6502 Opcode disassembly table - maps opcode byte to mnemonic and addressing mode
struct OpcodeInfo {
    const char *mnemonic;
    int bytes;
    enum {
        Implied,
        Accumulator,
        Immediate,
        ZeroPage,
        ZeroPageX,
        ZeroPageY,
        Absolute,
        AbsoluteX,
        AbsoluteY,
        Indirect,
        IndexedIndirect,
        IndirectIndexed,
        Relative
    } mode;
};

// Complete 6502 opcode table (256 entries)
static const OpcodeInfo opcode_table[256] = {
    // 0x00-0x0F
    {"BRK", 1, OpcodeInfo::Implied},
    {"ORA", 2, OpcodeInfo::IndexedIndirect},
    {"CALL_TRAP", 1, OpcodeInfo::Implied},
    {"???", 1, OpcodeInfo::Implied},
    {"???", 1, OpcodeInfo::Implied},
    {"ORA", 2, OpcodeInfo::ZeroPage},
    {"ASL", 2, OpcodeInfo::ZeroPage},
    {"???", 1, OpcodeInfo::Implied},
    {"PHP", 1, OpcodeInfo::Implied},
    {"ORA", 2, OpcodeInfo::Immediate},
    {"ASL", 1, OpcodeInfo::Accumulator},
    {"???", 1, OpcodeInfo::Implied},
    {"???", 1, OpcodeInfo::Implied},
    {"ORA", 3, OpcodeInfo::Absolute},
    {"ASL", 3, OpcodeInfo::Absolute},
    {"???", 1, OpcodeInfo::Implied},
    // 0x10-0x1F
    {"BPL", 2, OpcodeInfo::Relative},
    {"ORA", 2, OpcodeInfo::IndirectIndexed},
    {"???", 1, OpcodeInfo::Implied},
    {"???", 1, OpcodeInfo::Implied},
    {"???", 1, OpcodeInfo::Implied},
    {"ORA", 2, OpcodeInfo::ZeroPageX},
    {"ASL", 2, OpcodeInfo::ZeroPageX},
    {"???", 1, OpcodeInfo::Implied},
    {"CLC", 1, OpcodeInfo::Implied},
    {"ORA", 3, OpcodeInfo::AbsoluteY},
    {"???", 1, OpcodeInfo::Implied},
    {"???", 1, OpcodeInfo::Implied},
    {"???", 1, OpcodeInfo::Implied},
    {"ORA", 3, OpcodeInfo::AbsoluteX},
    {"ASL", 3, OpcodeInfo::AbsoluteX},
    {"???", 1, OpcodeInfo::Implied},
    // 0x20-0x2F
    {"JSR", 3, OpcodeInfo::Absolute},
    {"AND", 2, OpcodeInfo::IndexedIndirect},
    {"???", 1, OpcodeInfo::Implied},
    {"???", 1, OpcodeInfo::Implied},
    {"BIT", 2, OpcodeInfo::ZeroPage},
    {"AND", 2, OpcodeInfo::ZeroPage},
    {"ROL", 2, OpcodeInfo::ZeroPage},
    {"???", 1, OpcodeInfo::Implied},
    {"PLP", 1, OpcodeInfo::Implied},
    {"AND", 2, OpcodeInfo::Immediate},
    {"ROL", 1, OpcodeInfo::Accumulator},
    {"???", 1, OpcodeInfo::Implied},
    {"BIT", 3, OpcodeInfo::Absolute},
    {"AND", 3, OpcodeInfo::Absolute},
    {"ROL", 3, OpcodeInfo::Absolute},
    {"???", 1, OpcodeInfo::Implied},
    // 0x30-0x3F
    {"BMI", 2, OpcodeInfo::Relative},
    {"AND", 2, OpcodeInfo::IndirectIndexed},
    {"???", 1, OpcodeInfo::Implied},
    {"???", 1, OpcodeInfo::Implied},
    {"???", 1, OpcodeInfo::Implied},
    {"AND", 2, OpcodeInfo::ZeroPageX},
    {"ROL", 2, OpcodeInfo::ZeroPageX},
    {"???", 1, OpcodeInfo::Implied},
    {"SEC", 1, OpcodeInfo::Implied},
    {"AND", 3, OpcodeInfo::AbsoluteY},
    {"???", 1, OpcodeInfo::Implied},
    {"???", 1, OpcodeInfo::Implied},
    {"???", 1, OpcodeInfo::Implied},
    {"AND", 3, OpcodeInfo::AbsoluteX},
    {"ROL", 3, OpcodeInfo::AbsoluteX},
    {"???", 1, OpcodeInfo::Implied},
    // 0x40-0x4F
    {"RTI", 1, OpcodeInfo::Implied},
    {"EOR", 2, OpcodeInfo::IndexedIndirect},
    {"???", 1, OpcodeInfo::Implied},
    {"???", 1, OpcodeInfo::Implied},
    {"???", 1, OpcodeInfo::Implied},
    {"EOR", 2, OpcodeInfo::ZeroPage},
    {"LSR", 2, OpcodeInfo::ZeroPage},
    {"???", 1, OpcodeInfo::Implied},
    {"PHA", 1, OpcodeInfo::Implied},
    {"EOR", 2, OpcodeInfo::Immediate},
    {"LSR", 1, OpcodeInfo::Accumulator},
    {"???", 1, OpcodeInfo::Implied},
    {"JMP", 3, OpcodeInfo::Absolute},
    {"EOR", 3, OpcodeInfo::Absolute},
    {"LSR", 3, OpcodeInfo::Absolute},
    {"???", 1, OpcodeInfo::Implied},
    // 0x50-0x5F
    {"BVC", 2, OpcodeInfo::Relative},
    {"EOR", 2, OpcodeInfo::IndirectIndexed},
    {"???", 1, OpcodeInfo::Implied},
    {"???", 1, OpcodeInfo::Implied},
    {"???", 1, OpcodeInfo::Implied},
    {"EOR", 2, OpcodeInfo::ZeroPageX},
    {"LSR", 2, OpcodeInfo::ZeroPageX},
    {"???", 1, OpcodeInfo::Implied},
    {"CLI", 1, OpcodeInfo::Implied},
    {"EOR", 3, OpcodeInfo::AbsoluteY},
    {"???", 1, OpcodeInfo::Implied},
    {"???", 1, OpcodeInfo::Implied},
    {"???", 1, OpcodeInfo::Implied},
    {"EOR", 3, OpcodeInfo::AbsoluteX},
    {"LSR", 3, OpcodeInfo::AbsoluteX},
    {"???", 1, OpcodeInfo::Implied},
    // 0x60-0x6F
    {"RTS", 1, OpcodeInfo::Implied},
    {"ADC", 2, OpcodeInfo::IndexedIndirect},
    {"???", 1, OpcodeInfo::Implied},
    {"???", 1, OpcodeInfo::Implied},
    {"???", 1, OpcodeInfo::Implied},
    {"ADC", 2, OpcodeInfo::ZeroPage},
    {"ROR", 2, OpcodeInfo::ZeroPage},
    {"???", 1, OpcodeInfo::Implied},
    {"PLA", 1, OpcodeInfo::Implied},
    {"ADC", 2, OpcodeInfo::Immediate},
    {"ROR", 1, OpcodeInfo::Accumulator},
    {"???", 1, OpcodeInfo::Implied},
    {"JMP", 3, OpcodeInfo::Indirect},
    {"ADC", 3, OpcodeInfo::Absolute},
    {"ROR", 3, OpcodeInfo::Absolute},
    {"???", 1, OpcodeInfo::Implied},
    // 0x70-0x7F
    {"BVS", 2, OpcodeInfo::Relative},
    {"ADC", 2, OpcodeInfo::IndirectIndexed},
    {"???", 1, OpcodeInfo::Implied},
    {"???", 1, OpcodeInfo::Implied},
    {"???", 1, OpcodeInfo::Implied},
    {"ADC", 2, OpcodeInfo::ZeroPageX},
    {"ROR", 2, OpcodeInfo::ZeroPageX},
    {"???", 1, OpcodeInfo::Implied},
    {"SEI", 1, OpcodeInfo::Implied},
    {"ADC", 3, OpcodeInfo::AbsoluteY},
    {"???", 1, OpcodeInfo::Implied},
    {"???", 1, OpcodeInfo::Implied},
    {"???", 1, OpcodeInfo::Implied},
    {"ADC", 3, OpcodeInfo::AbsoluteX},
    {"ROR", 3, OpcodeInfo::AbsoluteX},
    {"???", 1, OpcodeInfo::Implied},
    // 0x80-0x8F
    {"???", 1, OpcodeInfo::Implied},
    {"STA", 2, OpcodeInfo::IndexedIndirect},
    {"???", 1, OpcodeInfo::Implied},
    {"???", 1, OpcodeInfo::Implied},
    {"STY", 2, OpcodeInfo::ZeroPage},
    {"STA", 2, OpcodeInfo::ZeroPage},
    {"STX", 2, OpcodeInfo::ZeroPage},
    {"???", 1, OpcodeInfo::Implied},
    {"DEY", 1, OpcodeInfo::Implied},
    {"???", 1, OpcodeInfo::Implied},
    {"TXA", 1, OpcodeInfo::Implied},
    {"???", 1, OpcodeInfo::Implied},
    {"STY", 3, OpcodeInfo::Absolute},
    {"STA", 3, OpcodeInfo::Absolute},
    {"STX", 3, OpcodeInfo::Absolute},
    {"???", 1, OpcodeInfo::Implied},
    // 0x90-0x9F
    {"BCC", 2, OpcodeInfo::Relative},
    {"STA", 2, OpcodeInfo::IndirectIndexed},
    {"???", 1, OpcodeInfo::Implied},
    {"???", 1, OpcodeInfo::Implied},
    {"STY", 2, OpcodeInfo::ZeroPageX},
    {"STA", 2, OpcodeInfo::ZeroPageX},
    {"STX", 2, OpcodeInfo::ZeroPageY},
    {"???", 1, OpcodeInfo::Implied},
    {"TYA", 1, OpcodeInfo::Implied},
    {"STA", 3, OpcodeInfo::AbsoluteY},
    {"TXS", 1, OpcodeInfo::Implied},
    {"???", 1, OpcodeInfo::Implied},
    {"???", 1, OpcodeInfo::Implied},
    {"STA", 3, OpcodeInfo::AbsoluteX},
    {"???", 1, OpcodeInfo::Implied},
    {"???", 1, OpcodeInfo::Implied},
    // 0xA0-0xAF
    {"LDY", 2, OpcodeInfo::Immediate},
    {"LDA", 2, OpcodeInfo::IndexedIndirect},
    {"LDX", 2, OpcodeInfo::Immediate},
    {"???", 1, OpcodeInfo::Implied},
    {"LDY", 2, OpcodeInfo::ZeroPage},
    {"LDA", 2, OpcodeInfo::ZeroPage},
    {"LDX", 2, OpcodeInfo::ZeroPage},
    {"???", 1, OpcodeInfo::Implied},
    {"TAY", 1, OpcodeInfo::Implied},
    {"LDA", 2, OpcodeInfo::Immediate},
    {"TAX", 1, OpcodeInfo::Implied},
    {"???", 1, OpcodeInfo::Implied},
    {"LDY", 3, OpcodeInfo::Absolute},
    {"LDA", 3, OpcodeInfo::Absolute},
    {"LDX", 3, OpcodeInfo::Absolute},
    {"???", 1, OpcodeInfo::Implied},
    // 0xB0-0xBF
    {"BCS", 2, OpcodeInfo::Relative},
    {"LDA", 2, OpcodeInfo::IndirectIndexed},
    {"???", 1, OpcodeInfo::Implied},
    {"???", 1, OpcodeInfo::Implied},
    {"LDY", 2, OpcodeInfo::ZeroPageX},
    {"LDA", 2, OpcodeInfo::ZeroPageX},
    {"LDX", 2, OpcodeInfo::ZeroPageY},
    {"???", 1, OpcodeInfo::Implied},
    {"CLV", 1, OpcodeInfo::Implied},
    {"LDA", 3, OpcodeInfo::AbsoluteY},
    {"TSX", 1, OpcodeInfo::Implied},
    {"???", 1, OpcodeInfo::Implied},
    {"LDY", 3, OpcodeInfo::AbsoluteX},
    {"LDA", 3, OpcodeInfo::AbsoluteX},
    {"LDX", 3, OpcodeInfo::AbsoluteY},
    {"???", 1, OpcodeInfo::Implied},
    // 0xC0-0xCF
    {"CPY", 2, OpcodeInfo::Immediate},
    {"CMP", 2, OpcodeInfo::IndexedIndirect},
    {"???", 1, OpcodeInfo::Implied},
    {"???", 1, OpcodeInfo::Implied},
    {"CPY", 2, OpcodeInfo::ZeroPage},
    {"CMP", 2, OpcodeInfo::ZeroPage},
    {"DEC", 2, OpcodeInfo::ZeroPage},
    {"???", 1, OpcodeInfo::Implied},
    {"INY", 1, OpcodeInfo::Implied},
    {"CMP", 2, OpcodeInfo::Immediate},
    {"DEX", 1, OpcodeInfo::Implied},
    {"???", 1, OpcodeInfo::Implied},
    {"CPY", 3, OpcodeInfo::Absolute},
    {"CMP", 3, OpcodeInfo::Absolute},
    {"DEC", 3, OpcodeInfo::Absolute},
    {"???", 1, OpcodeInfo::Implied},
    // 0xD0-0xDF
    {"BNE", 2, OpcodeInfo::Relative},
    {"CMP", 2, OpcodeInfo::IndirectIndexed},
    {"???", 1, OpcodeInfo::Implied},
    {"???", 1, OpcodeInfo::Implied},
    {"???", 1, OpcodeInfo::Implied},
    {"CMP", 2, OpcodeInfo::ZeroPageX},
    {"DEC", 2, OpcodeInfo::ZeroPageX},
    {"???", 1, OpcodeInfo::Implied},
    {"CLD", 1, OpcodeInfo::Implied},
    {"CMP", 3, OpcodeInfo::AbsoluteY},
    {"???", 1, OpcodeInfo::Implied},
    {"???", 1, OpcodeInfo::Implied},
    {"???", 1, OpcodeInfo::Implied},
    {"CMP", 3, OpcodeInfo::AbsoluteX},
    {"DEC", 3, OpcodeInfo::AbsoluteX},
    {"???", 1, OpcodeInfo::Implied},
    // 0xE0-0xEF
    {"CPX", 2, OpcodeInfo::Immediate},
    {"SBC", 2, OpcodeInfo::IndexedIndirect},
    {"???", 1, OpcodeInfo::Implied},
    {"???", 1, OpcodeInfo::Implied},
    {"CPX", 2, OpcodeInfo::ZeroPage},
    {"SBC", 2, OpcodeInfo::ZeroPage},
    {"INC", 2, OpcodeInfo::ZeroPage},
    {"???", 1, OpcodeInfo::Implied},
    {"INX", 1, OpcodeInfo::Implied},
    {"SBC", 2, OpcodeInfo::Immediate},
    {"NOP", 1, OpcodeInfo::Implied},
    {"???", 1, OpcodeInfo::Implied},
    {"CPX", 3, OpcodeInfo::Absolute},
    {"SBC", 3, OpcodeInfo::Absolute},
    {"INC", 3, OpcodeInfo::Absolute},
    {"???", 1, OpcodeInfo::Implied},
    // 0xF0-0xFF
    {"BEQ", 2, OpcodeInfo::Relative},
    {"SBC", 2, OpcodeInfo::IndirectIndexed},
    {"???", 1, OpcodeInfo::Implied},
    {"???", 1, OpcodeInfo::Implied},
    {"???", 1, OpcodeInfo::Implied},
    {"SBC", 2, OpcodeInfo::ZeroPageX},
    {"INC", 2, OpcodeInfo::ZeroPageX},
    {"???", 1, OpcodeInfo::Implied},
    {"SED", 1, OpcodeInfo::Implied},
    {"SBC", 3, OpcodeInfo::AbsoluteY},
    {"???", 1, OpcodeInfo::Implied},
    {"???", 1, OpcodeInfo::Implied},
    {"???", 1, OpcodeInfo::Implied},
    {"SBC", 3, OpcodeInfo::AbsoluteX},
    {"INC", 3, OpcodeInfo::AbsoluteX},
    {"???", 1, OpcodeInfo::Implied},
};

std::string format_disassembly(const Bus &bus, uint16_t pc) {
    const uint8_t *mem = bus.data();
    uint8_t opcode = mem[pc];
    const OpcodeInfo &info = opcode_table[opcode];

    std::ostringstream oss;
    oss << std::hex << std::uppercase << std::setfill('0');

    // Format: $ADDR: BYTES         MNEMONIC OPERAND
    oss << "$" << std::setw(4) << pc << ": ";

    // Output opcode bytes (padded to 9 characters for alignment)
    std::string bytes_str;
    for (int i = 0; i < info.bytes; ++i) {
        if (i > 0)
            bytes_str += " ";
        char buf[3];
        snprintf(buf, sizeof(buf), "%02X", mem[pc + i]);
        bytes_str += buf;
    }
    // Pad bytes string to 9 characters
    while (bytes_str.length() < 9) {
        bytes_str += " ";
    }
    oss << bytes_str << " ";

    // Output mnemonic (fixed width for alignment, space-padded)
    oss << std::setfill(' ') << std::left << std::setw(4) << info.mnemonic << std::right
        << std::setfill('0');

    // Format operand based on addressing mode
    if (info.bytes > 1) {
        uint8_t arg1 = mem[pc + 1];
        uint8_t arg2 = (info.bytes > 2) ? mem[pc + 2] : 0;
        uint16_t addr = arg1 | (arg2 << 8);

        switch (info.mode) {
        case OpcodeInfo::Immediate:
            oss << "#$" << std::setw(2) << static_cast<int>(arg1);
            break;
        case OpcodeInfo::ZeroPage:
            oss << "$" << std::setw(2) << static_cast<int>(arg1);
            break;
        case OpcodeInfo::ZeroPageX:
            oss << "$" << std::setw(2) << static_cast<int>(arg1) << ",X";
            break;
        case OpcodeInfo::ZeroPageY:
            oss << "$" << std::setw(2) << static_cast<int>(arg1) << ",Y";
            break;
        case OpcodeInfo::Absolute:
            oss << "$" << std::setw(4) << addr;
            break;
        case OpcodeInfo::AbsoluteX:
            oss << "$" << std::setw(4) << addr << ",X";
            break;
        case OpcodeInfo::AbsoluteY:
            oss << "$" << std::setw(4) << addr << ",Y";
            break;
        case OpcodeInfo::Indirect:
            oss << "($" << std::setw(4) << addr << ")";
            break;
        case OpcodeInfo::IndexedIndirect:
            oss << "($" << std::setw(2) << static_cast<int>(arg1) << ",X)";
            break;
        case OpcodeInfo::IndirectIndexed:
            oss << "($" << std::setw(2) << static_cast<int>(arg1) << "),Y";
            break;
        case OpcodeInfo::Relative: {
            // Calculate target address for branch
            int8_t offset = static_cast<int8_t>(arg1);
            uint16_t target = static_cast<uint16_t>(pc + 2 + offset);
            oss << "$" << std::setw(4) << target;
            break;
        }
        default:
            break;
        }
    }

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
        bus.set_write_trap_range(rom_base, 0xFFFF, [](uint16_t, uint8_t) { return true; });
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
                      << TrapManager::dump_cpu_state(cpu.state());
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
