#include "edasm/emulator/mli.hpp"
#include "edasm/emulator/traps.hpp"
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

    // 1:1 mapping policy: absolute ProDOS paths map to Linux absolute paths,
    // relative ProDOS paths are resolved against the Linux current directory.
    std::filesystem::path host = absolute ? (std::filesystem::path("/") / clean)
                                          : (std::filesystem::path(current_prefix()) / clean);
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

constexpr uint8_t ERR_PATH_NOT_FOUND = 0x4B;
constexpr uint8_t ERR_FILE_NOT_FOUND = 0x46;
constexpr uint8_t ERR_TOO_MANY_FILES = 0x52;
constexpr uint8_t ERR_ILLEGAL_PARAM = 0x2C;

} // namespace

void MLIHandler::set_trace(bool enabled) {
    TrapManager::set_trace(enabled);
}

bool MLIHandler::is_trace_enabled() {
    return TrapManager::is_trace_enabled();
}

void MLIHandler::set_success(CPUState &cpu) {
    cpu.A = 0;
    cpu.P &= ~StatusFlags::C;
    cpu.P &= ~StatusFlags::N;
    cpu.P &= ~StatusFlags::V;
    cpu.P |= StatusFlags::Z;
    cpu.P |= StatusFlags::U;
}

void MLIHandler::set_error(CPUState &cpu, uint8_t err) {
    cpu.A = err;
    cpu.P |= StatusFlags::C;
    cpu.P &= ~StatusFlags::Z;
    cpu.P |= StatusFlags::U;
}

void MLIHandler::set_error(CPUState &cpu, ProDOSError err) {
    set_error(cpu, static_cast<uint8_t>(err));
}

