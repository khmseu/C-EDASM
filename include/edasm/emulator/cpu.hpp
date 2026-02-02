/**
 * @file cpu.hpp
 * @brief 65C02 CPU emulator
 *
 * Implements a minimal 65C02 CPU emulator for running EDASM.SYSTEM binary.
 * Uses trap-first discovery: memory prefilled with trap opcode ($02) to
 * incrementally discover needed ProDOS and system services.
 *
 * Features:
 * - Full 65C02 instruction set (100+ opcodes)
 * - All addressing modes
 * - Trap handler for system call emulation
 * - Cycle-accurate timing (base cycles only)
 *
 * Reference: docs/EMULATOR_MINIMAL_PLAN.md, 65C02 datasheet
 */

#ifndef EDASM_CPU_HPP
#define EDASM_CPU_HPP

#include <cstdint>
#include <functional>

namespace edasm {

// Forward declaration
class Bus;

/**
 * @brief CPU status register flag bits
 *
 * Standard 6502 processor status flags.
 */
namespace StatusFlags {
constexpr uint8_t C = 0x01; ///< Carry
constexpr uint8_t Z = 0x02; ///< Zero
constexpr uint8_t I = 0x04; ///< Interrupt disable
constexpr uint8_t D = 0x08; ///< Decimal mode
constexpr uint8_t B = 0x10; ///< Break
constexpr uint8_t U = 0x20; ///< Unused (always 1)
constexpr uint8_t V = 0x40; ///< Overflow
constexpr uint8_t N = 0x80; ///< Negative
} // namespace StatusFlags

/**
 * @brief CPU register state for 65C02
 *
 * Contains all CPU registers and flags.
 */
struct CPUState {
    uint8_t A;   ///< Accumulator
    uint8_t X;   ///< X register
    uint8_t Y;   ///< Y register
    uint8_t SP;  ///< Stack pointer (points into $0100-$01FF)
    uint8_t P;   ///< Processor status flags
    uint16_t PC; ///< Program counter

    /**
     * @brief Construct CPU state with initial values
     *
     * Initializes registers to power-on state:
     * - A, X, Y = 0
     * - SP = $FF (stack at $01FF)
     * - P = Unused | Interrupt disable
     * - PC = 0
     */
    CPUState() : A(0), X(0), Y(0), SP(0xFF), P(StatusFlags::U | StatusFlags::I), PC(0) {}
};

/**
 * @brief Trap handler callback type
 *
 * Called when trap opcode ($02) is executed. Handler can emulate
 * system calls or debug the execution.
 *
 * @param cpu CPU state (modifiable)
 * @param bus Memory bus (modifiable)
 * @param trap_pc PC where trap was encountered
 * @return bool True to continue execution, false to halt
 */
using TrapHandler = std::function<bool(CPUState &cpu, Bus &bus, uint16_t trap_pc)>;

/**
 * @brief 65C02 CPU emulator
 *
 * Emulates the 65C02 processor with full instruction set.
 * Supports trap handling for incremental system call discovery.
 */
class CPU {
  public:
    /**
     * @brief Construct CPU with memory bus
     * @param bus Memory bus reference
     */
    explicit CPU(Bus &bus);

    /**
     * @brief Reset CPU to initial state
     *
     * Loads PC from reset vector at $FFFC/$FFFD and initializes registers.
     */
    void reset();

    /**
     * @brief Execute one instruction
     * @return bool False if halted (e.g., via trap), true otherwise
     */
    bool step();

    /**
     * @brief Set trap handler for opcode $02
     * @param handler Trap handler callback
     */
    void set_trap_handler(TrapHandler handler);

    /**
     * @brief Get mutable CPU state
     * @return CPUState& CPU state reference
     */
    CPUState &state() {
        return state_;
    }

    /**
     * @brief Get immutable CPU state
     * @return const CPUState& CPU state reference
     */
    const CPUState &state() const {
        return state_;
    }

    /**
     * @brief Get total instruction count
     * @return uint64_t Number of instructions executed
     */
    uint64_t instruction_count() const {
        return instruction_count_;
    }

  private:
    Bus &bus_;                   ///< Memory bus reference
    CPUState state_;             ///< CPU register state
    TrapHandler trap_handler_;   ///< Trap handler callback
    uint64_t instruction_count_; ///< Instructions executed counter

    // Instruction execution helpers

    /**
     * @brief Fetch byte at PC and increment PC
     * @return uint8_t Fetched byte
     */
    uint8_t fetch_byte();

    /**
     * @brief Fetch 16-bit word at PC and increment PC by 2
     * @return uint16_t Fetched word (little-endian)
     */
    uint16_t fetch_word();

    /**
     * @brief Push byte onto stack
     * @param value Byte to push
     */
    void push_byte(uint8_t value);

    /**
     * @brief Pull byte from stack
     * @return uint8_t Pulled byte
     */
    uint8_t pull_byte();

    /**
     * @brief Push 16-bit word onto stack
     * @param value Word to push
     */
    void push_word(uint16_t value);

    /**
     * @brief Pull 16-bit word from stack
     * @return uint16_t Pulled word
     */
    uint16_t pull_word();

    // Flag manipulation

    /**
     * @brief Set or clear a status flag
     * @param flag Flag bit mask
     * @param value True to set, false to clear
     */
    void set_flag(uint8_t flag, bool value);

    /**
     * @brief Get status flag value
     * @param flag Flag bit mask
     * @return bool True if flag set
     */
    bool get_flag(uint8_t flag) const;

    /**
     * @brief Update N and Z flags based on value
     * @param value Value to test
     */
    void update_nz(uint8_t value);

    /**
     * @brief Execute single instruction by opcode
     * @param opcode Instruction opcode
     * @return bool False if should halt
     */
    bool execute_instruction(uint8_t opcode);
};

} // namespace edasm

#endif // EDASM_CPU_HPP
