#include "edasm/emulator/bus.hpp"
#include <algorithm>
#include <fstream>

namespace edasm {

Bus::Bus() {
    reset();
}

void Bus::reset() {
    // Fill entire address space with trap opcode
    memory_.fill(TRAP_OPCODE);

    // Clear trap handlers
    clear_read_traps();
    clear_write_traps();
}

uint8_t Bus::read(uint16_t addr) {
    // Check for read trap handler
    if (read_traps_[addr]) {
        uint8_t value = 0;
        if (read_traps_[addr](addr, value)) {
            return value; // Trap handled, return provided value
        }
    }

    // Normal memory read
    return memory_[addr];
}

void Bus::write(uint16_t addr, uint8_t value) {
    // Check for write trap handler
    if (write_traps_[addr]) {
        if (write_traps_[addr](addr, value)) {
            return; // Trap handled, don't write to memory
        }
    }

    // Normal memory write
    memory_[addr] = value;
}

uint16_t Bus::read_word(uint16_t addr) {
    // Note: On 6502, word reads wrap within page for zero page addresses
    // For simplicity, we allow reads across page boundaries here
    uint8_t lo = read(addr);
    uint8_t hi = read(static_cast<uint16_t>(addr + 1)); // Cast handles overflow to 0x0000
    return static_cast<uint16_t>(lo) | (static_cast<uint16_t>(hi) << 8);
}

void Bus::write_word(uint16_t addr, uint16_t value) {
    write(addr, static_cast<uint8_t>(value & 0xFF));
    write(static_cast<uint16_t>(addr + 1),
          static_cast<uint8_t>((value >> 8) & 0xFF)); // Cast handles overflow
}

bool Bus::load_binary(uint16_t addr, const std::vector<uint8_t> &data) {
    if (addr + data.size() > MEMORY_SIZE) {
        return false; // Would overflow memory
    }

    // Copy data directly to memory (bypassing traps)
    std::copy(data.begin(), data.end(), memory_.begin() + addr);
    return true;
}

bool Bus::load_binary_from_file(uint16_t addr, const std::string &filename) {
    std::ifstream file(filename, std::ios::binary | std::ios::ate);
    if (!file) {
        return false;
    }

    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);

    std::vector<uint8_t> buffer(size);
    if (!file.read(reinterpret_cast<char *>(buffer.data()), size)) {
        return false;
    }

    return load_binary(addr, buffer);
}

void Bus::set_read_trap(uint16_t addr, ReadTrapHandler handler) {
    read_traps_[addr] = handler;
}

void Bus::set_write_trap(uint16_t addr, WriteTrapHandler handler) {
    write_traps_[addr] = handler;
}

void Bus::set_read_trap_range(uint16_t start, uint16_t end, ReadTrapHandler handler) {
    // Handle potential overflow when end = 0xFFFF
    for (uint32_t addr = start; addr <= end; ++addr) {
        read_traps_[addr] = handler;
    }
}

void Bus::set_write_trap_range(uint16_t start, uint16_t end, WriteTrapHandler handler) {
    // Handle potential overflow when end = 0xFFFF
    for (uint32_t addr = start; addr <= end; ++addr) {
        write_traps_[addr] = handler;
    }
}

void Bus::clear_read_traps() {
    read_traps_.fill(nullptr);
}

void Bus::clear_write_traps() {
    write_traps_.fill(nullptr);
}

} // namespace edasm
