/**
 * @file rel_file.hpp
 * @brief REL (Relocatable) file format parser and builder
 * 
 * Implements the EDASM REL file format with:
 * - CODE IMAGE: Machine code with 2-byte length header
 * - RLD (Relocation Dictionary): Locations needing relocation
 * - ESD (External Symbol Dictionary): Entry points and external refs
 * 
 * RLD entries describe:
 * - Absolute references (no relocation)
 * - Relative references (add module base)
 * - External references (resolve from other modules)
 * 
 * ESD entries define:
 * - Entry points (ENT directive): symbols exported to other modules
 * - External references (EXT directive): symbols imported from other modules
 * 
 * Reference: ASM3.S and LINKER/LINK.S from EDASM.SRC
 */

#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace edasm {

// REL (Relocatable) File Format
// Based on EDASM.SRC ASM3.S and LINKER/LINK.S
//
// File structure:
//   [CODE IMAGE with 2-byte length header]
//   [RLD - Relocation Dictionary]
//   [ESD - External Symbol Dictionary]

// RLD (Relocation Dictionary) Entry - 4 bytes
// Describes locations in code that need relocation
struct RLDEntry {
    uint8_t flags;      // RLD type/flags
    uint16_t address;   // Address in code to relocate (little-endian)
    uint8_t symbol_num; // Symbol number for external refs

    // RLD entry types (from LINK.S)
    static constexpr uint8_t TYPE_ABSOLUTE = 0x00;
    static constexpr uint8_t TYPE_RELATIVE = 0x01;
    static constexpr uint8_t TYPE_EXTERNAL = 0x02;

    // Serialize to bytes
    std::vector<uint8_t> to_bytes() const {
        return {flags, static_cast<uint8_t>(address & 0xFF), static_cast<uint8_t>(address >> 8),
                symbol_num};
    }

    // Deserialize from bytes
    static RLDEntry from_bytes(const uint8_t *data) {
        RLDEntry entry;
        entry.flags = data[0];
        entry.address = data[1] | (static_cast<uint16_t>(data[2]) << 8);
        entry.symbol_num = data[3];
        return entry;
    }
};

// ESD (External Symbol Dictionary) Entry - variable length
// Describes symbols that are entries (defined) or externals (referenced)
struct ESDEntry {
    uint8_t flags;      // Symbol type flags (ENTRY/EXTERN/etc)
    uint16_t address;   // Symbol value/address (little-endian)
    std::string name;   // Symbol name (p-string format in file)
    uint8_t symbol_num; // Symbol number (for EXTERN references)

    // Symbol flag bits (from COMMONEQUS.S)
    static constexpr uint8_t FLAG_UNDEFINED = 0x80;
    static constexpr uint8_t FLAG_UNREFERENCED = 0x40;
    static constexpr uint8_t FLAG_RELATIVE = 0x20;
    static constexpr uint8_t FLAG_EXTERNAL = 0x10;
    static constexpr uint8_t FLAG_ENTRY = 0x08;
    static constexpr uint8_t FLAG_MACRO = 0x04;
    static constexpr uint8_t FLAG_NOSUCHLABEL = 0x02;
    static constexpr uint8_t FLAG_FORWARD_REF = 0x01;

    bool is_entry() const {
        return (flags & FLAG_ENTRY) != 0;
    }
    bool is_external() const {
        return (flags & FLAG_EXTERNAL) != 0;
    }
    bool is_relative() const {
        return (flags & FLAG_RELATIVE) != 0;
    }
    bool is_undefined() const {
        return (flags & FLAG_UNDEFINED) != 0;
    }

    // Serialize to bytes (p-string format)
    std::vector<uint8_t> to_bytes() const {
        std::vector<uint8_t> bytes;
        bytes.push_back(flags);
        bytes.push_back(static_cast<uint8_t>(address & 0xFF));
        bytes.push_back(static_cast<uint8_t>(address >> 8));

        // P-string: length byte + string data
        bytes.push_back(static_cast<uint8_t>(name.length()));
        for (char c : name) {
            bytes.push_back(static_cast<uint8_t>(c));
        }

        return bytes;
    }

