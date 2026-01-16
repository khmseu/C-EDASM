#pragma once

#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

#include "edasm/constants.hpp"

namespace edasm {

// Symbol entry (from ASM symbol table structure)
struct Symbol {
    std::string name;
    uint16_t value{0};
    uint8_t flags{0};  // SYM_* flags from constants.hpp
    int line_defined{0};  // For error reporting
    uint8_t symbol_number{0};  // Symbol number for REL file EXTERN refs
    
    bool is_undefined() const { return (flags & SYM_UNDEFINED) != 0; }
    bool is_relative() const { return (flags & SYM_RELATIVE) != 0; }
    bool is_external() const { return (flags & SYM_EXTERNAL) != 0; }
    bool is_entry() const { return (flags & SYM_ENTRY) != 0; }
    bool is_forward_ref() const { return (flags & SYM_FORWARD_REF) != 0; }
    bool is_unreferenced() const { return (flags & SYM_UNREFERENCED) != 0; }
};

class SymbolTable {
  public:
    void reset();
    
    // Symbol definition and lookup
    void define(const std::string& name, uint16_t value, uint8_t flags = 0, int line_num = 0);
    void update_value(const std::string& name, uint16_t value);
    void update_flags(const std::string& name, uint8_t flags);
    void mark_referenced(const std::string& name);
    
    Symbol* lookup(const std::string& name);
    const Symbol* lookup(const std::string& name) const;
    std::optional<uint16_t> get_value(const std::string& name) const;
    
    bool is_defined(const std::string& name) const;
    
    // Symbol table inspection
    std::vector<Symbol> all_symbols() const;
    std::vector<Symbol> sorted_by_name() const;
    std::vector<Symbol> sorted_by_value() const;
    
    // Get all symbols as map (for iteration in REL generation)
    const std::unordered_map<std::string, Symbol>& get_all() const { return table_; }
    
    size_t size() const { return table_.size(); }

  private:
    std::unordered_map<std::string, Symbol> table_;
};

} // namespace edasm
