/**
 * @file prodos_file.hpp
 * @brief ProDOS filename parsing and mapping
 *
 * Parses Linux filenames to determine ProDOS file types and vice versa.
 * Handles extension-based type detection for compatibility with ProDOS
 * file semantics.
 */

#pragma once

#include <string>

#include "edasm/files/file_types.hpp"

namespace edasm {

/**
 * @brief Parsed ProDOS filename
 *
 * Separates stem (base name) from type (extension).
 */
struct ProdosFileName {
    std::string stem;                             ///< Base filename without extension
    ProdosFileType type{ProdosFileType::Unknown}; ///< File type from extension

    /**
     * @brief Convert to Linux filename with extension
     * @return std::string Linux filename with appropriate extension
     */
    std::string to_linux_name() const;
};

/**
 * @brief Parse Linux filename to extract type
 * @param name Linux filename (with or without path)
 * @return ProdosFileName Parsed filename components
 */
ProdosFileName parse_linux_name(const std::string &name);

} // namespace edasm
