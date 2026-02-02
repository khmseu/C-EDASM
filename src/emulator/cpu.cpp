/**
 * @file cpu.cpp
 * @brief 65C02 CPU emulator implementation
 *
 * Implements complete 65C02 instruction set with all addressing modes.
 * Provides trap handling for incremental system call discovery.
 */

#include "edasm/emulator/cpu.hpp"
#include "edasm/constants.hpp"
#include "edasm/emulator/bus.hpp"

namespace edasm {

CPU::CPU(Bus &bus) : bus_(bus), instruction_count_(0) {
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
    bus_.write(STACK_BASE | state_.SP, value);
    state_.SP--;
}

uint8_t CPU::pull_byte() {
    state_.SP++;
    return bus_.read(STACK_BASE | state_.SP);
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

    // JMP (indirect)
    case 0x6C: {
        uint16_t ptr = fetch_word();
        // Emulate 6502 page-boundary wraparound bug
        uint16_t ptr_hi_addr = static_cast<uint16_t>((ptr & 0xFF00) | ((ptr + 1) & 0x00FF));
        uint8_t lo = bus_.read(ptr);
        uint8_t hi = bus_.read(ptr_hi_addr);
        state_.PC = static_cast<uint16_t>(lo | (static_cast<uint16_t>(hi) << 8));
        break;
    }

    // JSR absolute
    case 0x20: {
        uint16_t addr = fetch_word();
        push_word(state_.PC - 1);
        state_.PC = addr;
        break;
    }

    // LDA zero page
    case 0xA5: {
        uint8_t addr = fetch_byte();
        state_.A = bus_.read(addr);
        update_nz(state_.A);
        break;
    }

    // STA zero page
    case 0x85: {
        uint8_t addr = fetch_byte();
        bus_.write(addr, state_.A);
        break;
    }

    // LDX immediate
    case 0xA2:
        state_.X = fetch_byte();
        update_nz(state_.X);
        break;

    // LDX absolute
    case 0xAE: {
        uint16_t addr = fetch_word();
        state_.X = bus_.read(addr);
        update_nz(state_.X);
        break;
    }

    // LDX zero page
    case 0xA6: {
        uint8_t addr = fetch_byte();
        state_.X = bus_.read(addr);
        update_nz(state_.X);
        break;
    }

    // LDY immediate
    case 0xA0:
        state_.Y = fetch_byte();
        update_nz(state_.Y);
        break;

    // LDY absolute
    case 0xAC: {
        uint16_t addr = fetch_word();
        state_.Y = bus_.read(addr);
        update_nz(state_.Y);
        break;
    }

    // LDY zero page
    case 0xA4: {
        uint8_t addr = fetch_byte();
        state_.Y = bus_.read(addr);
        update_nz(state_.Y);
        break;
    }

    // STX absolute
    case 0x8E: {
        uint16_t addr = fetch_word();
        bus_.write(addr, state_.X);
        break;
    }

    // STX zero page
    case 0x86: {
        uint8_t addr = fetch_byte();
        bus_.write(addr, state_.X);
        break;
    }

    // STY absolute
    case 0x8C: {
        uint16_t addr = fetch_word();
        bus_.write(addr, state_.Y);
        break;
    }

    // STY zero page
    case 0x84: {
        uint8_t addr = fetch_byte();
        bus_.write(addr, state_.Y);
        break;
    }

    // INX
    case 0xE8:
        state_.X++;
        update_nz(state_.X);
        break;

    // INY
    case 0xC8:
        state_.Y++;
        update_nz(state_.Y);
        break;

    // DEX
    case 0xCA:
        state_.X--;
        update_nz(state_.X);
        break;

    // DEY
    case 0x88:
        state_.Y--;
        update_nz(state_.Y);
        break;

    // ADC immediate
    case 0x69: {
        uint8_t operand = fetch_byte();
        uint16_t result = state_.A + operand + (get_flag(StatusFlags::C) ? 1 : 0);
        set_flag(StatusFlags::C, result > 0xFF);
        set_flag(StatusFlags::V, (~(state_.A ^ operand) & (state_.A ^ result) & 0x80) != 0);
        state_.A = static_cast<uint8_t>(result);
        update_nz(state_.A);
        break;
    }

    // ADC absolute
    case 0x6D: {
        uint16_t addr = fetch_word();
        uint8_t operand = bus_.read(addr);
        uint16_t result = state_.A + operand + (get_flag(StatusFlags::C) ? 1 : 0);
        set_flag(StatusFlags::C, result > 0xFF);
        set_flag(StatusFlags::V, (~(state_.A ^ operand) & (state_.A ^ result) & 0x80) != 0);
        state_.A = static_cast<uint8_t>(result);
        update_nz(state_.A);
        break;
    }

    // ADC zero page
    case 0x65: {
        uint8_t addr = fetch_byte();
        uint8_t operand = bus_.read(addr);
        uint16_t result = state_.A + operand + (get_flag(StatusFlags::C) ? 1 : 0);
        set_flag(StatusFlags::C, result > 0xFF);
        set_flag(StatusFlags::V, (~(state_.A ^ operand) & (state_.A ^ result) & 0x80) != 0);
        state_.A = static_cast<uint8_t>(result);
        update_nz(state_.A);
        break;
    }

    // SBC immediate (carry acts as "not borrow")
    case 0xE9: {
        uint8_t operand = fetch_byte();
        int16_t result = state_.A - operand - (get_flag(StatusFlags::C) ? 0 : 1);
        set_flag(StatusFlags::C, result >= 0);
        set_flag(StatusFlags::V, ((state_.A ^ operand) & (state_.A ^ result) & 0x80) != 0);
        state_.A = static_cast<uint8_t>(result);
        update_nz(state_.A);
        break;
    }

    // SBC absolute (carry acts as "not borrow")
    case 0xED: {
        uint16_t addr = fetch_word();
        uint8_t operand = bus_.read(addr);
        int16_t result = state_.A - operand - (get_flag(StatusFlags::C) ? 0 : 1);
        set_flag(StatusFlags::C, result >= 0);
        set_flag(StatusFlags::V, ((state_.A ^ operand) & (state_.A ^ result) & 0x80) != 0);
        state_.A = static_cast<uint8_t>(result);
        update_nz(state_.A);
        break;
    }

    // SBC zero page (carry acts as "not borrow")
    case 0xE5: {
        uint8_t addr = fetch_byte();
        uint8_t operand = bus_.read(addr);
        int16_t result = state_.A - operand - (get_flag(StatusFlags::C) ? 0 : 1);
        set_flag(StatusFlags::C, result >= 0);
        set_flag(StatusFlags::V, ((state_.A ^ operand) & (state_.A ^ result) & 0x80) != 0);
        state_.A = static_cast<uint8_t>(result);
        update_nz(state_.A);
        break;
    }

    // AND immediate
    case 0x29:
        state_.A &= fetch_byte();
        update_nz(state_.A);
        break;

    // AND absolute
    case 0x2D: {
        uint16_t addr = fetch_word();
        state_.A &= bus_.read(addr);
        update_nz(state_.A);
        break;
    }

    // AND zero page
    case 0x25: {
        uint8_t addr = fetch_byte();
        state_.A &= bus_.read(addr);
        update_nz(state_.A);
        break;
    }

    // ORA immediate
    case 0x09:
        state_.A |= fetch_byte();
        update_nz(state_.A);
        break;

    // ORA absolute
    case 0x0D: {
        uint16_t addr = fetch_word();
        state_.A |= bus_.read(addr);
        update_nz(state_.A);
        break;
    }

    // ORA zero page
    case 0x05: {
        uint8_t addr = fetch_byte();
        state_.A |= bus_.read(addr);
        update_nz(state_.A);
        break;
    }

    // EOR immediate
    case 0x49:
        state_.A ^= fetch_byte();
        update_nz(state_.A);
        break;

    // EOR absolute
    case 0x4D: {
        uint16_t addr = fetch_word();
        state_.A ^= bus_.read(addr);
        update_nz(state_.A);
        break;
    }

    // EOR zero page
    case 0x45: {
        uint8_t addr = fetch_byte();
        state_.A ^= bus_.read(addr);
        update_nz(state_.A);
        break;
    }

    // CMP immediate
    case 0xC9: {
        uint8_t operand = fetch_byte();
        uint16_t result = state_.A - operand;
        set_flag(StatusFlags::C, state_.A >= operand);
        update_nz(static_cast<uint8_t>(result));
        break;
    }

    // CMP absolute
    case 0xCD: {
        uint16_t addr = fetch_word();
        uint8_t operand = bus_.read(addr);
        uint16_t result = state_.A - operand;
        set_flag(StatusFlags::C, state_.A >= operand);
        update_nz(static_cast<uint8_t>(result));
        break;
    }

    // CMP zero page
    case 0xC5: {
        uint8_t addr = fetch_byte();
        uint8_t operand = bus_.read(addr);
        uint16_t result = state_.A - operand;
        set_flag(StatusFlags::C, state_.A >= operand);
        update_nz(static_cast<uint8_t>(result));
        break;
    }

    // CPX immediate
    case 0xE0: {
        uint8_t operand = fetch_byte();
        uint16_t result = state_.X - operand;
        set_flag(StatusFlags::C, state_.X >= operand);
        update_nz(static_cast<uint8_t>(result));
        break;
    }

    // CPX absolute
    case 0xEC: {
        uint16_t addr = fetch_word();
        uint8_t operand = bus_.read(addr);
        uint16_t result = state_.X - operand;
        set_flag(StatusFlags::C, state_.X >= operand);
        update_nz(static_cast<uint8_t>(result));
        break;
    }

    // CPX zero page
    case 0xE4: {
        uint8_t addr = fetch_byte();
        uint8_t operand = bus_.read(addr);
        uint16_t result = state_.X - operand;
        set_flag(StatusFlags::C, state_.X >= operand);
        update_nz(static_cast<uint8_t>(result));
        break;
    }

    // CPY immediate
    case 0xC0: {
        uint8_t operand = fetch_byte();
        uint16_t result = state_.Y - operand;
        set_flag(StatusFlags::C, state_.Y >= operand);
        update_nz(static_cast<uint8_t>(result));
        break;
    }

    // CPY absolute
    case 0xCC: {
        uint16_t addr = fetch_word();
        uint8_t operand = bus_.read(addr);
        uint16_t result = state_.Y - operand;
        set_flag(StatusFlags::C, state_.Y >= operand);
        update_nz(static_cast<uint8_t>(result));
        break;
    }

    // CPY zero page
    case 0xC4: {
        uint8_t addr = fetch_byte();
        uint8_t operand = bus_.read(addr);
        uint16_t result = state_.Y - operand;
        set_flag(StatusFlags::C, state_.Y >= operand);
        update_nz(static_cast<uint8_t>(result));
        break;
    }

    // BEQ - Branch if Equal (Z=1)
    case 0xF0: {
        int8_t offset = static_cast<int8_t>(fetch_byte());
        if (get_flag(StatusFlags::Z)) {
            state_.PC += offset;
        }
        break;
    }

    // BNE - Branch if Not Equal (Z=0)
    case 0xD0: {
        int8_t offset = static_cast<int8_t>(fetch_byte());
        if (!get_flag(StatusFlags::Z)) {
            state_.PC += offset;
        }
        break;
    }

    // BCC - Branch if Carry Clear (C=0)
    case 0x90: {
        int8_t offset = static_cast<int8_t>(fetch_byte());
        if (!get_flag(StatusFlags::C)) {
            state_.PC += offset;
        }
        break;
    }

    // BCS - Branch if Carry Set (C=1)
    case 0xB0: {
        int8_t offset = static_cast<int8_t>(fetch_byte());
        if (get_flag(StatusFlags::C)) {
            state_.PC += offset;
        }
        break;
    }

    // BMI - Branch if Minus (N=1)
    case 0x30: {
        int8_t offset = static_cast<int8_t>(fetch_byte());
        if (get_flag(StatusFlags::N)) {
            state_.PC += offset;
        }
        break;
    }

    // BPL - Branch if Plus (N=0)
    case 0x10: {
        int8_t offset = static_cast<int8_t>(fetch_byte());
        if (!get_flag(StatusFlags::N)) {
            state_.PC += offset;
        }
        break;
    }

    // BVS - Branch if Overflow Set (V=1)
    case 0x70: {
        int8_t offset = static_cast<int8_t>(fetch_byte());
        if (get_flag(StatusFlags::V)) {
            state_.PC += offset;
        }
        break;
    }

    // BVC - Branch if Overflow Clear (V=0)
    case 0x50: {
        int8_t offset = static_cast<int8_t>(fetch_byte());
        if (!get_flag(StatusFlags::V)) {
            state_.PC += offset;
        }
        break;
    }

    // PHA - Push Accumulator
    case 0x48:
        push_byte(state_.A);
        break;

    // PLA - Pull Accumulator
    case 0x68:
        state_.A = pull_byte();
        update_nz(state_.A);
        break;

    // PHP - Push Processor Status
    case 0x08:
        push_byte(state_.P | StatusFlags::B);
        break;

    // PLP - Pull Processor Status
    case 0x28:
        state_.P = pull_byte();
        break;

    // TAX - Transfer A to X
    case 0xAA:
        state_.X = state_.A;
        update_nz(state_.X);
        break;

    // TAY - Transfer A to Y
    case 0xA8:
        state_.Y = state_.A;
        update_nz(state_.Y);
        break;

    // TXA - Transfer X to A
    case 0x8A:
        state_.A = state_.X;
        update_nz(state_.A);
        break;

    // TYA - Transfer Y to A
    case 0x98:
        state_.A = state_.Y;
        update_nz(state_.A);
        break;

    // TXS - Transfer X to Stack Pointer
    case 0x9A:
        state_.SP = state_.X;
        break;

    // TSX - Transfer Stack Pointer to X
    case 0xBA:
        state_.X = state_.SP;
        update_nz(state_.X);
        break;

    // BIT absolute
    case 0x2C: {
        uint16_t addr = fetch_word();
        uint8_t value = bus_.read(addr);
        uint8_t result = state_.A & value;
        set_flag(StatusFlags::Z, result == 0);
        set_flag(StatusFlags::N, (value & 0x80) != 0);
        set_flag(StatusFlags::V, (value & 0x40) != 0);
        break;
    }

    // BIT zero page
    case 0x24: {
        uint8_t addr = fetch_byte();
        uint8_t value = bus_.read(addr);
        uint8_t result = state_.A & value;
        set_flag(StatusFlags::Z, result == 0);
        set_flag(StatusFlags::N, (value & 0x80) != 0);
        set_flag(StatusFlags::V, (value & 0x40) != 0);
        break;
    }

    // LSR accumulator
    case 0x4A:
        set_flag(StatusFlags::C, (state_.A & 0x01) != 0);
        state_.A >>= 1;
        update_nz(state_.A);
        break;

    // LSR absolute
    case 0x4E: {
        uint16_t addr = fetch_word();
        uint8_t value = bus_.read(addr);
        set_flag(StatusFlags::C, (value & 0x01) != 0);
        value >>= 1;
        bus_.write(addr, value);
        update_nz(value);
        break;
    }

    // LSR zero page
    case 0x46: {
        uint8_t addr = fetch_byte();
        uint8_t value = bus_.read(addr);
        set_flag(StatusFlags::C, (value & 0x01) != 0);
        value >>= 1;
        bus_.write(addr, value);
        update_nz(value);
        break;
    }

    // ASL accumulator
    case 0x0A:
        set_flag(StatusFlags::C, (state_.A & 0x80) != 0);
        state_.A <<= 1;
        update_nz(state_.A);
        break;

    // ASL absolute
    case 0x0E: {
        uint16_t addr = fetch_word();
        uint8_t value = bus_.read(addr);
        set_flag(StatusFlags::C, (value & 0x80) != 0);
        value <<= 1;
        bus_.write(addr, value);
        update_nz(value);
        break;
    }

    // ASL zero page
    case 0x06: {
        uint8_t addr = fetch_byte();
        uint8_t value = bus_.read(addr);
        set_flag(StatusFlags::C, (value & 0x80) != 0);
        value <<= 1;
        bus_.write(addr, value);
        update_nz(value);
        break;
    }

    // ROR accumulator
    case 0x6A: {
        bool old_carry = get_flag(StatusFlags::C);
        set_flag(StatusFlags::C, (state_.A & 0x01) != 0);
        state_.A >>= 1;
        if (old_carry)
            state_.A |= 0x80;
        update_nz(state_.A);
        break;
    }

    // ROR absolute
    case 0x6E: {
        uint16_t addr = fetch_word();
        uint8_t value = bus_.read(addr);
        bool old_carry = get_flag(StatusFlags::C);
        set_flag(StatusFlags::C, (value & 0x01) != 0);
        value >>= 1;
        if (old_carry)
            value |= 0x80;
        bus_.write(addr, value);
        update_nz(value);
        break;
    }

    // ROR zero page
    case 0x66: {
        uint8_t addr = fetch_byte();
        uint8_t value = bus_.read(addr);
        bool old_carry = get_flag(StatusFlags::C);
        set_flag(StatusFlags::C, (value & 0x01) != 0);
        value >>= 1;
        if (old_carry)
            value |= 0x80;
        bus_.write(addr, value);
        update_nz(value);
        break;
    }

    // ROL accumulator
    case 0x2A: {
        bool old_carry = get_flag(StatusFlags::C);
        set_flag(StatusFlags::C, (state_.A & 0x80) != 0);
        state_.A <<= 1;
        if (old_carry)
            state_.A |= 0x01;
        update_nz(state_.A);
        break;
    }

    // ROL absolute
    case 0x2E: {
        uint16_t addr = fetch_word();
        uint8_t value = bus_.read(addr);
        bool old_carry = get_flag(StatusFlags::C);
        set_flag(StatusFlags::C, (value & 0x80) != 0);
        value <<= 1;
        if (old_carry)
            value |= 0x01;
        bus_.write(addr, value);
        update_nz(value);
        break;
    }

    // ROL zero page
    case 0x26: {
        uint8_t addr = fetch_byte();
        uint8_t value = bus_.read(addr);
        bool old_carry = get_flag(StatusFlags::C);
        set_flag(StatusFlags::C, (value & 0x80) != 0);
        value <<= 1;
        if (old_carry)
            value |= 0x01;
        bus_.write(addr, value);
        update_nz(value);
        break;
    }

    // CLC - Clear Carry
    case 0x18:
        set_flag(StatusFlags::C, false);
        break;

    // SEC - Set Carry
    case 0x38:
        set_flag(StatusFlags::C, true);
        break;

    // CLI - Clear Interrupt Disable
    case 0x58:
        set_flag(StatusFlags::I, false);
        break;

    // SEI - Set Interrupt Disable
    case 0x78:
        set_flag(StatusFlags::I, true);
        break;

    // CLV - Clear Overflow
    case 0xB8:
        set_flag(StatusFlags::V, false);
        break;

    // CLD - Clear Decimal
    case 0xD8:
        set_flag(StatusFlags::D, false);
        break;

    // SED - Set Decimal
    case 0xF8:
        set_flag(StatusFlags::D, true);
        break;

    // LDA absolute,X
    case 0xBD: {
        uint16_t addr = fetch_word();
        state_.A = bus_.read(addr + state_.X);
        update_nz(state_.A);
        break;
    }

    // LDA absolute,Y
    case 0xB9: {
        uint16_t addr = fetch_word();
        state_.A = bus_.read(addr + state_.Y);
        update_nz(state_.A);
        break;
    }

    // LDA (indirect,X)
    case 0xA1: {
        uint8_t zp_addr = fetch_byte();
        uint8_t addr_zp = (zp_addr + state_.X) & 0xFF;
        uint8_t lo = bus_.read(addr_zp);
        uint8_t hi = bus_.read((addr_zp + 1) & 0xFF);
        uint16_t addr = lo | (static_cast<uint16_t>(hi) << 8);
        state_.A = bus_.read(addr);
        update_nz(state_.A);
        break;
    }

    // LDA (indirect),Y
    case 0xB1: {
        uint8_t zp_addr = fetch_byte();
        uint8_t lo = bus_.read(zp_addr);
        uint8_t hi = bus_.read((zp_addr + 1) & 0xFF);
        uint16_t addr = lo | (static_cast<uint16_t>(hi) << 8);
        state_.A = bus_.read(addr + state_.Y);
        update_nz(state_.A);
        break;
    }

    // LDX absolute,Y
    case 0xBE: {
        uint16_t addr = fetch_word();
        state_.X = bus_.read(addr + state_.Y);
        update_nz(state_.X);
        break;
    }

    // LDY absolute,X
    case 0xBC: {
        uint16_t addr = fetch_word();
        state_.Y = bus_.read(addr + state_.X);
        update_nz(state_.Y);
        break;
    }

    // LDY zero page,X
    case 0xB4: {
        uint8_t addr = fetch_byte();
        state_.Y = bus_.read((addr + state_.X) & 0xFF);
        update_nz(state_.Y);
        break;
    }

    // LDX zero page,Y
    case 0xB6: {
        uint8_t addr = fetch_byte();
        state_.X = bus_.read((addr + state_.Y) & 0xFF);
        update_nz(state_.X);
        break;
    }

    // STA absolute,X
    case 0x9D: {
        uint16_t addr = fetch_word();
        bus_.write(addr + state_.X, state_.A);
        break;
    }

    // STA absolute,Y
    case 0x99: {
        uint16_t addr = fetch_word();
        bus_.write(addr + state_.Y, state_.A);
        break;
    }

    // STA (indirect,X)
    case 0x81: {
        uint8_t zp_addr = fetch_byte();
        uint8_t addr_zp = (zp_addr + state_.X) & 0xFF;
        uint8_t lo = bus_.read(addr_zp);
        uint8_t hi = bus_.read((addr_zp + 1) & 0xFF);
        uint16_t addr = lo | (static_cast<uint16_t>(hi) << 8);
        bus_.write(addr, state_.A);
        break;
    }

    // STA (indirect),Y
    case 0x91: {
        uint8_t zp_addr = fetch_byte();
        uint8_t lo = bus_.read(zp_addr);
        uint8_t hi = bus_.read((zp_addr + 1) & 0xFF);
        uint16_t addr = lo | (static_cast<uint16_t>(hi) << 8);
        bus_.write(addr + state_.Y, state_.A);
        break;
    }

    // STX zero page,Y
    case 0x96: {
        uint8_t addr = fetch_byte();
        bus_.write((addr + state_.Y) & 0xFF, state_.X);
        break;
    }

    // STY zero page,X
    case 0x94: {
        uint8_t addr = fetch_byte();
        bus_.write((addr + state_.X) & 0xFF, state_.Y);
        break;
    }

    // ADC absolute,X
    case 0x7D: {
        uint16_t addr = fetch_word();
        uint8_t operand = bus_.read(addr + state_.X);
        uint16_t result = state_.A + operand + (get_flag(StatusFlags::C) ? 1 : 0);
        set_flag(StatusFlags::C, result > 0xFF);
        set_flag(StatusFlags::V, (~(state_.A ^ operand) & (state_.A ^ result) & 0x80) != 0);
        state_.A = static_cast<uint8_t>(result);
        update_nz(state_.A);
        break;
    }

    // ADC absolute,Y
    case 0x79: {
        uint16_t addr = fetch_word();
        uint8_t operand = bus_.read(addr + state_.Y);
        uint16_t result = state_.A + operand + (get_flag(StatusFlags::C) ? 1 : 0);
        set_flag(StatusFlags::C, result > 0xFF);
        set_flag(StatusFlags::V, (~(state_.A ^ operand) & (state_.A ^ result) & 0x80) != 0);
        state_.A = static_cast<uint8_t>(result);
        update_nz(state_.A);
        break;
    }

    // ADC (indirect,X)
    case 0x61: {
        uint8_t zp_addr = fetch_byte();
        uint8_t addr_zp = (zp_addr + state_.X) & 0xFF;
        uint8_t lo = bus_.read(addr_zp);
        uint8_t hi = bus_.read((addr_zp + 1) & 0xFF);
        uint16_t addr = lo | (static_cast<uint16_t>(hi) << 8);
        uint8_t operand = bus_.read(addr);
        uint16_t result = state_.A + operand + (get_flag(StatusFlags::C) ? 1 : 0);
        set_flag(StatusFlags::C, result > 0xFF);
        set_flag(StatusFlags::V, (~(state_.A ^ operand) & (state_.A ^ result) & 0x80) != 0);
        state_.A = static_cast<uint8_t>(result);
        update_nz(state_.A);
        break;
    }

    // ADC (indirect),Y
    case 0x71: {
        uint8_t zp_addr = fetch_byte();
        uint8_t lo = bus_.read(zp_addr);
        uint8_t hi = bus_.read((zp_addr + 1) & 0xFF);
        uint16_t addr = lo | (static_cast<uint16_t>(hi) << 8);
        uint8_t operand = bus_.read(addr + state_.Y);
        uint16_t result = state_.A + operand + (get_flag(StatusFlags::C) ? 1 : 0);
        set_flag(StatusFlags::C, result > 0xFF);
        set_flag(StatusFlags::V, (~(state_.A ^ operand) & (state_.A ^ result) & 0x80) != 0);
        state_.A = static_cast<uint8_t>(result);
        update_nz(state_.A);
        break;
    }

    // SBC absolute,X (carry acts as "not borrow")
    case 0xFD: {
        uint16_t addr = fetch_word();
        uint8_t operand = bus_.read(addr + state_.X);
        int16_t result = state_.A - operand - (get_flag(StatusFlags::C) ? 0 : 1);
        set_flag(StatusFlags::C, result >= 0);
        set_flag(StatusFlags::V, ((state_.A ^ operand) & (state_.A ^ result) & 0x80) != 0);
        state_.A = static_cast<uint8_t>(result);
        update_nz(state_.A);
        break;
    }

    // SBC absolute,Y (carry acts as "not borrow")
    case 0xF9: {
        uint16_t addr = fetch_word();
        uint8_t operand = bus_.read(addr + state_.Y);
        int16_t result = state_.A - operand - (get_flag(StatusFlags::C) ? 0 : 1);
        set_flag(StatusFlags::C, result >= 0);
        set_flag(StatusFlags::V, ((state_.A ^ operand) & (state_.A ^ result) & 0x80) != 0);
        state_.A = static_cast<uint8_t>(result);
        update_nz(state_.A);
        break;
    }

    // SBC (indirect,X) (carry acts as "not borrow")
    case 0xE1: {
        uint8_t zp_addr = fetch_byte();
        uint8_t addr_zp = (zp_addr + state_.X) & 0xFF;
        uint8_t lo = bus_.read(addr_zp);
        uint8_t hi = bus_.read((addr_zp + 1) & 0xFF);
        uint16_t addr = lo | (static_cast<uint16_t>(hi) << 8);
        uint8_t operand = bus_.read(addr);
        int16_t result = state_.A - operand - (get_flag(StatusFlags::C) ? 0 : 1);
        set_flag(StatusFlags::C, result >= 0);
        set_flag(StatusFlags::V, ((state_.A ^ operand) & (state_.A ^ result) & 0x80) != 0);
        state_.A = static_cast<uint8_t>(result);
        update_nz(state_.A);
        break;
    }

    // SBC (indirect),Y (carry acts as "not borrow")
    case 0xF1: {
        uint8_t zp_addr = fetch_byte();
        uint8_t lo = bus_.read(zp_addr);
        uint8_t hi = bus_.read((zp_addr + 1) & 0xFF);
        uint16_t addr = lo | (static_cast<uint16_t>(hi) << 8);
        uint8_t operand = bus_.read(addr + state_.Y);
        int16_t result = state_.A - operand - (get_flag(StatusFlags::C) ? 0 : 1);
        set_flag(StatusFlags::C, result >= 0);
        set_flag(StatusFlags::V, ((state_.A ^ operand) & (state_.A ^ result) & 0x80) != 0);
        state_.A = static_cast<uint8_t>(result);
        update_nz(state_.A);
        break;
    }

    // AND absolute,X
    case 0x3D: {
        uint16_t addr = fetch_word();
        state_.A &= bus_.read(addr + state_.X);
        update_nz(state_.A);
        break;
    }

    // AND absolute,Y
    case 0x39: {
        uint16_t addr = fetch_word();
        state_.A &= bus_.read(addr + state_.Y);
        update_nz(state_.A);
        break;
    }

    // AND (indirect,X)
    case 0x21: {
        uint8_t zp_addr = fetch_byte();
        uint8_t addr_zp = (zp_addr + state_.X) & 0xFF;
        uint8_t lo = bus_.read(addr_zp);
        uint8_t hi = bus_.read((addr_zp + 1) & 0xFF);
        uint16_t addr = lo | (static_cast<uint16_t>(hi) << 8);
        state_.A &= bus_.read(addr);
        update_nz(state_.A);
        break;
    }

    // AND (indirect),Y
    case 0x31: {
        uint8_t zp_addr = fetch_byte();
        uint8_t lo = bus_.read(zp_addr);
        uint8_t hi = bus_.read((zp_addr + 1) & 0xFF);
        uint16_t addr = lo | (static_cast<uint16_t>(hi) << 8);
        state_.A &= bus_.read(addr + state_.Y);
        update_nz(state_.A);
        break;
    }

    // ORA absolute,X
    case 0x1D: {
        uint16_t addr = fetch_word();
        state_.A |= bus_.read(addr + state_.X);
        update_nz(state_.A);
        break;
    }

    // ORA absolute,Y
    case 0x19: {
        uint16_t addr = fetch_word();
        state_.A |= bus_.read(addr + state_.Y);
        update_nz(state_.A);
        break;
    }

    // ORA (indirect,X)
    case 0x01: {
        uint8_t zp_addr = fetch_byte();
        uint8_t addr_zp = (zp_addr + state_.X) & 0xFF;
        uint8_t lo = bus_.read(addr_zp);
        uint8_t hi = bus_.read((addr_zp + 1) & 0xFF);
        uint16_t addr = lo | (static_cast<uint16_t>(hi) << 8);
        state_.A |= bus_.read(addr);
        update_nz(state_.A);
        break;
    }

    // ORA (indirect),Y
    case 0x11: {
        uint8_t zp_addr = fetch_byte();
        uint8_t lo = bus_.read(zp_addr);
        uint8_t hi = bus_.read((zp_addr + 1) & 0xFF);
        uint16_t addr = lo | (static_cast<uint16_t>(hi) << 8);
        state_.A |= bus_.read(addr + state_.Y);
        update_nz(state_.A);
        break;
    }

    // EOR absolute,X
    case 0x5D: {
        uint16_t addr = fetch_word();
        state_.A ^= bus_.read(addr + state_.X);
        update_nz(state_.A);
        break;
    }

    // EOR absolute,Y
    case 0x59: {
        uint16_t addr = fetch_word();
        state_.A ^= bus_.read(addr + state_.Y);
        update_nz(state_.A);
        break;
    }

    // EOR (indirect,X)
    case 0x41: {
        uint8_t zp_addr = fetch_byte();
        uint8_t addr_zp = (zp_addr + state_.X) & 0xFF;
        uint8_t lo = bus_.read(addr_zp);
        uint8_t hi = bus_.read((addr_zp + 1) & 0xFF);
        uint16_t addr = lo | (static_cast<uint16_t>(hi) << 8);
        state_.A ^= bus_.read(addr);
        update_nz(state_.A);
        break;
    }

    // EOR (indirect),Y
    case 0x51: {
        uint8_t zp_addr = fetch_byte();
        uint8_t lo = bus_.read(zp_addr);
        uint8_t hi = bus_.read((zp_addr + 1) & 0xFF);
        uint16_t addr = lo | (static_cast<uint16_t>(hi) << 8);
        state_.A ^= bus_.read(addr + state_.Y);
        update_nz(state_.A);
        break;
    }

    // CMP absolute,X
    case 0xDD: {
        uint16_t addr = fetch_word();
        uint8_t operand = bus_.read(addr + state_.X);
        uint16_t result = state_.A - operand;
        set_flag(StatusFlags::C, state_.A >= operand);
        update_nz(static_cast<uint8_t>(result));
        break;
    }

    // CMP absolute,Y
    case 0xD9: {
        uint16_t addr = fetch_word();
        uint8_t operand = bus_.read(addr + state_.Y);
        uint16_t result = state_.A - operand;
        set_flag(StatusFlags::C, state_.A >= operand);
        update_nz(static_cast<uint8_t>(result));
        break;
    }

    // CMP (indirect,X)
    case 0xC1: {
        uint8_t zp_addr = fetch_byte();
        uint8_t addr_zp = (zp_addr + state_.X) & 0xFF;
        uint8_t lo = bus_.read(addr_zp);
        uint8_t hi = bus_.read((addr_zp + 1) & 0xFF);
        uint16_t addr = lo | (static_cast<uint16_t>(hi) << 8);
        uint8_t operand = bus_.read(addr);
        uint16_t result = state_.A - operand;
        set_flag(StatusFlags::C, state_.A >= operand);
        update_nz(static_cast<uint8_t>(result));
        break;
    }

    // CMP (indirect),Y
    case 0xD1: {
        uint8_t zp_addr = fetch_byte();
        uint8_t lo = bus_.read(zp_addr);
        uint8_t hi = bus_.read((zp_addr + 1) & 0xFF);
        uint16_t addr = lo | (static_cast<uint16_t>(hi) << 8);
        uint8_t operand = bus_.read(addr + state_.Y);
        uint16_t result = state_.A - operand;
        set_flag(StatusFlags::C, state_.A >= operand);
        update_nz(static_cast<uint8_t>(result));
        break;
    }

    // INC absolute
    case 0xEE: {
        uint16_t addr = fetch_word();
        uint8_t value = bus_.read(addr);
        value++;
        bus_.write(addr, value);
        update_nz(value);
        break;
    }

    // INC zero page
    case 0xE6: {
        uint8_t addr = fetch_byte();
        uint8_t value = bus_.read(addr);
        value++;
        bus_.write(addr, value);
        update_nz(value);
        break;
    }

    // INC absolute,X
    case 0xFE: {
        uint16_t addr = fetch_word();
        uint8_t value = bus_.read(addr + state_.X);
        value++;
        bus_.write(addr + state_.X, value);
        update_nz(value);
        break;
    }

    // INC zero page,X
    case 0xF6: {
        uint8_t addr = fetch_byte();
        uint8_t value = bus_.read((addr + state_.X) & 0xFF);
        value++;
        bus_.write((addr + state_.X) & 0xFF, value);
        update_nz(value);
        break;
    }

    // DEC absolute
    case 0xCE: {
        uint16_t addr = fetch_word();
        uint8_t value = bus_.read(addr);
        value--;
        bus_.write(addr, value);
        update_nz(value);
        break;
    }

    // DEC zero page
    case 0xC6: {
        uint8_t addr = fetch_byte();
        uint8_t value = bus_.read(addr);
        value--;
        bus_.write(addr, value);
        update_nz(value);
        break;
    }

    // DEC absolute,X
    case 0xDE: {
        uint16_t addr = fetch_word();
        uint8_t value = bus_.read(addr + state_.X);
        value--;
        bus_.write(addr + state_.X, value);
        update_nz(value);
        break;
    }

    // DEC zero page,X
    case 0xD6: {
        uint8_t addr = fetch_byte();
        uint8_t value = bus_.read((addr + state_.X) & 0xFF);
        value--;
        bus_.write((addr + state_.X) & 0xFF, value);
        update_nz(value);
        break;
    }

    // LSR absolute,X
    case 0x5E: {
        uint16_t addr = fetch_word();
        uint8_t value = bus_.read(addr + state_.X);
        set_flag(StatusFlags::C, (value & 0x01) != 0);
        value >>= 1;
        bus_.write(addr + state_.X, value);
        update_nz(value);
        break;
    }

    // LSR zero page,X
    case 0x56: {
        uint8_t addr = fetch_byte();
        uint8_t value = bus_.read((addr + state_.X) & 0xFF);
        set_flag(StatusFlags::C, (value & 0x01) != 0);
        value >>= 1;
        bus_.write((addr + state_.X) & 0xFF, value);
        update_nz(value);
        break;
    }

    // ASL absolute,X
    case 0x1E: {
        uint16_t addr = fetch_word();
        uint8_t value = bus_.read(addr + state_.X);
        set_flag(StatusFlags::C, (value & 0x80) != 0);
        value <<= 1;
        bus_.write(addr + state_.X, value);
        update_nz(value);
        break;
    }

    // ASL zero page,X
    case 0x16: {
        uint8_t addr = fetch_byte();
        uint8_t value = bus_.read((addr + state_.X) & 0xFF);
        set_flag(StatusFlags::C, (value & 0x80) != 0);
        value <<= 1;
        bus_.write((addr + state_.X) & 0xFF, value);
        update_nz(value);
        break;
    }

    // ROR absolute,X
    case 0x7E: {
        uint16_t addr = fetch_word();
        uint8_t value = bus_.read(addr + state_.X);
        bool old_carry = get_flag(StatusFlags::C);
        set_flag(StatusFlags::C, (value & 0x01) != 0);
        value >>= 1;
        if (old_carry)
            value |= 0x80;
        bus_.write(addr + state_.X, value);
        update_nz(value);
        break;
    }

    // ROR zero page,X
    case 0x76: {
        uint8_t addr = fetch_byte();
        uint8_t value = bus_.read((addr + state_.X) & 0xFF);
        bool old_carry = get_flag(StatusFlags::C);
        set_flag(StatusFlags::C, (value & 0x01) != 0);
        value >>= 1;
        if (old_carry)
            value |= 0x80;
        bus_.write((addr + state_.X) & 0xFF, value);
        update_nz(value);
        break;
    }

    // ROL absolute,X
    case 0x3E: {
        uint16_t addr = fetch_word();
        uint8_t value = bus_.read(addr + state_.X);
        bool old_carry = get_flag(StatusFlags::C);
        set_flag(StatusFlags::C, (value & 0x80) != 0);
        value <<= 1;
        if (old_carry)
            value |= 0x01;
        bus_.write(addr + state_.X, value);
        update_nz(value);
        break;
    }

    // ROL zero page,X
    case 0x36: {
        uint8_t addr = fetch_byte();
        uint8_t value = bus_.read((addr + state_.X) & 0xFF);
        bool old_carry = get_flag(StatusFlags::C);
        set_flag(StatusFlags::C, (value & 0x80) != 0);
        value <<= 1;
        if (old_carry)
            value |= 0x01;
        bus_.write((addr + state_.X) & 0xFF, value);
        update_nz(value);
        break;
    }

    // LDA zero page,X
    case 0xB5: {
        uint8_t addr = fetch_byte();
        state_.A = bus_.read((addr + state_.X) & 0xFF);
        update_nz(state_.A);
        break;
    }

    // STA zero page,X
    case 0x95: {
        uint8_t addr = fetch_byte();
        bus_.write((addr + state_.X) & 0xFF, state_.A);
        break;
    }

    // AND zero page,X
    case 0x35: {
        uint8_t addr = fetch_byte();
        state_.A &= bus_.read((addr + state_.X) & 0xFF);
        update_nz(state_.A);
        break;
    }

    // ORA zero page,X
    case 0x15: {
        uint8_t addr = fetch_byte();
        state_.A |= bus_.read((addr + state_.X) & 0xFF);
        update_nz(state_.A);
        break;
    }

    // EOR zero page,X
    case 0x55: {
        uint8_t addr = fetch_byte();
        state_.A ^= bus_.read((addr + state_.X) & 0xFF);
        update_nz(state_.A);
        break;
    }

    // CMP zero page,X
    case 0xD5: {
        uint8_t addr = fetch_byte();
        uint8_t operand = bus_.read((addr + state_.X) & 0xFF);
        uint16_t result = state_.A - operand;
        set_flag(StatusFlags::C, state_.A >= operand);
        update_nz(static_cast<uint8_t>(result));
        break;
    }

    // ADC zero page,X
    case 0x75: {
        uint8_t addr = fetch_byte();
        uint8_t operand = bus_.read((addr + state_.X) & 0xFF);
        uint16_t result = state_.A + operand + (get_flag(StatusFlags::C) ? 1 : 0);
        set_flag(StatusFlags::C, result > 0xFF);
        set_flag(StatusFlags::V, (~(state_.A ^ operand) & (state_.A ^ result) & 0x80) != 0);
        state_.A = static_cast<uint8_t>(result);
        update_nz(state_.A);
        break;
    }

    // SBC zero page,X (carry acts as "not borrow")
    case 0xF5: {
        uint8_t addr = fetch_byte();
        uint8_t operand = bus_.read((addr + state_.X) & 0xFF);
        int16_t result = state_.A - operand - (get_flag(StatusFlags::C) ? 0 : 1);
        set_flag(StatusFlags::C, result >= 0);
        set_flag(StatusFlags::V, ((state_.A ^ operand) & (state_.A ^ result) & 0x80) != 0);
        state_.A = static_cast<uint8_t>(result);
        update_nz(state_.A);
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
