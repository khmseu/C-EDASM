#ifndef EDASM_MLI_HPP
#define EDASM_MLI_HPP

#include "bus.hpp"
#include "cpu.hpp"
#include <cstdint>
#include <string>

namespace edasm {

// ProDOS MLI (Machine Language Interface) handler
class MLIHandler {
  public:
    // Set trace mode (enables detailed logging)
    static void set_trace(bool enabled);
    static bool is_trace_enabled();

    // ProDOS MLI trap handler: decode and log MLI calls (for $BF00)
    static bool prodos_mli_trap_handler(CPUState &cpu, Bus &bus, uint16_t trap_pc);

    // Helper utilities used by MLI handler
    static void set_success(CPUState &cpu);
    static void set_error(CPUState &cpu, uint8_t err);
    static bool write_memory_dump(const Bus &bus, const std::string &filename);

  private:
    // Helper for ProDOS MLI decoding
    static std::string decode_prodos_call(uint8_t call_num);

    // Trace mode flag
    static bool s_trace_enabled;
};

} // namespace edasm

#endif // EDASM_MLI_HPP
