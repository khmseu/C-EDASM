#include "edasm/assembler/symbol_table.hpp"

namespace edasm {

void SymbolTable::reset() {
    table_.clear();
}

void SymbolTable::define(const std::string &name, int value) {
    table_[name] = value;
}

std::optional<int> SymbolTable::lookup(const std::string &name) const {
    auto it = table_.find(name);
    if (it == table_.end()) {
        return std::nullopt;
    }
    return it->second;
}

} // namespace edasm
