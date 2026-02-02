/**
 * @file disassembly.cpp
 * @brief 6502 instruction disassembler implementation
 * 
 * Provides disassembly of 6502 machine code to assembly mnemonics.
 * Used for debugging and tracing emulator execution.
 */

#include "edasm/emulator/disassembly.hpp"
#include "edasm/constants.hpp"
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

void register_default_disassembly_symbols() {
#define EDASM_REGISTER_SYMBOL(name) register_disassembly_symbol(name, #name)
    // Memory layout symbols
    EDASM_REGISTER_SYMBOL(STACK_BASE);
    EDASM_REGISTER_SYMBOL(INBUF);
    EDASM_REGISTER_SYMBOL(TXBUF2);
    EDASM_REGISTER_SYMBOL(SOFTEV);
    EDASM_REGISTER_SYMBOL(PWREDUP);
    EDASM_REGISTER_SYMBOL(USRADR);
    EDASM_REGISTER_SYMBOL(LOAD_ADDR_SYS);
    EDASM_REGISTER_SYMBOL(LOAD_ADDR_EDITOR);
    EDASM_REGISTER_SYMBOL(LOAD_ADDR_EI);
    EDASM_REGISTER_SYMBOL(TEXT_BUFFER_START);
    EDASM_REGISTER_SYMBOL(TEXT_BUFFER_END);
    EDASM_REGISTER_SYMBOL(IO_BUFFER_1);
    EDASM_REGISTER_SYMBOL(IO_BUFFER_2);
    EDASM_REGISTER_SYMBOL(GLOBAL_PAGE);
    EDASM_REGISTER_SYMBOL(GLOBAL_PAGE_2);
    EDASM_REGISTER_SYMBOL(CURRENT_PATHNAME);
    EDASM_REGISTER_SYMBOL(DEVCTLS);
    EDASM_REGISTER_SYMBOL(TABTABLE);
    EDASM_REGISTER_SYMBOL(DATETIME);
    EDASM_REGISTER_SYMBOL(EDASMDIR);
    EDASM_REGISTER_SYMBOL(PRTERROR);

    // Monitor ROM zero-page hook addresses
    EDASM_REGISTER_SYMBOL(CSWL);
    EDASM_REGISTER_SYMBOL(CSWH);
    EDASM_REGISTER_SYMBOL(KSWL);
    EDASM_REGISTER_SYMBOL(KSWH);

    // ProDOS symbols
    EDASM_REGISTER_SYMBOL(PRODOS8);
    EDASM_REGISTER_SYMBOL(LASTDEV);
    EDASM_REGISTER_SYMBOL(BITMAP);
    EDASM_REGISTER_SYMBOL(P8DATE);
    EDASM_REGISTER_SYMBOL(P8TIME);
    EDASM_REGISTER_SYMBOL(MACHID);
    EDASM_REGISTER_SYMBOL(SLTBYT);
    EDASM_REGISTER_SYMBOL(CMDADR);
    EDASM_REGISTER_SYMBOL(MINIVERS);
    EDASM_REGISTER_SYMBOL(IVERSION);

    // Memory management soft switches (register less common ones first)
    EDASM_REGISTER_SYMBOL(_80STOREOFF); // $C000 (also KBD - will be overwritten)
    EDASM_REGISTER_SYMBOL(_80STOREON);
    EDASM_REGISTER_SYMBOL(RAMRDOFF);
    EDASM_REGISTER_SYMBOL(RAMRDON);
    EDASM_REGISTER_SYMBOL(RAMWRTOFF);
    EDASM_REGISTER_SYMBOL(RAMWRTON);
    EDASM_REGISTER_SYMBOL(INTCXROMOFF);
    EDASM_REGISTER_SYMBOL(INTCXROMON);
    EDASM_REGISTER_SYMBOL(ALTZPOFF);
    EDASM_REGISTER_SYMBOL(ALTZPON);
    EDASM_REGISTER_SYMBOL(SLOTC3ROMOFF);
    EDASM_REGISTER_SYMBOL(SLOTC3ROMON);
    EDASM_REGISTER_SYMBOL(CLRROM);

    // Video control
    EDASM_REGISTER_SYMBOL(_80COLOFF);
    EDASM_REGISTER_SYMBOL(_80COLON);
    EDASM_REGISTER_SYMBOL(ALTCHARSETOFF);
    EDASM_REGISTER_SYMBOL(ALTCHARSETON);

    // Video mode switches
    EDASM_REGISTER_SYMBOL(TEXTOFF);
    EDASM_REGISTER_SYMBOL(TEXTON);
    EDASM_REGISTER_SYMBOL(MIXEDOFF);
    EDASM_REGISTER_SYMBOL(MIXEDON);
    EDASM_REGISTER_SYMBOL(PAGE20FF);
    EDASM_REGISTER_SYMBOL(PAGE20N);
    EDASM_REGISTER_SYMBOL(HIRESOFF);
    EDASM_REGISTER_SYMBOL(HIRESON);

    // Annunciator switches
    EDASM_REGISTER_SYMBOL(CLRAN0);
    EDASM_REGISTER_SYMBOL(SETAN0);
    EDASM_REGISTER_SYMBOL(CLRAN1);
    EDASM_REGISTER_SYMBOL(SETAN1);
    EDASM_REGISTER_SYMBOL(CLRAN2);
    EDASM_REGISTER_SYMBOL(SETAN2);
    EDASM_REGISTER_SYMBOL(CLRAN3);
    EDASM_REGISTER_SYMBOL(SETAN3);

    // Game controllers
    EDASM_REGISTER_SYMBOL(CASSIN);
    EDASM_REGISTER_SYMBOL(PB0);
    EDASM_REGISTER_SYMBOL(PB1);
    EDASM_REGISTER_SYMBOL(PB2);
    EDASM_REGISTER_SYMBOL(GC0);
    EDASM_REGISTER_SYMBOL(GC1);
    EDASM_REGISTER_SYMBOL(GC2);
    EDASM_REGISTER_SYMBOL(GC3);
    EDASM_REGISTER_SYMBOL(GCRESET);

    // Pascal 1.1 Firmware Protocol - Signature bytes for peripheral card slots
    EDASM_REGISTER_SYMBOL(S1PFPGS);
    EDASM_REGISTER_SYMBOL(S1PFPDS);
    EDASM_REGISTER_SYMBOL(S2PFPGS);
    EDASM_REGISTER_SYMBOL(S2PFPDS);
    EDASM_REGISTER_SYMBOL(S3PFPGS);
    EDASM_REGISTER_SYMBOL(S3PFPDS);
    EDASM_REGISTER_SYMBOL(S4PFPGS);
    EDASM_REGISTER_SYMBOL(S4PFPDS);
    EDASM_REGISTER_SYMBOL(S5PFPGS);
    EDASM_REGISTER_SYMBOL(S5PFPDS);
    EDASM_REGISTER_SYMBOL(S6PFPGS);
    EDASM_REGISTER_SYMBOL(S6PFPDS);
    EDASM_REGISTER_SYMBOL(S7PFPGS);
    EDASM_REGISTER_SYMBOL(S7PFPDS);

    // Soft switch status flags
    EDASM_REGISTER_SYMBOL(BSRBANK2);
    EDASM_REGISTER_SYMBOL(BSRREADRAM);
    EDASM_REGISTER_SYMBOL(RAMRD);
    EDASM_REGISTER_SYMBOL(RAMWRT);
    EDASM_REGISTER_SYMBOL(INTCXROM);
    EDASM_REGISTER_SYMBOL(ALTZP);
    EDASM_REGISTER_SYMBOL(SLOTC3ROM);
    EDASM_REGISTER_SYMBOL(_80STORE);
    EDASM_REGISTER_SYMBOL(VERTBLANK);
    EDASM_REGISTER_SYMBOL(TEXT);
    EDASM_REGISTER_SYMBOL(MIXED);
    EDASM_REGISTER_SYMBOL(PAGE2);
    EDASM_REGISTER_SYMBOL(HIRES);
    EDASM_REGISTER_SYMBOL(ALTCHARSET);
    EDASM_REGISTER_SYMBOL(_80COL);

    // Bank-switched RAM control
    EDASM_REGISTER_SYMBOL(READBSR2);
    EDASM_REGISTER_SYMBOL(WRITEBSR2);
    EDASM_REGISTER_SYMBOL(OFFBSR2);
    EDASM_REGISTER_SYMBOL(RDWRBSR2);
    EDASM_REGISTER_SYMBOL(READBSR1);
    EDASM_REGISTER_SYMBOL(WRITEBSR1);
    EDASM_REGISTER_SYMBOL(OFFBSR1);
    EDASM_REGISTER_SYMBOL(RDWRBSR1);

    // Keyboard and device I/O (register after overlapping addresses to take priority)
    EDASM_REGISTER_SYMBOL(KBD);     // $C000 - overwrites _80STOREOFF
    EDASM_REGISTER_SYMBOL(KBDSTRB); // $C010 - overwrites AKD
    EDASM_REGISTER_SYMBOL(CASSOUT);
    EDASM_REGISTER_SYMBOL(SPEAKER);
    EDASM_REGISTER_SYMBOL(GCSTROBE);

    // Monitor entry points
    EDASM_REGISTER_SYMBOL(SWEET16_ROM);
    EDASM_REGISTER_SYMBOL(BELL1);
    EDASM_REGISTER_SYMBOL(HOME);
    EDASM_REGISTER_SYMBOL(RDKEY);
    EDASM_REGISTER_SYMBOL(CROUT);
    EDASM_REGISTER_SYMBOL(COUT);
    EDASM_REGISTER_SYMBOL(MON);
#undef EDASM_REGISTER_SYMBOL
}

} // namespace edasm
