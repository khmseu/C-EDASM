#include "edasm/assembler/assembler.hpp"

namespace edasm {

Assembler::Assembler() = default;

Assembler::Result Assembler::assemble(const std::string &source) {
    Result result;
    result.success = true;
    // TODO: Translate 6502 EDASM assembler logic to C++.
    // This placeholder marks where pass 1/2, symbol resolution, and output
    // file emission will be implemented.
    (void)source;
    return result;
}

void Assembler::reset() {
    symbols_.reset();
}

} // namespace edasm
