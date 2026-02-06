/**
 * @file mli.cpp
 * @brief ProDOS MLI (Machine Language Interface) handler implementation
 *
 * Implements ProDOS system calls for the emulator, mapping ProDOS file
 * operations to Linux filesystem with 1:1 path correspondence.
 *
 * Reference: Apple ProDOS 8 Technical Reference Manual
 */

#include "edasm/emulator/mli.hpp"
#include "edasm/constants.hpp"
#include "edasm/emulator/traps.hpp"
#include "edasm/files/file_types.hpp"
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
    uint32_t mark = 0;                  // current file position
    uint32_t file_size = 0;             // bytes
    uint8_t newline_enable_mask = 0x00; // $00 = disabled, nonzero = enabled
    uint8_t newline_char = 0x0D;        // default to CR ($0D)
};

constexpr size_t kMaxFiles = 16; // ProDOS refnums are 1-15; slot 0 unused
std::array<FileEntry, kMaxFiles> s_file_table{};

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

void MLIHandler::set_success(CPUState &cpu) {
    cpu.A = 0;
    cpu.P &= ~StatusFlags::C;
    cpu.P &= ~StatusFlags::N;
    cpu.P &= ~StatusFlags::V;
    cpu.P |= StatusFlags::Z;
    cpu.P |= StatusFlags::U;
}

void MLIHandler::set_error(CPUState &cpu, ProDOSError err) {
    cpu.A = static_cast<uint8_t>(err);
    cpu.P |= StatusFlags::C;
    cpu.P &= ~StatusFlags::Z;
    cpu.P |= StatusFlags::U;
}

// MLI Call Descriptors
// Based on Apple ProDOS 8 Technical Reference Manual, Chapter 4

