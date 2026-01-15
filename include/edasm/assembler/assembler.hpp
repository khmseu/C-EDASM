#pragma once

#include <string>
#include <vector>

#include "edasm/assembler/symbol_table.hpp"

namespace edasm {

class Assembler {
  public:
    struct Result {
        bool success{false};
        std::vector<std::string> errors;
    };

    Assembler();
    Result assemble(const std::string &source);
    void reset();

  private:
    SymbolTable symbols_;
};

} // namespace edasm
