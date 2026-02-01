#include "edasm/emulator/bus.hpp"
#include "edasm/emulator/traps.hpp"
#include <algorithm>
#include <fstream>

namespace edasm {

Bus::Bus() {
    reset();
}

void Bus::reset() {
    // Fill entire memory pool with trap opcode
    memory_.fill(TRAP_OPCODE);

    // Initialize bank mappings to default (power-on state)
    reset_bank_mappings();

    // Clear trap handlers
    clear_read_traps();
    clear_write_traps();
}

void Bus::reset_bank_mappings() {
    // Power-on state per problem statement:
    // - 0000-CFFF (banks 0-25): point to main RAM
    // - D000-FFFF (banks 26-31): point to main RAM for reads (where ROM will be loaded),
    //   write-sink for writes (ROM is write-protected)

    for (size_t i = 0; i < NUM_BANKS; ++i) {
        uint32_t bank_start = static_cast<uint32_t>(i * BANK_SIZE);

        if (bank_start < 0xD000) {
            // Banks 0-25 (0x0000-0xCFFF): map to main RAM
            read_bank_offsets_[i] = MAIN_RAM_OFFSET + bank_start;
            write_bank_offsets_[i] = MAIN_RAM_OFFSET + bank_start;
        } else {
            // Banks 26-31 (0xD000-0xFFFF): at power-on, map to main RAM for reads (ROM location),
            // write-sink for writes (ROM is read-only)
            read_bank_offsets_[i] = MAIN_RAM_OFFSET + bank_start;
            write_bank_offsets_[i] = WRITE_SINK_OFFSET; // Writes go to sink (ignored)
        }
    }
}

ReadTrapHandler Bus::find_read_trap(uint16_t addr) {
    // Search through trap ranges to find a handler for this address
    for (const auto &range : read_trap_ranges_) {
        if (range.contains(addr)) {
            return range.handler;
        }
    }
    return nullptr;
}

WriteTrapHandler Bus::find_write_trap(uint16_t addr) {
    // Search through trap ranges to find a handler for this address
    for (const auto &range : write_trap_ranges_) {
        if (range.contains(addr)) {
            return range.handler;
        }
    }
    return nullptr;
}

uint8_t Bus::read(uint16_t addr) {
    // Check for read trap handler first (for C000-C7FF and screen 400-7FF)
    const ReadTrapRange *trap_range = find_read_trap_range(addr);
    if (trap_range) {
        // Record trap statistic
        std::string trap_name = trap_range->name.empty() ? "READ" : trap_range->name;
        TrapStatistics::record_trap(trap_name, addr, TrapKind::READ);
        
        uint8_t value = 0;
        if (trap_range->handler(addr, value)) {
            return value; // Trap handled, return provided value
        }
    }

    // Use bank-based lookup for normal reads
    uint8_t bank_index = addr / BANK_SIZE;      // Divide by 2KB to get bank number
    uint32_t offset_in_bank = addr % BANK_SIZE; // Offset within the bank
    uint32_t physical_offset = read_bank_offsets_[bank_index] + offset_in_bank;

    return memory_[physical_offset];
}

void Bus::write(uint16_t addr, uint8_t value) {
    // Check for write trap handler first (for C000-C7FF and screen 400-7FF)
    const WriteTrapRange *trap_range = find_write_trap_range(addr);
    if (trap_range) {
        // Record trap statistic
        std::string trap_name = trap_range->name.empty() ? "WRITE" : trap_range->name;
        TrapStatistics::record_trap(trap_name, addr, TrapKind::WRITE);
        
        if (trap_range->handler(addr, value)) {
            return; // Trap handled, don't write to memory
        }
    }

    // Use bank-based lookup for normal writes
    uint8_t bank_index = addr / BANK_SIZE;      // Divide by 2KB to get bank number
    uint32_t offset_in_bank = addr % BANK_SIZE; // Offset within the bank
    uint32_t physical_offset = write_bank_offsets_[bank_index] + offset_in_bank;

    memory_[physical_offset] = value;
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

void Bus::set_read_trap(uint16_t addr, ReadTrapHandler handler, const std::string &name) {
    set_read_trap_range(addr, addr, handler, name);
}

void Bus::set_write_trap(uint16_t addr, WriteTrapHandler handler, const std::string &name) {
    set_write_trap_range(addr, addr, handler, name);
}

void Bus::set_read_trap_range(uint16_t start, uint16_t end, ReadTrapHandler handler,
                              const std::string &name) {
    // Add a new trap range
    read_trap_ranges_.push_back({start, end, handler, name});
}

void Bus::set_write_trap_range(uint16_t start, uint16_t end, WriteTrapHandler handler,
                               const std::string &name) {
    // Add a new trap range
    write_trap_ranges_.push_back({start, end, handler, name});
}

void Bus::clear_read_traps() {
    read_trap_ranges_.clear();
}

void Bus::clear_write_traps() {
    write_trap_ranges_.clear();
}

const ReadTrapRange *Bus::find_read_trap_range(uint16_t addr) const {
    for (const auto &range : read_trap_ranges_) {
        if (range.contains(addr)) {
            return &range;
        }
    }
    return nullptr;
}

const WriteTrapRange *Bus::find_write_trap_range(uint16_t addr) const {
    for (const auto &range : write_trap_ranges_) {
        if (range.contains(addr)) {
            return &range;
        }
    }
    return nullptr;
}

void Bus::set_bank_mapping(uint8_t bank_index, uint32_t read_offset, uint32_t write_offset) {
    if (bank_index >= NUM_BANKS) {
        return; // Invalid bank index
    }
    read_bank_offsets_[bank_index] = read_offset;
    write_bank_offsets_[bank_index] = write_offset;
}

} // namespace edasm
