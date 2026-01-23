#include "edasm/traps.hpp"
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

namespace {

struct FileEntry {
    bool used = false;
    FILE *fp = nullptr;
    std::string host_path;
    uint32_t mark = 0;      // current file position
    uint32_t file_size = 0; // bytes
};

constexpr size_t kMaxFiles = 16; // ProDOS refnums are 1-15; slot 0 unused
std::array<FileEntry, kMaxFiles> s_file_table{};

uint16_t read_word_mem(const uint8_t *mem, uint16_t addr) {
    return static_cast<uint16_t>(mem[addr] | (mem[addr + 1] << 8));
}

std::string current_prefix() {
    char cwd_buf[PATH_MAX] = {0};
    if (!::getcwd(cwd_buf, sizeof(cwd_buf))) {
        return "/"; // fallback
    }
    std::string prefix = cwd_buf;
    prefix += "/";
    return prefix;
}

std::string s_prefix_host = current_prefix();
std::string s_prefix_prodos = "/"; // leading slash, trailing slash maintained

std::string normalize_prodos_path(const std::string &path) {
    std::string normalized = path;
    if (normalized.empty() || normalized.front() != '/') {
        normalized = "/" + normalized;
    }
    if (normalized.back() != '/') {
        normalized.push_back('/');
    }
    return normalized;
}

std::string prodos_path_to_host(const std::string &prodos_path) {
    bool absolute = !prodos_path.empty() && prodos_path.front() == '/';

    std::string clean = prodos_path;
    while (!clean.empty() && clean.front() == '/') {
        clean.erase(clean.begin());
    }

    std::filesystem::path base =
        absolute ? std::filesystem::path(current_prefix()) : std::filesystem::path(s_prefix_host);
    std::filesystem::path host = base / clean;
    return host.string();
}

void dump_file_table() {
    std::cerr << "=== FILE TABLE DUMP ===" << std::endl;
    for (size_t i = 0; i < s_file_table.size(); ++i) {
        const auto &entry = s_file_table[i];
        std::cerr << "  [" << i << "] used=" << entry.used << " fp=" << std::hex
                  << reinterpret_cast<uintptr_t>(entry.fp) << std::dec << " host_path=\""
                  << entry.host_path << "\" mark=" << entry.mark << " size=" << entry.file_size
                  << std::endl;
    }
    std::cerr << "=======================\n" << std::endl;
}

int alloc_refnum() {
    for (size_t i = 1; i < s_file_table.size(); ++i) {
        if (!s_file_table[i].used) {
            return static_cast<int>(i);
        }
    }
    std::cerr << "alloc_refnum: No free file slots available" << std::endl;
    dump_file_table();
    return -1;
}

FileEntry *get_refnum(uint8_t refnum) {
    if (refnum == 0 || refnum >= s_file_table.size()) {
        std::cerr << "get_refnum: Invalid refnum " << static_cast<int>(refnum)
                  << " (valid range: 1-" << (s_file_table.size() - 1) << ")" << std::endl;
        dump_file_table();
        return nullptr;
    }
    if (!s_file_table[refnum].used) {
        std::cerr << "get_refnum: Refnum " << static_cast<int>(refnum) << " is not in use"
                  << std::endl;
        dump_file_table();
        return nullptr;
    }
    return &s_file_table[refnum];
}

void close_entry(FileEntry &entry) {
    if (entry.fp) {
        std::fclose(entry.fp);
        entry.fp = nullptr;
    }
    entry.used = false;
    entry.host_path.clear();
    entry.mark = 0;
    entry.file_size = 0;
}

void set_success(CPUState &cpu) {
    cpu.A = 0;
    cpu.P &= ~StatusFlags::C;
    cpu.P &= ~StatusFlags::N;
    cpu.P &= ~StatusFlags::V;
    cpu.P |= StatusFlags::Z;
    cpu.P |= StatusFlags::U;
}

void set_error(CPUState &cpu, uint8_t err) {
    cpu.A = err;
    cpu.P |= StatusFlags::C;
    cpu.P &= ~StatusFlags::Z;
    cpu.P |= StatusFlags::U;
}

constexpr uint8_t ERR_PATH_NOT_FOUND = 0x4B;
constexpr uint8_t ERR_FILE_NOT_FOUND = 0x46;
constexpr uint8_t ERR_TOO_MANY_FILES = 0x52;
constexpr uint8_t ERR_ILLEGAL_PARAM = 0x2C;

} // namespace

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

