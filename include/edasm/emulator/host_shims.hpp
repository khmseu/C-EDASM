// Host shims header
#ifndef EDASM_HOST_SHIMS_HPP
#define EDASM_HOST_SHIMS_HPP

#include "bus.hpp"
#include "cpu.hpp"
#include <queue>
#include <string>
#include <vector>

namespace edasm {

// Host shims for ProDOS and monitor services
class HostShims {
  public:
    HostShims();

    // Install shims into bus for I/O traps
    void install_io_traps(Bus &bus);

    // Queue input lines for EXEC-like feeding
    void queue_input_line(const std::string &line);
    void queue_input_lines(const std::vector<std::string> &lines);

    // Check if input queue has data
    bool has_queued_input() const;

    // Get next character from input queue (returns 0 if empty)
    char get_next_char();

    // Check if emulator should stop (set when first screen char is 'E')
    bool should_stop() const;

  private:
    std::queue<std::string> input_lines_;
    std::string current_line_;
    size_t current_pos_;

    Bus *bus_;
    bool screen_dirty_;
    bool stop_requested_;

    // General I/O range handlers for $C000-$C7FF
    bool handle_io_read(uint16_t addr, uint8_t &value);
    bool handle_io_write(uint16_t addr, uint8_t value);

    // Report unimplemented I/O access and request emulator stop
    void report_unhandled_io(uint16_t addr, bool is_write, uint8_t value);

    // Specific device handlers
    bool handle_kbd_read(uint16_t addr, uint8_t &value);
    bool handle_kbdstrb_read(uint16_t addr, uint8_t &value);
    bool handle_speaker_toggle(uint16_t addr, uint8_t &value);
    bool handle_graphics_switches(uint16_t addr, uint8_t &value, bool is_write);

    void log_text_screen();

    // Apple II soft switch state
    uint8_t kbd_data_; // $C000: Keyboard data
    bool kbd_strobe_;  // Keyboard available flag
    bool text_mode_;   // $C050/$C051: Text/Graphics
    bool mixed_mode_;  // $C052/$C053: Full/Mixed screen
    bool page2_;       // $C054/$C055: Page 1/Page 2
    bool hires_;       // $C056/$C057: Lo-res/Hi-res
};

} // namespace edasm

#endif // EDASM_HOST_SHIMS_HPP
