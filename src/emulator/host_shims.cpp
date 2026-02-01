#include "edasm/emulator/host_shims.hpp"
#include "edasm/constants.hpp"
#include "edasm/emulator/traps.hpp"

#include <iomanip>
#include <iostream>

namespace edasm {

HostShims::HostShims()
    : current_pos_(0), bus_(nullptr), screen_dirty_(false), kbd_data_(0), kbd_strobe_(false),
      text_mode_(true), mixed_mode_(false), page2_(false), hires_(false), stop_requested_(false) {}

void HostShims::install_io_traps(Bus &bus) {
    bus_ = &bus;

    // Install I/O traps for full $C000-$C7FF range
    bus.set_read_trap_range(KBD, 0xC7FF, [this](uint16_t addr, uint8_t &value) {
        return this->handle_io_read(addr, value);
    });

    bus.set_write_trap_range(KBD, 0xC7FF, [this](uint16_t addr, uint8_t value) {
        return this->handle_io_write(addr, value);
    });

    // Install write trap for text page 1 ($0400-$07FF)
    bus.set_write_trap_range(0x0400, 0x07FF, [this](uint16_t addr, uint8_t value) {
        // Mark screen as dirty but allow normal write to proceed
        screen_dirty_ = true;

        // Check if writing to first character position ($0400)
        // Strip high bit and check for 'E' (handles normal, inverse, and flashing text)
        if (addr == 0x0400) {
            // Check for 'E' by masking high bit (handles all Apple II text modes)
            char ch = static_cast<char>(value & 0x7F);
            if (ch == 'E' || ch == 'e') {
                std::cout
                    << "\n[HostShims] First screen character set to 'E' - logging and stopping\n"
                    << std::endl;
                log_text_screen();
                if (bus_) {
                    TrapManager::write_memory_dump(*bus_, "memory_dump.bin");
                }
                stop_requested_ = true;
            }
        }

        return false;
    });

    // NOTE: Language card window ($D000-$FFFF) no longer uses traps
    // It's now handled via bank mapping in Bus::set_bank_mapping()
}

void HostShims::queue_input_line(const std::string &line) {
    input_lines_.push(line);
}

void HostShims::queue_input_lines(const std::vector<std::string> &lines) {
    for (const auto &line : lines) {
        input_lines_.push(line);
    }
}

bool HostShims::has_queued_input() const {
    return !input_lines_.empty() || current_pos_ < current_line_.size();
}

char HostShims::get_next_char() {
    // If current line exhausted, get next line
    if (current_pos_ >= current_line_.size()) {
        if (input_lines_.empty()) {
            return 0; // No more input
        }
        current_line_ = input_lines_.front();
        input_lines_.pop();
        current_line_ += '\r'; // Add carriage return
        current_pos_ = 0;
    }

    // Return next character
    char ch = current_line_[current_pos_++];
    return ch;
}

bool HostShims::handle_kbd_read(uint16_t addr, uint8_t &value) {
    if (screen_dirty_ && bus_) {
        log_text_screen();
        screen_dirty_ = false;
    }

    // Read keyboard data register
    if (kbd_strobe_) {
        // Key available - return with high bit set
        value = kbd_data_ | 0x80;
    } else {
        // Strobe cleared - return last key without high bit
        // Only fetch new character if kbd_data_ is 0 (no previous key)
        if (kbd_data_ == 0 && has_queued_input()) {
            char ch = get_next_char();
            if (ch != 0) {
                // Convert to Apple II keyboard format
                kbd_data_ = static_cast<uint8_t>(ch) & 0x7F;
                kbd_strobe_ = true;
                value = kbd_data_ | 0x80;
            } else {
                value = 0; // No key available
            }
        } else {
            // Return last key without high bit (strobe cleared)
            value = kbd_data_;
        }
    }
    return true; // Trap handled
}

bool HostShims::handle_kbdstrb_read(uint16_t addr, uint8_t &value) {
    // Reading KBDSTROBE clears the keyboard strobe
    value = 0;
    kbd_strobe_ = false;
    return true; // Trap handled
}

bool HostShims::handle_io_read(uint16_t addr, uint8_t &value) {
    // Dispatch to specific handlers based on address

    // $C000-$C00F: Keyboard and game I/O
    if (addr >= KBD && addr <= 0xC00F) {
        return handle_kbd_read(addr, value);
    }

    // $C010-$C01F: Keyboard strobe and soft switches
    if (addr >= KBDSTROBE && addr <= 0xC01F) {
        if (addr == KBDSTROBE) {
            return handle_kbdstrb_read(addr, value);
        }
        // Other addresses in this range: return 0
        value = 0;
        return true;
    }

    // $C020-$C02F: Cassette and misc I/O
    if (addr >= 0xC020 && addr <= 0xC02F) {
        value = 0;
        report_unhandled_io(addr, false, value);
        return true;
    }

    // $C030-$C03F: Speaker toggle
    if (addr >= 0xC030 && addr <= 0xC03F) {
        return handle_speaker_toggle(addr, value);
    }

    // $C040-$C04F: Utility strobe (not used much)
    if (addr >= 0xC040 && addr <= 0xC04F) {
        value = 0;
        report_unhandled_io(addr, false, value);
        return true;
    }

    // $C050-$C05F: Graphics mode switches
    if (addr >= 0xC050 && addr <= 0xC05F) {
        return handle_graphics_switches(addr, value, false);
    }

    // $C060-$C06F: Game paddles/buttons
    if (addr >= 0xC060 && addr <= 0xC06F) {
        // Return high bit clear (button not pressed)
        value = 0x00;
        return true;
    }

    // $C070-$C07F: Game paddle triggers
    if (addr >= 0xC070 && addr <= 0xC07F) {
        value = 0;
        report_unhandled_io(addr, false, value);
        return true;
    }

    // $C080-$C08F: Language card bank switching (IIe/IIc)
    if (addr >= 0xC080 && addr <= 0xC08F) {
        return handle_language_control_read(addr, value);
    }

    // $C090-$C7FF: Expansion slots and additional I/O
    // Each slot occupies 16 bytes: $C0n0-$C0nF (where n is slot 1-7)
    // For now, return 0 for undefined I/O
    value = 0;
    report_unhandled_io(addr, false, value);
    return true;
}

bool HostShims::handle_io_write(uint16_t addr, uint8_t value) {
    // Dispatch to specific handlers based on address

    // $C000-$C00F: Keyboard/game I/O (typically read-only)
    if (addr >= KBD && addr <= 0xC00F) {
        // Handle known soft-switches gracefully
        if (addr == CLR80VID) { // CLR80VID - clear 80-column mode
            eighty_col_enabled_ = false;
            return true;
        }
        if (addr == static_cast<uint16_t>(CLR80VID + 1)) { // SET80VID - set 80-column mode
            eighty_col_enabled_ = true;
            return true;
        }
        // Unknown writes: ignore but do not stop the emulator
        return true;
    }

    // $C010-$C01F: Keyboard strobe and soft switches
    if (addr >= KBDSTROBE && addr <= 0xC01F) {
        if (addr == KBDSTROBE) {
            // Writing to KBDSTROBE also clears strobe
            kbd_strobe_ = false;
        }
        return true;
    }

    // $C020-$C02F: Cassette output and misc
    if (addr >= 0xC020 && addr <= 0xC02F) {
        report_unhandled_io(addr, true, value);
        return true; // Ignore
    }

    // $C030-$C03F: Speaker toggle
    if (addr >= 0xC030 && addr <= 0xC03F) {
        uint8_t dummy;
        return handle_speaker_toggle(addr, dummy);
    }

    // $C040-$C04F: Utility strobe
    if (addr >= 0xC040 && addr <= 0xC04F) {
        return true; // Ignore
    }

    // $C050-$C05F: Graphics mode switches (write = read)
    if (addr >= 0xC050 && addr <= 0xC05F) {
        uint8_t dummy;
        return handle_graphics_switches(addr, dummy, true);
    }

    // $C060-$C07F: Game I/O (typically read-only)
    if (addr >= 0xC060 && addr <= 0xC07F) {
        report_unhandled_io(addr, true, value);
        return true; // Ignore writes
    }

    // $C080-$C08F: Language card bank switching
    if (addr >= 0xC080 && addr <= 0xC08F) {
        return handle_language_control_write(addr, value);
    }

    // $C090-$C7FF: Expansion slots and additional I/O
    // Ignore writes to undefined I/O
    report_unhandled_io(addr, true, value);
    return true;
}

bool HostShims::handle_speaker_toggle(uint16_t addr, uint8_t &value) {
    // Speaker toggle: any access to $C030 toggles speaker
    // We don't actually produce sound, just acknowledge the access
    value = 0;
    return true;
}

bool HostShims::handle_graphics_switches(uint16_t addr, uint8_t &value, bool is_write) {
    // Graphics soft switches (read or write has same effect)
    // $C050: TXTCLR (Graphics mode)
    // $C051: TXTSET (Text mode)
    // $C052: MIXCLR (Full screen graphics)
    // $C053: MIXSET (Mixed text/graphics)
    // $C054: LOWSCR (Page 1)
    // $C055: HISCR (Page 2)
    // $C056: LORES (Lo-res graphics)
    // $C057: HIRES (Hi-res graphics)
    // $C058: CLRAN0-$C05F: Various other switches

    switch (addr) {
    case 0xC050: // TXTCLR - Graphics mode
        text_mode_ = false;
        value = 0;
        break;
    case 0xC051: // TXTSET - Text mode
        text_mode_ = true;
        value = 0;
        break;
    case 0xC052: // MIXCLR - Full screen
        mixed_mode_ = false;
        value = 0;
        break;
    case 0xC053: // MIXSET - Mixed mode
        mixed_mode_ = true;
        value = 0;
        break;
    case 0xC054: // LOWSCR - Page 1
        page2_ = false;
        value = 0;
        break;
    case 0xC055: // HISCR - Page 2
        page2_ = true;
        value = 0;
        break;
    case 0xC056: // LORES - Lo-res graphics
        hires_ = false;
        value = 0;
        break;
    case 0xC057: // HIRES - Hi-res graphics
        hires_ = true;
        value = 0;
        break;
    default:
        // $C058-$C05F: Annunciators and other switches
        value = 0;
        break;
    }

    return true;
}

void HostShims::log_text_screen() {
    if (!bus_) {
        return;
    }

    const uint16_t base = page2_ ? 0x0800 : 0x0400;

    std::cout << "[HostShims] Text screen snapshot (page " << (page2_ ? 2 : 1) << ")\n";

    for (int row = 0; row < 24; ++row) {
        std::cout << std::setw(2) << row << ": ";
        for (int col = 0; col < 40; ++col) {
            const uint16_t addr =
                static_cast<uint16_t>(base + (row % 8) * 128 + (row / 8) * 40 + col);
            uint8_t byte = bus_->read(addr);
            char ch = static_cast<char>(byte & 0x7F);
            if (ch < 0x20 || ch > 0x7E) {
                ch = '.';
            }
            std::cout << ch;
        }
        std::cout << '\n';
    }

    std::cout.flush();
}

// Report unimplemented I/O access and request emulator stop
void HostShims::report_unhandled_io(uint16_t addr, bool is_write, uint8_t value) {
    std::cerr << "[HostShims] UNIMPLEMENTED I/O " << (is_write ? "WRITE" : "READ") << " at $"
              << std::hex << std::uppercase << std::setw(4) << std::setfill('0') << addr
              << " value=$" << std::setw(2) << static_cast<int>(value) << " - stopping"
              << std::endl;
    if (bus_) {
        log_text_screen();
        TrapManager::write_memory_dump(*bus_, "memory_dump.bin");
    }
    stop_requested_ = true;
}

// Language Card control: handle reads/writes to $C080..$C08F
// Reference: docs/APPLE_IIE_MEMORY_MAP.md "Bank-Switched RAM Control"
//
// The Apple IIe uses control bits in the address to determine behavior:
//   Bit 3: BANK-SELECT (1=Bank 1, 0=Bank 2)
//   Bit 2: (unused)
//   Bit 1: READ-SELECT (if equals write-select bit, read from RAM; else ROM)
//   Bit 0: WRITE-SELECT (1 + double-read = write to RAM; else ROM)
//
// Address mapping:
//   $C080 (READBSR2):  Bank 2, Read RAM, Write No
//   $C081 (WRITEBSR2): Bank 2, Read ROM, Write Yes (RR - requires 2 reads)
//   $C082 (OFFBSR2):   Bank 2, Read ROM, Write No
//   $C083 (RDWRBSR2):  Bank 2, Read RAM, Write Yes (RR - requires 2 reads)
//   $C088 (READBSR1):  Bank 1, Read RAM, Write No
//   $C089 (WRITEBSR1): Bank 1, Read ROM, Write Yes (RR - requires 2 reads)
//   $C08A (OFFBSR1):   Bank 1, Read ROM, Write No
//   $C08B (RDWRBSR1):  Bank 1, Read RAM, Write Yes (RR - requires 2 reads)
//   $C084-$C087 duplicate $C080-$C083, $C08C-$C08F duplicate $C088-$C08B
//
// NOTE: The current implementation does not handle the double-read requirement
// for write-enable (addresses ending in 1 or 3). A single read/write enables
// write mode, whereas the real hardware requires two successive reads.
bool HostShims::handle_language_control_read(uint16_t addr, uint8_t &value) {
    // Map addresses into bank and mode
    uint16_t offset = addr & 0x0F;                             // 0..15 within control page
    // Bit 3 set ($C088-$C08F) = Bank 1, Bit 3 clear ($C080-$C087) = Bank 2
    uint8_t bank = (addr >= 0xC088 && addr <= 0xC08F) ? 0 : 1; // bank1 => 0, bank2 => 1

    // Map offset to mode (group by 4)
    uint8_t group = offset & 0x03;               // 0..3
    LCBankMode mode = LCBankMode::READ_ROM_ONLY; // power-on default/most conservative
    switch (group) {
    case 0:
        mode = LCBankMode::READ_RAM_NO_WRITE; // C080/C088
        break;
    case 1:
        mode = LCBankMode::READ_ROM_WRITE_RAM; // C081/C089
        break;
    case 2:
        mode = LCBankMode::READ_ROM_ONLY; // C082/C08A
        break;
    case 3:
        mode = LCBankMode::READ_RAM_WRITE_RAM; // C083/C08B
        break;
    }

    lc_.bank_mode[bank] = mode;
    lc_.active_bank = bank;
    lc_.power_on_rom_active =
        (mode == LCBankMode::READ_ROM_ONLY || mode == LCBankMode::READ_ROM_WRITE_RAM);

    std::cout << "[HostShims] Language Card control read at $" << std::hex << std::uppercase
              << std::setw(4) << std::setfill('0') << addr << " -> bank=" << std::dec
              << static_cast<int>(bank) << " mode=" << static_cast<int>(mode) << std::endl;

    // Update bank mappings for D000-FFFF (banks 26-31)
    update_lc_bank_mappings();

    value = 0;
    return true;
}

bool HostShims::handle_language_control_write(uint16_t addr, uint8_t value) {
    // Writes have same effect as reads for soft switches
    uint8_t dummy;
    bool ok = handle_language_control_read(addr, dummy);
    std::cout << "[HostShims] Language Card control write at $" << std::hex << std::uppercase
              << std::setw(4) << std::setfill('0') << addr << " value=$" << std::setw(2)
              << static_cast<int>(value) << std::endl;
    return ok;
}

// Update bank mappings for the language card region (D000-FFFF)
// This replaces the old trap-based approach with direct bank mapping
void HostShims::update_lc_bank_mappings() {
    if (!bus_) {
        return;
    }

    uint8_t bank = lc_.active_bank & 0x1;
    auto mode = lc_.bank_mode[bank];

    // For each 2KB bank in the D000-FFFF range (banks 26-31), update mappings
    // Bank 26: D000-D7FF (first half of 4KB banked area)
    // Bank 27: D800-DFFF (second half of 4KB banked area)
    // Bank 28: E000-E7FF (first quarter of 8KB fixed area)
    // Bank 29: E800-EFFF (second quarter of 8KB fixed area)
    // Bank 30: F000-F7FF (third quarter of 8KB fixed area)
    // Bank 31: F800-FFFF (fourth quarter of 8KB fixed area)

    // D000-DFFF (banks 26-27): Banked region
    for (uint8_t bank_idx = 26; bank_idx <= 27; ++bank_idx) {
        uint32_t bank_addr = bank_idx * Bus::BANK_SIZE; // Address this bank represents
        uint32_t offset_in_region = bank_addr - 0xD000;
        uint32_t read_offset, write_offset;

        if (mode == LCBankMode::READ_RAM_NO_WRITE || mode == LCBankMode::READ_RAM_WRITE_RAM) {
            // Read from RAM (appropriate bank)
            if (bank == 0) {
                read_offset = Bus::LC_BANK1_OFFSET + offset_in_region;
            } else {
                read_offset = Bus::LC_BANK2_OFFSET + offset_in_region;
            }
        } else {
            // Read from ROM (in main RAM at D000+)
            read_offset = Bus::MAIN_RAM_OFFSET + bank_addr;
        }

        if (mode == LCBankMode::READ_RAM_WRITE_RAM || mode == LCBankMode::READ_ROM_WRITE_RAM) {
            // Writes go to RAM
            if (bank == 0) {
                write_offset = Bus::LC_BANK1_OFFSET + offset_in_region;
            } else {
                write_offset = Bus::LC_BANK2_OFFSET + offset_in_region;
            }
        } else {
            // Writes ignored (go to write sink)
            write_offset = Bus::WRITE_SINK_OFFSET;
        }

        bus_->set_bank_mapping(bank_idx, read_offset, write_offset);
    }

    // E000-FFFF (banks 28-31): Fixed RAM/ROM region
    for (uint8_t bank_idx = 28; bank_idx <= 31; ++bank_idx) {
        uint32_t bank_addr = bank_idx * Bus::BANK_SIZE; // Address this bank represents
        uint32_t offset_in_fixed = bank_addr - 0xE000;  // Offset from E000
        uint32_t read_offset, write_offset;

        if (mode == LCBankMode::READ_RAM_NO_WRITE || mode == LCBankMode::READ_RAM_WRITE_RAM) {
            // Read from fixed RAM
            read_offset = Bus::LC_FIXED_RAM_OFFSET + offset_in_fixed;
        } else {
            // Read from ROM (in main RAM at E000+)
            read_offset = Bus::MAIN_RAM_OFFSET + bank_addr;
        }

        if (mode == LCBankMode::READ_RAM_WRITE_RAM || mode == LCBankMode::READ_ROM_WRITE_RAM) {
            // Writes go to fixed RAM
            write_offset = Bus::LC_FIXED_RAM_OFFSET + offset_in_fixed;
        } else {
            // Writes ignored (go to write sink)
            write_offset = Bus::WRITE_SINK_OFFSET;
        }

        bus_->set_bank_mapping(bank_idx, read_offset, write_offset);
    }
}

// Handle reads from the entire language-card ROM/overlay window ($D000-$FFFF)
// NOTE: This is now unused - kept for compatibility during transition
bool HostShims::handle_lc_read(uint16_t addr, uint8_t &value) {
    // This function is no longer called since we removed the D000-FFFF read trap
    // Kept for reference/compatibility
    return false;
}

bool HostShims::handle_lc_write(uint16_t addr, uint8_t value) {
    // This function is no longer called since we removed the D000-FFFF write trap
    // Kept for reference/compatibility
    return false;
}

bool HostShims::should_stop() const {
    return stop_requested_;
}

} // namespace edasm
