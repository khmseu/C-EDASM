/**
 * @file opcode_table.cpp
 * @brief Opcode table implementation for EDASM assembler
 * 
 * Implements 6502/65C02 instruction encoding from EDASM.SRC/ASM/.
 * 
 * Primary reference: ASM1.S OpcodeT ($D835-$D909) - Opcode tables by addressing mode
 * 
 * Key data structures from ASM1.S:
 * - OpcodeT ($D835): Master opcode table organized by addressing mode
 * - CycTimes ($D90A): CPU cycle timing table (not implemented in C++)
 * - ModWrdL/ModWrdH: Addressing mode flag bytes (implemented in C++ as enums)
 * 
 * Original EDASM has 13 addressing modes stored in mode-specific tables.
 * C++ implementation uses a map-based approach with addressing mode detection.
 */

#include "edasm/assembler/opcode_table.hpp"

#include <algorithm>

namespace edasm {

OpcodeTable::OpcodeTable() {
    // Initialize all 6502 opcodes from 6502_INSTRUCTION_SET.md
    init_load_store();
    init_arithmetic();
    init_increment_decrement();
    init_logical();
    init_shift_rotate();
    init_compare();
    init_branch();
    init_jump();
    init_transfer();
    init_stack();
    init_flags();
    init_system();
}

void OpcodeTable::add(const std::string &mnem, AddressingMode mode, uint8_t code, int bytes,
                      int cycles, bool page_cross) {
    Opcode op;
    op.mnemonic = mnem;
    op.mode = mode;
    op.code = code;
    op.bytes = bytes;
    op.cycles = cycles;
    op.extra_cycle_on_page_cross = page_cross;
    table_[mnem][mode] = op;
}

const Opcode *OpcodeTable::lookup(const std::string &mnemonic, AddressingMode mode) const {
    auto mnem_it = table_.find(mnemonic);
    if (mnem_it == table_.end()) {
        return nullptr;
    }

    auto mode_it = mnem_it->second.find(mode);
    if (mode_it == mnem_it->second.end()) {
        return nullptr;
    }

    return &mode_it->second;
}

std::vector<AddressingMode> OpcodeTable::valid_modes(const std::string &mnemonic) const {
    std::vector<AddressingMode> modes;
    auto it = table_.find(mnemonic);
    if (it == table_.end()) {
        return modes;
    }

    for (const auto &[mode, opcode] : it->second) {
        modes.push_back(mode);
    }
    return modes;
}

bool OpcodeTable::is_valid_mnemonic(const std::string &mnemonic) const {
    return table_.find(mnemonic) != table_.end();
}

// =========================================
// Opcode Initialization (from 6502_INSTRUCTION_SET.md)
// =========================================

void OpcodeTable::init_load_store() {
    // LDA - Load Accumulator
    add("LDA", AddressingMode::Immediate, 0xA9, 2, 2);
    add("LDA", AddressingMode::ZeroPage, 0xA5, 2, 3);
    add("LDA", AddressingMode::ZeroPageX, 0xB5, 2, 4);
    add("LDA", AddressingMode::Absolute, 0xAD, 3, 4);
    add("LDA", AddressingMode::AbsoluteX, 0xBD, 3, 4, true);
    add("LDA", AddressingMode::AbsoluteY, 0xB9, 3, 4, true);
    add("LDA", AddressingMode::IndexedIndirect, 0xA1, 2, 6);
    add("LDA", AddressingMode::IndirectIndexed, 0xB1, 2, 5, true);

    // LDX - Load X
    add("LDX", AddressingMode::Immediate, 0xA2, 2, 2);
    add("LDX", AddressingMode::ZeroPage, 0xA6, 2, 3);
    add("LDX", AddressingMode::ZeroPageY, 0xB6, 2, 4);
    add("LDX", AddressingMode::Absolute, 0xAE, 3, 4);
    add("LDX", AddressingMode::AbsoluteY, 0xBE, 3, 4, true);

    // LDY - Load Y
    add("LDY", AddressingMode::Immediate, 0xA0, 2, 2);
    add("LDY", AddressingMode::ZeroPage, 0xA4, 2, 3);
    add("LDY", AddressingMode::ZeroPageX, 0xB4, 2, 4);
    add("LDY", AddressingMode::Absolute, 0xAC, 3, 4);
    add("LDY", AddressingMode::AbsoluteX, 0xBC, 3, 4, true);

    // STA - Store Accumulator
    add("STA", AddressingMode::ZeroPage, 0x85, 2, 3);
    add("STA", AddressingMode::ZeroPageX, 0x95, 2, 4);
    add("STA", AddressingMode::Absolute, 0x8D, 3, 4);
    add("STA", AddressingMode::AbsoluteX, 0x9D, 3, 5);
    add("STA", AddressingMode::AbsoluteY, 0x99, 3, 5);
    add("STA", AddressingMode::IndexedIndirect, 0x81, 2, 6);
    add("STA", AddressingMode::IndirectIndexed, 0x91, 2, 6);

    // STX - Store X
    add("STX", AddressingMode::ZeroPage, 0x86, 2, 3);
    add("STX", AddressingMode::ZeroPageY, 0x96, 2, 4);
    add("STX", AddressingMode::Absolute, 0x8E, 3, 4);

    // STY - Store Y
    add("STY", AddressingMode::ZeroPage, 0x84, 2, 3);
    add("STY", AddressingMode::ZeroPageX, 0x94, 2, 4);
    add("STY", AddressingMode::Absolute, 0x8C, 3, 4);
}

void OpcodeTable::init_arithmetic() {
    // ADC - Add with Carry
    add("ADC", AddressingMode::Immediate, 0x69, 2, 2);
    add("ADC", AddressingMode::ZeroPage, 0x65, 2, 3);
    add("ADC", AddressingMode::ZeroPageX, 0x75, 2, 4);
    add("ADC", AddressingMode::Absolute, 0x6D, 3, 4);
    add("ADC", AddressingMode::AbsoluteX, 0x7D, 3, 4, true);
    add("ADC", AddressingMode::AbsoluteY, 0x79, 3, 4, true);
    add("ADC", AddressingMode::IndexedIndirect, 0x61, 2, 6);
    add("ADC", AddressingMode::IndirectIndexed, 0x71, 2, 5, true);

    // SBC - Subtract with Carry
    add("SBC", AddressingMode::Immediate, 0xE9, 2, 2);
    add("SBC", AddressingMode::ZeroPage, 0xE5, 2, 3);
    add("SBC", AddressingMode::ZeroPageX, 0xF5, 2, 4);
    add("SBC", AddressingMode::Absolute, 0xED, 3, 4);
    add("SBC", AddressingMode::AbsoluteX, 0xFD, 3, 4, true);
    add("SBC", AddressingMode::AbsoluteY, 0xF9, 3, 4, true);
    add("SBC", AddressingMode::IndexedIndirect, 0xE1, 2, 6);
    add("SBC", AddressingMode::IndirectIndexed, 0xF1, 2, 5, true);
}

void OpcodeTable::init_increment_decrement() {
    // INC - Increment Memory
    add("INC", AddressingMode::ZeroPage, 0xE6, 2, 5);
    add("INC", AddressingMode::ZeroPageX, 0xF6, 2, 6);
    add("INC", AddressingMode::Absolute, 0xEE, 3, 6);
    add("INC", AddressingMode::AbsoluteX, 0xFE, 3, 7);

    // DEC - Decrement Memory
    add("DEC", AddressingMode::ZeroPage, 0xC6, 2, 5);
    add("DEC", AddressingMode::ZeroPageX, 0xD6, 2, 6);
    add("DEC", AddressingMode::Absolute, 0xCE, 3, 6);
    add("DEC", AddressingMode::AbsoluteX, 0xDE, 3, 7);

    // Register increment/decrement
    add("INX", AddressingMode::Implied, 0xE8, 1, 2);
    add("DEX", AddressingMode::Implied, 0xCA, 1, 2);
    add("INY", AddressingMode::Implied, 0xC8, 1, 2);
    add("DEY", AddressingMode::Implied, 0x88, 1, 2);
}

void OpcodeTable::init_logical() {
    // AND - Logical AND
    add("AND", AddressingMode::Immediate, 0x29, 2, 2);
    add("AND", AddressingMode::ZeroPage, 0x25, 2, 3);
    add("AND", AddressingMode::ZeroPageX, 0x35, 2, 4);
    add("AND", AddressingMode::Absolute, 0x2D, 3, 4);
    add("AND", AddressingMode::AbsoluteX, 0x3D, 3, 4, true);
    add("AND", AddressingMode::AbsoluteY, 0x39, 3, 4, true);
    add("AND", AddressingMode::IndexedIndirect, 0x21, 2, 6);
    add("AND", AddressingMode::IndirectIndexed, 0x31, 2, 5, true);

    // ORA - Logical OR
    add("ORA", AddressingMode::Immediate, 0x09, 2, 2);
    add("ORA", AddressingMode::ZeroPage, 0x05, 2, 3);
    add("ORA", AddressingMode::ZeroPageX, 0x15, 2, 4);
    add("ORA", AddressingMode::Absolute, 0x0D, 3, 4);
    add("ORA", AddressingMode::AbsoluteX, 0x1D, 3, 4, true);
    add("ORA", AddressingMode::AbsoluteY, 0x19, 3, 4, true);
    add("ORA", AddressingMode::IndexedIndirect, 0x01, 2, 6);
    add("ORA", AddressingMode::IndirectIndexed, 0x11, 2, 5, true);

    // EOR - Exclusive OR
    add("EOR", AddressingMode::Immediate, 0x49, 2, 2);
    add("EOR", AddressingMode::ZeroPage, 0x45, 2, 3);
    add("EOR", AddressingMode::ZeroPageX, 0x55, 2, 4);
    add("EOR", AddressingMode::Absolute, 0x4D, 3, 4);
    add("EOR", AddressingMode::AbsoluteX, 0x5D, 3, 4, true);
    add("EOR", AddressingMode::AbsoluteY, 0x59, 3, 4, true);
    add("EOR", AddressingMode::IndexedIndirect, 0x41, 2, 6);
    add("EOR", AddressingMode::IndirectIndexed, 0x51, 2, 5, true);
}

void OpcodeTable::init_shift_rotate() {
    // ASL - Arithmetic Shift Left
    add("ASL", AddressingMode::Accumulator, 0x0A, 1, 2);
    add("ASL", AddressingMode::ZeroPage, 0x06, 2, 5);
    add("ASL", AddressingMode::ZeroPageX, 0x16, 2, 6);
    add("ASL", AddressingMode::Absolute, 0x0E, 3, 6);
    add("ASL", AddressingMode::AbsoluteX, 0x1E, 3, 7);

    // LSR - Logical Shift Right
    add("LSR", AddressingMode::Accumulator, 0x4A, 1, 2);
    add("LSR", AddressingMode::ZeroPage, 0x46, 2, 5);
    add("LSR", AddressingMode::ZeroPageX, 0x56, 2, 6);
    add("LSR", AddressingMode::Absolute, 0x4E, 3, 6);
    add("LSR", AddressingMode::AbsoluteX, 0x5E, 3, 7);

    // ROL - Rotate Left
    add("ROL", AddressingMode::Accumulator, 0x2A, 1, 2);
    add("ROL", AddressingMode::ZeroPage, 0x26, 2, 5);
    add("ROL", AddressingMode::ZeroPageX, 0x36, 2, 6);
    add("ROL", AddressingMode::Absolute, 0x2E, 3, 6);
    add("ROL", AddressingMode::AbsoluteX, 0x3E, 3, 7);

    // ROR - Rotate Right
    add("ROR", AddressingMode::Accumulator, 0x6A, 1, 2);
    add("ROR", AddressingMode::ZeroPage, 0x66, 2, 5);
    add("ROR", AddressingMode::ZeroPageX, 0x76, 2, 6);
    add("ROR", AddressingMode::Absolute, 0x6E, 3, 6);
    add("ROR", AddressingMode::AbsoluteX, 0x7E, 3, 7);
}

void OpcodeTable::init_compare() {
    // CMP - Compare Accumulator
    add("CMP", AddressingMode::Immediate, 0xC9, 2, 2);
    add("CMP", AddressingMode::ZeroPage, 0xC5, 2, 3);
    add("CMP", AddressingMode::ZeroPageX, 0xD5, 2, 4);
    add("CMP", AddressingMode::Absolute, 0xCD, 3, 4);
    add("CMP", AddressingMode::AbsoluteX, 0xDD, 3, 4, true);
    add("CMP", AddressingMode::AbsoluteY, 0xD9, 3, 4, true);
    add("CMP", AddressingMode::IndexedIndirect, 0xC1, 2, 6);
    add("CMP", AddressingMode::IndirectIndexed, 0xD1, 2, 5, true);

    // CPX - Compare X
    add("CPX", AddressingMode::Immediate, 0xE0, 2, 2);
    add("CPX", AddressingMode::ZeroPage, 0xE4, 2, 3);
    add("CPX", AddressingMode::Absolute, 0xEC, 3, 4);

    // CPY - Compare Y
    add("CPY", AddressingMode::Immediate, 0xC0, 2, 2);
    add("CPY", AddressingMode::ZeroPage, 0xC4, 2, 3);
    add("CPY", AddressingMode::Absolute, 0xCC, 3, 4);

    // BIT - Bit Test
    add("BIT", AddressingMode::ZeroPage, 0x24, 2, 3);
    add("BIT", AddressingMode::Absolute, 0x2C, 3, 4);
}

void OpcodeTable::init_branch() {
    // All branch instructions use relative addressing
    add("BCC", AddressingMode::Relative, 0x90, 2, 2, true);
    add("BCS", AddressingMode::Relative, 0xB0, 2, 2, true);
    add("BEQ", AddressingMode::Relative, 0xF0, 2, 2, true);
    add("BNE", AddressingMode::Relative, 0xD0, 2, 2, true);
    add("BMI", AddressingMode::Relative, 0x30, 2, 2, true);
    add("BPL", AddressingMode::Relative, 0x10, 2, 2, true);
    add("BVC", AddressingMode::Relative, 0x50, 2, 2, true);
    add("BVS", AddressingMode::Relative, 0x70, 2, 2, true);
}

void OpcodeTable::init_jump() {
    // JMP - Jump
    add("JMP", AddressingMode::Absolute, 0x4C, 3, 3);
    add("JMP", AddressingMode::Indirect, 0x6C, 3, 5);

    // JSR - Jump to Subroutine
    add("JSR", AddressingMode::Absolute, 0x20, 3, 6);

    // RTS - Return from Subroutine
    add("RTS", AddressingMode::Implied, 0x60, 1, 6);

    // RTI - Return from Interrupt
    add("RTI", AddressingMode::Implied, 0x40, 1, 6);
}

void OpcodeTable::init_transfer() {
    add("TAX", AddressingMode::Implied, 0xAA, 1, 2);
    add("TAY", AddressingMode::Implied, 0xA8, 1, 2);
    add("TXA", AddressingMode::Implied, 0x8A, 1, 2);
    add("TYA", AddressingMode::Implied, 0x98, 1, 2);
    add("TSX", AddressingMode::Implied, 0xBA, 1, 2);
    add("TXS", AddressingMode::Implied, 0x9A, 1, 2);
}

void OpcodeTable::init_stack() {
    add("PHA", AddressingMode::Implied, 0x48, 1, 3);
    add("PHP", AddressingMode::Implied, 0x08, 1, 3);
    add("PLA", AddressingMode::Implied, 0x68, 1, 4);
    add("PLP", AddressingMode::Implied, 0x28, 1, 4);
}

void OpcodeTable::init_flags() {
    add("CLC", AddressingMode::Implied, 0x18, 1, 2);
    add("CLD", AddressingMode::Implied, 0xD8, 1, 2);
    add("CLI", AddressingMode::Implied, 0x58, 1, 2);
    add("CLV", AddressingMode::Implied, 0xB8, 1, 2);
    add("SEC", AddressingMode::Implied, 0x38, 1, 2);
    add("SED", AddressingMode::Implied, 0xF8, 1, 2);
    add("SEI", AddressingMode::Implied, 0x78, 1, 2);
}

void OpcodeTable::init_system() {
    add("BRK", AddressingMode::Implied, 0x00, 1, 7);
    add("NOP", AddressingMode::Implied, 0xEA, 1, 2);
}

// =========================================
// Addressing Mode Detection
// =========================================

AddressingMode AddressingModeDetector::detect(const std::string &operand,
                                              const std::string &mnemonic) {
    // Empty operand - Implied or Accumulator
    if (operand.empty()) {
        return AddressingMode::Implied;
    }

    // Check for accumulator mode ("A")
    if (operand == "A" || operand == "a") {
        return AddressingMode::Accumulator;
    }

    // Branch instructions always use relative
    if (is_branch_instruction(mnemonic)) {
        return AddressingMode::Relative;
    }

    // Immediate mode (#)
    if (operand[0] == '#') {
        return AddressingMode::Immediate;
    }

    // Indirect modes - check for parentheses
    if (operand.find('(') != std::string::npos) {
        if (operand.find(",X)") != std::string::npos || operand.find(",x)") != std::string::npos) {
            return AddressingMode::IndexedIndirect; // ($nn,X)
        } else if (operand.find("),Y") != std::string::npos ||
                   operand.find("),y") != std::string::npos) {
            return AddressingMode::IndirectIndexed; // ($nn),Y
        } else {
            return AddressingMode::Indirect; // ($nnnn) - JMP only
        }
    }

    // Indexed modes - check for ,X or ,Y
    bool has_x = operand.find(",X") != std::string::npos || operand.find(",x") != std::string::npos;
    bool has_y = operand.find(",Y") != std::string::npos || operand.find(",y") != std::string::npos;

    // Extract the address part (before ,X or ,Y if present)
    std::string addr_part = operand;
    size_t comma_pos = operand.find(',');
    if (comma_pos != std::string::npos) {
        addr_part = operand.substr(0, comma_pos);
    }

    // Trim whitespace
    addr_part.erase(0, addr_part.find_first_not_of(" \t"));
    addr_part.erase(addr_part.find_last_not_of(" \t") + 1);

    // Detect zero page vs absolute based on value
    // Zero page is $00-$FF (values 0-255)
    bool is_zero_page = false;

    // Check if it's a hex literal we can evaluate immediately
    if (!addr_part.empty() && addr_part[0] == '$') {
        std::string hex_str = addr_part.substr(1);
        // If it's 1 or 2 hex digits, it's zero page
        if (hex_str.length() <= 2) {
            is_zero_page = true;
        }
    }

    // Return appropriate mode
    if (has_x) {
        return is_zero_page ? AddressingMode::ZeroPageX : AddressingMode::AbsoluteX;
    } else if (has_y) {
        return is_zero_page ? AddressingMode::ZeroPageY : AddressingMode::AbsoluteY;
    } else {
        return is_zero_page ? AddressingMode::ZeroPage : AddressingMode::Absolute;
    }
}

bool AddressingModeDetector::is_branch_instruction(const std::string &mnemonic) {
    static const std::vector<std::string> branches = {"BCC", "BCS", "BEQ", "BNE",
                                                      "BMI", "BPL", "BVC", "BVS"};
    return std::find(branches.begin(), branches.end(), mnemonic) != branches.end();
}

} // namespace edasm
