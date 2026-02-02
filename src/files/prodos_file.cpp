// ProDOS file type mapping implementation
//
// This file implements ProDOS file type handling from EDASM.SRC/COMMONEQUS.S
// Reference: COMMONEQUS.S - ProDOS file type definitions
//
// ProDOS file types from COMMONEQUS.S:
//   TXT = $04  Text file
//   BIN = $06  Binary file
//   REL = $FE  Relocatable object file
//   SYS = $FF  System file
//
// This C++ implementation maps ProDOS file types to Linux file extensions
// and provides utilities for file type detection and conversion.
#include "edasm/files/prodos_file.hpp"

#include <algorithm>

namespace edasm {

namespace {
std::string to_lower(std::string_view text) {
    std::string out{text};
    std::transform(out.begin(), out.end(), out.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return out;
}
} // namespace

std::string extension_for_type(ProdosFileType type) {
    switch (type) {
    case ProdosFileType::Source:
        return ".src"; // EDASM source text
    case ProdosFileType::Object:
        return ".obj"; // binary object output
    case ProdosFileType::System:
        return ".sys"; // ProDOS SYS analog
    case ProdosFileType::Listing:
        return ".lst"; // assembler listing
    case ProdosFileType::Text:
        return ".txt";
    case ProdosFileType::Binary:
        return ".bin";
    case ProdosFileType::Unknown:
    default:
        return ".bin";
    }
}

ProdosFileType type_from_extension(std::string_view ext) {
    auto lower = to_lower(ext);
    if (lower == ".src") {
        return ProdosFileType::Source;
    }
    if (lower == ".obj") {
        return ProdosFileType::Object;
    }
    if (lower == ".sys") {
        return ProdosFileType::System;
    }
    if (lower == ".lst") {
        return ProdosFileType::Listing;
    }
    if (lower == ".txt") {
        return ProdosFileType::Text;
    }
    if (lower == ".bin") {
        return ProdosFileType::Binary;
    }
    return ProdosFileType::Unknown;
}

std::string ProdosFileName::to_linux_name() const {
    return stem + extension_for_type(type);
}

ProdosFileName parse_linux_name(const std::string &name) {
    ProdosFileName out;
    auto dot = name.find_last_of('.');
    if (dot == std::string::npos) {
        out.stem = name;
        out.type = ProdosFileType::Unknown;
        return out;
    }
    out.stem = name.substr(0, dot);
    out.type = type_from_extension(name.substr(dot));
    return out;
}

uint8_t prodos_type_code(ProdosFileType type) {
    // ProDOS numeric file type codes (commonly used values)
    // TXT = $04, BIN = $06, REL = $FE, SYS = $FF
    switch (type) {
    case ProdosFileType::Source:
        return 0x04; // treat source as text
    case ProdosFileType::Listing:
    case ProdosFileType::Text:
        return 0x04; // TXT
    case ProdosFileType::Object:
        return 0xFE; // REL
    case ProdosFileType::System:
        return 0xFF; // SYS
    case ProdosFileType::Binary:
        return 0x06; // BIN
    case ProdosFileType::Unknown:
    default:
        return 0x06; // default to BIN
    }
}

} // namespace edasm
