/**
 * @file mli.hpp
 * @brief ProDOS Machine Language Interface (MLI) handler
 *
 * Implements ProDOS MLI system calls for the emulator. Maps ProDOS file
 * operations to Linux filesystem operations with 1:1 path correspondence.
 *
 * Supported MLI calls:
 * - System: ALLOC_INTERRUPT, DEALLOC_INTERRUPT, QUIT, GET_TIME
 * - Housekeeping: CREATE, DESTROY, RENAME, GET/SET_FILE_INFO, ONLINE, GET/SET_PREFIX
 * - Filing: OPEN, NEWLINE, READ, WRITE, CLOSE, FLUSH, GET/SET_MARK, GET/SET_EOF, GET/SET_BUF
 * - Block: READ_BLOCK, WRITE_BLOCK
 *
 * Reference: Apple ProDOS 8 Technical Reference Manual
 */

#ifndef EDASM_MLI_HPP
#define EDASM_MLI_HPP

#include "bus.hpp"
#include "cpu.hpp"
#include <array>
#include <cstdint>
#include <string>
#include <variant>
#include <vector>

namespace edasm {

// ProDOS MLI Error Codes (from Apple ProDOS 8 Technical Reference Manual, section 4.8)
enum class ProDOSError : uint8_t {
    NO_ERROR = 0x00,              // No error
    BAD_CALL_NUMBER = 0x01,       // Bad system call number
    BAD_PARAM_COUNT = 0x04,       // Bad system call parameter count
    INTERRUPT_TABLE_FULL = 0x25,  // Interrupt vector table full
    IO_ERROR = 0x27,              // I/O error
    NO_DEVICE = 0x28,             // No device detected/connected
    WRITE_PROTECTED = 0x2B,       // Disk write protected
    DISK_SWITCHED = 0x2E,         // Disk switched
    INVALID_PATH_SYNTAX = 0x40,   // Invalid pathname syntax
    FCB_FULL = 0x42,              // File Control Block table full
    INVALID_REF_NUM = 0x43,       // Invalid reference number
    PATH_NOT_FOUND = 0x44,        // Path not found
    VOL_NOT_FOUND = 0x45,         // Volume directory not found
    FILE_NOT_FOUND = 0x46,        // File not found
    DUPLICATE_FILE = 0x47,        // Duplicate filename
    DISK_FULL = 0x48,             // Overrun error / disk full
    VOL_DIR_FULL = 0x49,          // Volume directory full
    INCOMPATIBLE_FORMAT = 0x4A,   // Incompatible file format
    UNSUPPORTED_STORAGE = 0x4B,   // Unsupported storage_type
    END_OF_FILE = 0x4C,           // End of file encountered
    POSITION_OUT_OF_RANGE = 0x4D, // Position out of range
    ACCESS_ERROR = 0x4E,          // Access error
    FILE_OPEN = 0x50,             // File is open
    DIR_COUNT_ERROR = 0x51,       // Directory count error
    NOT_PRODOS_DISK = 0x52,       // Not a ProDOS disk
    INVALID_PARAMETER = 0x53,     // Invalid parameter
    VCB_FULL = 0x55,              // Volume Control Block table full
    BAD_BUFFER_ADDR = 0x56,       // Bad buffer address
    DUPLICATE_VOLUME = 0x57,      // Duplicate volume
    BITMAP_IMPOSSIBLE = 0x5A      // Bit map disk address is impossible
};

// Parameter types for MLI calls
enum class MLIParamType : uint8_t {
    BYTE,         // Single byte value
    WORD,         // Two-byte value (little-endian)
    THREE_BYTE,   // Three-byte value (e.g., EOF)
    PATHNAME_PTR, // Pointer to pathname (length-prefixed string)
    BUFFER_PTR,   // Pointer to data buffer
    REF_NUM       // File reference number (byte)
};

// Direction of parameter (input, output, or both)
enum class MLIParamDirection : uint8_t {
    INPUT,       // Parameter is read by MLI
    OUTPUT,      // Parameter is written by MLI
    INPUT_OUTPUT // Parameter is both read and written
};

// Descriptor for a single parameter in an MLI call
struct MLIParamDescriptor {
    MLIParamType type;
    MLIParamDirection direction;
    const char *name; // For debugging/logging
};

// Union type for parameter values
using MLIParamValue = std::variant<uint8_t,             // BYTE, REF_NUM
                                   uint16_t,            // WORD
                                   uint32_t,            // THREE_BYTE (24-bit stored as 32-bit)
                                   std::string,         // Pathname (extracted from memory)
                                   std::vector<uint8_t> // Buffer data
                                   >;

// Handler function type - takes input params, fills output params, returns error code
using MLIHandlerFunc = ProDOSError (*)(Bus &bus, const std::vector<MLIParamValue> &inputs,
                                       std::vector<MLIParamValue> &outputs);

// Descriptor for a complete MLI call
struct MLICallDescriptor {
    uint8_t call_number;
    const char *name;
    uint8_t param_count;
    std::array<MLIParamDescriptor, 12> params; // Max 12 parameters (GET_FILE_INFO has 10)
    MLIHandlerFunc handler; // Handler function pointer (nullptr for unimplemented calls)
};

// ProDOS MLI (Machine Language Interface) handler
class MLIHandler {
  public:
    // ProDOS MLI trap handler: decode and log MLI calls (for $BF00)
    static bool prodos_mli_trap_handler(CPUState &cpu, Bus &bus, uint16_t trap_pc);