bool MLIHandler::write_memory_dump(const Bus &bus, const std::string &filename) {
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

// MLI Call Descriptors
// Based on Apple ProDOS 8 Technical Reference Manual, Chapter 4

namespace {
using PT = MLIParamType;
using PD = MLIParamDirection;

// Helper macros for cleaner descriptor definitions
#define PARAM(type, dir, name) MLIParamDescriptor{PT::type, PD::dir, name}
#define IN PARAM
#define OUT PARAM
#define INOUT PARAM

// Descriptor table (static storage)
static std::array<MLICallDescriptor, 26> s_call_descriptors = {{
    // System calls
    {0x40, "ALLOC_INTERRUPT", 2, {{
        IN(BYTE, INPUT, "int_num"),
        IN(WORD, INPUT, "int_code"),
    }}, nullptr},
    {0x41, "DEALLOC_INTERRUPT", 1, {{
        IN(BYTE, INPUT, "int_num"),
    }}, nullptr},
    {0x65, "QUIT", 4, {{
        IN(BYTE, INPUT, "quit_type"),
        IN(WORD, INPUT, "reserved1"),
        IN(BYTE, INPUT, "reserved2"),
        IN(WORD, INPUT, "reserved3"),
    }}, nullptr},
    {0x82, "GET_TIME", 1, {{
        IN(WORD, INPUT, "date_time_ptr"),
    }}, &MLIHandler::handle_get_time},

    // Block device calls
    {0x80, "READ_BLOCK", 3, {{
        IN(BYTE, INPUT, "unit_num"),
        IN(BUFFER_PTR, INPUT, "data_buffer"),
        IN(WORD, INPUT, "block_num"),
    }}, nullptr},
    {0x81, "WRITE_BLOCK", 3, {{
        IN(BYTE, INPUT, "unit_num"),
        IN(BUFFER_PTR, INPUT, "data_buffer"),
        IN(WORD, INPUT, "block_num"),
    }}, nullptr},

    // Housekeeping calls
    {0xC0, "CREATE", 7, {{
        IN(PATHNAME_PTR, INPUT, "pathname"),
        IN(BYTE, INPUT, "access"),
        IN(BYTE, INPUT, "file_type"),
        IN(WORD, INPUT, "aux_type"),
        IN(BYTE, INPUT, "storage_type"),
        IN(WORD, INPUT, "create_date"),
        IN(WORD, INPUT, "create_time"),
    }}, nullptr},
    {0xC1, "DESTROY", 1, {{
        IN(PATHNAME_PTR, INPUT, "pathname"),
    }}, nullptr},
    {0xC2, "RENAME", 2, {{
        IN(PATHNAME_PTR, INPUT, "pathname"),
        IN(PATHNAME_PTR, INPUT, "new_pathname"),
    }}, nullptr},
    {0xC3, "SET_FILE_INFO", 7, {{
        IN(PATHNAME_PTR, INPUT, "pathname"),
        IN(BYTE, INPUT, "access"),
        IN(BYTE, INPUT, "file_type"),
        IN(WORD, INPUT, "aux_type"),
        IN(BYTE, INPUT, "reserved1"),
        IN(WORD, INPUT, "mod_date"),
        IN(WORD, INPUT, "mod_time"),
    }}, nullptr},
    {0xC4, "GET_FILE_INFO", 10, {{
        IN(PATHNAME_PTR, INPUT, "pathname"),
        OUT(BYTE, OUTPUT, "access"),
        OUT(BYTE, OUTPUT, "file_type"),
        OUT(WORD, OUTPUT, "aux_type"),
        OUT(BYTE, OUTPUT, "storage_type"),
        OUT(WORD, OUTPUT, "blocks_used"),
        OUT(WORD, OUTPUT, "mod_date"),
        OUT(WORD, OUTPUT, "mod_time"),
        OUT(WORD, OUTPUT, "create_date"),
        OUT(WORD, OUTPUT, "create_time"),
    }}, &MLIHandler::handle_get_file_info},
    {0xC5, "ONLINE", 2, {{
        IN(BYTE, INPUT, "unit_num"),
        IN(BUFFER_PTR, INPUT_OUTPUT, "data_buffer"),
    }}, nullptr},
    {0xC6, "SET_PREFIX", 1, {{
        IN(PATHNAME_PTR, INPUT, "pathname"),
    }}, &MLIHandler::handle_set_prefix},
    {0xC7, "GET_PREFIX", 1, {{
        IN(BUFFER_PTR, OUTPUT, "data_buffer"),
    }}, &MLIHandler::handle_get_prefix},

    // Filing calls
    {0xC8, "OPEN", 3, {{
        IN(PATHNAME_PTR, INPUT, "pathname"),
        IN(BUFFER_PTR, INPUT, "io_buffer"),
        OUT(REF_NUM, OUTPUT, "ref_num"),
    }}, &MLIHandler::handle_open},
    {0xC9, "NEWLINE", 3, {{
        IN(REF_NUM, INPUT, "ref_num"),
        IN(BYTE, INPUT, "enable_mask"),
        IN(BYTE, INPUT, "newline_char"),
    }}, nullptr},
    {0xCA, "READ", 4, {{
        IN(REF_NUM, INPUT, "ref_num"),
        IN(BUFFER_PTR, INPUT_OUTPUT, "data_buffer"),
        IN(WORD, INPUT, "request_count"),
        OUT(WORD, OUTPUT, "transfer_count"),
    }}, &MLIHandler::handle_read},
    {0xCB, "WRITE", 4, {{
        IN(REF_NUM, INPUT, "ref_num"),
        IN(BUFFER_PTR, INPUT, "data_buffer"),
        IN(WORD, INPUT, "request_count"),
        OUT(WORD, OUTPUT, "transfer_count"),
    }}, &MLIHandler::handle_write},
    {0xCC, "CLOSE", 1, {{
        IN(REF_NUM, INPUT, "ref_num"),
    }}, &MLIHandler::handle_close},
    {0xCD, "FLUSH", 1, {{
        IN(REF_NUM, INPUT, "ref_num"),
    }}, &MLIHandler::handle_flush},
    {0xCE, "SET_MARK", 2, {{
        IN(REF_NUM, INPUT, "ref_num"),
        IN(THREE_BYTE, INPUT, "position"),
    }}, &MLIHandler::handle_set_mark},
    {0xCF, "GET_MARK", 2, {{
        IN(REF_NUM, INPUT, "ref_num"),
        OUT(THREE_BYTE, OUTPUT, "position"),
    }}, &MLIHandler::handle_get_mark},
    {0xD0, "SET_EOF", 2, {{
        IN(REF_NUM, INPUT, "ref_num"),
        IN(THREE_BYTE, INPUT, "eof"),
    }}, nullptr},
    {0xD1, "GET_EOF", 2, {{
        IN(REF_NUM, INPUT, "ref_num"),
        OUT(THREE_BYTE, OUTPUT, "eof"),
    }}, &MLIHandler::handle_get_eof},
    {0xD2, "SET_BUF", 2, {{
        IN(REF_NUM, INPUT, "ref_num"),
        IN(BUFFER_PTR, INPUT, "io_buffer"),
    }}, nullptr},
    {0xD3, "GET_BUF", 2, {{
        IN(REF_NUM, INPUT, "ref_num"),
        OUT(BUFFER_PTR, OUTPUT, "io_buffer"),
    }}, nullptr},
}};

// Lookup table: maps call_number to index in s_call_descriptors
// Initialized to -1 (0xFF) for invalid call numbers
static std::array<uint8_t, 256> s_call_lookup_table = []() {
    std::array<uint8_t, 256> table;
    table.fill(0xFF); // Initialize all entries to invalid
    
    // Build lookup table from descriptor array
    for (size_t i = 0; i < s_call_descriptors.size(); ++i) {
        uint8_t call_num = s_call_descriptors[i].call_number;
        table[call_num] = static_cast<uint8_t>(i);
    }
    
    return table;
}();

#undef PARAM
#undef IN
#undef OUT
#undef INOUT

} // anonymous namespace

const MLICallDescriptor *MLIHandler::get_call_descriptor(uint8_t call_num) {
    uint8_t index = s_call_lookup_table[call_num];
    if (index != 0xFF) {
        return &s_call_descriptors[index];
    }
    return nullptr;
}

std::vector<MLIParamValue> MLIHandler::read_input_params(
    const Bus &bus, uint16_t param_list_addr, const MLICallDescriptor &desc) {
    
    std::vector<MLIParamValue> values;
    const uint8_t *mem = bus.data();
    
    // Skip parameter count byte
    uint16_t offset = 1;
    
    for (uint8_t i = 0; i < desc.param_count; ++i) {
        const auto &param = desc.params[i];
        
        // Only read INPUT and INPUT_OUTPUT parameters
        if (param.direction == MLIParamDirection::OUTPUT) {
            values.push_back(uint8_t(0)); // Placeholder for output-only params
            continue;
        }
        
        switch (param.type) {
        case MLIParamType::BYTE:
        case MLIParamType::REF_NUM: {
            uint8_t val = mem[param_list_addr + offset];
            values.push_back(val);
            offset += 1;
            break;
        }
        case MLIParamType::WORD: {
            uint16_t val = mem[param_list_addr + offset] | 
                          (mem[param_list_addr + offset + 1] << 8);
            values.push_back(val);
            offset += 2;
            break;
        }
        case MLIParamType::THREE_BYTE: {
            uint32_t val = mem[param_list_addr + offset] | 
                          (mem[param_list_addr + offset + 1] << 8) |
                          (mem[param_list_addr + offset + 2] << 16);
            values.push_back(val);
            offset += 3;
            break;
        }
        case MLIParamType::PATHNAME_PTR: {
            uint16_t ptr = mem[param_list_addr + offset] | 
                          (mem[param_list_addr + offset + 1] << 8);
            offset += 2;
            
            // Read length-prefixed pathname
            if (ptr < Bus::MEMORY_SIZE) {
                uint8_t len = mem[ptr];
                std::string pathname;
                for (uint8_t j = 0; j < len && (ptr + 1 + j) < Bus::MEMORY_SIZE; ++j) {
                    pathname += static_cast<char>(mem[ptr + 1 + j]);
                }
                values.push_back(pathname);
            } else {
                values.push_back(std::string(""));
            }
            break;
        }
        case MLIParamType::BUFFER_PTR: {
            uint16_t ptr = mem[param_list_addr + offset] | 
                          (mem[param_list_addr + offset + 1] << 8);
            values.push_back(ptr); // Store as uint16_t for now
            offset += 2;
            break;
        }
        }
    }
    
    return values;
}

void MLIHandler::write_output_params(
    Bus &bus, uint16_t param_list_addr, const MLICallDescriptor &desc,
    const std::vector<MLIParamValue> &values) {
    
    if (values.size() != desc.param_count) {
        std::cerr << "Warning: Parameter count mismatch in write_output_params" << std::endl;
        return;
    }
    
    // Skip parameter count byte
    uint16_t offset = 1;
    
    for (uint8_t i = 0; i < desc.param_count; ++i) {
        const auto &param = desc.params[i];
        
        // Only write OUTPUT and INPUT_OUTPUT parameters
        if (param.direction == MLIParamDirection::INPUT) {
            // Skip input-only params
            switch (param.type) {
            case MLIParamType::BYTE:
            case MLIParamType::REF_NUM:
                offset += 1;
                break;
            case MLIParamType::WORD:
            case MLIParamType::PATHNAME_PTR:
            case MLIParamType::BUFFER_PTR:
                offset += 2;
                break;
            case MLIParamType::THREE_BYTE:
                offset += 3;
                break;
            }
            continue;
        }
        
        const auto &value = values[i];
        
        switch (param.type) {
        case MLIParamType::BYTE:
        case MLIParamType::REF_NUM: {
            uint8_t val = std::get<uint8_t>(value);
            bus.write(param_list_addr + offset, val);
            offset += 1;
            break;
        }
        case MLIParamType::WORD: {
            uint16_t val = std::get<uint16_t>(value);
            bus.write(param_list_addr + offset, static_cast<uint8_t>(val & 0xFF));
            bus.write(param_list_addr + offset + 1, static_cast<uint8_t>((val >> 8) & 0xFF));
            offset += 2;
            break;
        }
        case MLIParamType::THREE_BYTE: {
            uint32_t val = std::get<uint32_t>(value);
            bus.write(param_list_addr + offset, static_cast<uint8_t>(val & 0xFF));
            bus.write(param_list_addr + offset + 1, static_cast<uint8_t>((val >> 8) & 0xFF));
            bus.write(param_list_addr + offset + 2, static_cast<uint8_t>((val >> 16) & 0xFF));
            offset += 3;
            break;
        }
        case MLIParamType::PATHNAME_PTR:
        case MLIParamType::BUFFER_PTR:
            // Pointers are not typically written back
            offset += 2;
            break;
        }
    }
}

// Handler implementations

ProDOSError MLIHandler::handle_get_time(Bus &bus, const std::vector<MLIParamValue> &inputs, 
                                        std::vector<MLIParamValue> &outputs) {
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

    if (is_trace_enabled()) {
        std::cout << "GET_TIME: wrote date/time to $BF90-$BF93" << std::endl;
        std::cout << "  Year (since 1900): " << std::dec << static_cast<int>(year) << std::endl;
        std::cout << "  Month: " << static_cast<int>(month) << std::endl;
        std::cout << "  Day: " << static_cast<int>(day) << std::endl;
        std::cout << "  Hour: " << static_cast<int>(hour) << std::endl;
        std::cout << "  Minute: " << static_cast<int>(minute) << std::endl;
    }

    return ProDOSError::NO_ERROR;
}

ProDOSError MLIHandler::handle_set_prefix(Bus &bus, const std::vector<MLIParamValue> &inputs,
                                          std::vector<MLIParamValue> &outputs) {
    std::string prodos_path = std::get<std::string>(inputs[0]);

    if (prodos_path.empty()) {
        prodos_path = "/";
    }

    if (prodos_path.length() > 64) {
        std::cerr << "SET_PREFIX ($C6): path too long (" << prodos_path.length() << " > 64)" << std::endl;
        return ProDOSError::INVALID_PATH_SYNTAX;
    }

    std::filesystem::path target = prodos_path;

    std::error_code ec;
    std::filesystem::path canonical = std::filesystem::weakly_canonical(target, ec);
    std::filesystem::path verify =
        ec ? (std::filesystem::path(current_prefix()) / target) : canonical;
    if (!std::filesystem::is_directory(verify)) {
        std::cerr << "SET_PREFIX ($C6): directory does not exist: " << verify.string()
                  << std::endl;
        return ProDOSError::PATH_NOT_FOUND;
    }

    if (::chdir(target.c_str()) != 0) {
        std::cerr << "SET_PREFIX ($C6): chdir failed: " << ::strerror(errno) << " (path='"
                  << target.string() << "')" << std::endl;
        return ProDOSError::PATH_NOT_FOUND;
    }

    s_prefix_host = current_prefix();
    s_prefix_prodos = normalize_prodos_path(s_prefix_host);

    return ProDOSError::NO_ERROR;
}

ProDOSError MLIHandler::handle_get_prefix(Bus &bus, const std::vector<MLIParamValue> &inputs,
                                          std::vector<MLIParamValue> &outputs) {
    uint16_t buf_ptr = std::get<uint16_t>(inputs[0]);

    if (is_trace_enabled()) {
        std::cout << "GET_PREFIX: buffer ptr=$" << std::hex << std::uppercase << std::setw(4)
                  << std::setfill('0') << buf_ptr << std::endl;
    }

    char cwd_buf[PATH_MAX] = {0};
    if (!::getcwd(cwd_buf, sizeof(cwd_buf))) {
        std::cerr << "GET_PREFIX: getcwd failed: " << ::strerror(errno) << std::endl;
        return ProDOSError::IO_ERROR;
    }

    std::string prefix_str = cwd_buf;
    if (prefix_str.empty() || prefix_str.front() != '/') {
        prefix_str.insert(prefix_str.begin(), '/');
    }
    if (prefix_str.back() != '/') {
        prefix_str.push_back('/');
    }

    if (prefix_str.length() > 64) {
        std::cerr << "GET_PREFIX: prefix too long (" << prefix_str.length()
                  << " chars exceeds 64 byte limit)" << std::endl;
        return ProDOSError::INVALID_PATH_SYNTAX;
    }

    uint8_t prefix_len = static_cast<uint8_t>(prefix_str.length());
    bus.write(buf_ptr, prefix_len);
    if (is_trace_enabled()) {
        std::cout << "GET_PREFIX: writing prefix length=" << std::dec
                  << static_cast<int>(prefix_len) << " prefix=\"" << prefix_str << "\""
                  << std::endl;
    }

    for (size_t i = 0; i < prefix_str.length(); ++i) {
        uint8_t ch = static_cast<uint8_t>(prefix_str[i]) & 0x7F;
        bus.write(static_cast<uint16_t>(buf_ptr + 1 + i), ch);
    }

    return ProDOSError::NO_ERROR;
}

ProDOSError MLIHandler::handle_open(Bus &bus, const std::vector<MLIParamValue> &inputs,
                                    std::vector<MLIParamValue> &outputs) {
    std::string prodos_path = std::get<std::string>(inputs[0]);
    uint16_t iobuf_ptr = std::get<uint16_t>(inputs[1]);
    (void)iobuf_ptr;

    std::string host_path = prodos_path_to_host(prodos_path);

    int ref = alloc_refnum();
    if (ref < 0) {
        std::cerr << "OPEN ($C8): too many files open" << std::endl;
        return ProDOSError::FCB_FULL;
    }

    FILE *fp = std::fopen(host_path.c_str(), "r+b");
    if (!fp) {
        fp = std::fopen(host_path.c_str(), "rb");
    }
    if (!fp) {
        std::cerr << "OPEN ($C8): file not found: " << host_path << std::endl;
        return ProDOSError::FILE_NOT_FOUND;
    }

    std::fseek(fp, 0, SEEK_END);
    long file_size = std::ftell(fp);
    std::fseek(fp, 0, SEEK_SET);

    FileEntry &entry = s_file_table[ref];
    entry.used = true;
    entry.fp = fp;
    entry.host_path = host_path;
    entry.mark = 0;
    entry.file_size = static_cast<uint32_t>(file_size);

    if (is_trace_enabled()) {
        std::cout << "OPEN ($C8): opened " << host_path << " as refnum " << ref
                  << ", file_size=" << file_size << std::endl;
    }

    outputs.push_back(static_cast<uint8_t>(ref));
    return ProDOSError::NO_ERROR;
}

ProDOSError MLIHandler::handle_read(Bus &bus, const std::vector<MLIParamValue> &inputs,
                                    std::vector<MLIParamValue> &outputs) {
    uint8_t refnum = std::get<uint8_t>(inputs[0]);
    uint16_t data_buffer = std::get<uint16_t>(inputs[1]);
    uint16_t request_count = std::get<uint16_t>(inputs[2]);

    if (is_trace_enabled()) {
        std::cout << "READ ($CA): refnum=" << std::dec << static_cast<int>(refnum)
                  << ", data_buffer=$" << std::hex << std::uppercase << std::setw(4)
                  << std::setfill('0') << data_buffer << ", request_count=" << std::dec
                  << request_count << std::endl;
    }

    FileEntry *entry = get_refnum(refnum);
    if (!entry) {
        std::cerr << "READ ($CA): invalid refnum (" << std::dec << static_cast<int>(refnum)
                  << ")" << std::endl;
        return ProDOSError::INVALID_REF_NUM;
    }

    if (data_buffer + request_count > Bus::MEMORY_SIZE) {
        std::cerr << "READ ($CA): buffer overflow (data_buffer=$" << std::hex << std::uppercase
                  << std::setw(4) << std::setfill('0') << data_buffer
                  << ", request_count=" << std::dec << request_count << ")" << std::endl;
        return ProDOSError::BAD_BUFFER_ADDR;
    }

    if (!entry->fp) {
        std::cerr << "READ ($CA): file not open" << std::endl;
        return ProDOSError::INVALID_REF_NUM;
    }

    if (std::fseek(entry->fp, static_cast<long>(entry->mark), SEEK_SET) != 0) {
        std::cerr << "READ ($CA): fseek failed" << std::endl;
        return ProDOSError::IO_ERROR;
    }

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

        for (uint16_t i = 0; i < actual_read; ++i) {
            bus.write(static_cast<uint16_t>(data_buffer + i), buffer[i]);
        }

        entry->mark += actual_read;
    }

    if (is_trace_enabled()) {
        std::cout << "READ ($CA): read " << std::dec << actual_read
                  << " bytes, new mark=" << entry->mark << std::endl;
    }

    outputs.push_back(actual_read);

    if (actual_read == 0 && request_count > 0) {
        return ProDOSError::END_OF_FILE;
    }
    return ProDOSError::NO_ERROR;
}

