#ifndef EDASM_TRAPS_HPP
#define EDASM_TRAPS_HPP

#include "bus.hpp"
#include "cpu.hpp"
#include <cstdint>
#include <map>
#include <string>

namespace edasm {

// Trap manager for opcode and I/O traps
class TrapManager {
  public:
    TrapManager();

    // Set trace mode (enables detailed logging)
    static void set_trace(bool enabled);
    static bool is_trace_enabled();

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

    // ProDOS MLI trap handler: forwards to MLIHandler (for compatibility)
    static bool prodos_mli_trap_handler(CPUState &cpu, Bus &bus, uint16_t trap_pc);

    // Monitor ROM trap handler: SETNORM ($FE84) - sets InvFlg ($32) to $FF
    static bool monitor_setnorm_trap_handler(CPUState &cpu, Bus &bus, uint16_t trap_pc);

    // Create a logging trap handler
    static TrapHandler create_logging_handler(const std::string &name);

    // Log CPU state and memory window
    static void log_cpu_state(const CPUState &cpu, const Bus &bus, uint16_t pc);
    static void log_memory_window(const Bus &bus, uint16_t addr, size_t size = 16);

    // Dump state to string
    static std::string dump_cpu_state(const CPUState &cpu);
    static std::string dump_memory(const Bus &bus, uint16_t addr, size_t size = 16);

    // Write full memory dump to binary file
    static bool write_memory_dump(const Bus &bus, const std::string &filename);

  private:
    // Registry of address-specific trap handlers
    static std::map<uint16_t, TrapHandler> &get_handler_registry();

    // Trace mode flag
    static bool s_trace_enabled;
};

} // namespace edasm

#endif // EDASM_TRAPS_HPP
