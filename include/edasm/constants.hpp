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
constexpr uint8_t HIGH_BIT_MASK = 0x80; // Mask for setting/clearing high bit

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
constexpr uint8_t ZP_WNDWDTH = 0x21; // Window width
constexpr uint8_t ZP_CH = 0x24;      // Cursor horizontal
constexpr uint8_t ZP_CV = 0x25;      // Cursor vertical
constexpr uint8_t ZP_BASL = 0x28;    // Base address for text line
constexpr uint8_t ZP_INVFLG = 0x32;  // Inverse flag
constexpr uint8_t ZP_PROMPT = 0x33;  // Prompt character

// EdAsm shared zero page locations
constexpr uint8_t ZP_LOMEM = 0x0A;    // =$0801 (also TxtBgn, Reg5)
constexpr uint8_t ZP_TXTBGN = 0x0A;   // Points @ 1st char of curr edited file
constexpr uint8_t ZP_HIMEM = 0x0C;    // =$9900 (also Reg6)
constexpr uint8_t ZP_TXTEND = 0x0E;   // Points @ last char of file (also Reg7)
constexpr uint8_t ZP_STACKP = 0x49;   // Save area for H/W stack ptr
constexpr uint8_t ZP_VIDEOSLT = 0x50; // =$Cs where s=1-3 (if 80-col video card present)
constexpr uint8_t ZP_FILETYPE = 0x51; // File type
constexpr uint8_t ZP_EXECMODE = 0x53; // Exec mode
constexpr uint8_t ZP_PTRMODE = 0x54;  // =$80,$00 - Printer ON/OFF
constexpr uint8_t ZP_TABCHAR = 0x5F;  // Tab char (set by Editor)
constexpr uint8_t ZP_PRCOLUMN = 0x61; // Current print column
constexpr uint8_t ZP_USERTABT = 0x68; // $68-$71 User defined Tab table
constexpr uint8_t ZP_PRINTF = 0x73;   // -1=Print Cmd 0=List Cmd (also StackP2)
constexpr uint8_t ZP_SWAPMODE = 0x74; // Split-buf mode 0=normal,1=buf1,2=buf2
constexpr uint8_t ZP_CASEMODE = 0x75; // ucase/lcase
constexpr uint8_t ZP_CMDDELIM = 0x78; // Cmd Delimiter/Separator
constexpr uint8_t ZP_TRUNCF = 0x79;   // =$FF-truncate comments

// Sweet16 registers (when using 6502 instructions)
constexpr uint8_t ZP_REG0 = 0x00; // Doubles as the Accumulator
constexpr uint8_t ZP_REG1 = 0x02;
constexpr uint8_t ZP_REG2 = 0x04;
constexpr uint8_t ZP_REG3 = 0x06;
constexpr uint8_t ZP_REG4 = 0x08;
constexpr uint8_t ZP_REG5 = 0x0A; // Points @ 1st char of curr edited file (TxtBgn)
constexpr uint8_t ZP_REG6 = 0x0C; // HiMem
constexpr uint8_t ZP_REG7 = 0x0E; // Points @ last char of curr edited file (TxtEnd)
constexpr uint8_t ZP_REG8 = 0x10;
constexpr uint8_t ZP_REG9 = 0x12;
constexpr uint8_t ZP_REG10 = 0x14;
constexpr uint8_t ZP_REG11 = 0x16;
constexpr uint8_t ZP_REG12 = 0x18; // Subroutine return stack pointer
constexpr uint8_t ZP_REG13 = 0x1A; // Result of a comparison instruction
constexpr uint8_t ZP_REG14 = 0x1C; // Status Register
constexpr uint8_t ZP_REG15 = 0x1E; // Program Counter

// =================================================
// Memory addresses (Apple II specific, for reference)
// Original system used these addresses:
// - Text buffer: $0801 to $9900 (37K)
// - Global page: $BD00-$BEFF
// - I/O buffers: $A900, $AD00 (1K each)
// In C++ port, these become dynamically allocated
// =================================================

constexpr uint16_t STACK_BASE = 0x0100; // 6502 stack
constexpr uint16_t INBUF = 0x0200;      // Input buffer
constexpr uint16_t TXBUF2 = 0x0280;     // Secondary text buffer

// Monitor ROM zero-page addresses (reference: Apple IIe Monitor ROM listings)
constexpr uint16_t CSWL = 0x0036;   // COUT hook low byte (output routine)
constexpr uint16_t CSWH = 0x0037;   // COUT hook high byte
constexpr uint16_t KSWL = 0x0038;   // KEYIN hook low byte (input routine)
constexpr uint16_t KSWH = 0x0039;   // KEYIN hook high byte

