// Constants and shared definitions for EDASM
//
// This file implements shared constants from EDASM.SRC/COMMONEQUS.S
// Reference: COMMONEQUS.S - Shared equates across all EDASM modules
//
// Key definitions from COMMONEQUS.S:
//   - ASCII control codes (CTRL-A through DEL)
//   - ProDOS file types: TXT=$04, BIN=$06, REL=$FE, SYS=$FF
//   - Zero page locations shared across modules
//   - Apple II monitor entry points (adapted/removed for Linux)
//   - Sweet16 register definitions (R0-R15) - not needed in C++
//   - ProDOS MLI parameter offsets (adapted for Linux file I/O)
//
// This C++ header preserves the essential constants while removing
// Apple II-specific hardware addresses that don't apply to Linux.
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
constexpr uint8_t CTRL_Y = 0x19; // warm restart
constexpr uint8_t ESCAPE = 0x1B;
constexpr uint8_t SPACE = 0x20;
constexpr uint8_t UNDERSCORE = 0x5F;
constexpr uint8_t DEL = 0x7F;
constexpr uint8_t HIGH_BIT_MASK = 0x80;  // Mask for setting/clearing high bit

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
// Zero Page Locations (from COMMONEQUS.S)
// These are referenced by 2 or more of EdAsm's modules
// In C++ port, these become class member variables
// =================================================

// Apple ][ Standard Zero page
constexpr uint8_t ZP_WNDWDTH = 0x21;   // Window width
constexpr uint8_t ZP_CH = 0x24;         // Cursor horizontal
constexpr uint8_t ZP_CV = 0x25;         // Cursor vertical
constexpr uint8_t ZP_BASL = 0x28;       // Base address for text line
constexpr uint8_t ZP_INVFLG = 0x32;     // Inverse flag
constexpr uint8_t ZP_PROMPT = 0x33;     // Prompt character

// EdAsm shared zero page locations
constexpr uint8_t ZP_LOMEM = 0x0A;      // =$0801 (also TxtBgn, Reg5)
constexpr uint8_t ZP_TXTBGN = 0x0A;     // Points @ 1st char of curr edited file
constexpr uint8_t ZP_HIMEM = 0x0C;      // =$9900 (also Reg6)
constexpr uint8_t ZP_TXTEND = 0x0E;     // Points @ last char of file (also Reg7)
constexpr uint8_t ZP_STACKP = 0x49;     // Save area for H/W stack ptr
constexpr uint8_t ZP_VIDEOSLT = 0x50;   // =$Cs where s=1-3 (if 80-col video card present)
constexpr uint8_t ZP_FILETYPE = 0x51;   // File type
constexpr uint8_t ZP_EXECMODE = 0x53;   // Exec mode
constexpr uint8_t ZP_PTRMODE = 0x54;    // =$80,$00 - Printer ON/OFF
constexpr uint8_t ZP_TABCHAR = 0x5F;    // Tab char (set by Editor)
constexpr uint8_t ZP_PRCOLUMN = 0x61;   // Current print column
constexpr uint8_t ZP_USERTABT = 0x68;   // $68-$71 User defined Tab table
constexpr uint8_t ZP_PRINTF = 0x73;     // -1=Print Cmd 0=List Cmd (also StackP2)
constexpr uint8_t ZP_SWAPMODE = 0x74;   // Split-buf mode 0=normal,1=buf1,2=buf2
constexpr uint8_t ZP_CASEMODE = 0x75;   // ucase/lcase
constexpr uint8_t ZP_CMDDELIM = 0x78;   // Cmd Delimiter/Separator
constexpr uint8_t ZP_TRUNCF = 0x79;     // =$FF-truncate comments

// Sweet16 registers (when using 6502 instructions)
constexpr uint8_t ZP_REG0 = 0x00;       // Doubles as the Accumulator
constexpr uint8_t ZP_REG1 = 0x02;
constexpr uint8_t ZP_REG2 = 0x04;
constexpr uint8_t ZP_REG3 = 0x06;
constexpr uint8_t ZP_REG4 = 0x08;
constexpr uint8_t ZP_REG5 = 0x0A;       // Points @ 1st char of curr edited file (TxtBgn)
constexpr uint8_t ZP_REG6 = 0x0C;       // HiMem
constexpr uint8_t ZP_REG7 = 0x0E;       // Points @ last char of curr edited file (TxtEnd)
constexpr uint8_t ZP_REG8 = 0x10;
constexpr uint8_t ZP_REG9 = 0x12;
constexpr uint8_t ZP_REG10 = 0x14;
constexpr uint8_t ZP_REG11 = 0x16;
constexpr uint8_t ZP_REG12 = 0x18;      // Subroutine return stack pointer
constexpr uint8_t ZP_REG13 = 0x1A;      // Result of a comparison instruction
constexpr uint8_t ZP_REG14 = 0x1C;      // Status Register
constexpr uint8_t ZP_REG15 = 0x1E;      // Program Counter

// =================================================
// Memory addresses (Apple II specific, for reference)
// Original system used these addresses:
// - Text buffer: $0801 to $9900 (37K)
// - Global page: $BD00-$BEFF
// - I/O buffers: $A900, $AD00 (1K each)
// In C++ port, these become dynamically allocated
// =================================================

constexpr uint16_t STACK_BASE = 0x0100;         // 6502 stack
constexpr uint16_t INBUF = 0x0200;              // Input buffer
constexpr uint16_t TXBUF2 = 0x0280;             // Secondary text buffer
constexpr uint16_t SOFTEV = 0x03F2;             // RESET vector
constexpr uint16_t PWREDUP = 0x03F4;            // Power-up byte
constexpr uint16_t USRADR = 0x03F8;             // Ctrl-Y vector

