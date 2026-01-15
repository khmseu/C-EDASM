#pragma once

#include <cstdint>

namespace edasm {

// =================================================
// ASCII keycodes (from COMMONEQUS.S)
// =================================================

constexpr uint8_t CTRL_A = 0x01;
constexpr uint8_t CTRL_B = 0x02;
constexpr uint8_t CTRL_C = 0x03;
constexpr uint8_t CTRL_D = 0x04;
constexpr uint8_t CTRL_E = 0x05;
constexpr uint8_t CTRL_F = 0x06;
constexpr uint8_t BEL = 0x07;
constexpr uint8_t BS = 0x08;
constexpr uint8_t TAB = 0x09;
constexpr uint8_t FF = 0x0C;
constexpr uint8_t CR = 0x0D;
constexpr uint8_t CTRL_N = 0x0E;
constexpr uint8_t CTRL_O = 0x0F;
constexpr uint8_t CTRL_Q = 0x11;
constexpr uint8_t CTRL_R = 0x12;
constexpr uint8_t CTRL_S = 0x13;
constexpr uint8_t CTRL_T = 0x14;
constexpr uint8_t CTRL_U = 0x15;
constexpr uint8_t CTRL_V = 0x16;
constexpr uint8_t CTRL_W = 0x17;
constexpr uint8_t CTRL_X = 0x18; // cancel
constexpr uint8_t ESCAPE = 0x1B;
constexpr uint8_t SPACE = 0x20;
constexpr uint8_t UNDERSCORE = 0x5F;
constexpr uint8_t DEL = 0x7F;

// =================================================
// ProDOS file types (mapped to Linux extensions)
// =================================================

enum class FileType : uint8_t {
    TXT = 0x04, // Text file → .src, .txt
    BIN = 0x06, // Binary file → .bin, .obj
    DIR = 0x0D, // Directory
    REL = 0xFE, // Relocatable object → .rel
    SYS = 0xFF  // System file → .sys
};

// =================================================
// Memory addresses (Apple II specific, for reference)
// Original system used these addresses:
// - Text buffer: $0801 to $9900 (37K)
// - Global page: $BD00-$BEFF
// - I/O buffers: $A900, $AD00 (1K each)
// In C++ port, these become dynamically allocated
// =================================================

constexpr uint16_t LOAD_ADDR_EDITOR = 0x8900;
constexpr uint16_t LOAD_ADDR_EI = 0xB100;
constexpr uint16_t TEXT_BUFFER_START = 0x0801;
constexpr uint16_t TEXT_BUFFER_END = 0x9900;
constexpr uint16_t GLOBAL_PAGE = 0xBD00;
constexpr uint16_t IO_BUFFER_1 = 0xA900;
constexpr uint16_t IO_BUFFER_2 = 0xAD00;

// =================================================
// Symbol table flags (from ASM/EQUATES.S)
// =================================================

constexpr uint8_t SYM_UNDEFINED = 0x80;
constexpr uint8_t SYM_UNREFERENCED = 0x40;
constexpr uint8_t SYM_RELATIVE = 0x20;
constexpr uint8_t SYM_EXTERNAL = 0x10;
constexpr uint8_t SYM_ENTRY = 0x08;
constexpr uint8_t SYM_MACRO = 0x04;
constexpr uint8_t SYM_NO_SUCH_LABEL = 0x02;
constexpr uint8_t SYM_FORWARD_REF = 0x01;

// =================================================
// Assembler configuration
// =================================================

constexpr int MAX_SYMBOL_LENGTH = 16;
constexpr int SYMBOL_TABLE_SIZE = 256; // Hash table buckets
constexpr int MAX_LINE_LENGTH = 255;

// =================================================
// Default settings
// =================================================

constexpr char DEFAULT_CMD_DELIMITER = ']';
constexpr char DEFAULT_TAB_CHAR = ' ';
constexpr int DEFAULT_PAGE_LENGTH = 60;
constexpr int DEFAULT_COLUMNS_40 = 2;  // 2 columns for 40-col display
constexpr int DEFAULT_COLUMNS_80 = 4;  // 4 columns for 80-col display
constexpr int DEFAULT_COLUMNS_PRINTER = 6; // 6 columns for printer

} // namespace edasm
