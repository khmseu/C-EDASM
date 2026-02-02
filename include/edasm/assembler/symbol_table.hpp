/**
 * @file symbol_table.hpp
 * @brief Symbol table for assembler
 *
 * Implements the symbol table from ASM with support for labels, equates,
 * external symbols, and entry points. Uses hash-based lookup similar to
 * original EDASM (256 buckets in 6502, std::unordered_map in C++).
 */

#pragma once

#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

#include "edasm/constants.hpp"

namespace edasm {

/**
 * @brief Symbol table entry
 *
 * Represents a single symbol with its value and metadata flags.
 * Flags indicate symbol properties: relative, external, entry, undefined, etc.
 */
struct Symbol {
    std::string name;         ///< Symbol name
    uint16_t value{0};        ///< Symbol value (address or constant)
    uint8_t flags{0};         ///< SYM_* flags from constants.hpp
    int line_defined{0};      ///< Line where symbol was defined
    uint8_t symbol_number{0}; ///< Symbol number for REL file EXTERN refs

    /**
     * @brief Check if symbol is undefined
     * @return bool True if SYM_UNDEFINED flag set
     */
    bool is_undefined() const {
        return (flags & SYM_UNDEFINED) != 0;
    }

    /**
     * @brief Check if symbol is relative (relocatable)
     * @return bool True if SYM_RELATIVE flag set
     */
    bool is_relative() const {
        return (flags & SYM_RELATIVE) != 0;
    }

    /**
     * @brief Check if symbol is external reference
     * @return bool True if SYM_EXTERNAL flag set
     */
    bool is_external() const {
        return (flags & SYM_EXTERNAL) != 0;
    }

    /**
     * @brief Check if symbol is entry point
     * @return bool True if SYM_ENTRY flag set
     */
    bool is_entry() const {
        return (flags & SYM_ENTRY) != 0;
    }

    /**
     * @brief Check if symbol is forward reference
     * @return bool True if SYM_FORWARD_REF flag set
     */
    bool is_forward_ref() const {
        return (flags & SYM_FORWARD_REF) != 0;
    }

    /**
     * @brief Check if symbol is unreferenced
     * @return bool True if SYM_UNREFERENCED flag set
     */
    bool is_unreferenced() const {
        return (flags & SYM_UNREFERENCED) != 0;
    }
};

/**
 * @brief Symbol table for assembler
 *
 * Hash-based symbol storage and lookup. Provides symbol definition,
 * value updates, flag manipulation, and sorted iteration.
 */
class SymbolTable {
  public:
    /**
     * @brief Reset symbol table (clear all symbols)
     */
    void reset();

    // Symbol definition and lookup

    /**
     * @brief Define a new symbol
     * @param name Symbol name
     * @param value Symbol value
     * @param flags Symbol flags (default 0)
     * @param line_num Line where defined (default 0)
     */
    void define(const std::string &name, uint16_t value, uint8_t flags = 0, int line_num = 0);

    /**
     * @brief Update symbol value
     * @param name Symbol name
     * @param value New value
     */
    void update_value(const std::string &name, uint16_t value);

    /**
     * @brief Update symbol flags
     * @param name Symbol name
     * @param flags New flags
     */
    void update_flags(const std::string &name, uint8_t flags);

    /**
     * @brief Mark symbol as referenced (clear SYM_UNREFERENCED)
     * @param name Symbol name
     */
    void mark_referenced(const std::string &name);

    /**
     * @brief Look up symbol (mutable)
     * @param name Symbol name
     * @return Symbol* Pointer to symbol or nullptr
     */
    Symbol *lookup(const std::string &name);

    /**
     * @brief Look up symbol (const)
     * @param name Symbol name
     * @return const Symbol* Pointer to symbol or nullptr
     */
    const Symbol *lookup(const std::string &name) const;

    /**
     * @brief Get symbol value
     * @param name Symbol name
     * @return std::optional<uint16_t> Value if defined, nullopt otherwise
     */
    std::optional<uint16_t> get_value(const std::string &name) const;

    /**
     * @brief Check if symbol is defined
     * @param name Symbol name
     * @return bool True if symbol exists in table
     */
    bool is_defined(const std::string &name) const;

    // Symbol table inspection

    /**
     * @brief Get all symbols as vector
     * @return std::vector<Symbol> All symbols
     */
    std::vector<Symbol> all_symbols() const;

    /**
     * @brief Get symbols sorted by name
     * @return std::vector<Symbol> Sorted symbol list
     */
    std::vector<Symbol> sorted_by_name() const;

    /**
     * @brief Get symbols sorted by value
     * @return std::vector<Symbol> Sorted symbol list
     */
    std::vector<Symbol> sorted_by_value() const;

    /**
     * @brief Get underlying symbol map
     * @return const std::unordered_map<std::string, Symbol>& Symbol map
     */
    const std::unordered_map<std::string, Symbol> &get_all() const {
        return table_;
    }

    /**
     * @brief Get number of symbols in table
     * @return size_t Symbol count
     */
    size_t size() const {
        return table_.size();
    }

  private:
    std::unordered_map<std::string, Symbol> table_; ///< Hash-based symbol storage
};

} // namespace edasm