ProDOSError MLIHandler::handle_write(Bus &bus, const std::vector<MLIParamValue> &inputs,
                                     std::vector<MLIParamValue> &outputs) {
    uint8_t refnum = std::get<uint8_t>(inputs[0]);
    uint16_t data_buffer = std::get<uint16_t>(inputs[1]);
    uint16_t request_count = std::get<uint16_t>(inputs[2]);

    if (is_trace_enabled()) {
        std::cout << "WRITE ($CB): refnum=" << std::dec << static_cast<int>(refnum)
                  << ", data_buffer=$" << std::hex << std::uppercase << std::setw(4)
                  << std::setfill('0') << data_buffer << ", request_count=" << std::dec
                  << request_count << std::endl;
    }

    FileEntry *entry = get_refnum(refnum);
    if (!entry) {
        std::cerr << "WRITE ($CB): invalid refnum (" << std::dec << static_cast<int>(refnum)
                  << ")" << std::endl;
        return ProDOSError::INVALID_REF_NUM;
    }

    if (data_buffer + request_count > Bus::MEMORY_SIZE) {
        std::cerr << "WRITE ($CB): buffer overflow (data_buffer=$" << std::hex << std::uppercase
                  << std::setw(4) << std::setfill('0') << data_buffer
                  << ", request_count=" << std::dec << request_count << ")" << std::endl;
        return ProDOSError::BAD_BUFFER_ADDR;
    }

    if (!entry->fp) {
        std::cerr << "WRITE ($CB): file not open" << std::endl;
        return ProDOSError::INVALID_REF_NUM;
    }

    if (std::fseek(entry->fp, static_cast<long>(entry->mark), SEEK_SET) != 0) {
        std::cerr << "WRITE ($CB): fseek failed" << std::endl;
        return ProDOSError::IO_ERROR;
    }

    const uint8_t *mem = bus.data();
    std::vector<uint8_t> buffer(request_count);
    for (uint16_t i = 0; i < request_count; ++i) {
        buffer[i] = mem[data_buffer + i];
    }

    size_t actual_written = std::fwrite(buffer.data(), 1, request_count, entry->fp);
    uint16_t trans_count = static_cast<uint16_t>(actual_written);

    entry->mark += trans_count;
    if (entry->mark > entry->file_size) {
        entry->file_size = entry->mark;
    }

    if (is_trace_enabled()) {
        std::cout << "WRITE ($CB): wrote " << std::dec << trans_count
                  << " bytes, new mark=" << entry->mark << ", file_size=" << entry->file_size
                  << std::endl;
    }

    outputs.push_back(trans_count);

    if (trans_count < request_count) {
        return ProDOSError::DISK_FULL;
    }
    return ProDOSError::NO_ERROR;
}

