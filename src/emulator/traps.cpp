/**
 * @file traps.cpp
 * @brief Trap management and statistics implementation
 *
 * Provides trap handler registration, dispatch, and statistics tracking
 * for incremental discovery of system calls and I/O operations.
 */

#include "edasm/emulator/traps.hpp"
#include "edasm/constants.hpp"
#include "edasm/emulator/disassembly.hpp"
#include "edasm/emulator/mli.hpp"
#include <algorithm>
#include <cstdio>
#include <ctime>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits.h>
#include <sstream>
#include <string.h>
#include <unistd.h>

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

std::map<uint16_t, std::string> &TrapManager::get_name_registry() {
    static std::map<uint16_t, std::string> name_registry;
    return name_registry;
}

void TrapManager::install_address_handler(uint16_t address, TrapHandler handler,
                                          const std::string &name) {
    get_handler_registry()[address] = handler;
    if (!name.empty()) {
        get_name_registry()[address] = name;
    }
}

void TrapManager::clear_address_handler(uint16_t address) {
    get_handler_registry().erase(address);
    get_name_registry().erase(address);
}

void TrapManager::clear_all_handlers() {
    get_handler_registry().clear();
    get_name_registry().clear();
}

bool TrapManager::general_trap_handler(CPUState &cpu, Bus &bus, uint16_t trap_pc) {
    // Check if there's a specific handler registered for this address
    auto &registry = get_handler_registry();
    auto it = registry.find(trap_pc);

    if (it != registry.end()) {
        // Call the registered handler (which will record statistics)
        return it->second(cpu, bus, trap_pc);
    }

    // Fall back to default handler (which will also record statistics)
    return default_trap_handler(cpu, bus, trap_pc);
}

bool TrapManager::default_trap_handler(CPUState &cpu, Bus &bus, uint16_t trap_pc) {
    // Record trap statistic for unhandled traps
    TrapStatistics::record_trap("UNHANDLED", trap_pc, TrapKind::CALL);

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
    // Record trap statistic
    TrapStatistics::record_trap("MONITOR SETNORM", trap_pc, TrapKind::CALL);

    // SETNORM ($FE84): Set InvFlg ($32) to $FF (normal video mode) and Y to $FF
    bus.write(0x32, 0xFF);
    cpu.Y = 0xff;

    std::cout << "MONITOR SETNORM: Set InvFlg ($32) to $FF, Y to $FF" << std::endl;

    cpu.SP = static_cast<uint8_t>(cpu.SP + 1);
    uint8_t ret_lo = bus.read(STACK_BASE | cpu.SP);
    cpu.SP = static_cast<uint8_t>(cpu.SP + 1);
    uint8_t ret_hi = bus.read(STACK_BASE | cpu.SP);
    uint16_t ret_addr = static_cast<uint16_t>((ret_hi << 8) | ret_lo);

    cpu.PC = static_cast<uint16_t>(ret_addr + 1);

    return true;
}

// TrapStatistics implementation
std::vector<TrapStatistic> &TrapStatistics::get_statistics() {
    static std::vector<TrapStatistic> statistics;
    return statistics;
}

void TrapStatistics::record_trap(const std::string &name, uint16_t address, TrapKind kind,
                                 const std::string &mli_call, bool is_second_read) {
    auto &stats = get_statistics();

    // Find existing entry with matching characteristics
    for (auto &stat : stats) {
        if (stat.address == address && stat.kind == kind && stat.name == name &&
            stat.mli_call == mli_call && stat.is_second_read == is_second_read) {
            stat.count++;
            return;
        }
    }

    // Create new entry
    TrapStatistic new_stat(name, address, kind);
    new_stat.mli_call = mli_call;
    new_stat.is_second_read = is_second_read;
    new_stat.count = 1;
    stats.push_back(new_stat);
}