    // Helper utilities used by MLI handler
    static void set_success(CPUState &cpu);
    static void set_error(CPUState &cpu, ProDOSError err);
    // Parameter descriptor lookup
    static const MLICallDescriptor *get_call_descriptor(uint8_t call_num);

    // Parameter I/O functions
    static std::vector<MLIParamValue> read_input_params(const Bus &bus, uint16_t param_list_addr,
                                                        const MLICallDescriptor &desc);
    static void write_output_params(Bus &bus, uint16_t param_list_addr,
                                    const MLICallDescriptor &desc,
                                    const std::vector<MLIParamValue> &values);
    static MLIParamValue read_param_value(const Bus &bus, uint16_t param_list_addr,
                                          const MLICallDescriptor &desc, uint8_t param_index);

    // Individual MLI call handlers - return ProDOSError
    // System calls
    static ProDOSError handle_alloc_interrupt(Bus &bus, const std::vector<MLIParamValue> &inputs,
                                              std::vector<MLIParamValue> &outputs);
    static ProDOSError handle_dealloc_interrupt(Bus &bus, const std::vector<MLIParamValue> &inputs,
                                                std::vector<MLIParamValue> &outputs);
    static ProDOSError handle_quit(Bus &bus, const std::vector<MLIParamValue> &inputs,
                                   std::vector<MLIParamValue> &outputs);
    static ProDOSError handle_get_time(Bus &bus, const std::vector<MLIParamValue> &inputs,
                                       std::vector<MLIParamValue> &outputs);

    // Block device calls
    static ProDOSError handle_read_block(Bus &bus, const std::vector<MLIParamValue> &inputs,
                                         std::vector<MLIParamValue> &outputs);
    static ProDOSError handle_write_block(Bus &bus, const std::vector<MLIParamValue> &inputs,
                                          std::vector<MLIParamValue> &outputs);

    // Housekeeping calls
    static ProDOSError handle_create(Bus &bus, const std::vector<MLIParamValue> &inputs,
                                     std::vector<MLIParamValue> &outputs);
    static ProDOSError handle_destroy(Bus &bus, const std::vector<MLIParamValue> &inputs,
                                      std::vector<MLIParamValue> &outputs);
    static ProDOSError handle_rename(Bus &bus, const std::vector<MLIParamValue> &inputs,
                                     std::vector<MLIParamValue> &outputs);
    static ProDOSError handle_set_file_info(Bus &bus, const std::vector<MLIParamValue> &inputs,
                                            std::vector<MLIParamValue> &outputs);
    static ProDOSError handle_get_file_info(Bus &bus, const std::vector<MLIParamValue> &inputs,
                                            std::vector<MLIParamValue> &outputs);
    static ProDOSError handle_online(Bus &bus, const std::vector<MLIParamValue> &inputs,
                                     std::vector<MLIParamValue> &outputs);
    static ProDOSError handle_set_prefix(Bus &bus, const std::vector<MLIParamValue> &inputs,
                                         std::vector<MLIParamValue> &outputs);
    static ProDOSError handle_get_prefix(Bus &bus, const std::vector<MLIParamValue> &inputs,
                                         std::vector<MLIParamValue> &outputs);

    // Filing calls
    static ProDOSError handle_open(Bus &bus, const std::vector<MLIParamValue> &inputs,
                                   std::vector<MLIParamValue> &outputs);
    static ProDOSError handle_newline(Bus &bus, const std::vector<MLIParamValue> &inputs,
                                      std::vector<MLIParamValue> &outputs);
    static ProDOSError handle_read(Bus &bus, const std::vector<MLIParamValue> &inputs,
                                   std::vector<MLIParamValue> &outputs);
    static ProDOSError handle_write(Bus &bus, const std::vector<MLIParamValue> &inputs,
                                    std::vector<MLIParamValue> &outputs);
    static ProDOSError handle_close(Bus &bus, const std::vector<MLIParamValue> &inputs,
                                    std::vector<MLIParamValue> &outputs);
    static ProDOSError handle_flush(Bus &bus, const std::vector<MLIParamValue> &inputs,
                                    std::vector<MLIParamValue> &outputs);
    static ProDOSError handle_set_mark(Bus &bus, const std::vector<MLIParamValue> &inputs,
                                       std::vector<MLIParamValue> &outputs);
    static ProDOSError handle_get_mark(Bus &bus, const std::vector<MLIParamValue> &inputs,
                                       std::vector<MLIParamValue> &outputs);
    static ProDOSError handle_set_eof(Bus &bus, const std::vector<MLIParamValue> &inputs,
                                      std::vector<MLIParamValue> &outputs);
    static ProDOSError handle_get_eof(Bus &bus, const std::vector<MLIParamValue> &inputs,
                                      std::vector<MLIParamValue> &outputs);
    static ProDOSError handle_set_buf(Bus &bus, const std::vector<MLIParamValue> &inputs,
                                      std::vector<MLIParamValue> &outputs);
    static ProDOSError handle_get_buf(Bus &bus, const std::vector<MLIParamValue> &inputs,
                                      std::vector<MLIParamValue> &outputs);

  private:
    // Initialize descriptor table
    static void init_descriptors();
};

} // namespace edasm

#endif // EDASM_MLI_HPP
