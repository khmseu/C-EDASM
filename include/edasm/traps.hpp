#ifndef EDASM_TRAPS_HPP
#define EDASM_TRAPS_HPP

#include "cpu.hpp"
#include "bus.hpp"
#include <cstdint>
#include <string>
#include <vector>

namespace edasm {

// Trap manager for opcode and I/O traps
class TrapManager {
public:
    TrapManager();
    
    // Default trap handler: log and halt
    static bool default_trap_handler(CPUState& cpu, Bus& bus, uint16_t trap_pc);
    
    // Create a logging trap handler
    static TrapHandler create_logging_handler(const std::string& name);
    
    // Log CPU state and memory window
    static void log_cpu_state(const CPUState& cpu, const Bus& bus, uint16_t pc);
    static void log_memory_window(const Bus& bus, uint16_t addr, size_t size = 16);
    
    // Dump state to string
    static std::string dump_cpu_state(const CPUState& cpu);
    static std::string dump_memory(const Bus& bus, uint16_t addr, size_t size = 16);
};

} // namespace edasm

#endif // EDASM_TRAPS_HPP