bool TrapManager::prodos_mli_trap_handler(CPUState &cpu, Bus &bus, uint16_t trap_pc) {
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

    uint8_t ret_lo = mem[stack_base + sp + 1];
    uint8_t ret_hi = mem[stack_base + sp + 2];
    uint16_t ret_addr = static_cast<uint16_t>((ret_hi << 8) | ret_lo);
    uint16_t call_site = static_cast<uint16_t>(ret_addr + 1);

    uint8_t call_num = mem[call_site];
    uint8_t param_lo = mem[call_site + 1];
    uint8_t param_hi = mem[call_site + 2];
    uint16_t param_list = static_cast<uint16_t>((param_hi << 8) | param_lo);

    auto return_to_caller = [&]() {
        cpu.SP = static_cast<uint8_t>(cpu.SP + 2);
        cpu.PC = static_cast<uint16_t>((ret_addr + 1) + 3);
    };

    bool call_details_logged = false;
    auto log_call_details = [&](const std::string &reason) {
        if (call_details_logged)
            return;
        if (!s_trace_enabled && reason != "halt")
            return;

        call_details_logged = true;

        std::cout << std::endl;
        std::cout << "=== PRODOS MLI CALL DETECTED at PC=$BF00 ===" << std::endl;
        std::cout << dump_cpu_state(cpu) << std::endl;
        std::cout << std::endl;

        std::cout << "Stack Analysis:" << std::endl;
        std::cout << "  SP=$" << std::hex << std::uppercase << std::setw(2) << std::setfill('0')
                  << static_cast<int>(sp) << std::endl;
        std::cout << "  Return address on stack: $" << std::setw(4) << ret_addr << std::endl;
        std::cout << "  JSR call site: $" << std::setw(4) << (call_site - 3) << std::endl;
        std::cout << "  Parameters start at: $" << std::setw(4) << call_site << std::endl;
        std::cout << std::endl;

        std::cout << "MLI Call Information:" << std::endl;
        std::cout << "  Command number: $" << std::setw(2) << static_cast<int>(call_num) << " ("
                  << decode_prodos_call(call_num) << ")" << std::endl;
        std::cout << "  Parameter list pointer: $" << std::setw(4) << param_list << std::endl;

        std::cout << "  Memory at call site ($" << std::setw(4) << (call_site - 3)
                  << "):" << std::endl;
        std::cout << "    ";
        for (int i = -3; i <= 5; ++i) {
            std::cout << std::setw(2) << static_cast<int>(mem[call_site + i]) << " ";
        }
        std::cout << std::endl;
        std::cout << "    JSR ^ CM  PL  PH  --  --  --" << std::endl;
        std::cout << std::endl;

        if (param_list < Bus::MEMORY_SIZE) {
            uint8_t param_count = mem[param_list];
            std::cout << "Parameter List at $" << std::setw(4) << param_list << ":" << std::endl;
            std::cout << "  Parameter count: " << std::dec << static_cast<int>(param_count)
                      << std::endl;

            std::cout << "  Parameters (hex):";
            size_t bytes_to_show = std::min<size_t>(param_count * 2, 24);
            for (size_t i = 1; i <= bytes_to_show && (param_list + i) < Bus::MEMORY_SIZE; ++i) {
                if ((i - 1) % 8 == 0)
                    std::cout << std::endl << "    ";
                std::cout << " " << std::hex << std::setw(2) << std::setfill('0')
                          << static_cast<int>(mem[param_list + i]);
            }
            std::cout << std::endl;

            if (call_num == 0x82 && param_count >= 1 && (param_list + 2) < Bus::MEMORY_SIZE) {
                std::cout << std::endl << "  GET_TIME call parameters:" << std::endl;
                std::cout << "    Date/time buffer pointer: $" << std::hex << std::setw(4)
                          << (mem[param_list + 1] | (mem[param_list + 2] << 8)) << std::endl;
            } else if (call_num == 0xC0 && (param_list + 2) < Bus::MEMORY_SIZE) {
                std::cout << std::endl << "  CREATE call parameters:" << std::endl;
                uint16_t pathname_ptr = mem[param_list + 1] | (mem[param_list + 2] << 8);
                std::cout << "    Pathname pointer: $" << std::hex << std::setw(4) << pathname_ptr
                          << std::endl;

                if (pathname_ptr < Bus::MEMORY_SIZE) {
                    uint8_t path_len = mem[pathname_ptr];
                    std::cout << "    Pathname length: " << std::dec << static_cast<int>(path_len)
                              << std::endl;
                    std::cout << "    Pathname: \"";
                    for (int i = 1;
                         i <= path_len && i <= 64 && (pathname_ptr + i) < Bus::MEMORY_SIZE; ++i) {
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
        } else {
            std::cout << "Parameter list pointer out of range; skipping list dump" << std::endl;
        }
    };

    log_call_details("trace");

    if (call_num == 0x82) {
        auto now = std::chrono::system_clock::now();
        std::time_t t = std::chrono::system_clock::to_time_t(now);
        std::tm tm{};
        localtime_r(&t, &tm);

        uint8_t year = static_cast<uint8_t>(tm.tm_year);
        uint8_t month = static_cast<uint8_t>(tm.tm_mon + 1);
        uint8_t day = static_cast<uint8_t>(tm.tm_mday);
        uint8_t bf91 = static_cast<uint8_t>((year << 1) | ((month >> 3) & 0x01));
        uint8_t bf90 = static_cast<uint8_t>(((month & 0x07) << 5) | (day & 0x1F));

        uint8_t hour = static_cast<uint8_t>(tm.tm_hour);
        uint8_t minute = static_cast<uint8_t>(tm.tm_min);

        bus.write(0xBF91, bf91);
        bus.write(0xBF90, bf90);
        bus.write(0xBF93, hour);
        bus.write(0xBF92, minute);

        if (s_trace_enabled) {
            std::cout << "GET_TIME: wrote date/time to $BF90-$BF93" << std::endl;
            std::cout << "  Year (since 1900): " << std::dec << static_cast<int>(year) << std::endl;
            std::cout << "  Month: " << static_cast<int>(month) << std::endl;
            std::cout << "  Day: " << static_cast<int>(day) << std::endl;
            std::cout << "  Hour: " << static_cast<int>(hour) << std::endl;
            std::cout << "  Minute: " << static_cast<int>(minute) << std::endl;
        }

        cpu.A = 0;
        cpu.P &= ~StatusFlags::C;
        cpu.P &= ~StatusFlags::N;
        cpu.P &= ~StatusFlags::V;
        cpu.P |= StatusFlags::Z;
        cpu.P |= StatusFlags::U;

        cpu.SP = static_cast<uint8_t>(cpu.SP + 2);
        cpu.PC = static_cast<uint16_t>((ret_addr + 1) + 3);

        return true;
    }

    // Implement SET_PREFIX ($C6)
    if (call_num == 0xC6) {
        if (param_list + 2 >= Bus::MEMORY_SIZE) {
            std::cerr << "SET_PREFIX ($C6): param_list + 2 >= MEMORY_SIZE (param_list=$" << std::hex
                      << std::uppercase << std::setw(4) << std::setfill('0') << param_list << ")"
                      << std::endl;
            write_memory_dump(bus, "memory_dump.bin");
            log_call_details("error");
            return false;
        }

        uint16_t pathname_ptr = read_word_mem(mem, static_cast<uint16_t>(param_list + 1));
        if (pathname_ptr >= Bus::MEMORY_SIZE) {
            std::cerr << "SET_PREFIX ($C6): pathname_ptr >= MEMORY_SIZE (pathname_ptr=$" << std::hex
                      << std::uppercase << std::setw(4) << std::setfill('0') << pathname_ptr << ")"
                      << std::endl;
            write_memory_dump(bus, "memory_dump.bin");
            log_call_details("error");
            return false;
        }

        uint8_t path_len = mem[pathname_ptr];
        if (path_len == 0 || pathname_ptr + path_len >= Bus::MEMORY_SIZE || path_len > 64) {
            std::cerr << "SET_PREFIX ($C6): invalid path_len (path_len=" << std::dec
                      << static_cast<int>(path_len) << ", pathname_ptr=$" << std::hex
                      << std::uppercase << std::setw(4) << std::setfill('0') << pathname_ptr << ")"
                      << std::endl;
            write_memory_dump(bus, "memory_dump.bin");
            log_call_details("error");
            return false;
        }

        std::string prodos_path;
        prodos_path.reserve(path_len);
        for (uint8_t i = 0; i < path_len; ++i) {
            prodos_path.push_back(static_cast<char>(mem[pathname_ptr + 1 + i] & 0x7F));
        }

        s_prefix_prodos = normalize_prodos_path(prodos_path);

        // Check if this is an absolute path (starts with /)
        bool is_absolute = !prodos_path.empty() && prodos_path.front() == '/';

        std::string host_path;
        if (is_absolute) {
            // For absolute paths, use the path as-is (it's already a Linux path)
            host_path = prodos_path;
        } else {
            // For relative paths, resolve relative to current prefix
            std::string stripped = s_prefix_prodos;
            while (!stripped.empty() && stripped.front() == '/')
                stripped.erase(stripped.begin());
            std::filesystem::path host_candidate =
                std::filesystem::path(current_prefix()) / stripped;
            std::error_code ec;
            std::filesystem::path canonical = std::filesystem::weakly_canonical(host_candidate, ec);
            host_path = (ec ? host_candidate : canonical).string();
        }

        if (!host_path.empty() && host_path.back() != '/')
            host_path.push_back('/');
        s_prefix_host = host_path;

        set_success(cpu);
        return_to_caller();
        return true;
    }

    if (call_num == 0xC7) {
        if (!(param_list >= 0x0200 && param_list < 0xFFFF)) {
            std::cerr << "GET_PREFIX: parameter list pointer out of range: $" << std::hex
                      << std::uppercase << std::setw(4) << std::setfill('0') << param_list
                      << std::endl;
            write_memory_dump(bus, "memory_dump.bin");
            log_call_details("halt");
            return false;
        }

        uint8_t param_count = mem[param_list];
        if (param_count < 1) {
            std::cerr << "GET_PREFIX: parameter count < 1 (" << std::dec
                      << static_cast<int>(param_count) << ")" << std::endl;
            write_memory_dump(bus, "memory_dump.bin");
            log_call_details("halt");
            return false;
        }

        uint16_t buf_ptr = mem[param_list + 1] | (mem[param_list + 2] << 8);
        if (buf_ptr >= Bus::MEMORY_SIZE) {
            std::cerr << "GET_PREFIX: buffer pointer out of range: $" << std::hex << std::uppercase
                      << std::setw(4) << std::setfill('0') << buf_ptr << std::endl;
            write_memory_dump(bus, "memory_dump.bin");
            log_call_details("halt");
            return false;
        }

        if (s_trace_enabled) {
            std::cout << "GET_PREFIX: buffer ptr=$" << std::hex << std::uppercase << std::setw(4)
                      << std::setfill('0') << buf_ptr << std::endl;
        }

        char cwd_buf[PATH_MAX] = {0};
        if (!::getcwd(cwd_buf, sizeof(cwd_buf))) {
            std::cerr << "GET_PREFIX: getcwd failed: " << ::strerror(errno) << std::endl;
            write_memory_dump(bus, "memory_dump.bin");
            log_call_details("halt");
            return false;
        }

        std::string prefix_str = cwd_buf;
        prefix_str += "/";

        if (prefix_str.length() > 64) {
            std::cerr << "GET_PREFIX: prefix too long (" << prefix_str.length()
                      << " chars exceeds 64 byte limit)" << std::endl;
            write_memory_dump(bus, "memory_dump.bin");
            log_call_details("halt");
            return false;
        }

        uint8_t prefix_len = static_cast<uint8_t>(prefix_str.length());
        bus.write(buf_ptr, prefix_len);
        if (s_trace_enabled) {
            std::cout << "GET_PREFIX: writing prefix length=" << std::dec
                      << static_cast<int>(prefix_len) << " prefix=\"" << prefix_str << "\""
                      << std::endl;
        }

        for (size_t i = 0; i < prefix_str.length(); ++i) {
            uint8_t ch = static_cast<uint8_t>(prefix_str[i]) & 0x7F;
            bus.write(static_cast<uint16_t>(buf_ptr + 1 + i), ch);
        }

        cpu.A = 0;
        cpu.P &= ~StatusFlags::C;
        cpu.P &= ~StatusFlags::N;
        cpu.P &= ~StatusFlags::V;
        cpu.P |= StatusFlags::Z;
        cpu.P |= StatusFlags::U;

        cpu.SP = static_cast<uint8_t>(cpu.SP + 2);
        cpu.PC = static_cast<uint16_t>((ret_addr + 1) + 3);

        return true;
    }

    // Implement OPEN ($C8)
    if (call_num == 0xC8) {
        if (param_list + 6 >= Bus::MEMORY_SIZE) {
            std::cerr << "OPEN ($C8): param_list + 6 >= MEMORY_SIZE (param_list=$" << std::hex
                      << std::uppercase << std::setw(4) << std::setfill('0') << param_list << ")"
                      << std::endl;
            write_memory_dump(bus, "memory_dump.bin");
            log_call_details("error");
            return false;
        }

        uint16_t pathname_ptr = read_word_mem(mem, static_cast<uint16_t>(param_list + 1));
        uint16_t refnum_ptr = read_word_mem(mem, static_cast<uint16_t>(param_list + 3));
        uint16_t iobuf_ptr = read_word_mem(mem, static_cast<uint16_t>(param_list + 5));
        (void)iobuf_ptr; // unused for now

        if (pathname_ptr >= Bus::MEMORY_SIZE || refnum_ptr >= Bus::MEMORY_SIZE) {
            std::cerr << "OPEN ($C8): invalid pointers (pathname_ptr=$" << std::hex
                      << std::uppercase << std::setw(4) << std::setfill('0') << pathname_ptr
                      << ", refnum_ptr=$" << std::setw(4) << std::setfill('0') << refnum_ptr << ")"
                      << std::endl;
            write_memory_dump(bus, "memory_dump.bin");
            log_call_details("error");
            return false;
        }

        uint8_t path_len = mem[pathname_ptr];
        if (path_len == 0 || pathname_ptr + path_len >= Bus::MEMORY_SIZE) {
            std::cerr << "OPEN ($C8): invalid path_len (path_len=" << std::dec
                      << static_cast<int>(path_len) << ", pathname_ptr=$" << std::hex
                      << std::uppercase << std::setw(4) << std::setfill('0') << pathname_ptr << ")"
                      << std::endl;
            write_memory_dump(bus, "memory_dump.bin");
            log_call_details("error");
            return false;
        }

        std::string prodos_path;
        prodos_path.reserve(path_len);
        for (uint8_t i = 0; i < path_len; ++i) {
            prodos_path.push_back(static_cast<char>(mem[pathname_ptr + 1 + i] & 0x7F));
        }

        std::string host_path = prodos_path_to_host(prodos_path);

        int ref = alloc_refnum();
        if (ref < 0) {
            std::cerr << "OPEN ($C8): too many files open (ERR_TOO_MANY_FILES)" << std::endl;
            write_memory_dump(bus, "memory_dump.bin");
            log_call_details("error");
            return false;
        }

        // Try opening for read/write first, fallback to read-only
        FILE *fp = std::fopen(host_path.c_str(), "r+b");
        if (!fp) {
            fp = std::fopen(host_path.c_str(), "rb");
        }
        if (!fp) {
            std::cerr << "OPEN ($C8): file not found: " << host_path << " (ERR_FILE_NOT_FOUND)"
                      << std::endl;
            write_memory_dump(bus, "memory_dump.bin");
            log_call_details("error");
            cpu.A = 0x46; // File not found
            cpu.P |= StatusFlags::C;
            return_to_caller();
            return true;
        }

        // Get file size
        std::fseek(fp, 0, SEEK_END);
        long file_size = std::ftell(fp);
        std::fseek(fp, 0, SEEK_SET);

        // Store file entry
        FileEntry &entry = s_file_table[ref];
        entry.used = true;
        entry.fp = fp;
        entry.host_path = host_path;
        entry.mark = 0;
        entry.file_size = static_cast<uint32_t>(file_size);

        // Write refnum back to caller's parameter
        bus.write(refnum_ptr, static_cast<uint8_t>(ref));

        if (s_trace_enabled) {
            std::cout << "OPEN ($C8): opened " << host_path << " as refnum " << ref
                      << ", file_size=" << file_size << std::endl;
        }

        set_success(cpu);
        return_to_caller();
        return true;
    }

    // Implement READ ($CA)
    if (call_num == 0xCA) {
        if (param_list + 7 >= Bus::MEMORY_SIZE) {
            std::cerr << "READ ($CA): param_list + 7 >= MEMORY_SIZE (param_list=$" << std::hex
                      << std::uppercase << std::setw(4) << std::setfill('0') << param_list << ")"
                      << std::endl;
            write_memory_dump(bus, "memory_dump.bin");
            log_call_details("error");
            return false;
        }

        uint8_t refnum = mem[param_list + 1];
        uint16_t data_buffer = read_word_mem(mem, static_cast<uint16_t>(param_list + 2));
        uint16_t request_count = read_word_mem(mem, static_cast<uint16_t>(param_list + 4));
        uint16_t trans_count_ptr = static_cast<uint16_t>(param_list + 6);

        if (s_trace_enabled) {
            std::cout << "READ ($CA): refnum=" << std::dec << static_cast<int>(refnum)
                      << ", data_buffer=$" << std::hex << std::uppercase << std::setw(4)
                      << std::setfill('0') << data_buffer << ", request_count=" << std::dec
                      << request_count << std::endl;
        }

        FileEntry *entry = get_refnum(refnum);
        if (!entry) {
            std::cerr << "READ ($CA): invalid refnum (" << std::dec << static_cast<int>(refnum)
                      << ")" << std::endl;
            write_memory_dump(bus, "memory_dump.bin");
            log_call_details("error");
            cpu.A = 0x43; // Invalid reference number
            cpu.P |= StatusFlags::C;
            return_to_caller();
            return true;
        }

        if (data_buffer + request_count > Bus::MEMORY_SIZE) {
            std::cerr << "READ ($CA): buffer overflow (data_buffer=$" << std::hex << std::uppercase
                      << std::setw(4) << std::setfill('0') << data_buffer
                      << ", request_count=" << std::dec << request_count << ")" << std::endl;
            write_memory_dump(bus, "memory_dump.bin");
            log_call_details("error");
            cpu.A = 0x56; // Bad buffer address
            cpu.P |= StatusFlags::C;
            return_to_caller();
            return true;
        }

        if (!entry->fp) {
            std::cerr << "READ ($CA): file not open" << std::endl;
            write_memory_dump(bus, "memory_dump.bin");
            log_call_details("error");
            cpu.A = 0x43; // Invalid reference number
            cpu.P |= StatusFlags::C;
            return_to_caller();
            return true;
        }

        // Seek to current mark position
        if (std::fseek(entry->fp, static_cast<long>(entry->mark), SEEK_SET) != 0) {
            std::cerr << "READ ($CA): fseek failed" << std::endl;
            write_memory_dump(bus, "memory_dump.bin");
            log_call_details("error");
            cpu.A = 0x27; // I/O error
            cpu.P |= StatusFlags::C;
            return_to_caller();
            return true;
        }

        // Read from file
        uint16_t bytes_to_read = request_count;
        uint32_t bytes_available = entry->file_size - entry->mark;
        if (bytes_to_read > bytes_available) {
            bytes_to_read = static_cast<uint16_t>(bytes_available);
        }

        uint16_t actual_read = 0;
        if (bytes_to_read > 0) {
            std::vector<uint8_t> buffer(bytes_to_read);
            size_t n = std::fread(buffer.data(), 1, bytes_to_read, entry->fp);
            actual_read = static_cast<uint16_t>(n);

            // Write to memory
            for (uint16_t i = 0; i < actual_read; ++i) {
                bus.write(static_cast<uint16_t>(data_buffer + i), buffer[i]);
            }

            // Update mark
            entry->mark += actual_read;
        }

        // Write trans_count
        bus.write(trans_count_ptr, static_cast<uint8_t>(actual_read & 0xFF));
        bus.write(static_cast<uint16_t>(trans_count_ptr + 1),
                  static_cast<uint8_t>((actual_read >> 8) & 0xFF));

        if (s_trace_enabled) {
            std::cout << "READ ($CA): read " << std::dec << actual_read
                      << " bytes, new mark=" << entry->mark << std::endl;
        }

        // Return EOF error only if zero bytes were transferred
        if (actual_read == 0 && request_count > 0) {
            cpu.A = 0x4C; // End of file
            cpu.P |= StatusFlags::C;
        } else {
            set_success(cpu);
        }

        return_to_caller();
        return true;
    }

    // Implement WRITE ($CB)
    if (call_num == 0xCB) {
        if (param_list + 7 >= Bus::MEMORY_SIZE) {
            std::cerr << "WRITE ($CB): param_list + 7 >= MEMORY_SIZE (param_list=$" << std::hex
                      << std::uppercase << std::setw(4) << std::setfill('0') << param_list << ")"
                      << std::endl;
            write_memory_dump(bus, "memory_dump.bin");
            log_call_details("error");
            return false;
        }

        uint8_t refnum = mem[param_list + 1];
        uint16_t data_buffer = read_word_mem(mem, static_cast<uint16_t>(param_list + 2));
        uint16_t request_count = read_word_mem(mem, static_cast<uint16_t>(param_list + 4));
        uint16_t trans_count_ptr = static_cast<uint16_t>(param_list + 6);

        if (s_trace_enabled) {
            std::cout << "WRITE ($CB): refnum=" << std::dec << static_cast<int>(refnum)
                      << ", data_buffer=$" << std::hex << std::uppercase << std::setw(4)
                      << std::setfill('0') << data_buffer << ", request_count=" << std::dec
                      << request_count << std::endl;
        }

        FileEntry *entry = get_refnum(refnum);
        if (!entry) {
            std::cerr << "WRITE ($CB): invalid refnum (" << std::dec << static_cast<int>(refnum)
                      << ")" << std::endl;
            write_memory_dump(bus, "memory_dump.bin");
            log_call_details("error");
            cpu.A = 0x43; // Invalid reference number
            cpu.P |= StatusFlags::C;
            return_to_caller();
            return true;
        }

        if (data_buffer + request_count > Bus::MEMORY_SIZE) {
            std::cerr << "WRITE ($CB): buffer overflow (data_buffer=$" << std::hex << std::uppercase
                      << std::setw(4) << std::setfill('0') << data_buffer
                      << ", request_count=" << std::dec << request_count << ")" << std::endl;
            write_memory_dump(bus, "memory_dump.bin");
            log_call_details("error");
            cpu.A = 0x56; // Bad buffer address
            cpu.P |= StatusFlags::C;
            return_to_caller();
            return true;
        }

        if (!entry->fp) {
            std::cerr << "WRITE ($CB): file not open" << std::endl;
            write_memory_dump(bus, "memory_dump.bin");
            log_call_details("error");
            cpu.A = 0x43; // Invalid reference number
            cpu.P |= StatusFlags::C;
            return_to_caller();
            return true;
        }

        // Seek to current mark position
        if (std::fseek(entry->fp, static_cast<long>(entry->mark), SEEK_SET) != 0) {
            std::cerr << "WRITE ($CB): fseek failed" << std::endl;
            write_memory_dump(bus, "memory_dump.bin");
            log_call_details("error");
            cpu.A = 0x27; // I/O error
            cpu.P |= StatusFlags::C;
            return_to_caller();
            return true;
        }

        // Read from memory and write to file
        std::vector<uint8_t> buffer(request_count);
        for (uint16_t i = 0; i < request_count; ++i) {
            buffer[i] = mem[data_buffer + i];
        }

        size_t actual_written = std::fwrite(buffer.data(), 1, request_count, entry->fp);
        uint16_t trans_count = static_cast<uint16_t>(actual_written);

        // Update mark and file size if necessary
        entry->mark += trans_count;
        if (entry->mark > entry->file_size) {
            entry->file_size = entry->mark;
        }

        // Write trans_count
        bus.write(trans_count_ptr, static_cast<uint8_t>(trans_count & 0xFF));
        bus.write(static_cast<uint16_t>(trans_count_ptr + 1),
                  static_cast<uint8_t>((trans_count >> 8) & 0xFF));

        if (s_trace_enabled) {
            std::cout << "WRITE ($CB): wrote " << std::dec << trans_count
                      << " bytes, new mark=" << entry->mark << ", file_size=" << entry->file_size
                      << std::endl;
        }

        if (trans_count < request_count) {
            cpu.A = 0x48; // Overrun error: not enough disk space
            cpu.P |= StatusFlags::C;
        } else {
            set_success(cpu);
        }

        return_to_caller();
        return true;
    }

    // Implement CLOSE ($CC)
    if (call_num == 0xCC) {
        if (param_list + 1 >= Bus::MEMORY_SIZE) {
            std::cerr << "CLOSE ($CC): param_list + 1 >= MEMORY_SIZE (param_list=$" << std::hex
                      << std::uppercase << std::setw(4) << std::setfill('0') << param_list << ")"
                      << std::endl;
            write_memory_dump(bus, "memory_dump.bin");
            log_call_details("error");
            return false;
        }

        uint8_t refnum = mem[param_list + 1];

        if (s_trace_enabled) {
            std::cout << "CLOSE ($CC): refnum=" << std::dec << static_cast<int>(refnum)
                      << std::endl;
        }

        // Special case: refnum == 0 means close all files at or above current level
        // For simplicity, we'll close all files
        if (refnum == 0) {
            for (size_t i = 1; i < s_file_table.size(); ++i) {
                if (s_file_table[i].used) {
                    close_entry(s_file_table[i]);
                }
            }
            if (s_trace_enabled) {
                std::cout << "CLOSE ($CC): closed all files" << std::endl;
            }
            set_success(cpu);
            return_to_caller();
            return true;
        }

        FileEntry *entry = get_refnum(refnum);
        if (!entry) {
            std::cerr << "CLOSE ($CC): invalid refnum (" << std::dec << static_cast<int>(refnum)
                      << ")" << std::endl;
            write_memory_dump(bus, "memory_dump.bin");
            log_call_details("error");
            cpu.A = 0x43; // Invalid reference number
            cpu.P |= StatusFlags::C;
            return_to_caller();
            return true;
        }

        if (s_trace_enabled) {
            std::cout << "CLOSE ($CC): closing " << entry->host_path << std::endl;
        }

        close_entry(*entry);
        set_success(cpu);
        return_to_caller();
        return true;
    }

    // Implement FLUSH ($CD)
    if (call_num == 0xCD) {
        if (param_list + 1 >= Bus::MEMORY_SIZE) {
            std::cerr << "FLUSH ($CD): param_list + 1 >= MEMORY_SIZE (param_list=$" << std::hex
                      << std::uppercase << std::setw(4) << std::setfill('0') << param_list << ")"
                      << std::endl;
            write_memory_dump(bus, "memory_dump.bin");
            log_call_details("error");
            return false;
        }

        uint8_t refnum = mem[param_list + 1];

        if (s_trace_enabled) {
            std::cout << "FLUSH ($CD): refnum=" << std::dec << static_cast<int>(refnum)
                      << std::endl;
        }

        // Special case: refnum == 0 means flush all files at or above current level
        if (refnum == 0) {
            for (size_t i = 1; i < s_file_table.size(); ++i) {
                if (s_file_table[i].used && s_file_table[i].fp) {
                    std::fflush(s_file_table[i].fp);
                }
            }
            if (s_trace_enabled) {
                std::cout << "FLUSH ($CD): flushed all files" << std::endl;
            }
            set_success(cpu);
            return_to_caller();
            return true;
        }

        FileEntry *entry = get_refnum(refnum);
        if (!entry) {
            std::cerr << "FLUSH ($CD): invalid refnum (" << std::dec << static_cast<int>(refnum)
                      << ")" << std::endl;
            write_memory_dump(bus, "memory_dump.bin");
            log_call_details("error");
            cpu.A = 0x43; // Invalid reference number
            cpu.P |= StatusFlags::C;
            return_to_caller();
            return true;
        }

        if (entry->fp) {
            std::fflush(entry->fp);
        }

        if (s_trace_enabled) {
            std::cout << "FLUSH ($CD): flushed " << entry->host_path << std::endl;
        }

        set_success(cpu);
        return_to_caller();
        return true;
    }

    // Implement SET_MARK ($CE)
    if (call_num == 0xCE) {
        if (param_list + 3 >= Bus::MEMORY_SIZE) {
            std::cerr << "SET_MARK ($CE): param_list + 3 >= MEMORY_SIZE (param_list=$" << std::hex
                      << std::uppercase << std::setw(4) << std::setfill('0') << param_list << ")"
                      << std::endl;
            write_memory_dump(bus, "memory_dump.bin");
            log_call_details("error");
            return false;
        }
        uint8_t refnum = mem[param_list + 1];
        uint16_t mark_ptr = read_word_mem(mem, static_cast<uint16_t>(param_list + 2));
        if (mark_ptr + 1 >= Bus::MEMORY_SIZE) {
            std::cerr << "SET_MARK ($CE): mark_ptr + 1 >= MEMORY_SIZE (mark_ptr=$" << std::hex
                      << std::uppercase << std::setw(4) << std::setfill('0') << mark_ptr << ")"
                      << std::endl;
            write_memory_dump(bus, "memory_dump.bin");
            log_call_details("error");
            return false;
        }
        uint16_t new_mark = read_word_mem(mem, mark_ptr);
        FileEntry *entry = get_refnum(refnum);
        if (!entry) {
            std::cerr << "SET_MARK ($CE): invalid refnum (" << std::dec << static_cast<int>(refnum)
                      << ")" << std::endl;
            write_memory_dump(bus, "memory_dump.bin");
            log_call_details("error");
            return false;
        }
        entry->mark = std::min<uint32_t>(new_mark, entry->file_size);
        set_success(cpu);
        return_to_caller();
        return true;
    }

    // Implement GET_MARK ($CF)
    if (call_num == 0xCF) {
        if (param_list + 3 >= Bus::MEMORY_SIZE) {
            std::cerr << "GET_MARK ($CF): param_list + 3 >= MEMORY_SIZE (param_list=$" << std::hex
                      << std::uppercase << std::setw(4) << std::setfill('0') << param_list << ")"
                      << std::endl;
            write_memory_dump(bus, "memory_dump.bin");
            log_call_details("error");
            return false;
        }
        uint8_t refnum = mem[param_list + 1];
        uint16_t mark_ptr = read_word_mem(mem, static_cast<uint16_t>(param_list + 2));
        if (mark_ptr + 1 >= Bus::MEMORY_SIZE) {
            std::cerr << "GET_MARK ($CF): mark_ptr + 1 >= MEMORY_SIZE (mark_ptr=$" << std::hex
                      << std::uppercase << std::setw(4) << std::setfill('0') << mark_ptr << ")"
                      << std::endl;
            write_memory_dump(bus, "memory_dump.bin");
            log_call_details("error");
            return false;
        }
        FileEntry *entry = get_refnum(refnum);
        if (!entry) {
            std::cerr << "GET_MARK ($CF): invalid refnum (" << std::dec << static_cast<int>(refnum)
                      << ")" << std::endl;
            write_memory_dump(bus, "memory_dump.bin");
            log_call_details("error");
            return false;
        }
        uint16_t mark = static_cast<uint16_t>(entry->mark & 0xFFFF);
        bus.write(mark_ptr, static_cast<uint8_t>(mark & 0xFF));
        bus.write(static_cast<uint16_t>(mark_ptr + 1), static_cast<uint8_t>((mark >> 8) & 0xFF));
        set_success(cpu);
        return_to_caller();
        return true;
    }

    // Implement GET_EOF ($D1)
    if (call_num == 0xD1) {
        if (param_list + 3 >= Bus::MEMORY_SIZE) {
            std::cerr << "GET_EOF ($D1): param_list + 3 >= MEMORY_SIZE (param_list=$" << std::hex
                      << std::uppercase << std::setw(4) << std::setfill('0') << param_list << ")"
                      << std::endl;
            write_memory_dump(bus, "memory_dump.bin");
            log_call_details("error");
            return false;
        }
        uint8_t refnum = mem[param_list + 1];
        uint16_t eof_ptr = read_word_mem(mem, static_cast<uint16_t>(param_list + 2));
        if (eof_ptr + 1 >= Bus::MEMORY_SIZE) {
            std::cerr << "GET_EOF ($D1): eof_ptr + 1 >= MEMORY_SIZE (eof_ptr=$" << std::hex
                      << std::uppercase << std::setw(4) << std::setfill('0') << eof_ptr << ")"
                      << std::endl;
            write_memory_dump(bus, "memory_dump.bin");
            log_call_details("error");
            return false;
        }
        FileEntry *entry = get_refnum(refnum);
        if (!entry) {
            std::cerr << "GET_EOF ($D1): invalid refnum (" << std::dec << static_cast<int>(refnum)
                      << ")" << std::endl;
            write_memory_dump(bus, "memory_dump.bin");
            log_call_details("error");
            return false;
        }
        uint16_t eof_val = static_cast<uint16_t>(entry->file_size & 0xFFFF);
        bus.write(eof_ptr, static_cast<uint8_t>(eof_val & 0xFF));
        bus.write(static_cast<uint16_t>(eof_ptr + 1), static_cast<uint8_t>((eof_val >> 8) & 0xFF));
        set_success(cpu);
        return_to_caller();
        return true;
    }

    // Implement GET_FILE_INFO ($C4)
    if (call_num == 0xC4) {
        if (param_list + 1 >= Bus::MEMORY_SIZE) {
            std::cerr << "GET_FILE_INFO ($C4): param_list + 1 >= MEMORY_SIZE (param_list=$"
                      << std::hex << std::uppercase << std::setw(4) << std::setfill('0')
                      << param_list << ")" << std::endl;
            write_memory_dump(bus, "memory_dump.bin");
            log_call_details("error");
            return false;
        }

        uint8_t pcount = mem[param_list];
        if (pcount < 1 || param_list + (pcount * 2) >= Bus::MEMORY_SIZE) {
            std::cerr << "GET_FILE_INFO ($C4): invalid pcount (" << std::dec
                      << static_cast<int>(pcount) << ", param_list=$" << std::hex << std::uppercase
                      << std::setw(4) << std::setfill('0') << param_list << ")" << std::endl;
            write_memory_dump(bus, "memory_dump.bin");
            log_call_details("error");
            return false;
        }

        std::vector<uint16_t> params;
        params.reserve(pcount);
        for (uint8_t i = 0; i < pcount; ++i) {
            params.push_back(read_word_mem(mem, static_cast<uint16_t>(param_list + 1 + i * 2)));
        }

        uint16_t pathname_ptr = params[0];
        if (pathname_ptr >= Bus::MEMORY_SIZE) {
            std::cerr << "GET_FILE_INFO ($C4): pathname_ptr >= MEMORY_SIZE (pathname_ptr=$"
                      << std::hex << std::uppercase << std::setw(4) << std::setfill('0')
                      << pathname_ptr << ")" << std::endl;
            write_memory_dump(bus, "memory_dump.bin");
            log_call_details("error");
            return false;
        }
        uint8_t path_len = mem[pathname_ptr];
        if (path_len == 0 || pathname_ptr + path_len >= Bus::MEMORY_SIZE || path_len > 64) {
            std::cerr << "GET_FILE_INFO ($C4): invalid path_len (path_len=" << std::dec
                      << static_cast<int>(path_len) << ", pathname_ptr=$" << std::hex
                      << std::uppercase << std::setw(4) << std::setfill('0') << pathname_ptr << ")"
                      << std::endl;
            write_memory_dump(bus, "memory_dump.bin");
            log_call_details("error");
            return false;
        }
        std::string prodos_path;
        prodos_path.reserve(path_len);
        for (uint8_t i = 0; i < path_len; ++i) {
            prodos_path.push_back(static_cast<char>(mem[pathname_ptr + 1 + i] & 0x7F));
        }
        std::string host_path = prodos_path_to_host(prodos_path);

        std::error_code ec;
        auto file_size = std::filesystem::file_size(host_path, ec);
        if (ec) {
            std::cerr << "GET_FILE_INFO ($C4): file not found: " << host_path
                      << " (error: " << ec.message() << ")" << std::endl;
            write_memory_dump(bus, "memory_dump.bin");
            log_call_details("error");
            return false;
        }

        uint32_t size32 = static_cast<uint32_t>(file_size);
        uint16_t blocks_used = static_cast<uint16_t>((size32 + 511) / 512);

        auto write_byte = [&](size_t idx, uint8_t value) {
            if (idx < params.size() && params[idx] < Bus::MEMORY_SIZE) {
                bus.write(params[idx], value);
            }
        };

        auto write_word = [&](size_t idx, uint16_t value) {
            if (idx < params.size() && params[idx] + 1 < Bus::MEMORY_SIZE) {
                bus.write(params[idx], static_cast<uint8_t>(value & 0xFF));
                bus.write(static_cast<uint16_t>(params[idx] + 1),
                          static_cast<uint8_t>((value >> 8) & 0xFF));
            }
        };

        auto write_eof = [&](size_t idx, uint32_t value) {
            if (idx < params.size() && params[idx] + 2 < Bus::MEMORY_SIZE) {
                bus.write(params[idx], static_cast<uint8_t>(value & 0xFF));
                bus.write(static_cast<uint16_t>(params[idx] + 1),
                          static_cast<uint8_t>((value >> 8) & 0xFF));
                bus.write(static_cast<uint16_t>(params[idx] + 2),
                          static_cast<uint8_t>((value >> 16) & 0xFF));
            }
        };

        write_byte(1, 0xC3);        // access: read/write/delete
        write_byte(2, 0x06);        // file type: BIN
        write_word(3, 0x0000);      // aux type
        write_byte(4, 0x01);        // storage type: standard file
        write_word(5, blocks_used); // blocks used
        write_eof(6, size32);       // EOF
        write_word(7, 0);           // create date
        write_word(8, 0);           // create time
        write_word(9, 0);           // mod date/time best-effort
        if (params.size() > 10)
            write_word(10, 0);

        set_success(cpu);
        return_to_caller();
        return true;
    }

    log_call_details("halt");
    std::cout << std::endl;
    std::cout << "=== HALTING - ProDOS MLI not implemented ===" << std::endl;

    write_memory_dump(bus, "memory_dump.bin");

    return false;
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
