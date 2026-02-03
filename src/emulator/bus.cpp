/**
 * @file bus.cpp
 * @brief Memory bus implementation with bank switching
 *
 * Implements 64KB address space with language card bank switching and
 * memory traps for I/O emulation.
 */

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

std::vector<ReadMemoryRange> Bus::translate_read_range(uint16_t start_addr, size_t length) const {
    std::vector<ReadMemoryRange> ranges;
    if (length == 0) {
        return ranges;
    }

    uint16_t current_addr = start_addr;
    size_t remaining = length;

    while (remaining > 0) {
        // Calculate which bank this address falls in
        uint8_t bank_index = current_addr / BANK_SIZE;
        uint32_t offset_in_bank = current_addr % BANK_SIZE;
        uint32_t physical_offset = read_bank_offsets_[bank_index] + offset_in_bank;

        // How many bytes can we read from this bank before crossing to next?
        size_t bytes_in_this_bank = BANK_SIZE - offset_in_bank;
        size_t bytes_to_read = std::min(remaining, bytes_in_this_bank);

        // Check if this continues the previous range or starts a new one
        if (!ranges.empty() &&
            ranges.back().data() + ranges.back().size() == memory_.data() + physical_offset) {
            // Extend the previous range by creating a new span from the start
            const uint8_t *start = ranges.back().data();
            size_t new_size = ranges.back().size() + bytes_to_read;
            ranges.back() = std::span<const uint8_t>(start, new_size);
        } else {
            // Start a new range
            ranges.push_back(
                std::span<const uint8_t>(memory_.data() + physical_offset, bytes_to_read));
        }

        // Handle address wraparound at 64KB boundary
        current_addr = static_cast<uint16_t>(current_addr + bytes_to_read);
        remaining -= bytes_to_read;

        // If we wrapped around to 0, we've covered the entire address space
        if (current_addr == 0 && remaining > 0) {
            // Continue from address 0 for remaining bytes
        }
    }

    return ranges;
}

std::vector<WriteMemoryRange> Bus::translate_write_range(uint16_t start_addr, size_t length) {
    std::vector<WriteMemoryRange> ranges;
    if (length == 0) {
        return ranges;
    }

    uint16_t current_addr = start_addr;
    size_t remaining = length;

    while (remaining > 0) {
        // Calculate which bank this address falls in
        uint8_t bank_index = current_addr / BANK_SIZE;
        uint32_t offset_in_bank = current_addr % BANK_SIZE;
        uint32_t physical_offset = write_bank_offsets_[bank_index] + offset_in_bank;

        // How many bytes can we write to this bank before crossing to next?
        size_t bytes_in_this_bank = BANK_SIZE - offset_in_bank;
        size_t bytes_to_write = std::min(remaining, bytes_in_this_bank);

        // Check if this continues the previous range or starts a new one
        if (!ranges.empty() &&
            ranges.back().data() + ranges.back().size() == memory_.data() + physical_offset) {
            // Extend the previous range by creating a new span from the start
            uint8_t *start = ranges.back().data();
            size_t new_size = ranges.back().size() + bytes_to_write;
            ranges.back() = std::span<uint8_t>(start, new_size);
        } else {
            // Start a new range
            ranges.push_back(std::span<uint8_t>(memory_.data() + physical_offset, bytes_to_write));
        }

        // Handle address wraparound at 64KB boundary
        current_addr = static_cast<uint16_t>(current_addr + bytes_to_write);
        remaining -= bytes_to_write;

        // If we wrapped around to 0, we've covered the entire address space
        if (current_addr == 0 && remaining > 0) {
            // Continue from address 0 for remaining bytes
        }
    }

    return ranges;
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

uint8_t Bus::read(uint16_t addr) const {
    // Check for read trap handler first (for C000-C7FF and screen 400-7FF)
    const ReadTrapRange *trap_range = find_read_trap_range(addr);
    if (trap_range) {
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

uint16_t Bus::read_word(uint16_t addr) const {
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

bool Bus::initialize_memory(uint16_t addr, const std::vector<uint8_t> &data) {
    if (data.empty()) {
        return true; // Nothing to load
    }

    if (addr + data.size() > MEMORY_SIZE) {
        return false; // Would overflow address space
    }

    // Load binary data directly to physical main RAM, bypassing bank mappings
    // This is essential for loading ROM images at reset, since the power-on
    // state routes writes to 0xD000-0xFFFF to the write-sink (ROM is read-only)
    for (size_t i = 0; i < data.size(); ++i) {
        uint16_t target_addr = static_cast<uint16_t>(addr + i);
        uint32_t physical_offset = MAIN_RAM_OFFSET + target_addr;
        memory_[physical_offset] = data[i];
    }

    return true;
}

bool Bus::write_binary_data(uint16_t addr, const std::vector<uint8_t> &data) {
    if (data.empty()) {
        return true; // Nothing to load
    }

    if (addr + data.size() > MEMORY_SIZE) {
        return false; // Would overflow address space
    }

    // Use write translation to handle bank switching correctly
    // This respects the bank mapping but bypasses traps
    auto ranges = translate_write_range(addr, static_cast<uint16_t>(data.size()));

    size_t data_offset = 0;
    for (auto &range : ranges) {
        // Copy this portion of data to the physical memory location
        std::copy(data.begin() + data_offset, data.begin() + data_offset + range.size(),
                  range.begin());
        data_offset += range.size();
    }

    return true;
}
bool Bus::load_rom_from_file(uint16_t addr, const std::string &filename) {
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

    // Load ROM at startup, bypassing bank switching
    return initialize_memory(addr, buffer);
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

    // Load binary at runtime, respecting bank switching
    return write_binary_data(addr, buffer);
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