ProDOSError MLIHandler::handle_close(Bus &bus, const std::vector<MLIParamValue> &inputs,
                                     std::vector<MLIParamValue> &outputs) {
    uint8_t refnum = std::get<uint8_t>(inputs[0]);

    if (is_trace_enabled()) {
        std::cout << "CLOSE ($CC): refnum=" << std::dec << static_cast<int>(refnum)
                  << std::endl;
    }

    if (refnum == 0) {
        for (size_t i = 1; i < s_file_table.size(); ++i) {
            if (s_file_table[i].used) {
                close_entry(s_file_table[i]);
            }
        }
        if (is_trace_enabled()) {
            std::cout << "CLOSE ($CC): closed all files" << std::endl;
        }
        return ProDOSError::NO_ERROR;
    }

    FileEntry *entry = get_refnum(refnum);
    if (!entry) {
        std::cerr << "CLOSE ($CC): invalid refnum (" << std::dec << static_cast<int>(refnum)
                  << ")" << std::endl;
        return ProDOSError::INVALID_REF_NUM;
    }

    if (is_trace_enabled()) {
        std::cout << "CLOSE ($CC): closing " << entry->host_path << std::endl;
    }

    close_entry(*entry);
    return ProDOSError::NO_ERROR;
}

ProDOSError MLIHandler::handle_flush(Bus &bus, const std::vector<MLIParamValue> &inputs,
                                     std::vector<MLIParamValue> &outputs) {
    uint8_t refnum = std::get<uint8_t>(inputs[0]);

    if (is_trace_enabled()) {
        std::cout << "FLUSH ($CD): refnum=" << std::dec << static_cast<int>(refnum)
                  << std::endl;
    }

    if (refnum == 0) {
        for (size_t i = 1; i < s_file_table.size(); ++i) {
            if (s_file_table[i].used && s_file_table[i].fp) {
                std::fflush(s_file_table[i].fp);
            }
        }
        if (is_trace_enabled()) {
            std::cout << "FLUSH ($CD): flushed all files" << std::endl;
        }
        return ProDOSError::NO_ERROR;
    }

    FileEntry *entry = get_refnum(refnum);
    if (!entry) {
        std::cerr << "FLUSH ($CD): invalid refnum (" << std::dec << static_cast<int>(refnum)
                  << ")" << std::endl;
        return ProDOSError::INVALID_REF_NUM;
    }

    if (entry->fp) {
        std::fflush(entry->fp);
    }

    if (is_trace_enabled()) {
        std::cout << "FLUSH ($CD): flushed " << entry->host_path << std::endl;
    }

    return ProDOSError::NO_ERROR;
}