constexpr uint16_t SOFTEV = 0x03F2;     // RESET vector
constexpr uint16_t PWREDUP = 0x03F4;    // Power-up byte
constexpr uint16_t USRADR = 0x03F8;     // Ctrl-Y vector

constexpr uint16_t LOAD_ADDR_SYS = 0x2000;    // Load & Exec addr of SYS files
constexpr uint16_t LOAD_ADDR_EDITOR = 0x8900; // Load Addr of Editor Module
constexpr uint16_t LOAD_ADDR_EI = 0xB100;     // Load Addr of EI Module

constexpr uint16_t TEXT_BUFFER_START = 0x0801; // Start of text buffer
constexpr uint16_t TEXT_BUFFER_END = 0x9900;   // End of text buffer (HiMem)

constexpr uint16_t IO_BUFFER_1 = 0xA900; // 1024-byte I/O buffer for ProDOS
constexpr uint16_t IO_BUFFER_2 = 0xAD00; // Second 1024-byte I/O buffer

constexpr uint16_t GLOBAL_PAGE = 0xBD00;      // EdAsm Global Page (128 bytes)
constexpr uint16_t GLOBAL_PAGE_2 = 0xBD80;    // General Purpose buffers
constexpr uint16_t CURRENT_PATHNAME = 0xBE00; // $BE00-$BE3F (curr Pathname)
constexpr uint16_t DEVCTLS = 0xBE40;          // $BE40-$BE61 Init to $C3 if 80-col card
constexpr uint16_t TABTABLE = 0xBE60;         // $BE60-$BE62
constexpr uint16_t DATETIME = 0xBE64;         // $BE64-$73 Date/Time
constexpr uint16_t EDASMDIR = 0xBE79;         // Where EDASM lives
constexpr uint16_t PRTERROR = 0xBEFC;         // EdAsm Interpreter error message rtn

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

constexpr uint16_t PRODOS8 = 0xBF00;  // ProDOS MLI entry point
constexpr uint16_t LASTDEV = 0xBF30;  // Last device accessed
constexpr uint16_t BITMAP = 0xBF58;   // System bitmap
constexpr uint16_t P8DATE = 0xBF90;   // ProDOS date
constexpr uint16_t P8TIME = 0xBF92;   // ProDOS time
constexpr uint16_t MACHID = 0xBF98;   // Machine ID
constexpr uint16_t SLTBYT = 0xBF99;   // Slot ROM map
constexpr uint16_t CMDADR = 0xBF9C;   // Last MLI call return address
constexpr uint16_t MINIVERS = 0xBFFC; // Minimum interpreter version
constexpr uint16_t IVERSION = 0xBFFD; // Interpreter version

// =================================================
// Apple IIe I/O Memory Map ($C000-$C0FF)
// Reference: docs/APPLE_IIE_MEMORY_MAP.md
// =================================================

// Memory Management Soft Switches ($C000-$C00B)
constexpr uint16_t _80STOREOFF = 0xC000;  // PAGE2 switches video pages
constexpr uint16_t _80STOREON = 0xC001;   // PAGE2 switches main/aux. video memory
constexpr uint16_t RAMRDOFF = 0xC002;     // Read from main memory $200-$BFFF
constexpr uint16_t RAMRDON = 0xC003;      // Read from aux. memory $200-$BFFF
constexpr uint16_t RAMWRTOFF = 0xC004;    // Write to main memory $200-$BFFF
constexpr uint16_t RAMWRTON = 0xC005;     // Write to aux. memory $200-$BFFF
constexpr uint16_t INTCXROMOFF = 0xC006;  // Enable slot ROM $C100-$CFFF
constexpr uint16_t INTCXROMON = 0xC007;   // Enable internal ROM $C100-$CFFF
constexpr uint16_t ALTZPOFF = 0xC008;     // Enable main memory $0000-$01FF and main BSR
constexpr uint16_t ALTZPON = 0xC009;      // Enable aux. memory $0000-$01FF and aux. BSR
constexpr uint16_t SLOTC3ROMOFF = 0xC00A; // Enable internal ROM $C300-$C3FF
constexpr uint16_t SLOTC3ROMON = 0xC00B;  // Enable slot 3 ROM $C300-$C3FF

// ROM control ($CFFF)
constexpr uint16_t CLRROM = 0xCFFF; // Disable slot ROM, enable internal ROM

