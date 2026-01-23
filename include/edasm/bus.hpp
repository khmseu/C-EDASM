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

// Memory bus for 6502/65C02
class Bus {
  public:
    static constexpr size_t MEMORY_SIZE = 0x10000; // 64KB
    static constexpr uint8_t TRAP_OPCODE = 0x02;   // Opcode for host traps

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
    uint8_t *data() {
        return memory_.data();
    }
    const uint8_t *data() const {
        return memory_.data();
    }

  private:
    std::array<uint8_t, MEMORY_SIZE> memory_;
    std::array<ReadTrapHandler, MEMORY_SIZE> read_traps_;
    std::array<WriteTrapHandler, MEMORY_SIZE> write_traps_;
};

} // namespace edasm

#endif // EDASM_BUS_HPP
