#pragma once

#include <optional>
#include <string>

namespace edasm {

// Tokenized source line (from ASM2.S tokenization logic)
struct SourceLine {
    int line_number{0};
    std::string label;    // Optional label (symbol definition)
    std::string mnemonic; // Instruction or directive
    std::string operand;  // Operand field (may contain expressions)
    std::string comment;  // Comment (after semicolon)
    std::string raw_line; // Original line text

    bool has_label() const {
        return !label.empty();
    }
    bool has_mnemonic() const {
        return !mnemonic.empty();
    }
    bool has_operand() const {
        return !operand.empty();
    }
    bool is_comment_only() const {
        return !has_mnemonic() && !has_label();
    }
};

// Tokenizer for 6502 assembly source
class Tokenizer {
  public:
    // Parse a single line into components
    static SourceLine parse_line(const std::string &line, int line_number);

  private:
    // Helper functions
    static std::string trim(const std::string &str);
    static std::string to_upper(const std::string &str);
    static bool is_whitespace(char c);
    static bool is_label_start(char c);
    static bool is_label_char(char c);
};

} // namespace edasm