ProDOSError MLIHandler::handle_set_mark(Bus &bus, const std::vector<MLIParamValue> &inputs,
                                        std::vector<MLIParamValue> &outputs) {
    uint8_t refnum = std::get<uint8_t>(inputs[0]);
    uint32_t new_mark = std::get<uint32_t>(inputs[1]);

    FileEntry *entry = get_refnum(refnum);
    if (!entry) {
        std::cerr << "SET_MARK ($CE): invalid refnum (" << std::dec << static_cast<int>(refnum)
                  << ")" << std::endl;
        return ProDOSError::INVALID_REF_NUM;
    }
    entry->mark = std::min<uint32_t>(new_mark, entry->file_size);
    return ProDOSError::NO_ERROR;
}

ProDOSError MLIHandler::handle_get_mark(Bus &bus, const std::vector<MLIParamValue> &inputs,
                                        std::vector<MLIParamValue> &outputs) {
    uint8_t refnum = std::get<uint8_t>(inputs[0]);

    FileEntry *entry = get_refnum(refnum);
    if (!entry) {
        std::cerr << "GET_MARK ($CF): invalid refnum (" << std::dec << static_cast<int>(refnum)
                  << ")" << std::endl;
        return ProDOSError::INVALID_REF_NUM;
    }

    outputs.push_back(entry->mark);
    return ProDOSError::NO_ERROR;
}

