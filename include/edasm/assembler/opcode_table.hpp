#pragma once

#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

namespace edasm {

// Addressing modes (from 6502_INSTRUCTION_SET.md)
enum class AddressingMode {
    Implied,         // RTS
    Accumulator,     // ASL A
    Immediate,       // LDA #$42
    ZeroPage,        // LDA $42
    ZeroPageX,       // LDA $42,X
    ZeroPageY,       // LDX $42,Y
    Absolute,        // LDA $1234
    AbsoluteX,       // LDA $1234,X
    AbsoluteY,       // LDA $1234,Y
    Indirect,        // JMP ($1234)
    IndexedIndirect, // LDA ($42,X)
    IndirectIndexed, // LDA ($42),Y
    Relative         // BEQ label
};

// Opcode entry (from 6502_INSTRUCTION_SET.md opcode table)
struct Opcode {
    std::string mnemonic;
    AddressingMode mode;
    uint8_t code;
    int bytes;
    int cycles;
    bool extra_cycle_on_page_cross;
};

class OpcodeTable {
  public:
    OpcodeTable();

    // Lookup opcode by mnemonic and addressing mode
    const Opcode *lookup(const std::string &mnemonic, AddressingMode mode) const;

    // Get all valid addressing modes for a mnemonic
    std::vector<AddressingMode> valid_modes(const std::string &mnemonic) const;

    // Check if mnemonic exists
    bool is_valid_mnemonic(const std::string &mnemonic) const;

  private:
    // Nested map: mnemonic -> (mode -> opcode)
    std::unordered_map<std::string, std::unordered_map<AddressingMode, Opcode>> table_;

    void add(const std::string &mnem, AddressingMode mode, uint8_t code, int bytes, int cycles,
             bool page_cross = false);

    // Initialize all opcodes
    void init_load_store();
    void init_arithmetic();
    void init_increment_decrement();
    void init_logical();
    void init_shift_rotate();
    void init_compare();
    void init_branch();
    void init_jump();
    void init_transfer();
    void init_stack();
    void init_flags();
    void init_system();
};

// Addressing mode detection helper
class AddressingModeDetector {
  public:
    static AddressingMode detect(const std::string &operand, const std::string &mnemonic);

  private:
    static bool is_branch_instruction(const std::string &mnemonic);
};

} // namespace edasm
