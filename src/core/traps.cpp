#include "edasm/traps.hpp"
#include <iomanip>
#include <iostream>
#include <sstream>

namespace edasm {

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

bool TrapManager::prodos_mli_trap_handler(CPUState &cpu, Bus &bus, uint16_t trap_pc) {
    // This handler is only called for $BF00
    std::cout << std::endl;
    std::cout << "=== PRODOS MLI CALL DETECTED at PC=$BF00 ===" << std::endl;
    std::cout << dump_cpu_state(cpu) << std::endl;
    std::cout << std::endl;

    // When JSR $BF00 is executed, the return address-1 is pushed onto the stack
    // Stack pointer points to the next free location, so:
    // $01SP+1 = high byte of return address
    // $01SP+2 = low byte of return address
    // Return address points to the last byte of JSR, so actual return is +1
    // The MLI call structure is:
    //   JSR $BF00
    //   .BYTE command_number
    //   .WORD parameter_list_pointer
    //   [execution continues here]

    uint16_t stack_base = 0x0100;
    uint8_t sp = cpu.SP;
    const uint8_t *mem = bus.data();

    // Get return address from stack
    uint8_t ret_lo = mem[stack_base + sp + 1];
    uint8_t ret_hi = mem[stack_base + sp + 2];
    uint16_t ret_addr = (ret_hi << 8) | ret_lo;

    std::cout << "Stack Analysis:" << std::endl;
    std::cout << "  SP=$" << std::hex << std::uppercase << std::setw(2) << std::setfill('0')
              << static_cast<int>(sp) << std::endl;
    std::cout << "  Return address on stack: $" << std::setw(4) << ret_addr << std::endl;

    // The actual return point is +1 from what's on the stack
    uint16_t call_site = ret_addr + 1;
    std::cout << "  JSR call site: $" << std::setw(4) << (call_site - 3) << std::endl;
    std::cout << "  Parameters start at: $" << std::setw(4) << call_site << std::endl;
    std::cout << std::endl;

    // Read MLI call parameters
    uint8_t call_num = mem[call_site];
    uint8_t param_lo = mem[call_site + 1];
    uint8_t param_hi = mem[call_site + 2];
    uint16_t param_list = (param_hi << 8) | param_lo;

    std::cout << "MLI Call Information:" << std::endl;
    std::cout << "  Command number: $" << std::setw(2) << static_cast<int>(call_num) << " ("
              << decode_prodos_call(call_num) << ")" << std::endl;
    std::cout << "  Parameter list pointer: $" << std::setw(4) << param_list << std::endl;

    // Show memory around call site for debugging
    std::cout << "  Memory at call site ($" << std::setw(4) << (call_site - 3) << "):" << std::endl;
    std::cout << "    ";
    for (int i = -3; i <= 5; ++i) {
        std::cout << std::setw(2) << static_cast<int>(mem[call_site + i]) << " ";
    }
    std::cout << std::endl;
    std::cout << "    JSR ^ CM  PL  PH  --  --  --" << std::endl;
    std::cout << std::endl;

    // Read parameter list
    if (param_list >= 0x0200 && param_list < 0xFFFF) {
        uint8_t param_count = mem[param_list];
        std::cout << "Parameter List at $" << std::setw(4) << param_list << ":" << std::endl;
        std::cout << "  Parameter count: " << std::dec << static_cast<int>(param_count)
                  << std::endl;

        // Show the parameter bytes
        std::cout << "  Parameters (hex):";
        for (int i = 1; i <= param_count && i <= 16; ++i) {
            if (i % 8 == 1)
                std::cout << std::endl << "    ";
            std::cout << " " << std::hex << std::setw(2) << std::setfill('0')
                      << static_cast<int>(mem[param_list + i]);
        }
        std::cout << std::endl;

        // For certain calls, decode specific parameters
        if (call_num == 0x82) { // GET_TIME
            std::cout << std::endl << "  GET_TIME call parameters:" << std::endl;
            std::cout << "    This call retrieves the current date/time from ProDOS" << std::endl;
            std::cout << "    Parameter count should be 1 (4 bytes for date/time result)"
                      << std::endl;
            if (param_count == 1) {
                std::cout << "    Date/time buffer pointer: $" << std::hex << std::setw(4)
                          << (mem[param_list + 1] | (mem[param_list + 2] << 8)) << std::endl;
            }
        } else if (call_num == 0xC0) { // CREATE
            std::cout << std::endl << "  CREATE call parameters:" << std::endl;
            uint16_t pathname_ptr = mem[param_list + 1] | (mem[param_list + 2] << 8);
            std::cout << "    Pathname pointer: $" << std::hex << std::setw(4) << pathname_ptr
                      << std::endl;

            if (pathname_ptr >= 0x0200 && pathname_ptr < 0xFFFF) {
                uint8_t path_len = mem[pathname_ptr];
                std::cout << "    Pathname length: " << std::dec << static_cast<int>(path_len)
                          << std::endl;
                std::cout << "    Pathname: \"";
                for (int i = 1; i <= path_len && i <= 64; ++i) {
                    char c = mem[pathname_ptr + i];
                    std::cout << c;
                }
                std::cout << "\"" << std::endl;
                std::cout << "    Access: $" << std::hex << std::setw(2)
                          << static_cast<int>(mem[param_list + 3]) << std::endl;
                std::cout << "    File type: $" << std::setw(2)
                          << static_cast<int>(mem[param_list + 4]) << std::endl;
                std::cout << "    Storage type: $" << std::setw(2)
                          << static_cast<int>(mem[param_list + 6]) << std::endl;
            }
        }
    }

    std::cout << std::endl;
    std::cout << "=== HALTING - ProDOS MLI not implemented ===" << std::endl;
    return false; // Halt execution
}

std::string TrapManager::decode_prodos_call(uint8_t call_num) {
    switch (call_num) {
    case 0x40:
        return "ALLOC_INTERRUPT";
    case 0x41:
        return "DEALLOC_INTERRUPT";
    case 0x65:
        return "QUIT";
    case 0x80:
        return "READ_BLOCK";
    case 0x81:
        return "WRITE_BLOCK";
    case 0x82:
        return "GET_TIME";
    case 0xC0:
        return "CREATE";
    case 0xC1:
        return "DESTROY";
    case 0xC2:
        return "RENAME";
    case 0xC3:
        return "SET_FILE_INFO";
    case 0xC4:
        return "GET_FILE_INFO";
    case 0xC5:
        return "ONLINE";
    case 0xC6:
        return "SET_PREFIX";
    case 0xC7:
        return "GET_PREFIX";
    case 0xC8:
        return "OPEN";
    case 0xC9:
        return "NEWLINE";
    case 0xCA:
        return "READ";
    case 0xCB:
        return "WRITE";
    case 0xCC:
        return "CLOSE";
    case 0xCD:
        return "FLUSH";
    case 0xCE:
        return "SET_MARK";
    case 0xCF:
        return "GET_MARK";
    case 0xD0:
        return "SET_EOF";
    case 0xD1:
        return "GET_EOF";
    case 0xD2:
        return "SET_BUF";
    case 0xD3:
        return "GET_BUF";
    default:
        return "UNKNOWN";
    }
}

} // namespace edasm
