#include "edasm/emulator/traps.hpp"
#include "edasm/emulator/mli.hpp"
#include <algorithm>
#include <array>
#include <chrono>
#include <cstdio>
#include <ctime>
#include <errno.h>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits.h>
#include <optional>
#include <sstream>
#include <string.h>
#include <system_error>
#include <unistd.h>
#include <vector>

namespace edasm {



// Static trace flag
bool TrapManager::s_trace_enabled = false;

void TrapManager::set_trace(bool enabled) {
    s_trace_enabled = enabled;
}

bool TrapManager::is_trace_enabled() {
    return s_trace_enabled;
}

TrapManager::TrapManager() {}

std::map<uint16_t, TrapHandler> &TrapManager::get_handler_registry() {
    static std::map<uint16_t, TrapHandler> registry;
    return registry;
}

void TrapManager::install_address_handler(uint16_t address, TrapHandler handler) {
    get_handler_registry()[address] = handler;
}

void TrapManager::clear_address_handler(uint16_t address) {
    get_handler_registry().erase(address);
}

void TrapManager::clear_all_handlers() {
    get_handler_registry().clear();
}

bool TrapManager::general_trap_handler(CPUState &cpu, Bus &bus, uint16_t trap_pc) {
    // Check if there's a specific handler registered for this address
    auto &registry = get_handler_registry();
    auto it = registry.find(trap_pc);

    if (it != registry.end()) {
        // Call the registered handler
        return it->second(cpu, bus, trap_pc);
    }

    // Fall back to default handler
    return default_trap_handler(cpu, bus, trap_pc);
}

bool TrapManager::default_trap_handler(CPUState &cpu, Bus &bus, uint16_t trap_pc) {
    std::cerr << "=== UNHANDLED TRAP at PC=$" << std::hex << std::uppercase << std::setw(4)
              << std::setfill('0') << trap_pc << " ===" << std::endl;
    log_cpu_state(cpu, bus, trap_pc);
    log_memory_window(bus, trap_pc, 32);
    std::cerr << "=== HALTING ===" << std::endl;

    // Write memory dump before halting
    write_memory_dump(bus, "memory_dump.bin");

    return false; // Halt execution
}

TrapHandler TrapManager::create_logging_handler(const std::string &name) {
    return [name](CPUState &cpu, Bus &bus, uint16_t trap_pc) -> bool {
        std::cout << "[TRAP:" << name << "] PC=$" << std::hex << std::uppercase << std::setw(4)
                  << std::setfill('0') << trap_pc << std::endl;
        log_cpu_state(cpu, bus, trap_pc);
        return false; // Halt after logging
    };
}

void TrapManager::log_cpu_state(const CPUState &cpu, const Bus &bus, uint16_t pc) {
    std::cerr << dump_cpu_state(cpu) << std::endl;
}

void TrapManager::log_memory_window(const Bus &bus, uint16_t addr, size_t size) {
    std::cerr << dump_memory(bus, addr, size) << std::endl;
}

std::string TrapManager::dump_cpu_state(const CPUState &cpu) {
    std::ostringstream oss;
    oss << "CPU: A=$" << std::hex << std::uppercase << std::setw(2) << std::setfill('0')
        << static_cast<int>(cpu.A) << " X=$" << std::setw(2) << static_cast<int>(cpu.X) << " Y=$"
        << std::setw(2) << static_cast<int>(cpu.Y) << " SP=$" << std::setw(2)
        << static_cast<int>(cpu.SP) << " P=$" << std::setw(2) << static_cast<int>(cpu.P) << " PC=$"
        << std::setw(4) << cpu.PC;

    // Decode flags
    oss << " [";
    if (cpu.P & StatusFlags::N)
        oss << "N";
    else
        oss << "-";
    if (cpu.P & StatusFlags::V)
        oss << "V";
    else
        oss << "-";
    oss << "U"; // U flag always set on 6502
    if (cpu.P & StatusFlags::B)
        oss << "B";
    else
        oss << "-";
    if (cpu.P & StatusFlags::D)
        oss << "D";
    else
        oss << "-";
    if (cpu.P & StatusFlags::I)
        oss << "I";
    else
        oss << "-";
    if (cpu.P & StatusFlags::Z)
        oss << "Z";
    else
        oss << "-";
    if (cpu.P & StatusFlags::C)
        oss << "C";
    else
        oss << "-";
    oss << "]";

    return oss.str();
}

std::string TrapManager::dump_memory(const Bus &bus, uint16_t addr, size_t size) {
    std::ostringstream oss;
    oss << "Memory at $" << std::hex << std::uppercase << std::setw(4) << std::setfill('0') << addr
        << ":" << std::endl;

    const uint8_t *data = bus.data();
    for (size_t i = 0; i < size; ++i) {
        if (i % 16 == 0) {
            if (i > 0)
                oss << std::endl;
            oss << "  $" << std::setw(4) << (addr + i) << ": ";
        } else if (i % 8 == 0) {
            oss << " ";
        }
        oss << std::setw(2) << static_cast<int>(data[addr + i]) << " ";
    }

    return oss.str();
}

bool TrapManager::write_memory_dump(const Bus &bus, const std::string &filename) {
    std::ofstream file(filename, std::ios::binary);
    if (!file) {
        std::cerr << "Error: Failed to open " << filename << " for writing" << std::endl;
        return false;
    }

    const uint8_t *mem = bus.data();
    file.write(reinterpret_cast<const char *>(mem), Bus::MEMORY_SIZE);

    if (!file) {
        std::cerr << "Error: Failed to write memory dump" << std::endl;
        return false;
    }

    file.close();
    std::cout << "Memory dump written to: " << filename << " (" << Bus::MEMORY_SIZE << " bytes)"
              << std::endl;
    return true;
}

// Forward to MLI handler for ProDOS MLI calls
bool TrapManager::prodos_mli_trap_handler(CPUState &cpu, Bus &bus, uint16_t trap_pc) {
    return MLIHandler::prodos_mli_trap_handler(cpu, bus, trap_pc);
}

bool TrapManager::monitor_setnorm_trap_handler(CPUState &cpu, Bus &bus, uint16_t trap_pc) {
    // SETNORM ($FE84): Set InvFlg ($32) to $FF (normal video mode) and Y to $FF
    bus.write(0x32, 0xFF);
    cpu.Y = 0xff;

    std::cout << "MONITOR SETNORM: Set InvFlg ($32) to $FF, Y to $FF" << std::endl;

    cpu.SP = static_cast<uint8_t>(cpu.SP + 1);
    uint8_t ret_lo = bus.read(0x0100 | cpu.SP);
    cpu.SP = static_cast<uint8_t>(cpu.SP + 1);
    uint8_t ret_hi = bus.read(0x0100 | cpu.SP);
    uint16_t ret_addr = static_cast<uint16_t>((ret_hi << 8) | ret_lo);

    cpu.PC = static_cast<uint16_t>(ret_addr + 1);

    return true;
}


} // namespace edasm