ProDOSError MLIHandler::handle_get_eof(Bus &bus, const std::vector<MLIParamValue> &inputs,
                                       std::vector<MLIParamValue> &outputs) {
    uint8_t refnum = std::get<uint8_t>(inputs[0]);

    FileEntry *entry = get_refnum(refnum);
    if (!entry) {
        std::cerr << "GET_EOF ($D1): invalid refnum (" << std::dec << static_cast<int>(refnum)
                  << ")" << std::endl;
        return ProDOSError::INVALID_REF_NUM;
    }

    outputs.push_back(entry->file_size);
    return ProDOSError::NO_ERROR;
}

ProDOSError MLIHandler::handle_get_file_info(Bus &bus, const std::vector<MLIParamValue> &inputs,
                                             std::vector<MLIParamValue> &outputs) {
    std::string prodos_path = std::get<std::string>(inputs[0]);
    std::string host_path = prodos_path_to_host(prodos_path);

    std::error_code ec;
    auto file_size = std::filesystem::file_size(host_path, ec);
    if (ec) {
        std::cerr << "GET_FILE_INFO ($C4): file not found: " << host_path
                  << " (error: " << ec.message() << ")" << std::endl;
        return ProDOSError::FILE_NOT_FOUND;
    }

    uint32_t size32 = static_cast<uint32_t>(file_size);
    uint16_t blocks_used = static_cast<uint16_t>((size32 + 511) / 512);

    outputs.push_back(uint8_t(0xC3));      // access
    outputs.push_back(uint8_t(0x06));      // file_type
    outputs.push_back(uint16_t(0x0000));   // aux_type
    outputs.push_back(uint8_t(0x01));      // storage_type
    outputs.push_back(blocks_used);         // blocks_used
    outputs.push_back(uint16_t(0));        // mod_date
    outputs.push_back(uint16_t(0));        // mod_time
    outputs.push_back(uint16_t(0));        // create_date
    outputs.push_back(uint16_t(0));        // create_time

    // Manually write EOF as 3-byte value (descriptor missing EOF field)
    const uint8_t *mem = bus.data();
    uint16_t param_list = 0;
    for (uint16_t addr = 0; addr < Bus::MEMORY_SIZE - 13; ++addr) {
        if (mem[addr] == 10) {  // GET_FILE_INFO has 10 params
            uint16_t pathname_ptr = mem[addr + 1] | (mem[addr + 2] << 8);
            if (pathname_ptr < Bus::MEMORY_SIZE) {
                uint8_t path_len = mem[pathname_ptr];
                if (path_len < 64 && pathname_ptr + path_len < Bus::MEMORY_SIZE) {
                    std::string check_path(reinterpret_cast<const char*>(&mem[pathname_ptr + 1]), path_len);
                    if (check_path == prodos_path) {
                        param_list = addr;
                        break;
                    }
                }
            }
        }
    }

    if (param_list > 0) {
        uint16_t eof_offset = param_list + 10;
        bus.write(eof_offset, static_cast<uint8_t>(size32 & 0xFF));
        bus.write(static_cast<uint16_t>(eof_offset + 1), static_cast<uint8_t>((size32 >> 8) & 0xFF));
        bus.write(static_cast<uint16_t>(eof_offset + 2), static_cast<uint8_t>((size32 >> 16) & 0xFF));
    }

    return ProDOSError::NO_ERROR;
}

