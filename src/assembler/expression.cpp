#include "edasm/assembler/expression.hpp"
#include "edasm/assembler/symbol_table.hpp"

#include <cctype>
#include <algorithm>

namespace edasm {

ExpressionEvaluator::ExpressionEvaluator(const SymbolTable& symbols)
    : symbols_(symbols) {}

ExpressionResult ExpressionEvaluator::evaluate(const std::string& expr, int pass) {
    if (expr.empty()) {
        ExpressionResult result;
        result.success = false;
        result.error_message = "Empty expression";
        return result;
    }
    
    // For Phase 3, implement simple expression parsing
    // No operators yet, just:
    // - Hex literals ($xxxx)
    // - Decimal literals (nnnn)
    // - Binary literals (%nnnn)
    // - Symbols
    // - Immediate mode (#)
    
    return parse_simple(expr, pass);
}

ExpressionResult ExpressionEvaluator::parse_simple(const std::string& expr, int pass) {
    ExpressionResult result;
    
    // Trim whitespace
    std::string trimmed = expr;
    trimmed.erase(0, trimmed.find_first_not_of(" \t"));
    trimmed.erase(trimmed.find_last_not_of(" \t") + 1);
    
    if (trimmed.empty()) {
        result.error_message = "Empty expression";
        return result;
    }
    
    // Skip '#' for immediate mode
    size_t pos = 0;
    if (trimmed[pos] == '#') {
        pos++;
    }
    
    // Skip whitespace after #
    while (pos < trimmed.length() && (trimmed[pos] == ' ' || trimmed[pos] == '\t')) {
        pos++;
    }
    
    if (pos >= trimmed.length()) {
        result.error_message = "Invalid expression";
        return result;
    }
    
    std::string value_str = trimmed.substr(pos);
    
    // Try hex ($xxxx)
    if (value_str[0] == '$') {
        auto val = parse_hex(value_str.substr(1));
        if (val.has_value()) {
            result.success = true;
            result.value = val.value();
            return result;
        }
        result.error_message = "Invalid hex literal";
        return result;
    }
    
    // Try binary (%nnnn)
    if (value_str[0] == '%') {
        auto val = parse_binary(value_str.substr(1));
        if (val.has_value()) {
            result.success = true;
            result.value = val.value();
            return result;
        }
        result.error_message = "Invalid binary literal";
        return result;
    }
    
    // Try decimal (starts with digit)
    if (std::isdigit(static_cast<unsigned char>(value_str[0]))) {
        auto val = parse_decimal(value_str);
        if (val.has_value()) {
            result.success = true;
            result.value = val.value();
            return result;
        }
        result.error_message = "Invalid decimal literal";
        return result;
    }
    
    // Must be a symbol
    if (is_symbol(value_str)) {
        auto sym = symbols_.lookup(value_str);
        if (!sym) {
            // Symbol not found
            if (pass == 1) {
                // Pass 1: Forward reference is OK
                result.success = true;
                result.value = 0;  // Placeholder
                result.is_forward_ref = true;
            } else {
                // Pass 2: Undefined symbol is an error
                result.success = false;
                result.error_message = "Undefined symbol: " + value_str;
            }
            return result;
        }
        
        // Symbol found
        result.success = true;
        result.value = sym->value;
        result.is_relative = (sym->flags & SYM_RELATIVE) != 0;
        result.is_external = (sym->flags & SYM_EXTERNAL) != 0;
        return result;
    }
    
    result.error_message = "Invalid expression: " + value_str;
    return result;
}

std::optional<uint16_t> ExpressionEvaluator::parse_hex(const std::string& str) {
    if (str.empty()) {
        return std::nullopt;
    }
    
    uint16_t value = 0;
    for (char c : str) {
        if (!std::isxdigit(static_cast<unsigned char>(c))) {
            return std::nullopt;
        }
        value *= 16;
        if (c >= '0' && c <= '9') {
            value += c - '0';
        } else if (c >= 'A' && c <= 'F') {
            value += c - 'A' + 10;
        } else if (c >= 'a' && c <= 'f') {
            value += c - 'a' + 10;
        }
    }
    return value;
}

std::optional<uint16_t> ExpressionEvaluator::parse_decimal(const std::string& str) {
    if (str.empty()) {
        return std::nullopt;
    }
    
    uint16_t value = 0;
    for (char c : str) {
        if (!std::isdigit(static_cast<unsigned char>(c))) {
            return std::nullopt;
        }
        value = value * 10 + (c - '0');
    }
    return value;
}

std::optional<uint16_t> ExpressionEvaluator::parse_binary(const std::string& str) {
    if (str.empty()) {
        return std::nullopt;
    }
    
    uint16_t value = 0;
    for (char c : str) {
        if (c != '0' && c != '1') {
            return std::nullopt;
        }
        value = value * 2 + (c - '0');
    }
    return value;
}

bool ExpressionEvaluator::is_symbol(const std::string& str) {
    if (str.empty()) {
        return false;
    }
    
    // Symbol must start with letter or underscore
    char first = str[0];
    if (!std::isalpha(static_cast<unsigned char>(first)) && first != '_' && first != '@') {
        return false;
    }
    
    // Rest must be alphanumeric or underscore
    for (size_t i = 1; i < str.length(); ++i) {
        char c = str[i];
        if (!std::isalnum(static_cast<unsigned char>(c)) && c != '_' && c != '@') {
            return false;
        }
    }
    
    return true;
}

} // namespace edasm
