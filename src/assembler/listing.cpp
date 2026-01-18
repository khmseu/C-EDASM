// Listing generation implementation for EDASM assembler
//
// This file implements assembly listing output from EDASM.SRC/ASM/
// Primary reference: ASM1.S DoPass3 ($D000) - Symbol table printing and listing
//
// Key routines from ASM1.S:
//   - DoPass3 ($D000): Entry point for Pass 3 - symbol table listing
//   - DoSort ($D1D6): Shell sort for symbol table (alphabetic or by address)
//   - PrSymTbl ($D2D8): Print sorted symbols with addresses and references
//
// Listing format features from original EDASM:
//   - Line number, address, object code bytes, source line
//   - Symbol table with 2, 4, or 6 column layout
//   - Optional symbol sorting by name or address
//   - Reference markers: * (undefined), ? (unreferenced), X (external), N (entry)
//
// This C++ implementation preserves EDASM listing format while using modern
// I/O streams and string formatting.
#include "edasm/assembler/listing.hpp"

#include <algorithm>
#include <iomanip>
#include <sstream>

namespace edasm {

ListingGenerator::ListingGenerator(const Options &opts) : options_(opts) {}

void ListingGenerator::add_line(const ListingLine &line) {
    lines_.push_back(line);
}

void ListingGenerator::set_symbol_table(const SymbolTable &symbols) {
    symbols_ = &symbols;
}

bool ListingGenerator::write_to_file(const std::string &filename) {
    std::ofstream file(filename);
    if (!file) {
        return false;
    }

    file << to_string();
    return file.good();
}

std::string ListingGenerator::to_string() const {
    std::ostringstream oss;

    // Write header
    oss << "Line# Addr  Bytes        Source\n";
    oss << "----- ----  ----------   ---------------------------\n";

    // Write each listing line
    for (const auto &line : lines_) {
        oss << format_listing_line(line) << "\n";
    }

    // Write symbol table if requested
    if (options_.include_symbols && symbols_) {
        oss << "\n" << generate_symbol_table();
    }

    return oss.str();
}

std::string ListingGenerator::format_listing_line(const ListingLine &line) const {
    std::ostringstream oss;

    // Line number (5 chars)
    oss << format_line_number(line.line_number) << "  ";

    if (line.has_address) {
        // Address (4 chars)
        oss << format_address(line.address) << "  ";

        // Bytes (12 chars) - show up to 3 bytes on first line
        oss << std::left << std::setw(12) << format_bytes(line.bytes, 3) << " ";
    } else {
        // No address/bytes - just spaces
        oss << "      " << std::string(12, ' ') << " ";
    }

    // Source line
    oss << line.source_line;

    // If more than 3 bytes, continue on next lines
    if (line.bytes.size() > 3) {
        for (size_t i = 3; i < line.bytes.size(); i += 3) {
            oss << "\n";
            oss << "      " << format_address(line.address + i) << "  ";

            // Get next chunk of bytes
            std::vector<uint8_t> chunk;
            for (size_t j = i; j < i + 3 && j < line.bytes.size(); ++j) {
                chunk.push_back(line.bytes[j]);
            }
            oss << std::left << std::setw(12) << format_bytes(chunk, 3);
        }
    }

    return oss.str();
}

std::string ListingGenerator::format_line_number(int line_num) const {
    std::ostringstream oss;

    if (options_.line_numbers_bcd) {
        // BCD format: convert to BCD (from ASM2.S)
        // This is a simplified version - full BCD would be more complex
        oss << std::setw(4) << std::setfill('0') << line_num;
    } else {
        // Decimal format
        oss << std::setw(4) << std::setfill('0') << line_num;
    }

    return oss.str();
}

std::string ListingGenerator::format_address(uint16_t addr) const {
    std::ostringstream oss;
    oss << std::hex << std::uppercase << std::setw(4) << std::setfill('0') << addr;
    return oss.str();
}

std::string ListingGenerator::format_bytes(const std::vector<uint8_t> &bytes,
                                           size_t max_bytes) const {
    std::ostringstream oss;

    size_t count = std::min(bytes.size(), max_bytes);
    for (size_t i = 0; i < count; ++i) {
        if (i > 0)
            oss << " ";
        oss << std::hex << std::uppercase << std::setw(2) << std::setfill('0')
            << static_cast<int>(bytes[i]);
    }

    return oss.str();
}

std::string ListingGenerator::generate_symbol_table() const {
    std::ostringstream oss;

    // Get symbols sorted by name or value
    std::vector<Symbol> symbols;
    if (options_.sort_by_value) {
        symbols = symbols_->sorted_by_value();
    } else {
        symbols = symbols_->sorted_by_name();
    }

    if (symbols.empty()) {
        return "";
    }

    // Write symbol table header
    oss << "Symbol Table";
    if (options_.sort_by_value) {
        oss << " (by value)";
    } else {
        oss << " (by name)";
    }
    oss << ":\n";
    oss << std::string(60, '=') << "\n";

    // Format symbols in columns
    oss << format_symbols_in_columns(symbols, options_.symbol_columns);

    return oss.str();
}

std::string ListingGenerator::format_symbols_in_columns(const std::vector<Symbol> &symbols,
                                                        int columns) const {

    std::ostringstream oss;

    // Calculate rows needed
    int rows = (symbols.size() + columns - 1) / columns;

    // Column width: name (17) + "$" + value (4) + " " + flags (1-4) = ~27
    const int col_width = 27;

    for (int row = 0; row < rows; ++row) {
        for (int col = 0; col < columns; ++col) {
            int idx = row + col * rows;
            if (idx < static_cast<int>(symbols.size())) {
                std::string formatted = format_symbol(symbols[idx]);
                oss << std::left << std::setw(col_width) << formatted;
            }
        }
        oss << "\n";
    }

    return oss.str();
}

std::string ListingGenerator::format_symbol(const Symbol &sym) const {
    std::ostringstream oss;

    // Symbol name (left-justified, 17 chars including space after)
    oss << std::left << std::setw(17) << sym.name;

    // Value (hex with $ prefix, right-justified hex digits)
    oss << "$";
    // Format the value as 4-digit hex (reset alignment)
    oss << std::right << std::hex << std::uppercase << std::setw(4) << std::setfill('0')
        << sym.value;

    // Reset formatting for flags
    oss << std::dec << std::setfill(' ');

    // Flags
    std::string flags;
    if (sym.is_relative())
        flags += "R";
    if (sym.is_external())
        flags += "X";
    if (sym.is_entry())
        flags += "E";
    if (sym.is_undefined())
        flags += "U";

    if (!flags.empty()) {
        oss << " " << flags;
    }

    return oss.str();
}

} // namespace edasm
