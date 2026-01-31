#ifndef EDASM_BUS_HPP
#define EDASM_BUS_HPP

#include <array>
#include <cstdint>
#include <functional>
#include <string>
#include <vector>

namespace edasm {

// Trap callback types
// Returns true to allow normal memory access, false to block it
using ReadTrapHandler = std::function<bool(uint16_t addr, uint8_t &value)>;
using WriteTrapHandler = std::function<bool(uint16_t addr, uint8_t value)>;

// Trap range entry - stores a range of addresses with a single handler
struct TrapRange {
    uint16_t start;
    uint16_t end;
    ReadTrapHandler handler;

    bool contains(uint16_t addr) const {
        return addr >= start && addr <= end;
    }
};

struct WriteTrapRange {
    uint16_t start;
    uint16_t end;
    WriteTrapHandler handler;

    bool contains(uint16_t addr) const {
        return addr >= start && addr <= end;
    }
};

// Memory bus for 6502/65C02
class Bus {
  public:
    static constexpr size_t MEMORY_SIZE = 0x10000; // 64KB - main 6502 address space
    static constexpr uint8_t TRAP_OPCODE = 0x02;   // Opcode for host traps
    
    // Language card extended memory regions (beyond main 64KB)
    static constexpr size_t LC_BANK1_OFFSET = 0x10000; // First 4KB bank at offset 64KB
    static constexpr size_t LC_BANK2_OFFSET = 0x11000; // Second 4KB bank at offset 68KB
    static constexpr size_t LC_FIXED_RAM_OFFSET = 0x12000; // 8KB fixed RAM at offset 72KB
    static constexpr size_t LC_ROM_OFFSET = 0x14000; // 12KB ROM at offset 80KB
    static constexpr size_t TOTAL_MEMORY_SIZE = 0x17000; // 92KB total (64KB + 28KB LC)

    Bus();

    // Memory operations
    uint8_t read(uint16_t addr);
    void write(uint16_t addr, uint8_t value);

    // Read 16-bit word (little-endian)
    uint16_t read_word(uint16_t addr);
    void write_word(uint16_t addr, uint16_t value);

    // Load binary data at specific address
    bool load_binary(uint16_t addr, const std::vector<uint8_t> &data);
    bool load_binary_from_file(uint16_t addr, const std::string &filename);

    // Reset memory to trap opcode
    void reset();

    // Set read/write trap handlers for specific address or range
    void set_read_trap(uint16_t addr, ReadTrapHandler handler);
    void set_write_trap(uint16_t addr, WriteTrapHandler handler);
    void set_read_trap_range(uint16_t start, uint16_t end, ReadTrapHandler handler);
    void set_write_trap_range(uint16_t start, uint16_t end, WriteTrapHandler handler);

    // Clear trap handlers
    void clear_read_traps();
    void clear_write_traps();

    // Direct memory access (bypasses traps)
    // Only provides access to main 64KB for compatibility
    uint8_t *data() {
        return memory_.data();
    }
    const uint8_t *data() const {
        return memory_.data();
    }
    
    // Direct access to extended memory regions (for language card implementation)
    uint8_t *lc_bank1() {
        return memory_.data() + LC_BANK1_OFFSET;
    }
    uint8_t *lc_bank2() {
        return memory_.data() + LC_BANK2_OFFSET;
    }
    uint8_t *lc_fixed_ram() {
        return memory_.data() + LC_FIXED_RAM_OFFSET;
    }
    uint8_t *lc_rom() {
        return memory_.data() + LC_ROM_OFFSET;
    }

  private:
    // Extended memory buffer: 64KB main + 28KB language card regions
    std::array<uint8_t, TOTAL_MEMORY_SIZE> memory_;
    
    // Sparse trap storage - only store ranges that have handlers
    std::vector<TrapRange> read_trap_ranges_;
    std::vector<WriteTrapRange> write_trap_ranges_;
    
    // Helper to find trap handler for an address
    ReadTrapHandler find_read_trap(uint16_t addr);
    WriteTrapHandler find_write_trap(uint16_t addr);
};

} // namespace edasm

#endif // EDASM_BUS_HPP
