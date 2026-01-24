#include "edasm/assembler/tokenizer.hpp"

#include <algorithm>
#include <cctype>

namespace edasm {

SourceLine Tokenizer::parse_line(const std::string &line, int line_number) {
    SourceLine result;
    result.line_number = line_number;
    result.raw_line = line;

    // Check for comment-only line (starts with * or ;)
    if (line.empty() || line[0] == '*' || line[0] == ';') {
        result.comment = line;
        return result;
    }

    size_t pos = 0;
    const size_t len = line.length();

    // Parse label (if present)
    // Label starts in column 0 (no leading whitespace) and ends with whitespace or colon
    if (pos < len && !is_whitespace(line[pos]) && is_label_start(line[pos])) {
        size_t label_end = pos;
        while (label_end < len && is_label_char(line[label_end])) {
            label_end++;
        }
        result.label = line.substr(pos, label_end - pos);
        pos = label_end;

        // Skip optional colon after label
        if (pos < len && line[pos] == ':') {
            pos++;
        }
    }

    // Skip whitespace before mnemonic
    while (pos < len && is_whitespace(line[pos])) {
        pos++;
    }

    // Parse mnemonic (instruction or directive)
    if (pos < len && !is_whitespace(line[pos]) && line[pos] != ';') {
        size_t mnem_end = pos;
        while (mnem_end < len && !is_whitespace(line[mnem_end]) && line[mnem_end] != ';') {
            mnem_end++;
        }
        result.mnemonic = to_upper(line.substr(pos, mnem_end - pos));
        pos = mnem_end;
    }

    // Skip whitespace before operand
    while (pos < len && is_whitespace(line[pos])) {
        pos++;
    }

    // Parse operand (everything up to comment or end of line)
    if (pos < len && line[pos] != ';') {
        size_t operand_end = pos;
        // Find comment or end of line
        while (operand_end < len && line[operand_end] != ';') {
            operand_end++;
        }
        // Trim trailing whitespace from operand
        result.operand = trim(line.substr(pos, operand_end - pos));
        pos = operand_end;
    }

    // Parse comment (if present)
    if (pos < len && line[pos] == ';') {
        result.comment = line.substr(pos);
    }

    return result;
}

std::string Tokenizer::trim(const std::string &str) {
    const char *whitespace = " \t\r\n";
    size_t start = str.find_first_not_of(whitespace);
    if (start == std::string::npos) {
        return "";
    }
    size_t end = str.find_last_not_of(whitespace);
    return str.substr(start, end - start + 1);
}

std::string Tokenizer::to_upper(const std::string &str) {
    std::string result = str;
    std::transform(result.begin(), result.end(), result.begin(),
                   [](unsigned char c) { return std::toupper(c); });
    return result;
}

bool Tokenizer::is_whitespace(char c) {
    return c == ' ' || c == '\t';
}

bool Tokenizer::is_label_start(char c) {
    return std::isalpha(static_cast<unsigned char>(c)) || c == '_' || c == '@';
}

bool Tokenizer::is_label_char(char c) {
    return std::isalnum(static_cast<unsigned char>(c)) || c == '_' || c == '@';
}

} // namespace edasm
