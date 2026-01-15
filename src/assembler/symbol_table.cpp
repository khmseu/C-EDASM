#include "edasm/assembler/symbol_table.hpp"

#include <algorithm>

namespace edasm {

void SymbolTable::reset() {
    table_.clear();
}

void SymbolTable::define(const std::string& name, uint16_t value, uint8_t flags, int line_num) {
    Symbol sym;
    sym.name = name;
    sym.value = value;
    sym.flags = flags;
    sym.line_defined = line_num;
    table_[name] = sym;
}

void SymbolTable::update_value(const std::string& name, uint16_t value) {
    auto it = table_.find(name);
    if (it != table_.end()) {
        it->second.value = value;
    }
}

void SymbolTable::update_flags(const std::string& name, uint8_t flags) {
    auto it = table_.find(name);
    if (it != table_.end()) {
        it->second.flags = flags;
    }
}

Symbol* SymbolTable::lookup(const std::string& name) {
    auto it = table_.find(name);
    if (it == table_.end()) {
        return nullptr;
    }
    return &it->second;
}

const Symbol* SymbolTable::lookup(const std::string& name) const {
    auto it = table_.find(name);
    if (it == table_.end()) {
        return nullptr;
    }
    return &it->second;
}

std::optional<uint16_t> SymbolTable::get_value(const std::string& name) const {
    auto sym = lookup(name);
    if (!sym || sym->is_undefined()) {
        return std::nullopt;
    }
    return sym->value;
}

bool SymbolTable::is_defined(const std::string& name) const {
    auto sym = lookup(name);
    return sym && !sym->is_undefined();
}

std::vector<Symbol> SymbolTable::all_symbols() const {
    std::vector<Symbol> result;
    result.reserve(table_.size());
    for (const auto& [name, sym] : table_) {
        result.push_back(sym);
    }
    return result;
}

std::vector<Symbol> SymbolTable::sorted_by_name() const {
    auto result = all_symbols();
    std::sort(result.begin(), result.end(),
              [](const Symbol& a, const Symbol& b) {
                  return a.name < b.name;
              });
    return result;
}

std::vector<Symbol> SymbolTable::sorted_by_value() const {
    auto result = all_symbols();
    std::sort(result.begin(), result.end(),
              [](const Symbol& a, const Symbol& b) {
                  if (a.value != b.value) {
                      return a.value < b.value;
                  }
                  return a.name < b.name;
              });
    return result;
}

} // namespace edasm
