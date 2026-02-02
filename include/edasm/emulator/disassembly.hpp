/**
 * @file disassembly.hpp
 * @brief 6502 instruction disassembler
 *
 * Provides disassembly of 6502 machine code to assembly mnemonics.
 * Used for debugging and tracing emulator execution.
 */

#pragma once

#include "edasm/emulator/bus.hpp"
#include <string>

namespace edasm {

/**
 * @brief 6502 opcode information for disassembly
 *
 * Contains mnemonic, byte count, and addressing mode for each opcode.
 */
struct OpcodeInfo {
    const char *mnemonic; ///< Instruction mnemonic
    int bytes;            ///< Instruction length in bytes

    /**
     * @brief Addressing mode enumeration
     */
    enum {
        Implied,         ///< No operand
        Accumulator,     ///< Accumulator
        Immediate,       ///< Immediate value
        ZeroPage,        ///< Zero page address
        ZeroPageX,       ///< Zero page indexed by X
        ZeroPageY,       ///< Zero page indexed by Y
        Absolute,        ///< Absolute address
        AbsoluteX,       ///< Absolute indexed by X
        AbsoluteY,       ///< Absolute indexed by Y
        Indirect,        ///< Indirect
        IndexedIndirect, ///< Indexed indirect (X)
        IndirectIndexed, ///< Indirect indexed (Y)
        Relative         ///< Relative branch
    } mode;              ///< Addressing mode
};

/**
 * @brief Complete 6502 opcode table
 *
 * Array of 256 entries, one per possible opcode byte.
 */
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
