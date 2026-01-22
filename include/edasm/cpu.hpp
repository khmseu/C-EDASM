#ifndef EDASM_CPU_HPP
#define EDASM_CPU_HPP

#include <cstdint>
#include <functional>

namespace edasm {

// Forward declaration
class Bus;

// CPU status register flags
namespace StatusFlags {
    constexpr uint8_t C = 0x01; // Carry
    constexpr uint8_t Z = 0x02; // Zero
    constexpr uint8_t I = 0x04; // Interrupt disable
    constexpr uint8_t D = 0x08; // Decimal mode
    constexpr uint8_t B = 0x10; // Break
    constexpr uint8_t U = 0x20; // Unused (always 1)
    constexpr uint8_t V = 0x40; // Overflow
    constexpr uint8_t N = 0x80; // Negative
}

// CPU state for 65C02
struct CPUState {
    uint8_t A;      // Accumulator
    uint8_t X;      // X register
    uint8_t Y;      // Y register
    uint8_t SP;     // Stack pointer
    uint8_t P;      // Processor status
    uint16_t PC;    // Program counter
    
    CPUState() : A(0), X(0), Y(0), SP(0xFF), P(StatusFlags::U | StatusFlags::I), PC(0) {}
};

// Trap handler callback type
// Returns true to continue execution, false to halt
using TrapHandler = std::function<bool(CPUState& cpu, Bus& bus, uint16_t trap_pc)>;

// 65C02 CPU emulator
class CPU {
public:
    explicit CPU(Bus& bus);
    
    // Reset CPU to initial state
    void reset();
    
    // Execute one instruction
    // Returns false if halted (e.g., via trap)
    bool step();
    
    // Set trap handler for opcode $02
    void set_trap_handler(TrapHandler handler);
    
    // Access CPU state
    CPUState& state() { return state_; }
    const CPUState& state() const { return state_; }
    
    // Get instruction count
    uint64_t instruction_count() const { return instruction_count_; }
    
private:
    Bus& bus_;
    CPUState state_;
    TrapHandler trap_handler_;
    uint64_t instruction_count_;
    
    // Instruction execution helpers
    uint8_t fetch_byte();
    uint16_t fetch_word();
    void push_byte(uint8_t value);
    uint8_t pull_byte();
    void push_word(uint16_t value);
    uint16_t pull_word();
    
    // Flag manipulation
    void set_flag(uint8_t flag, bool value);
    bool get_flag(uint8_t flag) const;
    void update_nz(uint8_t value);
    
    // Execute single instruction
    bool execute_instruction(uint8_t opcode);
};

} // namespace edasm

#endif // EDASM_CPU_HPP
