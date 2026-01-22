#ifndef EDASM_HOST_SHIMS_HPP
#define EDASM_HOST_SHIMS_HPP

#include "cpu.hpp"
#include "bus.hpp"
#include <string>
#include <vector>
#include <queue>

namespace edasm {

// Host shims for ProDOS and monitor services
class HostShims {
public:
    HostShims();
    
    // Install shims into bus for I/O traps
    void install_io_traps(Bus& bus);
    
    // Queue input lines for EXEC-like feeding
    void queue_input_line(const std::string& line);
    void queue_input_lines(const std::vector<std::string>& lines);
    
    // Check if input queue has data
    bool has_queued_input() const;
    
    // Get next character from input queue (returns 0 if empty)
    char get_next_char();
    
private:
    std::queue<std::string> input_lines_;
    std::string current_line_;
    size_t current_pos_;
    
    // Soft switch handlers for $C0xx
    bool handle_kbd_read(uint16_t addr, uint8_t& value);
    bool handle_kbdstrb_read(uint16_t addr, uint8_t& value);
    
    // Apple II keyboard state
    uint8_t kbd_data_;
    bool kbd_strobe_;
};

} // namespace edasm

#endif // EDASM_HOST_SHIMS_HPP
