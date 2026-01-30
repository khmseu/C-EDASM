#include "edasm/emulator/host_shims.hpp"
#include "edasm/emulator/traps.hpp"

#include <iomanip>
#include <iostream>

namespace edasm {

// Apple II keyboard memory locations
constexpr uint16_t KBD = 0xC000;     // Keyboard data
constexpr uint16_t KBDSTRB = 0xC010; // Keyboard strobe clear

HostShims::HostShims()
    : current_pos_(0), bus_(nullptr), screen_dirty_(false), kbd_data_(0), kbd_strobe_(false),
      text_mode_(true), mixed_mode_(false), page2_(false), hires_(false), stop_requested_(false) {}

void HostShims::install_io_traps(Bus &bus) {
    bus_ = &bus;

    // Install I/O traps for full $C000-$C7FF range
    bus.set_read_trap_range(0xC000, 0xC7FF, [this](uint16_t addr, uint8_t &value) {
        return this->handle_io_read(addr, value);
    });

    bus.set_write_trap_range(0xC000, 0xC7FF, [this](uint16_t addr, uint8_t value) {
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
    // Reading KBDSTRB clears the keyboard strobe
    value = 0;
    kbd_strobe_ = false;
    return true; // Trap handled
}

bool HostShims::handle_io_read(uint16_t addr, uint8_t &value) {
    // Dispatch to specific handlers based on address

    // $C000-$C00F: Keyboard and game I/O
    if (addr >= 0xC000 && addr <= 0xC00F) {
        return handle_kbd_read(addr, value);
    }

    // $C010-$C01F: Keyboard strobe and soft switches
    if (addr >= 0xC010 && addr <= 0xC01F) {
        if (addr == 0xC010) {
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
        // Minimal implementation: return 0
        value = 0;
        report_unhandled_io(addr, false, value);
        return true;
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
    if (addr >= 0xC000 && addr <= 0xC00F) {
        report_unhandled_io(addr, true, value);
        return true; // Ignore writes but report
    }

    // $C010-$C01F: Keyboard strobe and soft switches
    if (addr >= 0xC010 && addr <= 0xC01F) {
        if (addr == 0xC010) {
            // Writing to KBDSTRB also clears strobe
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
        // Minimal implementation: ignore
        report_unhandled_io(addr, true, value);
        return true;
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

bool HostShims::should_stop() const {
    return stop_requested_;
}

} // namespace edasm
