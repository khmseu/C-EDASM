/**
 * @file traps.hpp
 * @brief Trap management and statistics for emulator
 * 
 * Provides trap handler registration, dispatch, and statistics tracking.
 * Traps are used for incremental discovery of system calls and I/O operations.
 * 
 * Features:
 * - Address-specific trap handlers (opcode $02)
 * - Read/write memory traps
 * - Trap statistics and reporting
 * - Default handlers for common operations
 * 
 * Reference: docs/EMULATOR_MINIMAL_PLAN.md
 */

#ifndef EDASM_TRAPS_HPP
#define EDASM_TRAPS_HPP

#include "bus.hpp"
#include "cpu.hpp"
#include <cstdint>
#include <map>
#include <string>
#include <vector>

namespace edasm {

// Trap kinds for statistics
enum class TrapKind {
    CALL,       // Opcode trap (call/JSR to trap address)
    READ,       // Memory read trap
    WRITE,      // Memory write trap
    DOUBLE_READ // Memory read trap that triggers twice
};

// Information about a single trap occurrence
struct TrapStatistic {
    std::string name;     // Trap name (e.g., "ProDOS MLI", "KBD", etc.)
    uint16_t address;     // Trap address
    TrapKind kind;        // Type of trap
    uint64_t count;       // Number of times this trap was triggered
    std::string mli_call; // For MLI traps: which MLI call (e.g., "OPEN", "READ")
    bool is_second_read;  // For double-read traps: true if second read

    TrapStatistic(const std::string &n, uint16_t addr, TrapKind k)
        : name(n), address(addr), kind(k), count(0), mli_call(""), is_second_read(false) {}
};

// Trap statistics manager
class TrapStatistics {
  public:
    // Record a trap occurrence
    static void record_trap(const std::string &name, uint16_t address, TrapKind kind,
                            const std::string &mli_call = "", bool is_second_read = false);

    // Print statistics table to stdout, ordered by trap address
    static void print_statistics();

    // Clear all statistics
    static void clear();

  private:
    static std::vector<TrapStatistic> &get_statistics();
};

// Trap manager for opcode and I/O traps
class TrapManager {
  public:
    TrapManager();

    // Set trace mode (enables detailed logging)
    static void set_trace(bool enabled);
    static bool is_trace_enabled();

    // Install a specific trap handler for a given address with optional name
    static void install_address_handler(uint16_t address, TrapHandler handler,
                                        const std::string &name = "");

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

    // Registry of trap names for statistics
    static std::map<uint16_t, std::string> &get_name_registry();

    // Trace mode flag
    static bool s_trace_enabled;
};

} // namespace edasm

#endif // EDASM_TRAPS_HPP
