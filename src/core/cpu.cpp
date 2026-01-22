#include "edasm/cpu.hpp"
#include "edasm/bus.hpp"

namespace edasm {

CPU::CPU(Bus& bus) 
    : bus_(bus), instruction_count_(0) {
    reset();
}

void CPU::reset() {
    state_ = CPUState();
    instruction_count_ = 0;
    
    // Set PC to reset vector or default entrypoint
    // For EDASM.SYSTEM loaded at $2000, we'll set PC to $2000
    state_.PC = 0x2000;
}

void CPU::set_trap_handler(TrapHandler handler) {
    trap_handler_ = handler;
}

bool CPU::step() {
    // Fetch opcode
    uint8_t opcode = fetch_byte();
    
    // Check for trap opcode ($02)
    if (opcode == Bus::TRAP_OPCODE) {
        if (trap_handler_) {
            // Call trap handler, return its result (true = continue, false = halt)
            return trap_handler_(state_, bus_, state_.PC - 1);
        } else {
            // No trap handler, halt
            return false;
        }
    }
    
    // Execute instruction
    bool result = execute_instruction(opcode);
    instruction_count_++;
    return result;
}

uint8_t CPU::fetch_byte() {
    uint8_t value = bus_.read(state_.PC);
    state_.PC++;
    return value;
}

uint16_t CPU::fetch_word() {
    uint8_t lo = fetch_byte();
    uint8_t hi = fetch_byte();
    return static_cast<uint16_t>(lo) | (static_cast<uint16_t>(hi) << 8);
}

void CPU::push_byte(uint8_t value) {
    bus_.write(0x0100 | state_.SP, value);
    state_.SP--;
}

uint8_t CPU::pull_byte() {
    state_.SP++;
    return bus_.read(0x0100 | state_.SP);
}

void CPU::push_word(uint16_t value) {
    push_byte(static_cast<uint8_t>((value >> 8) & 0xFF)); // High byte first
    push_byte(static_cast<uint8_t>(value & 0xFF));        // Low byte second
}

uint16_t CPU::pull_word() {
    uint8_t lo = pull_byte();
    uint8_t hi = pull_byte();
    return static_cast<uint16_t>(lo) | (static_cast<uint16_t>(hi) << 8);
}

void CPU::set_flag(uint8_t flag, bool value) {
    if (value) {
        state_.P |= flag;
    } else {
        state_.P &= ~flag;
    }
}

bool CPU::get_flag(uint8_t flag) const {
    return (state_.P & flag) != 0;
}

void CPU::update_nz(uint8_t value) {
    set_flag(StatusFlags::Z, value == 0);
    set_flag(StatusFlags::N, (value & 0x80) != 0);
}

bool CPU::execute_instruction(uint8_t opcode) {
    // Minimal instruction set implementation
    // This is a skeleton that will be expanded as needed
    
    switch (opcode) {
        // NOP
        case 0xEA:
            break;
            
        // BRK
        case 0x00:
            push_word(state_.PC);
            push_byte(state_.P | StatusFlags::B);
            set_flag(StatusFlags::I, true);
            state_.PC = bus_.read_word(0xFFFE);
            break;
            
        // RTI
        case 0x40:
            state_.P = pull_byte();
            state_.PC = pull_word();
            break;
            
        // RTS
        case 0x60:
            state_.PC = pull_word() + 1;
            break;
            
        // LDA immediate
        case 0xA9:
            state_.A = fetch_byte();
            update_nz(state_.A);
            break;
            
        // LDA absolute
        case 0xAD: {
            uint16_t addr = fetch_word();
            state_.A = bus_.read(addr);
            update_nz(state_.A);
            break;
        }
        
        // STA absolute
        case 0x8D: {
            uint16_t addr = fetch_word();
            bus_.write(addr, state_.A);
            break;
        }
        
        // JMP absolute
        case 0x4C:
            state_.PC = fetch_word();
            break;
            
        // JSR absolute
        case 0x20: {
            uint16_t addr = fetch_word();
            push_word(state_.PC - 1);
            state_.PC = addr;
            break;
        }
        
        default:
            // Unimplemented opcode - treat as trap
            // Decrement PC to point to the unimplemented opcode
            state_.PC--;
            if (trap_handler_) {
                return trap_handler_(state_, bus_, state_.PC);
            }
            return false; // Halt on unimplemented opcode
    }
    
    return true; // Continue execution
}

} // namespace edasm
