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

// Register a symbol for an address (last registration wins)
void register_disassembly_symbol(uint16_t address, std::string name);

// Lookup a symbol for an address (returns nullptr if not found)
const std::string *lookup_disassembly_symbol(uint16_t address);

// Register all built-in address constants as disassembly symbols
void register_default_disassembly_symbols();

} // namespace edasm