// Video Control ($C00C-$C00F)
constexpr uint16_t _80COLOFF = 0xC00C;     // Turn off 80-column display
constexpr uint16_t _80COLON = 0xC00D;      // Turn on 80-column display
constexpr uint16_t ALTCHARSETOFF = 0xC00E; // Turn off alternate characters
constexpr uint16_t ALTCHARSETON = 0xC00F;  // Turn on alternate characters

// Keyboard and Built-In Device I/O ($C000, $C010-$C070)
constexpr uint16_t KBD = 0xC000;      // Keyboard data (bits 0-6: ASCII) / strobe (bit 7)
constexpr uint16_t KBDSTRB = 0xC010;  // Clear keyboard strobe
constexpr uint16_t AKD = 0xC010;      // 1=key being pressed, 0=all keys released (R7)
constexpr uint16_t CASSOUT = 0xC020;  // Toggle cassette output port state
constexpr uint16_t SPEAKER = 0xC030;  // Toggle speaker state (click)
constexpr uint16_t GCSTROBE = 0xC040; // Generate game I/O connector strobe signal

// Video Mode Soft Switches ($C050-$C057)
constexpr uint16_t TEXTOFF = 0xC050;  // Select graphics mode
constexpr uint16_t TEXTON = 0xC051;   // Select text mode
constexpr uint16_t MIXEDOFF = 0xC052; // Full screen graphics
constexpr uint16_t MIXEDON = 0xC053;  // Graphics with 4 lines of text
constexpr uint16_t PAGE20FF = 0xC054; // Select page1 (or main video memory)
constexpr uint16_t PAGE20N = 0xC055;  // Select page2 (or aux. video memory)
constexpr uint16_t HIRESOFF = 0xC056; // Select low-resolution graphics
constexpr uint16_t HIRESON = 0xC057;  // Select high-resolution graphics

// Annunciator Switches ($C058-$C05F)
constexpr uint16_t CLRAN0 = 0xC058; // Turn off annunciator 0
constexpr uint16_t SETAN0 = 0xC059; // Turn on annunciator 0
constexpr uint16_t CLRAN1 = 0xC05A; // Turn off annunciator 1
constexpr uint16_t SETAN1 = 0xC05B; // Turn on annunciator 1
constexpr uint16_t CLRAN2 = 0xC05C; // Turn off annunciator 2
constexpr uint16_t SETAN2 = 0xC05D; // Turn on annunciator 2
constexpr uint16_t CLRAN3 = 0xC05E; // Turn off annunciator 3
constexpr uint16_t SETAN3 = 0xC05F; // Turn on annunciator 3

// Game Controllers ($C060-$C070)
constexpr uint16_t CASSIN = 0xC060;  // 1=cassette input on
constexpr uint16_t PB0 = 0xC061;     // 1=push button 0 pressed
constexpr uint16_t PB1 = 0xC062;     // 1=push button 1 pressed
constexpr uint16_t PB2 = 0xC063;     // 1=push button 2 pressed (OPEN-APPLE)
constexpr uint16_t GC0 = 0xC064;     // 0=game controller 0 timed out
constexpr uint16_t GC1 = 0xC065;     // 0=game controller 1 timed out
constexpr uint16_t GC2 = 0xC066;     // 0=game controller 2 timed out
constexpr uint16_t GC3 = 0xC067;     // 0=game controller 3 timed out
constexpr uint16_t GCRESET = 0xC070; // Reset game controller timers

// Soft Switch Status Flags ($C011-$C01F)
constexpr uint16_t BSRBANK2 = 0xC011;   // 1=bank2 BSR available, 0=bank1 available (R7)
constexpr uint16_t BSRREADRAM = 0xC012; // 1=BSR active for reads, 0=ROM active (R7)
constexpr uint16_t RAMRD = 0xC013;      // 0=main $200-$BFFF active, 1=aux. active (R7)
constexpr uint16_t RAMWRT = 0xC014;     // 0=main $200-$BFFF active, 1=aux. active (R7)
constexpr uint16_t INTCXROM = 0xC015;   // 1=internal $C100-$CFFF active, 0=slot ROM (R7)
constexpr uint16_t ALTZP = 0xC016;      // 1=aux. ZP/stack/BSR, 0=main ZP/stack/BSR (R7)
constexpr uint16_t SLOTC3ROM = 0xC017;  // 1=slot 3 ROM active, 0=internal $C3 ROM (R7)
constexpr uint16_t _80STORE = 0xC018;   // 1=PAGE2 switches main/aux., 0=pages (R7)
constexpr uint16_t VERTBLANK = 0xC019;  // 1=vertical retrace on, 0=off (R7)
constexpr uint16_t TEXT = 0xC01A;       // 1=text mode, 0=graphics mode (R7)
constexpr uint16_t MIXED = 0xC01B;      // 1=mixed graphics/text, 0=full screen (R7)
constexpr uint16_t PAGE2 = 0xC01C;      // 1=page2 or aux. video, 0=page1 or main (R7)
constexpr uint16_t HIRES = 0xC01D;      // 1=high-res graphics, 0=low-res graphics (R7)
constexpr uint16_t ALTCHARSET = 0xC01E; // 1=alternate charset on, 0=primary (R7)
constexpr uint16_t _80COL = 0xC01F;     // 1=80-column display on, 0=40-column (R7)

