#pragma once

#include <string>

#include "edasm/files/file_types.hpp"

namespace edasm {

struct ProdosFileName {
    std::string stem;
    ProdosFileType type{ProdosFileType::Unknown};

    std::string to_linux_name() const; // maps ProDOS type to extension
};

ProdosFileName parse_linux_name(const std::string &name);

} // namespace edasm
