#pragma once

#include "edasm/emulator/bus.hpp"
#include <string>

namespace edasm {

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
extern const OpcodeInfo opcode_table[256];

// Format a single instruction at PC for disassembly output
std::string format_disassembly(const Bus &bus, uint16_t pc);

} // namespace edasm