constexpr uint16_t LOAD_ADDR_SYS = 0x2000;      // Load & Exec addr of SYS files
constexpr uint16_t LOAD_ADDR_EDITOR = 0x8900;   // Load Addr of Editor Module
constexpr uint16_t LOAD_ADDR_EI = 0xB100;       // Load Addr of EI Module

constexpr uint16_t TEXT_BUFFER_START = 0x0801;  // Start of text buffer
constexpr uint16_t TEXT_BUFFER_END = 0x9900;    // End of text buffer (HiMem)

constexpr uint16_t IO_BUFFER_1 = 0xA900;        // 1024-byte I/O buffer for ProDOS
constexpr uint16_t IO_BUFFER_2 = 0xAD00;        // Second 1024-byte I/O buffer

constexpr uint16_t GLOBAL_PAGE = 0xBD00;        // EdAsm Global Page (128 bytes)
constexpr uint16_t GLOBAL_PAGE_2 = 0xBD80;      // General Purpose buffers
constexpr uint16_t CURRENT_PATHNAME = 0xBE00;   // $BE00-$BE3F (curr Pathname)
constexpr uint16_t DEVCTLS = 0xBE40;            // $BE40-$BE61 Init to $C3 if 80-col card
constexpr uint16_t TABTABLE = 0xBE60;           // $BE60-$BE62
constexpr uint16_t DATETIME = 0xBE64;           // $BE64-$73 Date/Time
constexpr uint16_t EDASMDIR = 0xBE79;           // Where EDASM lives
constexpr uint16_t PRTERROR = 0xBEFC;           // EdAsm Interpreter error message rtn

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
// ProDOS 8 Global Page (for reference)
// =================================================

constexpr uint16_t PRODOS8 = 0xBF00;            // ProDOS MLI entry point
constexpr uint16_t LASTDEV = 0xBF30;            // Last device accessed
constexpr uint16_t BITMAP = 0xBF58;             // System bitmap
constexpr uint16_t P8DATE = 0xBF90;             // ProDOS date
constexpr uint16_t P8TIME = 0xBF92;             // ProDOS time
constexpr uint16_t MACHID = 0xBF98;             // Machine ID
constexpr uint16_t SLTBYT = 0xBF99;             // Slot ROM map
constexpr uint16_t CMDADR = 0xBF9C;             // Last MLI call return address
constexpr uint16_t MINIVERS = 0xBFFC;           // Minimum interpreter version
constexpr uint16_t IVERSION = 0xBFFD;           // Interpreter version

// =================================================
// Apple ][ Soft Switches (for reference)
// =================================================

constexpr uint16_t KBD = 0xC000;                // Keyboard
constexpr uint16_t CLR80VID = 0xC00C;           // Clear 80-column mode
constexpr uint16_t KBDSTROBE = 0xC010;          // Keyboard strobe
constexpr uint16_t RDBANK2 = 0xC080;            // Read only RAM bank 2
constexpr uint16_t LCBANK2 = 0xC083;            // Read/Write RAM bank 2

// =================================================
// Apple ][ Monitor Entry Points (for reference)
// =================================================

constexpr uint16_t SWEET16_ROM = 0xF689;        // Original IntegerBASIC ROM entry point
constexpr uint16_t BELL1 = 0xFBDD;              // Bell
constexpr uint16_t HOME = 0xFC58;               // Clear screen
constexpr uint16_t RDKEY = 0xFD0C;              // Read key
constexpr uint16_t CROUT = 0xFD8E;              // Carriage return
constexpr uint16_t COUT = 0xFDED;               // Output char
constexpr uint16_t MON = 0xFF65;                // Monitor

// =================================================
// Default settings
// =================================================

constexpr char DEFAULT_CMD_DELIMITER = ']';
constexpr char DEFAULT_TAB_CHAR = ' ';
constexpr int DEFAULT_PAGE_LENGTH = 60;
constexpr int DEFAULT_COLUMNS_40 = 2;  // 2 columns for 40-col display
constexpr int DEFAULT_COLUMNS_80 = 4;  // 4 columns for 80-col display
constexpr int DEFAULT_COLUMNS_PRINTER = 6; // 6 columns for printer

// =================================================
// ProDOS MLI parameter offsets (from COMMONEQUS.S)
// =================================================

constexpr int C_PCNT = 0;       // Parameter count
constexpr int C_DEVNUM = 1;     // Device number
constexpr int C_REFNUM = 1;     // Reference number
constexpr int C_PATH = 1;       // Pathname pointer (2 bytes)
constexpr int C_DATABUF = 2;    // Data buffer pointer (2 bytes)
constexpr int C_ATTR = 3;       // File attributes
constexpr int C_FILEID = 4;     // File ID
constexpr int C_AUXID = 5;      // Auxiliary ID (2 bytes)
constexpr int C_FKIND = 7;      // File kind
constexpr int C_DATE = 8;       // Date (2 bytes)
constexpr int C_TIME = 10;      // Time (2 bytes)
constexpr int C_MODDATE = 10;   // Modification date (2 bytes)
constexpr int C_MODTIME = 12;   // Modification time (2 bytes)
constexpr int C_CREDATE = 14;   // Creation date (2 bytes)
constexpr int C_CRETIME = 16;   // Creation time (2 bytes)

} // namespace edasm
