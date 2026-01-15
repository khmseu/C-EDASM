#pragma once

#include <optional>
#include <string>
#include <unordered_map>

namespace edasm {

class SymbolTable {
  public:
    void reset();
    void define(const std::string &name, int value);
    std::optional<int> lookup(const std::string &name) const;

  private:
    std::unordered_map<std::string, int> table_;
};

} // namespace edasm
