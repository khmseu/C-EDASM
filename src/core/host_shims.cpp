#include "edasm/host_shims.hpp"

namespace edasm {

// Apple II keyboard memory locations
constexpr uint16_t KBD = 0xC000;      // Keyboard data
constexpr uint16_t KBDSTRB = 0xC010;  // Keyboard strobe clear

HostShims::HostShims() 
    : current_pos_(0), kbd_data_(0), kbd_strobe_(false) {
}

void HostShims::install_io_traps(Bus& bus) {
    // Install keyboard traps
    bus.set_read_trap(KBD, [this](uint16_t addr, uint8_t& value) {
        return this->handle_kbd_read(addr, value);
    });
    
    bus.set_read_trap(KBDSTRB, [this](uint16_t addr, uint8_t& value) {
        return this->handle_kbdstrb_read(addr, value);
    });
}

void HostShims::queue_input_line(const std::string& line) {
    input_lines_.push(line);
}

void HostShims::queue_input_lines(const std::vector<std::string>& lines) {
    for (const auto& line : lines) {
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

bool HostShims::handle_kbd_read(uint16_t addr, uint8_t& value) {
    // Read keyboard data register
    if (kbd_strobe_) {
        value = kbd_data_ | 0x80; // High bit set indicates key available
    } else {
        // Check if we have queued input
        if (has_queued_input()) {
            char ch = get_next_char();
            if (ch != 0) {
                // Convert to Apple II keyboard format (high bit set)
                kbd_data_ = static_cast<uint8_t>(ch) & 0x7F;
                kbd_strobe_ = true;
                value = kbd_data_ | 0x80;
            } else {
                value = 0; // No key available
            }
        } else {
            value = 0; // No key available
        }
    }
    return true; // Trap handled
}

bool HostShims::handle_kbdstrb_read(uint16_t addr, uint8_t& value) {
    // Reading KBDSTRB clears the keyboard strobe
    value = 0;
    kbd_strobe_ = false;
    return true; // Trap handled
}

} // namespace edasm