// Stub handlers for unimplemented calls

ProDOSError MLIHandler::handle_alloc_interrupt(Bus &bus, const std::vector<MLIParamValue> &inputs,
                                               std::vector<MLIParamValue> &outputs) {
    std::cerr << "ALLOC_INTERRUPT ($40): not implemented" << std::endl;
    return ProDOSError::BAD_CALL_NUMBER;
}

ProDOSError MLIHandler::handle_dealloc_interrupt(Bus &bus, const std::vector<MLIParamValue> &inputs,
                                                 std::vector<MLIParamValue> &outputs) {
    std::cerr << "DEALLOC_INTERRUPT ($41): not implemented" << std::endl;
    return ProDOSError::BAD_CALL_NUMBER;
}

ProDOSError MLIHandler::handle_quit(Bus &bus, const std::vector<MLIParamValue> &inputs,
                                    std::vector<MLIParamValue> &outputs) {
    std::cerr << "QUIT ($65): not implemented" << std::endl;
    return ProDOSError::BAD_CALL_NUMBER;
}

ProDOSError MLIHandler::handle_read_block(Bus &bus, const std::vector<MLIParamValue> &inputs,
                                          std::vector<MLIParamValue> &outputs) {
    std::cerr << "READ_BLOCK ($80): not implemented" << std::endl;
    return ProDOSError::BAD_CALL_NUMBER;
}

ProDOSError MLIHandler::handle_write_block(Bus &bus, const std::vector<MLIParamValue> &inputs,
                                           std::vector<MLIParamValue> &outputs) {
    std::cerr << "WRITE_BLOCK ($81): not implemented" << std::endl;
    return ProDOSError::BAD_CALL_NUMBER;
}

ProDOSError MLIHandler::handle_create(Bus &bus, const std::vector<MLIParamValue> &inputs,
                                      std::vector<MLIParamValue> &outputs) {
    std::cerr << "CREATE ($C0): not implemented" << std::endl;
    return ProDOSError::BAD_CALL_NUMBER;
}

ProDOSError MLIHandler::handle_destroy(Bus &bus, const std::vector<MLIParamValue> &inputs,
                                       std::vector<MLIParamValue> &outputs) {
    std::cerr << "DESTROY ($C1): not implemented" << std::endl;
    return ProDOSError::BAD_CALL_NUMBER;
}

ProDOSError MLIHandler::handle_rename(Bus &bus, const std::vector<MLIParamValue> &inputs,
                                      std::vector<MLIParamValue> &outputs) {
    std::cerr << "RENAME ($C2): not implemented" << std::endl;
    return ProDOSError::BAD_CALL_NUMBER;
}

ProDOSError MLIHandler::handle_set_file_info(Bus &bus, const std::vector<MLIParamValue> &inputs,
                                             std::vector<MLIParamValue> &outputs) {
    std::cerr << "SET_FILE_INFO ($C3): not implemented" << std::endl;
    return ProDOSError::BAD_CALL_NUMBER;
}

ProDOSError MLIHandler::handle_online(Bus &bus, const std::vector<MLIParamValue> &inputs,
                                      std::vector<MLIParamValue> &outputs) {
    std::cerr << "ONLINE ($C5): not implemented" << std::endl;
    return ProDOSError::BAD_CALL_NUMBER;
}

ProDOSError MLIHandler::handle_newline(Bus &bus, const std::vector<MLIParamValue> &inputs,
                                       std::vector<MLIParamValue> &outputs) {
    std::cerr << "NEWLINE ($C9): not implemented" << std::endl;
    return ProDOSError::BAD_CALL_NUMBER;
}

ProDOSError MLIHandler::handle_set_eof(Bus &bus, const std::vector<MLIParamValue> &inputs,
                                       std::vector<MLIParamValue> &outputs) {
    std::cerr << "SET_EOF ($D0): not implemented" << std::endl;
    return ProDOSError::BAD_CALL_NUMBER;
}