void TrapStatistics::print_statistics() {
    auto &stats = get_statistics();

    if (stats.empty()) {
        std::cout << "\nNo trap statistics collected." << std::endl;
        return;
    }

    // Sort by trap address
    std::sort(stats.begin(), stats.end(),
              [](const TrapStatistic &a, const TrapStatistic &b) { return a.address < b.address; });

    // Reset output format to defaults
    std::cout << std::dec << std::setfill(' ');

    std::cout << "\n=== TRAP STATISTICS ===" << std::endl;
    std::cout << std::left << std::setw(6) << "Addr"
              << " " << std::setw(8) << "Kind"
              << " " << std::setw(20) << "Name"
              << " " << std::setw(6) << "Count"
              << " " << std::setw(20) << "Details"
              << " "
              << "Symbol" << std::endl;
    std::cout << std::string(90, '-') << std::endl;

    // Accumulate SCREEN WRITE entries
    uint64_t screen_write_total = 0;
    std::vector<size_t> screen_write_indices;

    for (size_t i = 0; i < stats.size(); ++i) {
        const auto &stat = stats[i];
        if (stat.name == "SCREEN" && stat.kind == TrapKind::WRITE) {
            screen_write_total += stat.count;
            // Only mark for consolidation if there's no symbol defined for this address
            const std::string *symbol = lookup_disassembly_symbol(stat.address);
            if (!symbol) {
                screen_write_indices.push_back(i);
            }
        }
    }

    // Print accumulated SCREEN WRITE entry once (unless all have symbols)
    bool printed_accumulated_screen = false;
    if (!screen_write_indices.empty()) {
        std::cout << std::left;
        std::cout << std::setw(6) << ""; // Addr (empty, consolidated)
        std::cout << std::setw(8) << "WRITE"
                  << " ";
        std::cout << std::setw(20) << "SCREEN"
                  << " ";
        std::cout << std::dec << std::setw(6) << screen_write_total << " ";
        std::cout << std::setw(20) << "(consolidated)"
                  << " ";
        std::cout << std::endl;
        printed_accumulated_screen = true;
    }

    // Print all other entries, plus SCREEN entries that have symbols
    for (size_t i = 0; i < stats.size(); ++i) {
        const auto &stat = stats[i];

        // Skip SCREEN WRITE entries that we've consolidated (unless they have symbols)
        if (stat.name == "SCREEN" && stat.kind == TrapKind::WRITE) {
            const std::string *symbol = lookup_disassembly_symbol(stat.address);
            if (!symbol && printed_accumulated_screen) {
                continue; // Skip consolidated SCREEN entries
            }
            // If has symbol, print it separately below
        }

        // Format address using stringstream to avoid stream state issues
        std::ostringstream addr_ss;
        addr_ss << "$" << std::hex << std::uppercase << std::setw(4) << std::setfill('0')
                << stat.address;

        // Print address
        std::cout << addr_ss.str() << " ";

        // Print kind
        std::string kind_str;
        switch (stat.kind) {
        case TrapKind::CALL:
            kind_str = "CALL";
            break;
        case TrapKind::READ:
            kind_str = "READ";
            break;
        case TrapKind::WRITE:
            kind_str = "WRITE";
            break;
        case TrapKind::DOUBLE_READ:
            kind_str = "DBL_READ";
            break;
        }
        std::cout << std::left << std::setw(8) << kind_str << " ";

        // Print name
        std::cout << std::setw(20) << stat.name << " ";

        // Print count
        std::cout << std::dec << std::setw(6) << stat.count << " ";

        // Print details
        std::string details;
        if (!stat.mli_call.empty()) {
            details = "MLI:" + stat.mli_call;
        }
        if (stat.kind == TrapKind::DOUBLE_READ) {
            if (!details.empty())
                details += ", ";
            details += stat.is_second_read ? "2nd read" : "1st read";
        }
        std::cout << std::setw(20) << details << " ";

        // Look up and print symbol name if available
        const std::string *symbol = lookup_disassembly_symbol(stat.address);
        if (symbol) {
            std::cout << "<" << *symbol << ">";
        }

        std::cout << std::endl;
    }

    std::cout << std::string(90, '-') << std::endl;
    std::cout << "Total trap entries: " << stats.size() << std::endl;
    std::cout << "=======================" << std::endl;
}

void TrapStatistics::clear() {
    get_statistics().clear();
}

} // namespace edasm
