#pragma once

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

} // namespace edasm