// Bank-Switched RAM Control ($C080-$C08F)
constexpr uint16_t READBSR2 = 0xC080;  // Bank 2, read RAM, write-protect
constexpr uint16_t WRITEBSR2 = 0xC081; // Bank 2, read ROM, write-enable (RR)
constexpr uint16_t OFFBSR2 = 0xC082;   // Bank 2, read ROM, write-protect
constexpr uint16_t RDWRBSR2 = 0xC083;  // Bank 2, read RAM, write-enable (RR)
constexpr uint16_t READBSR1 = 0xC088;  // Bank 1, read RAM, write-protect
constexpr uint16_t WRITEBSR1 = 0xC089; // Bank 1, read ROM, write-enable (RR)
constexpr uint16_t OFFBSR1 = 0xC08A;   // Bank 1, read ROM, write-protect
constexpr uint16_t RDWRBSR1 = 0xC08B;  // Bank 1, read RAM, write-enable (RR)

// Legacy aliases (kept for compatibility)
constexpr uint16_t CLR80VID = _80COLOFF; // Clear 80-column mode
constexpr uint16_t KBDSTROBE = KBDSTRB;  // Keyboard strobe
constexpr uint16_t RDBANK2 = READBSR2;   // Read only RAM bank 2
constexpr uint16_t LCBANK2 = RDWRBSR2;   // Read/Write RAM bank 2

// =================================================
// Apple ][ Monitor Entry Points (for reference)
// =================================================

constexpr uint16_t SWEET16_ROM = 0xF689; // Original IntegerBASIC ROM entry point
constexpr uint16_t BELL1 = 0xFBDD;       // Bell
constexpr uint16_t HOME = 0xFC58;        // Clear screen
constexpr uint16_t RDKEY = 0xFD0C;       // Read key
constexpr uint16_t CROUT = 0xFD8E;       // Carriage return
constexpr uint16_t COUT = 0xFDED;        // Output char
constexpr uint16_t MON = 0xFF65;         // Monitor

// =================================================
// Default settings
// =================================================

constexpr char DEFAULT_CMD_DELIMITER = ']';
constexpr char DEFAULT_TAB_CHAR = ' ';
constexpr int DEFAULT_PAGE_LENGTH = 60;
constexpr int DEFAULT_COLUMNS_40 = 2;      // 2 columns for 40-col display
constexpr int DEFAULT_COLUMNS_80 = 4;      // 4 columns for 80-col display
constexpr int DEFAULT_COLUMNS_PRINTER = 6; // 6 columns for printer

// =================================================
// ProDOS MLI parameter offsets (from COMMONEQUS.S)
// =================================================

constexpr int C_PCNT = 0;     // Parameter count
constexpr int C_DEVNUM = 1;   // Device number
constexpr int C_REFNUM = 1;   // Reference number
constexpr int C_PATH = 1;     // Pathname pointer (2 bytes)
constexpr int C_DATABUF = 2;  // Data buffer pointer (2 bytes)
constexpr int C_ATTR = 3;     // File attributes
constexpr int C_FILEID = 4;   // File ID
constexpr int C_AUXID = 5;    // Auxiliary ID (2 bytes)
constexpr int C_FKIND = 7;    // File kind
constexpr int C_DATE = 8;     // Date (2 bytes)
constexpr int C_TIME = 10;    // Time (2 bytes)
constexpr int C_MODDATE = 10; // Modification date (2 bytes)
constexpr int C_MODTIME = 12; // Modification time (2 bytes)
constexpr int C_CREDATE = 14; // Creation date (2 bytes)
constexpr int C_CRETIME = 16; // Creation time (2 bytes)

} // namespace edasm