    // Deserialize from bytes (returns bytes consumed)
    static ESDEntry from_bytes(const uint8_t *data, size_t &bytes_read) {
        ESDEntry entry;
        entry.flags = data[0];
        entry.address = data[1] | (static_cast<uint16_t>(data[2]) << 8);
        uint8_t name_len = data[3];

        entry.name.reserve(name_len);
        for (int i = 0; i < name_len; ++i) {
            entry.name += static_cast<char>(data[4 + i]);
        }

        bytes_read = 4 + name_len;
        return entry;
    }
};

// REL File Builder
// Collects RLD and ESD entries during assembly and generates REL file format
class RELFileBuilder {
  public:
    RELFileBuilder() = default;

    // Add relocation entry (called when code needs relocation)
    void add_rld_entry(uint16_t address, uint8_t flags, uint8_t symbol_num = 0) {
        RLDEntry entry;
        entry.address = address;
        entry.flags = flags;
        entry.symbol_num = symbol_num;
        rld_entries_.push_back(entry);
    }

    // Add external symbol dictionary entry
    void add_esd_entry(const std::string &name, uint16_t address, uint8_t flags,
                       uint8_t symbol_num = 0) {
        ESDEntry entry;
        entry.name = name;
        entry.address = address;
        entry.flags = flags;
        entry.symbol_num = symbol_num;
        esd_entries_.push_back(entry);
    }

    // Build complete REL file format: [length header][code][RLD][ESD]
    std::vector<uint8_t> build(const std::vector<uint8_t> &code) const {
        std::vector<uint8_t> rel_file;

        // Code image with 2-byte length header (little-endian)
        uint16_t code_len = static_cast<uint16_t>(code.size());
        rel_file.push_back(static_cast<uint8_t>(code_len & 0xFF));
        rel_file.push_back(static_cast<uint8_t>(code_len >> 8));

        // Code image
        rel_file.insert(rel_file.end(), code.begin(), code.end());

        // RLD entries (4 bytes each)
        for (const auto &entry : rld_entries_) {
            auto bytes = entry.to_bytes();
            rel_file.insert(rel_file.end(), bytes.begin(), bytes.end());
        }

        // RLD terminator (0x00)
        rel_file.push_back(0x00);

        // ESD entries (variable length)
        for (const auto &entry : esd_entries_) {
            auto bytes = entry.to_bytes();
            rel_file.insert(rel_file.end(), bytes.begin(), bytes.end());
        }

        // ESD terminator (0x00)
        rel_file.push_back(0x00);

        return rel_file;
    }

    // Parse REL file format
    static bool parse(const std::vector<uint8_t> &data, std::vector<uint8_t> &code,
                      std::vector<RLDEntry> &rld_entries, std::vector<ESDEntry> &esd_entries) {
        if (data.size() < 2)
            return false;

        // Read code length
        uint16_t code_len = data[0] | (static_cast<uint16_t>(data[1]) << 8);
        if (data.size() < 2 + code_len)
            return false;

        // Extract code
        code.assign(data.begin() + 2, data.begin() + 2 + code_len);

        // Parse RLD entries
        size_t pos = 2 + code_len;
        while (pos + 4 <= data.size()) {
            if (data[pos] == 0x00) {
                // RLD terminator
                pos++;
                break;
            }

            RLDEntry entry = RLDEntry::from_bytes(&data[pos]);
            rld_entries.push_back(entry);
            pos += 4;
        }

        // Parse ESD entries
        while (pos < data.size()) {
            if (data[pos] == 0x00) {
                // ESD terminator
                break;
            }

            size_t bytes_read = 0;
            ESDEntry entry = ESDEntry::from_bytes(&data[pos], bytes_read);
            esd_entries.push_back(entry);
            pos += bytes_read;
        }

        return true;
    }

    void reset() {
        rld_entries_.clear();
        esd_entries_.clear();
    }

    const std::vector<RLDEntry> &rld_entries() const {
        return rld_entries_;
    }
    const std::vector<ESDEntry> &esd_entries() const {
        return esd_entries_;
    }

  private:
    std::vector<RLDEntry> rld_entries_;
    std::vector<ESDEntry> esd_entries_;
};

} // namespace edasm
