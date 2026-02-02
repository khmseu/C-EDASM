#pragma once

#include <cstdint>
#include <string>
#include <string_view>

namespace edasm {

enum class ProdosFileType {
    Source,
    Object,
    System,
    Listing,
    Text,
    Binary,
    Unknown,
};

std::string extension_for_type(ProdosFileType type);
ProdosFileType type_from_extension(std::string_view ext);

// Returns the numeric ProDOS file type code for the given ProdosFileType
uint8_t prodos_type_code(ProdosFileType type);

} // namespace edasm