namespace {
using PT = MLIParamType;
using PD = MLIParamDirection;

// Helper macros for cleaner descriptor definitions
#define PARAM(type, dir, name)                                                                     \
    MLIParamDescriptor {                                                                           \
        PT::type, PD::dir, name                                                                    \
    }
#define IN PARAM
#define OUT PARAM
#define INOUT PARAM

// Descriptor table (static storage)
static std::array<MLICallDescriptor, 26> s_call_descriptors = {{
    // System calls
    {0x40,
     "ALLOC_INTERRUPT",
     2,
     {{
         IN(BYTE, INPUT, "int_num"),
         IN(WORD, INPUT, "int_code"),
     }},
     nullptr},
    {0x41,
     "DEALLOC_INTERRUPT",
     1,
     {{
         IN(BYTE, INPUT, "int_num"),
     }},
     nullptr},
    {0x65,
     "QUIT",
     4,
     {{
         IN(BYTE, INPUT, "quit_type"),
         IN(WORD, INPUT, "reserved1"),
         IN(BYTE, INPUT, "reserved2"),
         IN(WORD, INPUT, "reserved3"),
     }},
     nullptr},
    {0x82, "GET_TIME", 0, {{}}, &MLIHandler::handle_get_time},

    // Block device calls
    {0x80,
     "READ_BLOCK",
     3,
     {{
         IN(BYTE, INPUT, "unit_num"),
         IN(BUFFER_PTR, INPUT, "data_buffer"),
         IN(WORD, INPUT, "block_num"),
     }},
     nullptr},
    {0x81,
     "WRITE_BLOCK",
     3,
     {{
         IN(BYTE, INPUT, "unit_num"),
         IN(BUFFER_PTR, INPUT, "data_buffer"),
         IN(WORD, INPUT, "block_num"),
     }},
     nullptr},

    // Housekeeping calls
    {0xC0,
     "CREATE",
     7,
     {{
         IN(PATHNAME_PTR, INPUT, "pathname"),
         IN(BYTE, INPUT, "access"),
         IN(BYTE, INPUT, "file_type"),
         IN(WORD, INPUT, "aux_type"),
         IN(BYTE, INPUT, "storage_type"),
         IN(WORD, INPUT, "create_date"),
         IN(WORD, INPUT, "create_time"),
     }},
     &MLIHandler::handle_create},
    {0xC1,
     "DESTROY",
     1,
     {{
         IN(PATHNAME_PTR, INPUT, "pathname"),
     }},
     nullptr},
    {0xC2,
     "RENAME",
     2,
     {{
         IN(PATHNAME_PTR, INPUT, "pathname"),
         IN(PATHNAME_PTR, INPUT, "new_pathname"),
     }},
     nullptr},
    {0xC3,
     "SET_FILE_INFO",
     7,
     {{
         IN(PATHNAME_PTR, INPUT, "pathname"),
         IN(BYTE, INPUT, "access"),
         IN(BYTE, INPUT, "file_type"),
         IN(WORD, INPUT, "aux_type"),
         IN(BYTE, INPUT, "reserved1"),
         IN(WORD, INPUT, "mod_date"),
         IN(WORD, INPUT, "mod_time"),
     }},
     &MLIHandler::handle_set_file_info},
    {0xC4,
     "GET_FILE_INFO",
     11,
     {{
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
         OUT(THREE_BYTE, OUTPUT, "eof"),
     }},
     &MLIHandler::handle_get_file_info},
    {0xC5,
     "ONLINE",
     2,
     {{
         IN(BYTE, INPUT, "unit_num"),
         IN(BUFFER_PTR, INPUT_OUTPUT, "data_buffer"),
     }},
     nullptr},
    {0xC6,
     "SET_PREFIX",
     1,
     {{
         IN(PATHNAME_PTR, INPUT, "pathname"),
     }},
     &MLIHandler::handle_set_prefix},
    {0xC7,
     "GET_PREFIX",
     1,
     {{
         IN(PATHNAME_PTR, INPUT_OUTPUT, "data_buffer"),
     }},
     &MLIHandler::handle_get_prefix},

    // Filing calls
    {0xC8,
     "OPEN",
     3,
     {{
         IN(PATHNAME_PTR, INPUT, "pathname"),
         IN(BUFFER_PTR, INPUT, "io_buffer"),
         OUT(REF_NUM, OUTPUT, "ref_num"),
     }},
     &MLIHandler::handle_open},
    {0xC9,
     "NEWLINE",
     3,
     {{
         IN(REF_NUM, INPUT, "ref_num"),
         IN(BYTE, INPUT, "enable_mask"),
         IN(BYTE, INPUT, "newline_char"),
     }},
     &MLIHandler::handle_newline},
    {0xCA,
     "READ",
     4,
     {{
         IN(REF_NUM, INPUT, "ref_num"),
         IN(BUFFER_PTR, INPUT_OUTPUT, "data_buffer"),
         IN(WORD, INPUT, "request_count"),
         OUT(WORD, OUTPUT, "transfer_count"),
     }},
     &MLIHandler::handle_read},
    {0xCB,
     "WRITE",
     4,
     {{
         IN(REF_NUM, INPUT, "ref_num"),
         IN(BUFFER_PTR, INPUT, "data_buffer"),
         IN(WORD, INPUT, "request_count"),
         OUT(WORD, OUTPUT, "transfer_count"),
     }},
     &MLIHandler::handle_write},
    {0xCC,
     "CLOSE",
     1,
     {{
         IN(REF_NUM, INPUT, "ref_num"),
     }},
     &MLIHandler::handle_close},
    {0xCD,
     "FLUSH",
     1,
     {{
         IN(REF_NUM, INPUT, "ref_num"),
     }},
     &MLIHandler::handle_flush},
    {0xCE,
     "SET_MARK",
     2,
     {{
         IN(REF_NUM, INPUT, "ref_num"),
         IN(THREE_BYTE, INPUT, "position"),
     }},
     &MLIHandler::handle_set_mark},
    {0xCF,
     "GET_MARK",
     2,
     {{
         IN(REF_NUM, INPUT, "ref_num"),
         OUT(THREE_BYTE, OUTPUT, "position"),
     }},
     &MLIHandler::handle_get_mark},
    {0xD0,
     "SET_EOF",
     2,
     {{
         IN(REF_NUM, INPUT, "ref_num"),
         IN(THREE_BYTE, INPUT, "eof"),
     }},
     nullptr},
    {0xD1,
     "GET_EOF",
     2,
     {{
         IN(REF_NUM, INPUT, "ref_num"),
         OUT(THREE_BYTE, OUTPUT, "eof"),
     }},
     &MLIHandler::handle_get_eof},
    {0xD2,
     "SET_BUF",
     2,
     {{
         IN(REF_NUM, INPUT, "ref_num"),
         IN(BUFFER_PTR, INPUT, "io_buffer"),
     }},
     nullptr},
    {0xD3,
     "GET_BUF",
     2,
     {{
         IN(REF_NUM, INPUT, "ref_num"),
         OUT(BUFFER_PTR, OUTPUT, "io_buffer"),
     }},
     nullptr},
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

std::vector<MLIParamValue> MLIHandler::read_input_params(const Bus &bus, uint16_t param_list_addr,
                                                         const MLICallDescriptor &desc) {

    std::vector<MLIParamValue> values;

    // Skip parameter count byte
    uint16_t offset = 1;

    for (uint8_t i = 0; i < desc.param_count; ++i) {
        const auto &param = desc.params[i];

        // For OUTPUT parameters:
        // - Pointer types (BUFFER_PTR, PATHNAME_PTR): READ the pointer (handler needs to know where
        // to write)
        // - Value types (BYTE, WORD, THREE_BYTE, REF_NUM): SKIP (handler will generate and return
        // via outputs)
        if (param.direction == MLIParamDirection::OUTPUT) {
            if (param.type == MLIParamType::BUFFER_PTR ||
                param.type == MLIParamType::PATHNAME_PTR) {
                // Read the pointer value (handler needs to know where to write output)
                uint16_t ptr = bus.read_word(static_cast<uint16_t>(param_list_addr + offset));
                values.push_back(ptr);
                offset += 2;
            } else {
                // Skip value-type OUTPUT parameters (handler will write to outputs vector)
                switch (param.type) {
                case MLIParamType::BYTE:
                case MLIParamType::REF_NUM:
                    offset += 1;
                    break;
                case MLIParamType::WORD:
                    offset += 2;
                    break;
                case MLIParamType::THREE_BYTE:
                    offset += 3;
                    break;
                default:
                    break; // BUFFER_PTR and PATHNAME_PTR handled above
                }
            }
            continue;
        }

        // Read INPUT and INPUT_OUTPUT parameters
        switch (param.type) {
        case MLIParamType::BYTE:
        case MLIParamType::REF_NUM: {
            uint8_t val = bus.read(static_cast<uint16_t>(param_list_addr + offset));
            values.push_back(val);
            offset += 1;
            break;
        }
        case MLIParamType::WORD: {
            uint16_t val = bus.read_word(static_cast<uint16_t>(param_list_addr + offset));
            values.push_back(val);
            offset += 2;
            break;
        }
        case MLIParamType::THREE_BYTE: {
            uint32_t val = bus.read(static_cast<uint16_t>(param_list_addr + offset)) |
                           (bus.read(static_cast<uint16_t>(param_list_addr + offset + 1)) << 8) |
                           (bus.read(static_cast<uint16_t>(param_list_addr + offset + 2)) << 16);
            values.push_back(val);
            offset += 3;
            break;
        }
        case MLIParamType::PATHNAME_PTR: {
            uint16_t ptr = bus.read_word(static_cast<uint16_t>(param_list_addr + offset));
            offset += 2;

            if (param.direction == MLIParamDirection::INPUT_OUTPUT) {
                // Keep pointer for INPUT_OUTPUT so handlers can use buffer address
                values.push_back(ptr);
                break;
            }

            // Read length-prefixed pathname
            uint8_t len = bus.read(ptr);
            std::string pathname;
            // Read at most 64 characters, preventing overflow and wrapping
            uint8_t max_len = (len > 64) ? 64 : len;
            uint16_t str_start = static_cast<uint16_t>(ptr + 1);
            for (uint8_t j = 0; j < max_len; ++j) {
                uint16_t addr = static_cast<uint16_t>(str_start + j);
                pathname += static_cast<char>(bus.read(addr));
            }
            values.push_back(pathname);
            break;
        }
        case MLIParamType::BUFFER_PTR: {
            uint16_t ptr = bus.read_word(static_cast<uint16_t>(param_list_addr + offset));
            values.push_back(ptr); // Store as uint16_t for now
            offset += 2;
            break;
        }
        }
    }

    return values;
}

MLIParamValue MLIHandler::read_param_value(const Bus &bus, uint16_t param_list_addr,
                                           const MLICallDescriptor &desc, uint8_t param_index) {
    if (param_index >= desc.param_count) {
        throw std::out_of_range("Parameter index out of range");
    }

    uint16_t offset = 1; // Skip parameter count byte

    // Calculate offset to the requested parameter
    for (uint8_t i = 0; i < param_index; ++i) {
        const auto &param = desc.params[i];
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
    }

    // Read the parameter value
    const auto &param = desc.params[param_index];
    switch (param.type) {
    case MLIParamType::BYTE:
    case MLIParamType::REF_NUM: {
        uint8_t val = bus.read(static_cast<uint16_t>(param_list_addr + offset));
        return val;
    }
    case MLIParamType::WORD: {
        uint16_t val = bus.read_word(static_cast<uint16_t>(param_list_addr + offset));
        return val;
    }
    case MLIParamType::THREE_BYTE: {
        uint32_t val = bus.read(static_cast<uint16_t>(param_list_addr + offset)) |
                       (bus.read(static_cast<uint16_t>(param_list_addr + offset + 1)) << 8) |
                       (bus.read(static_cast<uint16_t>(param_list_addr + offset + 2)) << 16);
        return val;
    }
    case MLIParamType::PATHNAME_PTR: {
        uint16_t ptr = bus.read_word(static_cast<uint16_t>(param_list_addr + offset));

        // Read length-prefixed pathname
        uint8_t len = bus.read(ptr);
        std::string pathname;
        uint8_t max_len = (len > 64) ? 64 : len;
        uint16_t str_start = static_cast<uint16_t>(ptr + 1);
        for (uint8_t j = 0; j < max_len; ++j) {
            uint16_t addr = static_cast<uint16_t>(str_start + j);
            pathname += static_cast<char>(bus.read(addr));
        }
        return pathname;
    }
    case MLIParamType::BUFFER_PTR: {
        uint16_t ptr = bus.read_word(static_cast<uint16_t>(param_list_addr + offset));
        return ptr;
    }
    }

    // Should never reach here
    return uint8_t(0);
}

void MLIHandler::write_output_params(Bus &bus, uint16_t param_list_addr,
                                     const MLICallDescriptor &desc,
                                     const std::vector<MLIParamValue> &values) {

    // Handlers return OUTPUT value parameters and INPUT_OUTPUT parameters.
    // OUTPUT pointer parameters (BUFFER_PTR, PATHNAME_PTR) are handled directly by the handler.

    // Count expected outputs (exclude OUTPUT/INPUT_OUTPUT pointer types)
    size_t expected_outputs = 0;
    for (uint8_t i = 0; i < desc.param_count; ++i) {
        const auto &param = desc.params[i];
        if (param.direction != MLIParamDirection::INPUT) {
            // Skip pointer types (handler writes directly to memory for both OUTPUT and
            // INPUT_OUTPUT)
            if ((param.direction == MLIParamDirection::OUTPUT ||
                 param.direction == MLIParamDirection::INPUT_OUTPUT) &&
                (param.type == MLIParamType::BUFFER_PTR ||
                 param.type == MLIParamType::PATHNAME_PTR)) {
                continue;
            }
            ++expected_outputs;
        }
    }

    if (values.size() != expected_outputs) {
        std::cerr << "Warning: Parameter count mismatch in write_output_params - expected "
                  << expected_outputs << " got " << values.size() << std::endl;
        // Continue but be defensive: only process as many as provided
    }

    // Skip parameter count byte
    uint16_t offset = 1;

    size_t out_idx =
        0; // index into values (only output/input_output params, excluding OUTPUT pointers)

    for (uint8_t i = 0; i < desc.param_count; ++i) {
        const auto &param = desc.params[i];

        // Input-only params: skip over parameter bytes
        if (param.direction == MLIParamDirection::INPUT) {
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

        // OUTPUT/INPUT_OUTPUT pointer types: skip (handler writes directly to memory)
        if ((param.direction == MLIParamDirection::OUTPUT ||
             param.direction == MLIParamDirection::INPUT_OUTPUT) &&
            (param.type == MLIParamType::BUFFER_PTR || param.type == MLIParamType::PATHNAME_PTR)) {
            offset += 2; // Pointer is always 2 bytes
            continue;
        }

        // If handler did not provide an output for this parameter, skip writing
        if (out_idx >= values.size()) {
            // Advance offset as if parameter exists, but do not write
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

        const auto &value = values[out_idx++];

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
            // Pointers are not typically written back; skip (but advance)
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

    bus.write(static_cast<uint16_t>(P8DATE + 1), bf91);
    bus.write(P8DATE, bf90);
    bus.write(static_cast<uint16_t>(P8TIME + 1), hour);
    bus.write(P8TIME, minute);

    return ProDOSError::NO_ERROR;
}

ProDOSError MLIHandler::handle_set_prefix(Bus &bus, const std::vector<MLIParamValue> &inputs,
                                          std::vector<MLIParamValue> &outputs) {
    std::string prodos_path = std::get<std::string>(inputs[0]);

    if (prodos_path.empty()) {
        prodos_path = "/";
    }

    if (prodos_path.length() > 64) {
        std::cerr << "SET_PREFIX ($C6): path too long (" << prodos_path.length() << " > 64)"
                  << std::endl;
        return ProDOSError::INVALID_PATH_SYNTAX;
    }

    std::filesystem::path target = prodos_path;

    std::error_code ec;
    std::filesystem::path canonical = std::filesystem::weakly_canonical(target, ec);
    std::filesystem::path verify =
        ec ? (std::filesystem::path(current_prefix()) / target) : canonical;
    if (!std::filesystem::is_directory(verify)) {
        std::cerr << "SET_PREFIX ($C6): directory does not exist: " << verify.string() << std::endl;
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

    outputs.push_back(static_cast<uint8_t>(ref));
    return ProDOSError::NO_ERROR;
}

ProDOSError MLIHandler::handle_read(Bus &bus, const std::vector<MLIParamValue> &inputs,
                                    std::vector<MLIParamValue> &outputs) {
    uint8_t refnum = std::get<uint8_t>(inputs[0]);
    uint16_t data_buffer = std::get<uint16_t>(inputs[1]);
    uint16_t request_count = std::get<uint16_t>(inputs[2]);

    FileEntry *entry = get_refnum(refnum);
    if (!entry) {
        std::cerr << "READ ($CA): invalid refnum (" << std::dec << static_cast<int>(refnum) << ")"
                  << std::endl;
        outputs.push_back(uint16_t(0)); // trans_count = 0 on error
        return ProDOSError::INVALID_REF_NUM;
    }

    if (data_buffer + request_count > Bus::MEMORY_SIZE) {
        std::cerr << "READ ($CA): buffer overflow (data_buffer=$" << std::hex << std::uppercase
                  << std::setw(4) << std::setfill('0') << data_buffer
                  << ", request_count=" << std::dec << request_count << ")" << std::endl;
        outputs.push_back(uint16_t(0)); // trans_count = 0 on error
        return ProDOSError::BAD_BUFFER_ADDR;
    }

    if (!entry->fp) {
        std::cerr << "READ ($CA): file not open" << std::endl;
        outputs.push_back(uint16_t(0)); // trans_count = 0 on error
        return ProDOSError::INVALID_REF_NUM;
    }

    if (std::fseek(entry->fp, static_cast<long>(entry->mark), SEEK_SET) != 0) {
        std::cerr << "READ ($CA): fseek failed" << std::endl;
        outputs.push_back(uint16_t(0)); // trans_count = 0 on error
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

        // Check for newline character if newline mode is enabled
        bool newline_found = false;
        if (entry->newline_enable_mask != 0x00) {
            for (uint16_t i = 0; i < actual_read; ++i) {
                uint8_t ch = buffer[i];
                bus.write(static_cast<uint16_t>(data_buffer + i), ch);

                // Check if this character matches the newline char (after masking)
                if ((ch & entry->newline_enable_mask) == entry->newline_char) {
                    // Found newline - terminate read after this character
                    actual_read = i + 1;
                    newline_found = true;
                    break;
                }
            }
        } else {
            // Newline mode disabled - just copy all bytes
            for (uint16_t i = 0; i < actual_read; ++i) {
                bus.write(static_cast<uint16_t>(data_buffer + i), buffer[i]);
            }
        }

        entry->mark += actual_read;
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

    FileEntry *entry = get_refnum(refnum);
    if (!entry) {
        std::cerr << "WRITE ($CB): invalid refnum (" << std::dec << static_cast<int>(refnum) << ")"
                  << std::endl;
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

    // Read data from bus memory into buffer
    std::vector<uint8_t> buffer(request_count);
    for (uint16_t i = 0; i < request_count; ++i) {
        buffer[i] = bus.read(static_cast<uint16_t>(data_buffer + i));
    }

    size_t actual_written = std::fwrite(buffer.data(), 1, request_count, entry->fp);
    uint16_t trans_count = static_cast<uint16_t>(actual_written);

    entry->mark += trans_count;
    if (entry->mark > entry->file_size) {
        entry->file_size = entry->mark;
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

    if (refnum == 0) {
        for (size_t i = 1; i < s_file_table.size(); ++i) {
            if (s_file_table[i].used) {
                close_entry(s_file_table[i]);
            }
        }
        return ProDOSError::NO_ERROR;
    }

    FileEntry *entry = get_refnum(refnum);
    if (!entry) {
        std::cerr << "CLOSE ($CC): invalid refnum (" << std::dec << static_cast<int>(refnum) << ")"
                  << std::endl;
        return ProDOSError::INVALID_REF_NUM;
    }

    close_entry(*entry);
    return ProDOSError::NO_ERROR;
}

ProDOSError MLIHandler::handle_flush(Bus &bus, const std::vector<MLIParamValue> &inputs,
                                     std::vector<MLIParamValue> &outputs) {
    uint8_t refnum = std::get<uint8_t>(inputs[0]);

    if (refnum == 0) {
        for (size_t i = 1; i < s_file_table.size(); ++i) {
            if (s_file_table[i].used && s_file_table[i].fp) {
                std::fflush(s_file_table[i].fp);
            }
        }
        return ProDOSError::NO_ERROR;
    }

    FileEntry *entry = get_refnum(refnum);
    if (!entry) {
        std::cerr << "FLUSH ($CD): invalid refnum (" << std::dec << static_cast<int>(refnum) << ")"
                  << std::endl;
        return ProDOSError::INVALID_REF_NUM;
    }

    if (entry->fp) {
        std::fflush(entry->fp);
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

    // Check if path exists
    std::error_code ec;
    bool exists = std::filesystem::exists(host_path, ec);
    if (!exists || ec) {
        std::cerr << "GET_FILE_INFO ($C4): file not found: " << host_path
                  << " (error: " << ec.message() << ")" << std::endl;
        // Push zero placeholders for all 10 output parameters
        outputs.push_back(uint8_t(0));  // access
        outputs.push_back(uint8_t(0));  // file_type
        outputs.push_back(uint16_t(0)); // aux_type
        outputs.push_back(uint8_t(0));  // storage_type
        outputs.push_back(uint16_t(0)); // blocks_used
        outputs.push_back(uint16_t(0)); // mod_date
        outputs.push_back(uint16_t(0)); // mod_time
        outputs.push_back(uint16_t(0)); // create_date
        outputs.push_back(uint16_t(0)); // create_time
        outputs.push_back(uint32_t(0)); // eof (3 bytes)
        return ProDOSError::FILE_NOT_FOUND;
    }

    // Check if this is a directory
    bool is_dir = std::filesystem::is_directory(host_path, ec);
    
    uint32_t size32;
    uint16_t blocks_used;
    uint8_t storage_type;
    uint8_t prodos_ftype;

    if (is_dir) {
        // Directory handling
        // Count entries in the directory
        size_t entry_count = 0;
        try {
            for (const auto& entry : std::filesystem::directory_iterator(host_path)) {
                (void)entry; // Suppress unused variable warning
                entry_count++;
            }
        } catch (const std::filesystem::filesystem_error& e) {
            // If we can't read the directory, treat it as empty
            entry_count = 0;
        }

        // ProDOS directory format:
        // - storage_type = 0x0D (directory)
        // - file_type = 0x0F (directory)
        // - EOF = header + entries, where each entry is 39 bytes
        // - Header block is 512 bytes, subsequent entry blocks are 512 bytes
        storage_type = 0x0D;
        prodos_ftype = 0x0F;
        
        // Calculate EOF: header (512 bytes) + entries (39 bytes each)
        size32 = 512 + (entry_count * 39);
        blocks_used = static_cast<uint16_t>((size32 + 511) / 512);
    } else {
        // Regular file handling
        auto file_size = std::filesystem::file_size(host_path, ec);
        if (ec) {
            std::cerr << "GET_FILE_INFO ($C4): cannot get file size: " << host_path
                      << " (error: " << ec.message() << ")" << std::endl;
            // Push zero placeholders for all 10 output parameters
            outputs.push_back(uint8_t(0));  // access
            outputs.push_back(uint8_t(0));  // file_type
            outputs.push_back(uint16_t(0)); // aux_type
            outputs.push_back(uint8_t(0));  // storage_type
            outputs.push_back(uint16_t(0)); // blocks_used
            outputs.push_back(uint16_t(0)); // mod_date
            outputs.push_back(uint16_t(0)); // mod_time
            outputs.push_back(uint16_t(0)); // create_date
            outputs.push_back(uint16_t(0)); // create_time
            outputs.push_back(uint32_t(0)); // eof (3 bytes)
            return ProDOSError::FILE_NOT_FOUND;
        }

        size32 = static_cast<uint32_t>(file_size);
        blocks_used = static_cast<uint16_t>((size32 + 511) / 512);
        storage_type = 0x01; // seedling file

        // Determine a likely ProDOS file type from the host filename extension
        std::filesystem::path p(host_path);
        std::string ext = p.extension().string();
        auto pf_type = edasm::type_from_extension(ext);
        prodos_ftype = edasm::prodos_type_code(pf_type);
    }

    outputs.push_back(uint8_t(0xC3)); // access
    outputs.push_back(prodos_ftype);  // file_type
    outputs.push_back(uint16_t(0x0000)); // aux_type
    outputs.push_back(storage_type);  // storage_type
    outputs.push_back(blocks_used);   // blocks_used
    outputs.push_back(uint16_t(0));   // mod_date
    outputs.push_back(uint16_t(0));   // mod_time
    outputs.push_back(uint16_t(0));   // create_date
    outputs.push_back(uint16_t(0));   // create_time
    outputs.push_back(size32);        // eof (3 bytes)

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
    // Inputs: pathname, access, file_type, aux_type, storage_type, create_date, create_time
    std::string prodos_path = std::get<std::string>(inputs[0]);
    uint8_t access = std::get<uint8_t>(inputs[1]);
    uint8_t file_type = std::get<uint8_t>(inputs[2]);
    uint16_t aux_type = std::get<uint16_t>(inputs[3]);
    uint8_t storage_type = std::get<uint8_t>(inputs[4]);
    uint16_t create_date = std::get<uint16_t>(inputs[5]);
    uint16_t create_time = std::get<uint16_t>(inputs[6]);

    (void)access;
    (void)file_type;
    (void)aux_type;
    (void)storage_type;
    (void)create_date;
    (void)create_time;

    std::string host_path = prodos_path_to_host(prodos_path);
    std::filesystem::path p(host_path);

    // Check if file already exists
    if (std::filesystem::exists(p)) {
        return ProDOSError::DUPLICATE_FILE;
    }

    // Create file (empty)
    std::ofstream ofs(host_path, std::ios::binary);
    if (!ofs) {
        return ProDOSError::PATH_NOT_FOUND;
    }
    ofs.close();

    // Optionally set file type/aux info (not implemented)
    // Could use extended attributes or file extension

    // Success
    return ProDOSError::NO_ERROR;
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
    // Inputs: pathname, access, file_type, aux_type, reserved1, mod_date, mod_time
    std::string prodos_path = std::get<std::string>(inputs[0]);
    uint8_t access = std::get<uint8_t>(inputs[1]);
    uint8_t file_type = std::get<uint8_t>(inputs[2]);
    uint16_t aux_type = std::get<uint16_t>(inputs[3]);
    uint8_t reserved1 = std::get<uint8_t>(inputs[4]);
    uint16_t mod_date = std::get<uint16_t>(inputs[5]);
    uint16_t mod_time = std::get<uint16_t>(inputs[6]);

    (void)access;      // Currently not stored (would need extended attributes)
    (void)file_type;   // Currently not stored (would need extended attributes)
    (void)aux_type;    // Currently not stored (would need extended attributes)
    (void)reserved1;   // Reserved field, ignored per ProDOS spec
    (void)mod_date;    // Currently not implemented (would need to decode ProDOS date format)
    (void)mod_time;    // Currently not implemented (would need to decode ProDOS time format)

    std::string host_path = prodos_path_to_host(prodos_path);

    // Check if file exists
    if (!std::filesystem::exists(host_path)) {
        std::cerr << "SET_FILE_INFO ($C3): file not found: " << host_path << std::endl;
        return ProDOSError::FILE_NOT_FOUND;
    }

    // Note: In a full implementation, we would:
    // 1. Store file_type and aux_type (possibly using extended attributes or a metadata file)
    // 2. Store access permissions (would map to Unix file permissions)
    // 3. Decode mod_date and mod_time from ProDOS format and update file modification time
    //
    // For now, this is a minimal implementation that validates the file exists
    // and returns success. This allows ProDOS programs that call SET_FILE_INFO
    // to continue running without errors.

    if (TrapManager::is_trace_enabled()) {
        std::cout << "SET_FILE_INFO ($C3): " << prodos_path 
                  << " (access=$" << std::hex << std::setw(2) << std::setfill('0') 
                  << static_cast<int>(access)
                  << ", type=$" << std::hex << std::setw(2) << std::setfill('0')
                  << static_cast<int>(file_type)
                  << ", aux=$" << std::hex << std::setw(4) << std::setfill('0')
                  << aux_type << ")" << std::endl;
    }

    return ProDOSError::NO_ERROR;
}

ProDOSError MLIHandler::handle_online(Bus &bus, const std::vector<MLIParamValue> &inputs,
                                      std::vector<MLIParamValue> &outputs) {
    std::cerr << "ONLINE ($C5): not implemented" << std::endl;
    return ProDOSError::BAD_CALL_NUMBER;
}

ProDOSError MLIHandler::handle_newline(Bus &bus, const std::vector<MLIParamValue> &inputs,
                                       std::vector<MLIParamValue> &outputs) {
    uint8_t refnum = std::get<uint8_t>(inputs[0]);
    uint8_t enable_mask = std::get<uint8_t>(inputs[1]);
    uint8_t newline_char = std::get<uint8_t>(inputs[2]);

    if (TrapManager::is_trace_enabled()) {
        std::cout << "NEWLINE ($C9): refnum=" << std::dec << static_cast<int>(refnum)
                  << ", enable_mask=$" << std::hex << std::uppercase << std::setw(2)
                  << std::setfill('0') << static_cast<int>(enable_mask) << ", newline_char=$"
                  << std::hex << std::uppercase << std::setw(2) << std::setfill('0')
                  << static_cast<int>(newline_char) << std::endl;
    }

    FileEntry *entry = get_refnum(refnum);
    if (!entry) {
        std::cerr << "NEWLINE ($C9): invalid refnum (" << std::dec << static_cast<int>(refnum)
                  << ")" << std::endl;
        return ProDOSError::INVALID_REF_NUM;
    }

    // Set newline mode parameters
    entry->newline_enable_mask = enable_mask;
    entry->newline_char = newline_char;

    if (TrapManager::is_trace_enabled()) {
        if (enable_mask == 0x00) {
            std::cout << "NEWLINE ($C9): newline mode DISABLED" << std::endl;
        } else {
            std::cout << "NEWLINE ($C9): newline mode ENABLED, char=$" << std::hex << std::uppercase
                      << std::setw(2) << std::setfill('0') << static_cast<int>(newline_char)
                      << ", mask=$" << std::hex << std::uppercase << std::setw(2)
                      << std::setfill('0') << static_cast<int>(enable_mask) << std::endl;
        }
    }

    return ProDOSError::NO_ERROR;
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

// Helper functions for new logging format
namespace {

// Convert ProDOS date/time format to ISO 8601 string
std::string prodos_datetime_to_iso8601(uint16_t date_word, uint16_t time_word) {
    // ProDOS date format (2 bytes):
    // High byte: YYYYYYYM (year bits 6-0, month bit 3)
    // Low byte: MMMDDDDD (month bits 2-0, day bits 4-0)
    // Year is offset from 1900

    // ProDOS time format (2 bytes):
    // High byte: hour (0-23)
    // Low byte: minute (0-59)

    uint8_t date_low = date_word & 0xFF;
    uint8_t date_high = (date_word >> 8) & 0xFF;

    uint8_t time_low = time_word & 0xFF;
    uint8_t time_high = (time_word >> 8) & 0xFF;

    int year = ((date_high >> 1) & 0x7F) + 1900;
    int month = (((date_high & 0x01) << 3) | ((date_low >> 5) & 0x07));
    int day = date_low & 0x1F;

    int hour = time_high;
    int minute = time_low;

    // Handle zero date/time (not set)
    if (date_word == 0 && time_word == 0) {
        return "(not set)";
    }

    // Format as ISO 8601: YYYY-MM-DDTHH:MM
    std::ostringstream oss;
    oss << std::setfill('0') << std::setw(4) << year << "-" << std::setw(2) << month << "-"
        << std::setw(2) << day << "T" << std::setw(2) << hour << ":" << std::setw(2) << minute;
    return oss.str();
}

// Check if parameter name is a date parameter (has matching time parameter)
bool is_date_param(const char *name) {
    std::string param_name = name;
    return param_name.find("_date") != std::string::npos;
}

// Check if parameter name is a time parameter
bool is_time_param(const char *name) {
    std::string param_name = name;
    return param_name.find("_time") != std::string::npos;
}

// Format a parameter value for logging
std::string format_param_value(const MLIParamDescriptor &param, const MLIParamValue &value,
                               const Bus &bus, uint16_t param_list_addr, uint8_t param_index) {
    std::ostringstream oss;

    switch (param.type) {
    case MLIParamType::BYTE:
    case MLIParamType::REF_NUM: {
        uint8_t val = std::get<uint8_t>(value);
        oss << "$" << std::hex << std::uppercase << std::setw(2) << std::setfill('0')
            << static_cast<int>(val);
        break;
    }
    case MLIParamType::WORD: {
        uint16_t val = std::get<uint16_t>(value);
        oss << "$" << std::hex << std::uppercase << std::setw(4) << std::setfill('0') << val;
        break;
    }
    case MLIParamType::THREE_BYTE: {
        uint32_t val = std::get<uint32_t>(value);
        oss << "$" << std::hex << std::uppercase << std::setw(6) << std::setfill('0') << val;
        break;
    }
    case MLIParamType::PATHNAME_PTR: {
        if (const auto *pathname = std::get_if<std::string>(&value)) {
            oss << "\"" << *pathname << "\"";
            break;
        }
        if (const auto *ptr = std::get_if<uint16_t>(&value)) {
            uint8_t len = bus.read(*ptr);
            uint8_t max_len = (len > 64) ? 64 : len;
            uint16_t str_start = static_cast<uint16_t>(*ptr + 1);
            std::string pathname;
            for (uint8_t j = 0; j < max_len; ++j) {
                uint16_t addr = static_cast<uint16_t>(str_start + j);
                pathname += static_cast<char>(bus.read(addr));
            }
            oss << "\"" << pathname << "\"";
            break;
        }
        oss << "\"\"";
        break;
    }
    case MLIParamType::BUFFER_PTR: {
        uint16_t ptr = std::get<uint16_t>(value);
        oss << "$" << std::hex << std::uppercase << std::setw(4) << std::setfill('0') << ptr;
        break;
    }
    }

    return oss.str();
}

// Get error message for ProDOS error code
std::string get_error_message(ProDOSError error) {
    switch (error) {
    case ProDOSError::NO_ERROR:
        return "Success";
    case ProDOSError::BAD_CALL_NUMBER:
        return "Bad system call number";
    case ProDOSError::BAD_PARAM_COUNT:
        return "Bad system call parameter count";
    case ProDOSError::INTERRUPT_TABLE_FULL:
        return "Interrupt vector table full";
    case ProDOSError::IO_ERROR:
        return "I/O error";
    case ProDOSError::NO_DEVICE:
        return "No device detected";
    case ProDOSError::WRITE_PROTECTED:
        return "Disk write protected";
    case ProDOSError::DISK_SWITCHED:
        return "Disk switched";
    case ProDOSError::INVALID_PATH_SYNTAX:
        return "Invalid pathname syntax";
    case ProDOSError::FCB_FULL:
        return "File Control Block table full";
    case ProDOSError::INVALID_REF_NUM:
        return "Invalid reference number";
    case ProDOSError::PATH_NOT_FOUND:
        return "Path not found";
    case ProDOSError::VOL_NOT_FOUND:
        return "Volume directory not found";
    case ProDOSError::FILE_NOT_FOUND:
        return "File not found";
    case ProDOSError::DUPLICATE_FILE:
        return "Duplicate filename";
    case ProDOSError::DISK_FULL:
        return "Disk full";
    case ProDOSError::VOL_DIR_FULL:
        return "Volume directory full";
    case ProDOSError::INCOMPATIBLE_FORMAT:
        return "Incompatible file format";
    case ProDOSError::UNSUPPORTED_STORAGE:
        return "Unsupported storage type";
    case ProDOSError::END_OF_FILE:
        return "End of file encountered";
    case ProDOSError::POSITION_OUT_OF_RANGE:
        return "Position out of range";
    case ProDOSError::ACCESS_ERROR:
        return "Access error";
    case ProDOSError::FILE_OPEN:
        return "File is open";
    case ProDOSError::DIR_COUNT_ERROR:
        return "Directory count error";
    case ProDOSError::NOT_PRODOS_DISK:
        return "Not a ProDOS disk";
    case ProDOSError::INVALID_PARAMETER:
        return "Invalid parameter";
    case ProDOSError::VCB_FULL:
        return "Volume Control Block table full";
    case ProDOSError::BAD_BUFFER_ADDR:
        return "Bad buffer address";
    case ProDOSError::DUPLICATE_VOLUME:
        return "Duplicate volume";
    case ProDOSError::BITMAP_IMPOSSIBLE:
        return "Bit map disk address is impossible";
    default:
        return "Unknown error";
    }
}

// Log input parameters (first line)
void log_mli_input(const MLICallDescriptor &desc, const std::vector<MLIParamValue> &inputs,
                   const Bus &bus, uint16_t param_list_addr) {
    if (!TrapManager::is_trace_enabled())
        return;

    std::ostringstream oss;
    oss << desc.name << " ($" << std::hex << std::uppercase << std::setw(2) << std::setfill('0')
        << static_cast<int>(desc.call_number) << ")";

    // Special case: GET_TIME has no parameter list
    if (desc.call_number == 0x82) {
        std::cout << oss.str() << std::endl;
        return;
    }

    // Add input parameters
    size_t input_idx = 0;
    uint16_t offset = 1; // Skip parameter count byte
    std::vector<bool> param_logged(desc.param_count, false);

    for (uint8_t i = 0; i < desc.param_count; ++i) {
        const auto &param = desc.params[i];

        size_t param_size = 1;
        switch (param.type) {
        case MLIParamType::BYTE:
        case MLIParamType::REF_NUM:
            param_size = 1;
            break;
        case MLIParamType::WORD:
        case MLIParamType::PATHNAME_PTR:
        case MLIParamType::BUFFER_PTR:
            param_size = 2;
            break;
        case MLIParamType::THREE_BYTE:
            param_size = 3;
            break;
        }

        if (param_logged[i]) {
            offset = static_cast<uint16_t>(offset + param_size);
            continue; // Already logged as part of a date/time pair
        }

        // Only log INPUT and INPUT_OUTPUT parameters
        if (param.direction == MLIParamDirection::OUTPUT) {
            offset = static_cast<uint16_t>(offset + param_size);
            continue;
        }

        if (input_idx >= inputs.size()) {
            break;
        }

        // Check if this is a date parameter with a matching time parameter
        if (is_date_param(param.name)) {
            std::string date_name = param.name;
            std::string time_name = date_name;
            size_t pos = time_name.find("_date");
            if (pos != std::string::npos) {
                time_name.replace(pos, 5, "_time");

                // Find the matching time parameter
                for (uint8_t j = i + 1; j < desc.param_count; ++j) {
                    if (std::string(desc.params[j].name) == time_name &&
                        desc.params[j].direction != MLIParamDirection::OUTPUT) {

                        // Found matching time - format as datetime
                        if (input_idx + 1 < inputs.size()) {
                            uint16_t date_val = std::get<uint16_t>(inputs[input_idx]);
                            uint16_t time_val = std::get<uint16_t>(inputs[input_idx + 1]);

                            // Use base name (without _date suffix) for the combined field
                            std::string base_name = date_name.substr(0, pos);
                            oss << " " << base_name << "=";
                            oss << prodos_datetime_to_iso8601(date_val, time_val);

                            param_logged[i] = true;
                            param_logged[j] = true;
                            input_idx += 2; // Skip both date and time
                            offset = static_cast<uint16_t>(offset + param_size);
                            goto next_param;
                        }
                    }
                }
            }
        }

        // Normal parameter logging (not part of a date/time pair)
        if (param.direction == MLIParamDirection::INPUT_OUTPUT &&
            param.type == MLIParamType::PATHNAME_PTR) {
            // For INPUT_OUTPUT PATHNAME_PTR, defer logging until after the call
            param_logged[i] = true;
            input_idx++;
            offset = static_cast<uint16_t>(offset + param_size);
            goto next_param;
        }

        oss << " " << param.name << "=";
        if (param.type == MLIParamType::PATHNAME_PTR) {
            uint16_t ptr = bus.read_word(static_cast<uint16_t>(param_list_addr + offset));
            oss << "ptr=$" << std::hex << std::uppercase << std::setw(4) << std::setfill('0') << ptr
                << " ";
            oss << format_param_value(param, inputs[input_idx], bus, param_list_addr, i);
        } else if (param.type == MLIParamType::BUFFER_PTR) {
            uint16_t ptr = bus.read_word(static_cast<uint16_t>(param_list_addr + offset));
            oss << "ptr=$" << std::hex << std::uppercase << std::setw(4) << std::setfill('0')
                << ptr;
        } else {
            oss << format_param_value(param, inputs[input_idx], bus, param_list_addr, i);
        }
        param_logged[i] = true;
        input_idx++;
        offset = static_cast<uint16_t>(offset + param_size);

    next_param:;
    }

    std::cout << oss.str() << std::endl;
}

// Log output parameters and result (second line)
void log_mli_output(const MLICallDescriptor &desc, const std::vector<MLIParamValue> &outputs,
                    ProDOSError error, const Bus &bus, uint16_t param_list_addr) {
    if (!TrapManager::is_trace_enabled())
        return;

    // Special case: GET_TIME - log the ProDOS system date-time from the system page
    if (desc.call_number == 0x82) {
        // Read the date/time that was just written to the ProDOS system page
        uint16_t date_word = bus.read(P8DATE) | (bus.read(static_cast<uint16_t>(P8DATE + 1)) << 8);
        uint16_t time_word = bus.read(P8TIME) | (bus.read(static_cast<uint16_t>(P8TIME + 1)) << 8);

        std::ostringstream oss;
        oss << "  Result: success datetime=" << prodos_datetime_to_iso8601(date_word, time_word);
        std::cout << oss.str() << std::endl;
        return;
    }

    std::ostringstream oss;
    oss << "  Result:";

    // Log result first
    if (error == ProDOSError::NO_ERROR) {
        oss << " success";
    } else {
        oss << " error=$" << std::hex << std::uppercase << std::setw(2) << std::setfill('0')
            << static_cast<int>(error) << " (" << get_error_message(error) << ")";
    }

    // Add output parameters (excluding pointers)
    if (error == ProDOSError::NO_ERROR) {
        size_t output_idx = 0;
        std::vector<bool> param_logged(desc.param_count, false);

        for (uint8_t i = 0; i < desc.param_count; ++i) {
            if (param_logged[i]) {
                continue; // Already logged as part of a date/time pair
            }

            const auto &param = desc.params[i];

            // Only log OUTPUT and INPUT_OUTPUT parameters (excluding pointers)
            if (param.direction == MLIParamDirection::INPUT) {
                continue;
            }

            // For INPUT_OUTPUT PATHNAME_PTR, log after call from memory
            if (param.direction == MLIParamDirection::INPUT_OUTPUT &&
                param.type == MLIParamType::PATHNAME_PTR) {
                MLIParamValue value = MLIHandler::read_param_value(bus, param_list_addr, desc, i);
                oss << " " << param.name << "=";
                oss << format_param_value(param, value, bus, param_list_addr, i);
                param_logged[i] = true;
                goto next_param;
            }

            // Skip pointer types
            if (param.type == MLIParamType::BUFFER_PTR ||
                param.type == MLIParamType::PATHNAME_PTR) {
                continue;
            }

            // Check if this is a date parameter with a matching time parameter
            if (is_date_param(param.name)) {
                std::string date_name = param.name;
                std::string time_name = date_name;
                size_t pos = time_name.find("_date");
                if (pos != std::string::npos) {
                    time_name.replace(pos, 5, "_time");

                    // Find the matching time parameter
                    for (uint8_t j = i + 1; j < desc.param_count; ++j) {
                        if (std::string(desc.params[j].name) == time_name &&
                            desc.params[j].direction != MLIParamDirection::INPUT &&
                            desc.params[j].type != MLIParamType::BUFFER_PTR &&
                            desc.params[j].type != MLIParamType::PATHNAME_PTR) {

                            // Found matching time - format as datetime
                            if (output_idx + 1 < outputs.size()) {
                                uint16_t date_val = std::get<uint16_t>(outputs[output_idx]);
                                uint16_t time_val = std::get<uint16_t>(outputs[output_idx + 1]);

                                // Use base name (without _date suffix) for the combined field
                                std::string base_name = date_name.substr(0, pos);
                                oss << " " << base_name << "=";
                                oss << prodos_datetime_to_iso8601(date_val, time_val);

                                param_logged[i] = true;
                                param_logged[j] = true;
                                output_idx += 2; // Skip both date and time
                                goto next_param;
                            }
                        }
                    }
                }
            }

            // Normal parameter logging (not part of a date/time pair)
            if (output_idx >= outputs.size()) {
                break;
            }
            oss << " " << param.name << "=";
            oss << format_param_value(param, outputs[output_idx], bus, param_list_addr, i);
            param_logged[i] = true;
            output_idx++;

        next_param:;
        }
    }

    std::cout << oss.str() << std::endl;
}

} // anonymous namespace

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

    uint16_t stack_base = STACK_BASE;
    uint8_t sp = cpu.SP;

    uint8_t ret_lo = bus.read(static_cast<uint16_t>(stack_base + sp + 1));
    uint8_t ret_hi = bus.read(static_cast<uint16_t>(stack_base + sp + 2));
    uint16_t ret_addr = static_cast<uint16_t>((ret_hi << 8) | ret_lo);
    uint16_t call_site = static_cast<uint16_t>(ret_addr + 1);

    uint8_t call_num = bus.read(call_site);
    uint8_t param_lo = bus.read(static_cast<uint16_t>(call_site + 1));
    uint8_t param_hi = bus.read(static_cast<uint16_t>(call_site + 2));
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
        if (!TrapManager::is_trace_enabled() && reason != "halt") {
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
        std::cout << "  Command number: $" << std::setw(2) << static_cast<int>(call_num) << " (";
        {
            const MLICallDescriptor *desc = get_call_descriptor(call_num);
            std::cout << (desc ? desc->name : "UNKNOWN");
        }
        std::cout << ")" << std::endl;
        std::cout << "  Parameter list pointer: $" << std::setw(4) << param_list << std::endl;

        std::cout << "  Memory at call site ($" << std::setw(4) << (call_site - 3)
                  << "):" << std::endl;
        std::cout << "    ";
        for (int i = -3; i <= 5; ++i) {
            std::cout << std::setw(2)
                      << static_cast<int>(bus.read(static_cast<uint16_t>(call_site + i))) << " ";
        }
        std::cout << std::endl;
        std::cout << "    JSR ^ CM  PL  PH  --  --  --" << std::endl;
        std::cout << std::endl;

        uint8_t param_count = bus.read(param_list);
        std::cout << "Parameter List at $" << std::setw(4) << param_list << ":" << std::endl;
        std::cout << "  Parameter count: " << std::dec << static_cast<int>(param_count)
                  << std::endl;

        std::cout << "  Parameters (hex):";
        size_t bytes_to_show = std::min<size_t>(param_count * 2, 24);
        for (size_t i = 1; i <= bytes_to_show; ++i) {
            if ((i - 1) % 8 == 0)
                std::cout << std::endl << "    ";
            std::cout << " " << std::hex << std::setw(2) << std::setfill('0')
                      << static_cast<int>(bus.read(static_cast<uint16_t>(param_list + i)));
        }
        std::cout << std::endl;

        // Use descriptor table to log parameters if available
        const MLICallDescriptor *desc = get_call_descriptor(call_num);
        if (desc && desc->param_count > 0) {
            std::cout << std::endl << "  " << desc->name << " call parameters:" << std::endl;

            uint16_t offset = 1; // Skip parameter count byte
            for (uint8_t i = 0; i < desc->param_count && i < param_count; ++i) {
                const auto &param = desc->params[i];

                std::cout << "    " << param.name << ": ";

                switch (param.type) {
                case MLIParamType::BYTE:
                case MLIParamType::REF_NUM:
                    std::cout << "$" << std::hex << std::setw(2) << std::setfill('0')
                              << static_cast<int>(
                                     bus.read(static_cast<uint16_t>(param_list + offset)));
                    offset += 1;
                    break;
                case MLIParamType::WORD: {
                    uint16_t val = bus.read_word(static_cast<uint16_t>(param_list + offset));
                    std::cout << "$" << std::hex << std::setw(4) << std::setfill('0') << val;
                }
                    offset += 2;
                    break;
                case MLIParamType::THREE_BYTE: {
                    uint32_t val = bus.read(static_cast<uint16_t>(param_list + offset)) |
                                   (bus.read(static_cast<uint16_t>(param_list + offset + 1)) << 8) |
                                   (bus.read(static_cast<uint16_t>(param_list + offset + 2)) << 16);
                    std::cout << "$" << std::hex << std::setw(6) << std::setfill('0') << val;
                }
                    offset += 3;
                    break;
                case MLIParamType::PATHNAME_PTR: {
                    uint16_t ptr = bus.read_word(static_cast<uint16_t>(param_list + offset));
                    std::cout << "ptr=$" << std::hex << std::setw(4) << std::setfill('0') << ptr;

                    uint8_t path_len = bus.read(ptr);
                    std::cout << " \"";
                    for (uint8_t j = 0; j < path_len && j < 64; ++j) {
                        std::cout << static_cast<char>(
                            bus.read(static_cast<uint16_t>(ptr + 1 + j)));
                    }
                    std::cout << "\"";
                }
                    offset += 2;
                    break;
                case MLIParamType::BUFFER_PTR: {
                    uint16_t ptr = bus.read_word(static_cast<uint16_t>(param_list + offset));
                    std::cout << "ptr=$" << std::hex << std::setw(4) << std::setfill('0') << ptr;
                }
                    offset += 2;
                    break;
                }
                std::cout << std::endl;
            }
        }
    };

    log_call_details("trace");

    // New architecture: use descriptor-based dispatch
    const MLICallDescriptor *desc = get_call_descriptor(call_num);

    // Record trap statistic with MLI call name if known
    std::string mli_call_name = desc ? desc->name : "UNKNOWN";
    TrapStatistics::record_trap("ProDOS MLI", trap_pc, TrapKind::CALL, mli_call_name);

    if (!desc) {
        // Unknown call number
        log_call_details("halt");
        std::cout << std::endl;
        std::cout << "=== ProDOS MLI call $" << std::hex << std::uppercase << std::setw(2)
                  << std::setfill('0') << static_cast<int>(call_num) << " unknown ===" << std::endl;
        return TrapManager::halt_and_dump("Unknown ProDOS MLI call", cpu, bus, cpu.PC);
    }

    // Check if handler is implemented
    if (!desc->handler) {
        // Unimplemented call - return error instead of halting
        std::cout << "[MLI STUB] Call $" << std::hex << std::uppercase << std::setw(2)
                  << std::setfill('0') << static_cast<int>(call_num) << " (" << desc->name
                  << ") not yet implemented" << std::endl;
        return TrapManager::halt_and_dump("MLI call not implemented: " + std::string(desc->name),
                                          cpu, bus, cpu.PC);
        set_error(cpu, ProDOSError::BAD_CALL_NUMBER);
        return_to_caller();
        return true;
    }

    // Read input parameters
    std::vector<MLIParamValue> inputs = read_input_params(bus, param_list, *desc);

    // Log input parameters (first line)
    log_mli_input(*desc, inputs, bus, param_list);

    // Create empty outputs vector
    std::vector<MLIParamValue> outputs;

    // Call handler
    ProDOSError error = desc->handler(bus, inputs, outputs);

    // Write output parameters
    write_output_params(bus, param_list, *desc, outputs);

    // Log output parameters (second line) - do this before error handling
    log_mli_output(*desc, outputs, error, bus, param_list);

    // Set CPU state based on error code
    if (error == ProDOSError::NO_ERROR) {
        set_success(cpu);
    } else {
        // Log the error, dump memory, and halt
        std::cout << "\n=== MLI CALL FAILED ===" << std::endl;
        std::cout << "Call: $" << std::hex << std::uppercase << std::setw(2) << std::setfill('0')
                  << static_cast<int>(call_num) << " (" << desc->name << ")" << std::endl;
        std::cout << "Error code: $" << std::hex << std::uppercase << std::setw(2)
                  << std::setfill('0') << static_cast<int>(error) << std::endl;

        // Map error code to descriptive message
        const char *error_msg = "Unknown error";
        switch (error) {
        case ProDOSError::BAD_CALL_NUMBER:
            error_msg = "Bad system call number";
            break;
        case ProDOSError::BAD_PARAM_COUNT:
            error_msg = "Bad system call parameter count";
            break;
        case ProDOSError::IO_ERROR:
            error_msg = "I/O error";
            break;
        case ProDOSError::NO_DEVICE:
            error_msg = "No device detected";
            break;
        case ProDOSError::WRITE_PROTECTED:
            error_msg = "Disk write protected";
            break;
        case ProDOSError::INVALID_PATH_SYNTAX:
            error_msg = "Invalid pathname syntax";
            break;
        case ProDOSError::FCB_FULL:
            error_msg = "File Control Block table full";
            break;
        case ProDOSError::INVALID_REF_NUM:
            error_msg = "Invalid reference number";
            break;
        case ProDOSError::PATH_NOT_FOUND:
            error_msg = "Path not found";
            break;
        case ProDOSError::VOL_NOT_FOUND:
            error_msg = "Volume directory not found";
            break;
        case ProDOSError::FILE_NOT_FOUND:
            error_msg = "File not found";
            break;
        case ProDOSError::DUPLICATE_FILE:
            error_msg = "Duplicate filename";
            break;
        case ProDOSError::DISK_FULL:
            error_msg = "Disk full";
            break;
        case ProDOSError::VOL_DIR_FULL:
            error_msg = "Volume directory full";
            break;
        case ProDOSError::INCOMPATIBLE_FORMAT:
            error_msg = "Incompatible file format";
            break;
        case ProDOSError::UNSUPPORTED_STORAGE:
            error_msg = "Unsupported storage type";
            break;
        case ProDOSError::END_OF_FILE:
            error_msg = "End of file encountered";
            break;
        case ProDOSError::POSITION_OUT_OF_RANGE:
            error_msg = "Position out of range";
            break;
        case ProDOSError::ACCESS_ERROR:
            error_msg = "Access error";
            break;
        case ProDOSError::FILE_OPEN:
            error_msg = "File is open";
            break;
        case ProDOSError::INVALID_PARAMETER:
            error_msg = "Invalid parameter";
            break;
        default:
            break;
        }

        std::cout << "Message: " << error_msg << std::endl;

        set_error(cpu, error);

        // Allow READ ($CA) to return EOF ($4C) without halting
        // if (desc->call_number != 0xCA || error != ProDOSError::END_OF_FILE) {
        if (error == ProDOSError::BAD_CALL_NUMBER) {
            return TrapManager::halt_and_dump("MLI call failed: " + std::string(desc->name), cpu,
                                              bus, cpu.PC);
        }
    }

    // Return to caller
    return_to_caller();

    return true;
}
} // namespace edasm