ProDOSError MLIHandler::handle_set_buf(Bus &bus, const std::vector<MLIParamValue> &inputs,
                                       std::vector<MLIParamValue> &outputs) {
    std::cerr << "SET_BUF ($D2): not implemented" << std::endl;
    return ProDOSError::BAD_CALL_NUMBER;
}

ProDOSError MLIHandler::handle_get_buf(Bus &bus, const std::vector<MLIParamValue> &inputs,
                                       std::vector<MLIParamValue> &outputs) {
    std::cerr << "GET_BUF ($D3): not implemented" << std::endl;
    return ProDOSError::BAD_CALL_NUMBER;
}

bool MLIHandler::prodos_mli_trap_handler(CPUState &cpu, Bus &bus, uint16_t trap_pc) {
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
        if (call_details_logged) {
            // std::cout << "Call details already logged, skipping duplicate log." << std::endl;
            return;
        }
        if (reason == "trace") {
            return; // Skip verbose logging for normal traced calls
        }
        if (!is_trace_enabled() && reason != "halt") {
            // std::cout << "No logs s_trace_enabled=" << is_trace_enabled() << " reason=" <<
            // reason;
            return;
        }

        call_details_logged = true;

        std::cout << std::endl;
        std::cout << "=== PRODOS MLI CALL DETECTED at PC=$BF00 ===" << std::endl;
        std::cout << TrapManager::dump_cpu_state(cpu) << std::endl;
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

            uint8_t path_len = mem[pathname_ptr];
            std::cout << "    Pathname length: " << std::dec << static_cast<int>(path_len)
                      << std::endl;
            std::cout << "    Pathname: \"";
            for (int i = 1; i <= path_len && i <= 64 && (pathname_ptr + i) < Bus::MEMORY_SIZE;
                 ++i) {
                char c = mem[pathname_ptr + i];
                std::cout << c;
            }
            std::cout << "\"" << std::endl;
            std::cout << "    Access: $" << std::hex << std::setw(2)
                      << static_cast<int>(mem[param_list + 3]) << std::endl;
            std::cout << "    File type: $" << std::setw(2) << static_cast<int>(mem[param_list + 4])
                      << std::endl;
            std::cout << "    Storage type: $" << std::setw(2)
                      << static_cast<int>(mem[param_list + 6]) << std::endl;
        }
    };

    auto log_call_summary = [&]() {
        if (!is_trace_enabled())
            return;
        std::ostringstream oss;
        oss << "[PRODOS] call=$" << std::hex << std::uppercase << std::setw(2) << std::setfill('0')
            << static_cast<int>(call_num) << " (" << decode_prodos_call(call_num) << ") params=$"
            << std::setw(4) << param_list;

        // Capture a snapshot of the parameter block (up to 16 bytes)
        oss << " params_bytes=";
        size_t max_bytes = std::min<size_t>(16, Bus::MEMORY_SIZE - param_list);
        oss << "[";
        for (size_t i = 0; i < max_bytes; ++i) {
            if (i > 0)
                oss << ' ';
            oss << std::hex << std::setw(2) << std::setfill('0')
                << static_cast<int>(mem[param_list + i]);
        }
        oss << "]";

        oss << " A=$" << std::hex << std::uppercase << std::setw(2) << std::setfill('0')
            << static_cast<int>(cpu.A);

        std::cout << oss.str() << std::endl;
    };

    auto return_success = [&]() -> bool {
        if (cpu.A == 0) {
            log_call_summary();
        }
        return true;
    };

    log_call_details("trace");

    // New architecture: use descriptor-based dispatch
    const MLICallDescriptor *desc = get_call_descriptor(call_num);
    if (!desc) {
        // Unknown call number
        log_call_details("halt");
        std::cout << std::endl;
        std::cout << "=== HALTING - ProDOS MLI call $" << std::hex << std::uppercase 
                  << std::setw(2) << std::setfill('0') << static_cast<int>(call_num) 
                  << " unknown ===" << std::endl;
        write_memory_dump(bus, "memory_dump.bin");
        return false;
    }

    // Check if handler is implemented
    if (!desc->handler) {
        // Unimplemented call - return error instead of halting
        std::cout << "[MLI STUB] Call $" << std::hex << std::uppercase << std::setw(2) 
                  << std::setfill('0') << static_cast<int>(call_num) << " (" 
                  << desc->name << ") not yet implemented" << std::endl;
        set_error(cpu, ProDOSError::BAD_CALL_NUMBER);
        return_to_caller();
        if (cpu.A == 0) {
            log_call_summary();
        }
        return true;
    }

    // Read input parameters
    std::vector<MLIParamValue> inputs = read_input_params(bus, param_list, *desc);

    // Create empty outputs vector
    std::vector<MLIParamValue> outputs;

    // Call handler
    ProDOSError error = desc->handler(bus, inputs, outputs);

    // Write output parameters
    write_output_params(bus, param_list, *desc, outputs);

    // Set CPU state based on error code
    if (error == ProDOSError::NO_ERROR) {
        set_success(cpu);
    } else {
        set_error(cpu, error);
    }

    // Return to caller
    return_to_caller();

    // Log if successful
    if (cpu.A == 0) {
        log_call_summary();
    }

    return true;
}
std::string MLIHandler::decode_prodos_call(uint8_t call_num) {
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
