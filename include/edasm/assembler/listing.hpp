/**
 * @file listing.hpp
 * @brief Assembly listing file generator
 * 
 * Generates formatted listing output with addresses, machine code bytes,
 * source lines, and symbol table. Implements listing logic from ASM1.S
 * and ASM2.S.
 * 
 * Listing format:
 * - Line number, address, bytes, source text
 * - Symbol table at end (optional, in columns)
 * 
 * Reference: ASM1.S DoPass3, ASM2.S listing routines
 */

#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "edasm/assembler/symbol_table.hpp"

namespace edasm {

/**
 * @brief Listing file generator
 * 
 * Accumulates listing lines during assembly and generates formatted
 * output with optional symbol table.
 */
class ListingGenerator {
  public:
    /**
     * @brief Single line in the listing
     */
    struct ListingLine {
        int line_number{0};         ///< Source line number
        uint16_t address{0};        ///< Assembly address
        std::vector<uint8_t> bytes; ///< Generated machine code bytes
        std::string source_line;    ///< Original source text
        bool has_address{false};    ///< True if line generates code
    };

    /**
     * @brief Listing generation options
     */
    struct Options {
        bool include_symbols = true;   ///< Include symbol table at end
        bool sort_by_value = false;    ///< Sort symbols by value vs name
        int symbol_columns = 4;        ///< Symbol table columns (2, 4, or 6)
        bool line_numbers_bcd = false; ///< Use BCD format for line numbers
    };

    /**
     * @brief Construct listing generator with options
     * @param opts Listing options
     */
    ListingGenerator(const Options &opts);

    /**
     * @brief Add a line to the listing
     * @param line Listing line to add
     */
    void add_line(const ListingLine &line);

    /**
     * @brief Set symbol table for inclusion in listing
     * @param symbols Symbol table reference
     */
    void set_symbol_table(const SymbolTable &symbols);

    /**
     * @brief Write listing to file
     * @param filename Output file path
     * @return bool True if successful
     */
    bool write_to_file(const std::string &filename);

    /**
     * @brief Get listing as string
     * @return std::string Complete listing text
     */
    std::string to_string() const;

  private:
    Options options_;                   ///< Listing options
    std::vector<ListingLine> lines_;    ///< Accumulated listing lines
    const SymbolTable *symbols_{nullptr}; ///< Symbol table reference

    /**
     * @brief Format a single listing line
     * @param line Line to format
     * @return std::string Formatted line text
     */
    std::string format_listing_line(const ListingLine &line) const;

    /**
     * @brief Format line number (decimal or BCD)
     * @param line_num Line number
     * @return std::string Formatted line number
     */
    std::string format_line_number(int line_num) const;

    /**
     * @brief Format address as hex with $ prefix
     * @param addr Address value
     * @return std::string Formatted address
     */
    std::string format_address(uint16_t addr) const;

    /**
     * @brief Format bytes as hex dump
     * @param bytes Byte values
     * @param max_bytes Maximum bytes to show per line
     * @return std::string Formatted hex bytes
     */
    std::string format_bytes(const std::vector<uint8_t> &bytes, size_t max_bytes = 3) const;

    /**
     * @brief Generate symbol table section
     * @return std::string Formatted symbol table
     */
    std::string generate_symbol_table() const;

    /**
     * @brief Format symbols in columns
     * @param symbols Symbol list
     * @param columns Number of columns
     * @return std::string Formatted multi-column output
     */
    std::string format_symbols_in_columns(const std::vector<Symbol> &symbols, int columns) const;

    /**
     * @brief Format a single symbol entry
     * @param sym Symbol to format
     * @return std::string Formatted symbol (name=value flags)
     */
    std::string format_symbol(const Symbol &sym) const;
};

} // namespace edasm
