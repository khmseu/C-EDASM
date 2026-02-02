/**
 * @file opcode_table.hpp
 * @brief 6502 opcode lookup table
 * 
 * Provides fast lookup of 6502 opcodes by mnemonic and addressing mode.
 * Contains all legal 6502 opcodes with their binary codes, byte counts,
 * and cycle timings.
 * 
 * Reference: 6502_INSTRUCTION_SET.md, ASM opcode tables
 */

#pragma once

#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

namespace edasm {

/**
 * @brief 6502 addressing modes
 * 
 * All addressing modes supported by the 6502 processor.
 * Reference: 6502_INSTRUCTION_SET.md
 */
enum class AddressingMode {
    Implied,         ///< No operand (e.g., RTS)
    Accumulator,     ///< Accumulator (e.g., ASL A)
    Immediate,       ///< Immediate value (e.g., LDA #$42)
    ZeroPage,        ///< Zero page address (e.g., LDA $42)
    ZeroPageX,       ///< Zero page indexed by X (e.g., LDA $42,X)
    ZeroPageY,       ///< Zero page indexed by Y (e.g., LDX $42,Y)
    Absolute,        ///< Absolute address (e.g., LDA $1234)
    AbsoluteX,       ///< Absolute indexed by X (e.g., LDA $1234,X)
    AbsoluteY,       ///< Absolute indexed by Y (e.g., LDA $1234,Y)
    Indirect,        ///< Indirect (e.g., JMP ($1234))
    IndexedIndirect, ///< Indexed indirect (e.g., LDA ($42,X))
    IndirectIndexed, ///< Indirect indexed (e.g., LDA ($42),Y)
    Relative         ///< Relative branch (e.g., BEQ label)
};

/**
 * @brief Opcode entry with metadata
 * 
 * Contains all information about a specific 6502 opcode variant.
 */
struct Opcode {
    std::string mnemonic;           ///< Instruction mnemonic
    AddressingMode mode;            ///< Addressing mode
    uint8_t code;                   ///< Binary opcode
    int bytes;                      ///< Instruction length in bytes
    int cycles;                     ///< Base cycle count
    bool extra_cycle_on_page_cross; ///< True if page crossing adds cycle
};

/**
 * @brief Opcode lookup table
 * 
 * Fast lookup of 6502 opcodes by mnemonic and addressing mode.
 * Initialized with all legal 6502 opcodes at construction.
 */
class OpcodeTable {
  public:
    /**
     * @brief Construct and initialize opcode table
     */
    OpcodeTable();

    /**
     * @brief Look up opcode by mnemonic and addressing mode
     * @param mnemonic Instruction mnemonic (e.g., "LDA")
     * @param mode Addressing mode
     * @return const Opcode* Opcode entry or nullptr if not found
     */
    const Opcode *lookup(const std::string &mnemonic, AddressingMode mode) const;

    /**
     * @brief Get all valid addressing modes for a mnemonic
     * @param mnemonic Instruction mnemonic
     * @return std::vector<AddressingMode> List of valid modes
     */
    std::vector<AddressingMode> valid_modes(const std::string &mnemonic) const;

    /**
     * @brief Check if mnemonic is valid
     * @param mnemonic Instruction mnemonic
     * @return bool True if mnemonic exists
     */
    bool is_valid_mnemonic(const std::string &mnemonic) const;

  private:
    /// Nested map: mnemonic -> (mode -> opcode)
    std::unordered_map<std::string, std::unordered_map<AddressingMode, Opcode>> table_;

    /**
     * @brief Add an opcode to the table
     */
    void add(const std::string &mnem, AddressingMode mode, uint8_t code, int bytes, int cycles,
             bool page_cross = false);

    // Initialization methods for opcode groups
    void init_load_store();         ///< Load/store instructions
    void init_arithmetic();         ///< Arithmetic instructions
    void init_increment_decrement(); ///< INC/DEC instructions
    void init_logical();            ///< Logical operations
    void init_shift_rotate();       ///< Shift/rotate instructions
    void init_compare();            ///< Compare instructions
    void init_branch();             ///< Branch instructions
    void init_jump();               ///< Jump instructions
    void init_transfer();           ///< Register transfer
    void init_stack();              ///< Stack operations
    void init_flags();              ///< Flag operations
    void init_system();             ///< System instructions
};

/**
 * @brief Addressing mode detection helper
 * 
 * Detects addressing mode from operand syntax.
 */
class AddressingModeDetector {
  public:
    /**
     * @brief Detect addressing mode from operand string
     * @param operand Operand string (e.g., "#$42", "$1234,X")
     * @param mnemonic Instruction mnemonic (for context)
     * @return AddressingMode Detected mode
     */
    static AddressingMode detect(const std::string &operand, const std::string &mnemonic);

  private:
    /**
     * @brief Check if instruction is a branch
     * @param mnemonic Instruction mnemonic
     * @return bool True if branch instruction
     */
    static bool is_branch_instruction(const std::string &mnemonic);
};

} // namespace edasm
