#include "edasm/traps.hpp"
#include <iostream>
#include <iomanip>
#include <sstream>

namespace edasm {

TrapManager::TrapManager() {
}

bool TrapManager::default_trap_handler(CPUState& cpu, Bus& bus, uint16_t trap_pc) {
    std::cerr << "=== UNHANDLED TRAP at PC=$" << std::hex << std::uppercase 
              << std::setw(4) << std::setfill('0') << trap_pc << " ===" << std::endl;
    log_cpu_state(cpu, bus, trap_pc);
    log_memory_window(bus, trap_pc, 32);
    std::cerr << "=== HALTING ===" << std::endl;
    return false; // Halt execution
}

TrapHandler TrapManager::create_logging_handler(const std::string& name) {
    return [name](CPUState& cpu, Bus& bus, uint16_t trap_pc) -> bool {
        std::cout << "[TRAP:" << name << "] PC=$" << std::hex << std::uppercase 
                  << std::setw(4) << std::setfill('0') << trap_pc << std::endl;
        log_cpu_state(cpu, bus, trap_pc);
        return false; // Halt after logging
    };
}

void TrapManager::log_cpu_state(const CPUState& cpu, const Bus& bus, uint16_t pc) {
    std::cerr << dump_cpu_state(cpu) << std::endl;
}

void TrapManager::log_memory_window(const Bus& bus, uint16_t addr, size_t size) {
    std::cerr << dump_memory(bus, addr, size) << std::endl;
}

std::string TrapManager::dump_cpu_state(const CPUState& cpu) {
    std::ostringstream oss;
    oss << "CPU: A=$" << std::hex << std::uppercase << std::setw(2) << std::setfill('0') 
        << static_cast<int>(cpu.A)
        << " X=$" << std::setw(2) << static_cast<int>(cpu.X)
        << " Y=$" << std::setw(2) << static_cast<int>(cpu.Y)
        << " SP=$" << std::setw(2) << static_cast<int>(cpu.SP)
        << " P=$" << std::setw(2) << static_cast<int>(cpu.P)
        << " PC=$" << std::setw(4) << cpu.PC;
    
    // Decode flags
    oss << " [";
    if (cpu.P & StatusFlags::N) oss << "N"; else oss << "-";
    if (cpu.P & StatusFlags::V) oss << "V"; else oss << "-";
    if (cpu.P & StatusFlags::U) oss << "-"; else oss << "-"; // U always set
    if (cpu.P & StatusFlags::B) oss << "B"; else oss << "-";
    if (cpu.P & StatusFlags::D) oss << "D"; else oss << "-";
    if (cpu.P & StatusFlags::I) oss << "I"; else oss << "-";
    if (cpu.P & StatusFlags::Z) oss << "Z"; else oss << "-";
    if (cpu.P & StatusFlags::C) oss << "C"; else oss << "-";
    oss << "]";
    
    return oss.str();
}

std::string TrapManager::dump_memory(const Bus& bus, uint16_t addr, size_t size) {
    std::ostringstream oss;
    oss << "Memory at $" << std::hex << std::uppercase << std::setw(4) 
        << std::setfill('0') << addr << ":" << std::endl;
    
    const uint8_t* data = bus.data();
    for (size_t i = 0; i < size; ++i) {
        if (i % 16 == 0) {
            if (i > 0) oss << std::endl;
            oss << "  $" << std::setw(4) << (addr + i) << ": ";
        } else if (i % 8 == 0) {
            oss << " ";
        }
        oss << std::setw(2) << static_cast<int>(data[addr + i]) << " ";
    }
    
    return oss.str();
}

} // namespace edasm
