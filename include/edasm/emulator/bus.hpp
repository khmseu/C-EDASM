/**
 * @file bus.hpp
 * @brief Memory bus for 65C02 emulator
 *
 * Implements 64KB address space with bank switching for Apple II language card.
 * Supports memory traps for incremental system call discovery.
 *
 * Memory layout:
 * - 64KB main RAM (address space $0000-$FFFF)
 * - 16KB language card RAM (bank-switched into $D000-$FFFF)
 * - 2KB write-sink for ROM writes
 * - Total: 82KB physical memory
 *
 * Features:
 * - Read/write traps for address ranges
 * - Bank mapping for language card emulation
 * - Trap opcode ($02) initialization for discovery
 *
 * Reference: docs/EMULATOR_MINIMAL_PLAN.md
 */

#ifndef EDASM_BUS_HPP
#define EDASM_BUS_HPP

#include <array>
#include <cstdint>
#include <functional>
#include <span>
#include <string>
#include <vector>

namespace edasm {

// Memory range mapping - uses std::span to provide direct access to physical memory
// Represents a contiguous physical memory range corresponding to 6502 addresses
// Split into read-only and writable variants for const-correctness
using ReadMemoryRange = std::span<const uint8_t>;
using WriteMemoryRange = std::span<uint8_t>;

// Trap callback types
// Returns true to allow normal memory access, false to block it
using ReadTrapHandler = std::function<bool(uint16_t addr, uint8_t &value)>;
using WriteTrapHandler = std::function<bool(uint16_t addr, uint8_t value)>;

// Trap range entry - stores a range of addresses with a single handler
struct ReadTrapRange {
    uint16_t start;
    uint16_t end;
    ReadTrapHandler handler;
    std::string name; // Name for statistics tracking

    bool contains(uint16_t addr) const {
        return addr >= start && addr <= end;
    }
};

struct WriteTrapRange {
    uint16_t start;
    uint16_t end;
    WriteTrapHandler handler;
    std::string name; // Name for statistics tracking

    bool contains(uint16_t addr) const {
        return addr >= start && addr <= end;
    }
};

// Memory bus for 6502/65C02
class Bus {
  public:
    static constexpr size_t MEMORY_SIZE = 0x10000; // 64KB - main 6502 address space
    static constexpr uint8_t TRAP_OPCODE = 0x02;   // Opcode for host traps

    // Bank-based memory system: 64K address space divided into 2K banks (32 banks)
    static constexpr size_t BANK_SIZE = 0x0800; // 2KB per bank
    static constexpr size_t NUM_BANKS = 32;     // 64KB / 2KB = 32 banks

    // Memory pool layout: 64K main + 16K LC + 2K write-sink = 82K total
    // The 16K LC area contains: 4KB bank1 + 4KB bank2 + 8KB fixed RAM (12KB ROM uses same space as
    // banks)
    static constexpr size_t MAIN_RAM_SIZE = 0x10000; // 64KB main RAM
    static constexpr size_t LC_RAM_SIZE =
        0x4000; // 16KB language card RAM (2x4KB banks + 8KB fixed)
    static constexpr size_t WRITE_SINK_SIZE = 0x0800; // 2KB write-ignore sink
    static constexpr size_t TOTAL_MEMORY_SIZE =
        MAIN_RAM_SIZE + LC_RAM_SIZE + WRITE_SINK_SIZE; // 82KB

    // Offsets within memory pool
    static constexpr size_t MAIN_RAM_OFFSET = 0x00000;     // Main RAM starts at 0
    static constexpr size_t LC_BANK1_OFFSET = 0x10000;     // First 4KB bank at offset 64KB
    static constexpr size_t LC_BANK2_OFFSET = 0x11000;     // Second 4KB bank at offset 68KB
    static constexpr size_t LC_FIXED_RAM_OFFSET = 0x12000; // 8KB fixed RAM at offset 72KB
    static constexpr size_t WRITE_SINK_OFFSET = 0x14000;   // 2KB write-sink at offset 80KB

    Bus();

    // Address translation - converts 6502 address ranges to physical memory spans
    // Returns a vector of memory spans that cover the requested range
    // Filtered through bank switching mechanism (read vs write may differ)
    // Note: length can be larger than 64KB for special operations like memory dumps
    std::vector<ReadMemoryRange> translate_read_range(uint16_t start_addr, size_t length) const;
    std::vector<WriteMemoryRange> translate_write_range(uint16_t start_addr, size_t length);

    // Memory operations
    uint8_t read(uint16_t addr) const;
    void write(uint16_t addr, uint8_t value);

    // Read 16-bit word (little-endian)
    uint16_t read_word(uint16_t addr) const;
    void write_word(uint16_t addr, uint16_t value);

    // Write full memory dump to binary file
    bool write_memory_dump(const std::string &filename) const;

    // Load binary data at specific address
    bool initialize_memory(uint16_t addr, const std::vector<uint8_t> &data);
    bool write_binary_data(uint16_t addr, const std::vector<uint8_t> &data);

    // Load ROM from file at startup (bypasses bank switching)
    bool load_rom_from_file(uint16_t addr, const std::string &filename);

    // Load binary from file at runtime (respects bank switching)
    bool load_binary_from_file(uint16_t addr, const std::string &filename);

    // Reset memory to trap opcode
    void reset();

    // Set read/write trap handlers for address ranges
    void set_read_trap_range(uint16_t start, uint16_t end, ReadTrapHandler handler,
                             const std::string &name = "");
    void set_write_trap_range(uint16_t start, uint16_t end, WriteTrapHandler handler,
                              const std::string &name = "");

    // Clear trap handlers
    void clear_read_traps();
    void clear_write_traps();

    // Language card control - updates bank mapping tables
    void set_bank_mapping(uint8_t bank_index, uint32_t read_offset, uint32_t write_offset);
    void reset_bank_mappings();

  private:
    // Memory pool: 82KB total
    std::array<uint8_t, TOTAL_MEMORY_SIZE> memory_;

    // Bank lookup tables: for each 2KB bank, store offset to read/write from
    // Using 32-bit offsets to address full 82KB pool
    std::array<uint32_t, NUM_BANKS> read_bank_offsets_;
    std::array<uint32_t, NUM_BANKS> write_bank_offsets_;

    // Sparse trap storage - only store ranges that have handlers
    std::vector<ReadTrapRange> read_trap_ranges_;
    std::vector<WriteTrapRange> write_trap_ranges_;

    // Helper to find trap handler for an address
    ReadTrapHandler find_read_trap(uint16_t addr);
    WriteTrapHandler find_write_trap(uint16_t addr);

    // Helper to find trap range (including name) for an address
    const ReadTrapRange *find_read_trap_range(uint16_t addr) const;
    const WriteTrapRange *find_write_trap_range(uint16_t addr) const;
};

} // namespace edasm

#endif // EDASM_BUS_HPP
