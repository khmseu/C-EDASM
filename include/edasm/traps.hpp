#ifndef EDASM_TRAPS_HPP
#define EDASM_TRAPS_HPP

#include "bus.hpp"
#include "cpu.hpp"
#include <cstdint>
#include <functional>
#include <map>
#include <string>
#include <vector>

namespace edasm {

// Trap manager for opcode and I/O traps
class TrapManager {
  public:
    TrapManager();

    // Install a specific trap handler for a given address
    static void install_address_handler(uint16_t address, TrapHandler handler);

    // Clear specific handler for an address
    static void clear_address_handler(uint16_t address);

    // Clear all address-specific handlers
    static void clear_all_handlers();

    // General trap handler: checks for address-specific handlers, falls back to default
    static bool general_trap_handler(CPUState &cpu, Bus &bus, uint16_t trap_pc);

    // Default trap handler: log and halt
    static bool default_trap_handler(CPUState &cpu, Bus &bus, uint16_t trap_pc);

    // ProDOS MLI trap handler: decode and log MLI calls (only for $BF00)
    static bool prodos_mli_trap_handler(CPUState &cpu, Bus &bus, uint16_t trap_pc);

    // Create a logging trap handler
    static TrapHandler create_logging_handler(const std::string &name);

    // Log CPU state and memory window
    static void log_cpu_state(const CPUState &cpu, const Bus &bus, uint16_t pc);
    static void log_memory_window(const Bus &bus, uint16_t addr, size_t size = 16);

    // Dump state to string
    static std::string dump_cpu_state(const CPUState &cpu);
    static std::string dump_memory(const Bus &bus, uint16_t addr, size_t size = 16);

  private:
    // Helper for ProDOS MLI decoding
    static std::string decode_prodos_call(uint8_t call_num);

    // Registry of address-specific trap handlers
    static std::map<uint16_t, TrapHandler> &get_handler_registry();
};

} // namespace edasm

#endif // EDASM_TRAPS_HPP
