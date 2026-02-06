/**
 * @file host_shims.hpp
 * @brief Host I/O shims for emulator
 *
 * Provides shims for Apple II I/O operations, mapping them to host system.
 * Handles keyboard input, screen output, and other peripherals needed for
 * EDASM.SYSTEM execution.
 *
 * Features:
 * - I/O trap handlers for Apple II soft switches
 * - Input queue for automated testing
 * - Keyboard and screen emulation
 */

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
    HostShims(Bus &bus);

    // Install shims into bus for I/O traps
    void install_io_traps();

    // Queue input lines for EXEC-like feeding
    void queue_input_line(const std::string &line);
    void queue_input_lines(const std::vector<std::string> &lines);

    // Check if input queue has data
    bool has_queued_input() const;

    // Get next character from input queue (returns 0 if empty)
    char get_next_char();

    // Check if emulator should stop (set when first screen char is 'E')
    bool should_stop() const;

    // Static utility to dump text screen (page 1 or 2) to stdout
    static void dump_text_screen(const Bus &bus, bool page2 = false, const std::string &label = "");

  private:
    std::queue<std::string> input_lines_;
    std::string current_line_;
    size_t current_pos_;

    Bus &bus_;
    bool screen_dirty_;
    bool stop_requested_;

    // General I/O range handlers for $C000-$C7FF
    bool handle_io_read(uint16_t addr, uint8_t &value);
    bool handle_io_write(uint16_t addr, uint8_t value);

    // Report unimplemented I/O access and request emulator stop
    void report_unhandled_io(uint16_t addr, bool is_write, uint8_t value);

    // Dump screen and memory, then request stop
    void dump_and_stop(const std::string &reason);

    // Specific device handlers
    bool handle_kbd_read(uint16_t addr, uint8_t &value);
    bool handle_kbdstrb_read(uint16_t addr, uint8_t &value);
    bool handle_speaker_toggle(uint16_t addr, uint8_t &value);
    bool handle_graphics_switches(uint16_t addr, uint8_t &value, bool is_write);

    // Apple II soft switch state
    uint8_t kbd_value_; // $C000: Keyboard data (with high bit indicating new key available)
    int kbd_no_input_count_; // Counter for KBD reads with high bit off and no input
    bool text_mode_;    // $C050/$C051: Text/Graphics
    bool mixed_mode_;   // $C052/$C053: Full/Mixed screen
    bool page2_;        // $C054/$C055: Page 1/Page 2
    bool hires_;        // $C056/$C057: Lo-res/Hi-res

    // 80-column state (CLR80VID / SET80VID)
    bool eighty_col_enabled_ = false;
    // Language Card state
    enum class LCBankMode {
        READ_RAM_NO_WRITE,
        READ_ROM_WRITE_RAM,
        READ_ROM_ONLY,
        READ_RAM_WRITE_RAM
    };
    struct LanguageCardState {
        // Per-bank mode (applies primarily to behavior of D000..DFFF mapping and ROM vs RAM read
        // semantics) Modes stored per bank index (0 or 1) indicate what happens when that bank is
        // active.
        std::array<LCBankMode, 2> bank_mode{LCBankMode::READ_ROM_ONLY, LCBankMode::READ_ROM_ONLY};

        // active window bank: 0 = first bank, 1 = second bank
        uint8_t active_bank = 0;

        // Power-on state: RAM disabled and ROM active (per hardware behavior)
        bool power_on_rom_active = true;

        // Track last accessed address for double-read requirement
        // Addresses $C081, $C083, $C089, $C08B require two successive reads to enable write
        uint16_t last_control_addr = 0xFFFF;
        bool write_enable_pending = false;
    };

    LanguageCardState lc_;

    // Language Card handlers
    bool handle_language_control_read(uint16_t addr, uint8_t &value);
    bool handle_language_control_write(uint16_t addr, uint8_t value);
    void update_lc_bank_mappings(); // Update bank mappings based on LC state
};

} // namespace edasm

#endif // EDASM_HOST_SHIMS_HPP
