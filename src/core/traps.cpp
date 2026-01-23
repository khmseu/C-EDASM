#include "edasm/traps.hpp"
#include <chrono>
#include <ctime>
#include <errno.h>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits.h>
#include <sstream>
#include <string.h>
#include <unistd.h>

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

    // Fast-path for GET_TIME ($82): no parameter list, write to $BF90-$BF93 and return success
    if (call_num == 0x82) {
        // Compute date/time from host
        auto now = std::chrono::system_clock::now();
        std::time_t t = std::chrono::system_clock::to_time_t(now);
        std::tm tm{};
        localtime_r(&t, &tm);

        // ProDOS encoding:
        // DATE (BF91:BF90): year (7 bits, since 1900), month (4 bits), day (5 bits)
        //   BF91: bits7-1 year, bit0 = month bit3
        //   BF90: bits7-5 month bits2-0, bits4-0 day
        uint8_t year = static_cast<uint8_t>(tm.tm_year);     // years since 1900, 0-127 valid
        uint8_t month = static_cast<uint8_t>(tm.tm_mon + 1); // 1-12
        uint8_t day = static_cast<uint8_t>(tm.tm_mday);      // 1-31
        uint8_t bf91 = static_cast<uint8_t>((year << 1) | ((month >> 3) & 0x01));
        uint8_t bf90 = static_cast<uint8_t>(((month & 0x07) << 5) | (day & 0x1F));

        // TIME (BF93:BF92): hour (0-23), minute (0-59)
        uint8_t hour = static_cast<uint8_t>(tm.tm_hour);  // 0-23
        uint8_t minute = static_cast<uint8_t>(tm.tm_min); // 0-59

        bus.write(0xBF91, bf91);
        bus.write(0xBF90, bf90);
        bus.write(0xBF93, hour);
        bus.write(0xBF92, minute);

        std::cout << "GET_TIME: wrote date/time to $BF90-$BF93" << std::endl;
        std::cout << "  Year (since 1900): " << std::dec << static_cast<int>(year) << std::endl;
        std::cout << "  Month: " << static_cast<int>(month) << std::endl;
        std::cout << "  Day: " << static_cast<int>(day) << std::endl;
        std::cout << "  Hour: " << static_cast<int>(hour) << std::endl;
        std::cout << "  Minute: " << static_cast<int>(minute) << std::endl;

        // ProDOS successful return: C clear, A=0, Z set, return to caller (JSR+3) and unwind stack
        cpu.A = 0;
        cpu.P &= ~StatusFlags::C;
        cpu.P &= ~StatusFlags::N;
        cpu.P &= ~StatusFlags::V;
        cpu.P |= StatusFlags::Z;
        cpu.P |= StatusFlags::U; // ensure U stays set

        // Mimic RTS from MLI: pop return address and continue after operands
        cpu.SP = static_cast<uint8_t>(cpu.SP + 2); // discard pushed return
        cpu.PC =
            static_cast<uint16_t>((ret_addr + 1) + 3); // JSR+3: skip command byte and param pointer

        return true; // continue execution
    }

    // Implement GET_PREFIX ($C7): return system prefix bracketed by slashes
    // ProDOS spec: prefix bracketed by slashes, length byte first, e.g. $7/APPLE/
    if (call_num == 0xC7) {
        if (!(param_list >= 0x0200 && param_list < 0xFFFF)) {
            std::cerr << "GET_PREFIX: parameter list pointer out of range: $" << std::hex
                      << std::uppercase << std::setw(4) << std::setfill('0') << param_list
                      << std::endl;
            write_memory_dump(bus, "memory_dump.bin");
            return false;
        }

        uint8_t param_count = mem[param_list];
        if (param_count < 1) {
            std::cerr << "GET_PREFIX: parameter count < 1 (" << std::dec
                      << static_cast<int>(param_count) << ")" << std::endl;
            write_memory_dump(bus, "memory_dump.bin");
            return false;
        }

        uint16_t buf_ptr = mem[param_list + 1] | (mem[param_list + 2] << 8);
        if (buf_ptr >= Bus::MEMORY_SIZE) {
            std::cerr << "GET_PREFIX: buffer pointer out of range: $" << std::hex << std::uppercase
                      << std::setw(4) << std::setfill('0') << buf_ptr << std::endl;
            write_memory_dump(bus, "memory_dump.bin");
            return false;
        }

        std::cout << "GET_PREFIX: buffer ptr=$" << std::hex << std::uppercase << std::setw(4)
                  << std::setfill('0') << buf_ptr << std::endl;

        // Get current working directory
        char cwd_buf[PATH_MAX] = {0};
        if (!::getcwd(cwd_buf, sizeof(cwd_buf))) {
            std::cerr << "GET_PREFIX: getcwd failed: " << ::strerror(errno) << std::endl;
            write_memory_dump(bus, "memory_dump.bin");
            return false;
        }

        // ProDOS format: length byte + /path/ (bracketed by slashes)
        // Maximum prefix length is 64 characters per spec
        // getcwd() already returns absolute path starting with /
        std::string prefix_str = cwd_buf;
        prefix_str += "/";

        // Limit to 64 chars including length byte (63 chars max for path+slashes)
        if (prefix_str.length() > 64) {
            std::cerr << "GET_PREFIX: prefix too long (" << prefix_str.length()
                      << " chars exceeds 64 byte limit)" << std::endl;
            write_memory_dump(bus, "memory_dump.bin");
            return false;
        }

        // Write length byte followed by prefix string (with high bits cleared)
        uint8_t prefix_len = static_cast<uint8_t>(prefix_str.length());
        bus.write(buf_ptr, prefix_len);
        std::cout << "GET_PREFIX: writing prefix length=" << std::dec
                  << static_cast<int>(prefix_len) << " prefix=\"" << prefix_str << "\""
                  << std::endl;

        for (size_t i = 0; i < prefix_str.length(); ++i) {
            uint8_t ch = static_cast<uint8_t>(prefix_str[i]) & 0x7F; // Clear high bit
            bus.write(static_cast<uint16_t>(buf_ptr + 1 + i), ch);
        }

        // ProDOS successful return
        cpu.A = 0;
        cpu.P &= ~StatusFlags::C;
        cpu.P &= ~StatusFlags::N;
        cpu.P &= ~StatusFlags::V;
        cpu.P |= StatusFlags::Z;
        cpu.P |= StatusFlags::U;

        // Mimic RTS from MLI: pop return address and continue after operands
        cpu.SP = static_cast<uint8_t>(cpu.SP + 2);
        cpu.PC = static_cast<uint16_t>((ret_addr + 1) + 3);

        return true;
    }

    // Read parameter list (not used for GET_TIME)
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

    // Write memory dump before halting
    write_memory_dump(bus, "memory_dump.bin");

    return false; // Halt execution
}

bool TrapManager::monitor_setnorm_trap_handler(CPUState &cpu, Bus &bus, uint16_t trap_pc) {
    // SETNORM ($FE84): Set InvFlg ($32) to $FF (normal video mode) and Y to $FF
    // InvFlg is at zero page $32
    bus.write(0x32, 0xFF);
    cpu.Y = 0xff;

    std::cout << "MONITOR SETNORM: Set InvFlg ($32) to $FF, Y to $FF" << std::endl;

    // Monitor routines use JSR/RTS, so pop return address and continue
    // RTS pulls from stack: increment SP, read lo byte, increment SP, read hi byte
    cpu.SP = static_cast<uint8_t>(cpu.SP + 1);
    uint8_t ret_lo = bus.read(0x0100 | cpu.SP);
    cpu.SP = static_cast<uint8_t>(cpu.SP + 1);
    uint8_t ret_hi = bus.read(0x0100 | cpu.SP);
    uint16_t ret_addr = (ret_hi << 8) | ret_lo;

    // JSR stores PC of last byte of instruction on stack
    // RTS adds 1 to get to next instruction
    cpu.PC = static_cast<uint16_t>(ret_addr + 1);

    return true; // Continue execution
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
