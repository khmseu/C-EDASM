#include "edasm/emulator/disassembly.hpp"
#include <cstring>
#include <iomanip>
#include <sstream>
#include <unordered_map>

namespace edasm {

namespace {

std::unordered_map<uint16_t, std::string> &symbol_table() {
    static std::unordered_map<uint16_t, std::string> table;
    return table;
}

void append_symbol(std::ostringstream &oss, uint16_t address) {
    const auto &table = symbol_table();
    auto it = table.find(address);
    if (it == table.end()) {
        return;
    }
    oss << " <" << it->second << ">";
}

} // namespace

// Complete 6502 opcode table (256 entries)
const OpcodeInfo opcode_table[256] = {
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
            append_symbol(oss, static_cast<uint16_t>(arg1));
            break;
        case OpcodeInfo::ZeroPageX:
            oss << "$" << std::setw(2) << static_cast<int>(arg1) << ",X";
            append_symbol(oss, static_cast<uint16_t>(arg1));
            break;
        case OpcodeInfo::ZeroPageY:
            oss << "$" << std::setw(2) << static_cast<int>(arg1) << ",Y";
            append_symbol(oss, static_cast<uint16_t>(arg1));
            break;
        case OpcodeInfo::Absolute:
            oss << "$" << std::setw(4) << addr;
            append_symbol(oss, addr);
            break;
        case OpcodeInfo::AbsoluteX:
            oss << "$" << std::setw(4) << addr << ",X";
            append_symbol(oss, addr);
            break;
        case OpcodeInfo::AbsoluteY:
            oss << "$" << std::setw(4) << addr << ",Y";
            append_symbol(oss, addr);
            break;
        case OpcodeInfo::Indirect:
            oss << "($" << std::setw(4) << addr << ")";
            append_symbol(oss, addr);
            break;
        case OpcodeInfo::IndexedIndirect:
            oss << "($" << std::setw(2) << static_cast<int>(arg1) << ",X)";
            append_symbol(oss, static_cast<uint16_t>(arg1));
            break;
        case OpcodeInfo::IndirectIndexed:
            oss << "($" << std::setw(2) << static_cast<int>(arg1) << "),Y";
            append_symbol(oss, static_cast<uint16_t>(arg1));
            break;
        case OpcodeInfo::Relative: {
            // Calculate target address for branch
            int8_t offset = static_cast<int8_t>(arg1);
            uint16_t target = static_cast<uint16_t>(pc + 2 + offset);
            oss << "$" << std::setw(4) << target;
            append_symbol(oss, target);
            break;
        }
        default:
            break;
        }
    } else if (std::strcmp(info.mnemonic, "CALL_TRAP") == 0) {
        append_symbol(oss, pc);
    }

    return oss.str();
}

void register_disassembly_symbol(uint16_t address, std::string name) {
    symbol_table()[address] = std::move(name);
}

const std::string *lookup_disassembly_symbol(uint16_t address) {
    const auto &table = symbol_table();
    auto it = table.find(address);
    if (it == table.end()) {
        return nullptr;
    }
    return &it->second;
}

} // namespace edasm
