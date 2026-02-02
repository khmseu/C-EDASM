/**
 * @file file_types.hpp
 * @brief ProDOS file type mappings
 * 
 * Maps ProDOS file types to Linux file extensions and vice versa.
 * Supports standard EDASM file types: source, object, listing, etc.
 * 
 * Reference: docs/Apple ProDOS 8 Technical Reference Manual.txt
 */

#pragma once

#include <string>
#include <string_view>

namespace edasm {

/**
 * @brief ProDOS file type enumeration
 * 
 * Common file types used in EDASM development.
 */
enum class ProdosFileType {
    Source,  ///< Source code (.src, .txt) - TXT ($04)
    Object,  ///< Object code (.obj, .bin) - BIN ($06)
    System,  ///< System file (.sys) - SYS ($FF)
    Listing, ///< Listing file (.lst) - TXT ($04)
    Text,    ///< Generic text (.txt) - TXT ($04)
    Binary,  ///< Generic binary (.bin) - BIN ($06)
    Unknown, ///< Unknown/unsupported type
};

/**
 * @brief Get file extension for ProDOS file type
 * @param type ProDOS file type
 * @return std::string File extension (e.g., ".src")
 */
std::string extension_for_type(ProdosFileType type);

/**
 * @brief Determine ProDOS file type from extension
 * @param ext File extension (with or without leading dot)
 * @return ProdosFileType File type
 */
ProdosFileType type_from_extension(std::string_view ext);

} // namespace edasm
