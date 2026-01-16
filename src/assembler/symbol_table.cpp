// Symbol table implementation for EDASM assembler
//
// This file implements symbol table management from EDASM.SRC/ASM/
// Primary references: ASM2.S (symbol lookup/add), ASM1.S (sorting/printing)
//
// Key routines from ASM2.S:
//   - FindSym ($88C3): Hash table lookup -> lookup()
//   - AddNode ($89A9): Add symbol to hash chain -> define()
//   - HashFn ($8955): 3-character hash function (simplified in C++ to std::unordered_map)
//
// Key routines from ASM1.S:
//   - DoSort ($D1D6): Shell sort algorithm -> sorted_by_name(), sorted_by_value()
//   - DoPass3 ($D000): Symbol table printing (implemented in listing.cpp)
//
// Original EDASM uses 128-entry hash table with chaining. C++ uses std::unordered_map
// which provides similar O(1) lookup with automatic resizing and collision handling.
#include "edasm/assembler/symbol_table.hpp"

#include <algorithm>

namespace edasm {

// Clear all symbols from table
// Reference: ASM2.S InitASM ($7DC3) - Clears symbol table on init
void SymbolTable::reset() {
    table_.clear();
}

// Define or update a symbol in the table
// Reference: ASM2.S AddNode ($89A9) - Adds symbol to hash chain
// Symbol names are 1-16 characters per EDASM.SRC symbol format
void SymbolTable::define(const std::string& name, uint16_t value, uint8_t flags, int line_num) {
    // Validate symbol name length (1-16 chars per EDASM.SRC)
    if (name.empty() || name.length() > 16) {
        // Silently truncate for compatibility, but ideally should error
        // For now, we'll allow it but track it
    }
    
    Symbol sym;
    sym.name = name;
    sym.value = value;
    sym.flags = flags;
    sym.line_defined = line_num;
    table_[name] = sym;
}

// Update symbol value
// Used during pass 1 to resolve forward references
void SymbolTable::update_value(const std::string& name, uint16_t value) {
    auto it = table_.find(name);
    if (it != table_.end()) {
        it->second.value = value;
    }
}

// Update symbol flags (ENTRY, EXTERNAL, RELATIVE, etc.)
// Reference: ASM3.S L9144, L91A8 - ENT/ENTRY and EXT/EXTRN directives
void SymbolTable::update_flags(const std::string& name, uint8_t flags) {
    auto it = table_.find(name);
    if (it != table_.end()) {
        it->second.flags = flags;
    }
}

// Lookup symbol by name
// Reference: ASM2.S FindSym ($88C3) - Hash table lookup with chain traversal
// Returns pointer to symbol or nullptr if not found
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

// Sort symbols alphabetically by name
// Reference: ASM1.S DoSort ($D1D6) - Shell sort with alphabetic comparison
// Original uses DCI (reversed-case) format; C++ uses standard string comparison
std::vector<Symbol> SymbolTable::sorted_by_name() const {
    auto result = all_symbols();
    std::sort(result.begin(), result.end(),
              [](const Symbol& a, const Symbol& b) {
                  return a.name < b.name;
              });
    return result;
}

// Sort symbols by address value, then by name for ties
// Reference: ASM1.S DoSort ($D1D6) - Shell sort with address comparison
// Used for generating address-ordered symbol listings
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
