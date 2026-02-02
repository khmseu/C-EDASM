/**
 * @file tokenizer.hpp
 * @brief Source line tokenizer for 6502 assembly
 * 
 * Parses assembly source lines into components: label, mnemonic, operand, and comment.
 * Implements tokenization logic from ASM2.S.
 */

#pragma once

#include <string>

namespace edasm {

/**
 * @brief Tokenized source line representation
 * 
 * Represents a single line of 6502 assembly source broken into its components.
 * Based on ASM2.S tokenization logic.
 */
struct SourceLine {
    int line_number{0};     ///< Line number in source file
    std::string label;      ///< Optional label (symbol definition)
    std::string mnemonic;   ///< Instruction or directive
    std::string operand;    ///< Operand field (may contain expressions)
    std::string comment;    ///< Comment (after semicolon)
    std::string raw_line;   ///< Original line text

    /**
     * @brief Check if line has a label
     * @return bool True if label field is non-empty
     */
    bool has_label() const {
        return !label.empty();
    }
    
    /**
     * @brief Check if line has a mnemonic
     * @return bool True if mnemonic field is non-empty
     */
    bool has_mnemonic() const {
        return !mnemonic.empty();
    }
    
    /**
     * @brief Check if line has an operand
     * @return bool True if operand field is non-empty
     */
    bool has_operand() const {
        return !operand.empty();
    }
    
    /**
     * @brief Check if line is comment-only
     * @return bool True if line has no mnemonic or label
     */
    bool is_comment_only() const {
        return !has_mnemonic() && !has_label();
    }
};

/**
 * @brief Tokenizer for 6502 assembly source
 * 
 * Parses source lines into structured SourceLine objects.
 * Handles label detection, mnemonic extraction, operand parsing,
 * and comment separation.
 */
class Tokenizer {
  public:
    /**
     * @brief Parse a single line into components
     * @param line Source line text
     * @param line_number Line number for tracking
     * @return SourceLine Tokenized line structure
     */
    static SourceLine parse_line(const std::string &line, int line_number);

  private:
    /**
     * @brief Trim whitespace from string
     * @param str String to trim
     * @return std::string Trimmed string
     */
    static std::string trim(const std::string &str);
    
    /**
     * @brief Convert string to uppercase
     * @param str String to convert
     * @return std::string Uppercase string
     */
    static std::string to_upper(const std::string &str);
    
    /**
     * @brief Check if character is whitespace
     * @param c Character to check
     * @return bool True if whitespace
     */
    static bool is_whitespace(char c);
    
    /**
     * @brief Check if character can start a label
     * @param c Character to check
     * @return bool True if valid label start
     */
    static bool is_label_start(char c);
    
    /**
     * @brief Check if character can be in a label
     * @param c Character to check
     * @return bool True if valid label character
     */
    static bool is_label_char(char c);
};

} // namespace edasm
