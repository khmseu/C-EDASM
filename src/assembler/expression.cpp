/**
 * @file expression.cpp
 * @brief Expression evaluation implementation for EDASM assembler
 * 
 * Implements expression parsing and evaluation from EDASM.SRC/ASM/.
 * 
 * Primary reference: ASM2.S ($8561-$8829) and ASM3.S
 * 
 * Key routines from ASM2.S:
 * - EvalExpr ($8561): Main expression evaluator -> evaluate()
 * - EvalTerm ($8724): Parse terms (constants, identifiers) -> parse_simple()
 * - EvalSExpr ($8662): Evaluate sub-expressions -> parse_full()
 * - ExprADD/SUB/MUL/DIV/AND/EOR/ORA ($8787-$8829): Binary operators
 * 
 * Expression operators from ASM3.S Operators table ($8829):
 * - + (addition), - (subtraction), * (multiplication), / (division)
 * - & (bitwise AND), | (bitwise OR), ^ (bitwise XOR), ! (bitwise NOT)
 * - < (low byte), > (high byte)
 * 
 * Original EDASM uses recursive descent parser with operator precedence.
 * This C++ implementation follows the same approach with modern syntax.
 */

#include "edasm/assembler/expression.hpp"
#include "edasm/assembler/symbol_table.hpp"

#include <cctype>

namespace edasm {

ExpressionEvaluator::ExpressionEvaluator(const SymbolTable &symbols) : symbols_(symbols) {}

// Main expression evaluation entry point
// Reference: ASM2.S EvalExpr ($8561) - Recursive descent parser
ExpressionResult ExpressionEvaluator::evaluate(const std::string &expr, int pass) {
    if (expr.empty()) {
        ExpressionResult result;
        result.success = false;
        result.error_message = "Empty expression";
        return result;
    }

    // Check if expression contains operators
    // Reference: ASM2.S EvalExpr ($8561+) - Detects operator presence
    // Need to be careful with '-' which could be unary or binary
    // If it does, use full parser, otherwise use simple parser
    bool has_operators = false;
    size_t check_pos = 0;

    // Skip leading # and whitespace (immediate mode indicator)
    if (expr[check_pos] == '#') {
        check_pos++;
        while (check_pos < expr.length() && (expr[check_pos] == ' ' || expr[check_pos] == '\t')) {
            check_pos++;
        }
    }

    // Skip < or > byte operators - these require full parser
    // Reference: ASM3.S - < and > extract low/high bytes
    if (check_pos < expr.length() && (expr[check_pos] == '<' || expr[check_pos] == '>')) {
        has_operators = true; // Byte operators need full parser
        check_pos++;
    }

    // Skip leading unary +/-
    if (check_pos < expr.length() && (expr[check_pos] == '+' || expr[check_pos] == '-')) {
        check_pos++;
    }

    // Now check for binary operators
    // Reference: ASM3.S Operators table - +, -, *, /, &, |, ^, !, (, )
    for (size_t i = check_pos; i < expr.length(); i++) {
        char c = expr[i];
        if (c == '+' || c == '*' || c == '/' || c == '&' || c == '|' || c == '^' || c == '!' ||
            c == '(' || c == ')') {
            has_operators = true;
            break;
        }
        // Check for binary minus (not at start of term)
        if (c == '-' && i > check_pos &&
            (std::isalnum(expr[i - 1]) || expr[i - 1] == ')' || expr[i - 1] == '$')) {
            has_operators = true;
            break;
        }
    }

    if (has_operators) {
        return parse_full(expr, pass);
    } else {
        return parse_simple(expr, pass);
    }
}

// Simple expression parser for constants and single symbols
// Reference: ASM2.S EvalTerm ($8724) - Parse simple terms
// Handles: $hex, %binary, decimal, and symbol names
ExpressionResult ExpressionEvaluator::parse_simple(const std::string &expr, int pass) {
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
                result.value = 0; // Placeholder
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

std::optional<uint16_t> ExpressionEvaluator::parse_hex(const std::string &str) {
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

std::optional<uint16_t> ExpressionEvaluator::parse_decimal(const std::string &str) {
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

std::optional<uint16_t> ExpressionEvaluator::parse_binary(const std::string &str) {
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

bool ExpressionEvaluator::is_symbol(const std::string &str) {
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

// Full expression parser with operator support (from ASM2.S EvalExpr line 2561+)
// Implements operators: +, -, *, /, &, |, ^
// Also handles: < (low byte), > (high byte), unary -/+
ExpressionResult ExpressionEvaluator::parse_full(const std::string &expr, int pass) {
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
        while (pos < trimmed.length() && (trimmed[pos] == ' ' || trimmed[pos] == '\t')) {
            pos++;
        }
    }

    // Check for byte extraction operators (< for low byte, > for high byte)
    // From ASM2.S line 2574-2583
    bool low_byte = false;
    bool high_byte = false;

    if (pos < trimmed.length() && trimmed[pos] == '<') {
        low_byte = true;
        pos++;
    } else if (pos < trimmed.length() && trimmed[pos] == '>') {
        high_byte = true;
        pos++;
    }

    // Skip whitespace after byte operator
    while (pos < trimmed.length() && (trimmed[pos] == ' ' || trimmed[pos] == '\t')) {
        pos++;
    }

    // Check for unary +/- (from ASM2.S line 2585-2593)
    bool unary_minus = false;
    if (pos < trimmed.length() && trimmed[pos] == '-') {
        unary_minus = true;
        pos++;
    } else if (pos < trimmed.length() && trimmed[pos] == '+') {
        pos++; // Skip unary plus
    }

    // Parse the expression using simple precedence climbing
    // This implements the operator precedence from ASM2.S Operators table (line 3029+):
    // Operators: + - * / ! ^ |
    // In EDASM: ! is XOR (EOR), ^ is AND, | is OR

    result = parse_term(trimmed, pos, pass);
    if (!result.success) {
        return result;
    }

    uint16_t value = result.value;

    // Process binary operators
    while (pos < trimmed.length()) {
        // Skip whitespace
        while (pos < trimmed.length() && (trimmed[pos] == ' ' || trimmed[pos] == '\t')) {
            pos++;
        }

        if (pos >= trimmed.length()) {
            break;
        }

        char op = trimmed[pos];
        if (!is_operator(op)) {
            break; // No more operators
        }

        pos++; // Skip operator

        // Parse right-hand term
        ExpressionResult rhs = parse_term(trimmed, pos, pass);
        if (!rhs.success) {
            return rhs;
        }

        // Apply operator
        value = apply_operator(op, value, rhs.value);
    }

    // Apply unary minus if needed
    if (unary_minus) {
        value = static_cast<uint16_t>(-static_cast<int16_t>(value));
    }

    // Apply byte extraction if needed (from ASM2.S line 2638-2648)
    if (low_byte) {
        value = value & 0xFF;
    } else if (high_byte) {
        value = (value >> 8) & 0xFF;
    }

    result.success = true;
    result.value = value;
    return result;
}

// Parse a single term (number, symbol, or parenthesized expression)
ExpressionResult ExpressionEvaluator::parse_term(const std::string &expr, size_t &pos, int pass) {
    ExpressionResult result;

    // Skip whitespace
    while (pos < expr.length() && (expr[pos] == ' ' || expr[pos] == '\t')) {
        pos++;
    }

    if (pos >= expr.length()) {
        result.error_message = "Unexpected end of expression";
        return result;
    }

    // Handle parenthesized expressions
    if (expr[pos] == '(') {
        pos++; // Skip '('
        result = parse_full(expr.substr(pos), pass);
        if (!result.success) {
            return result;
        }
        // Find matching ')'
        int paren_count = 1;
        while (pos < expr.length() && paren_count > 0) {
            if (expr[pos] == '(')
                paren_count++;
            if (expr[pos] == ')')
                paren_count--;
            pos++;
        }
        return result;
    }

    // Extract the term (up to next operator or end)
    size_t term_start = pos;
    while (pos < expr.length()) {
        char c = expr[pos];
        if (c == ' ' || c == '\t' || is_operator(c) || c == ')') {
            break;
        }
        pos++;
    }

    std::string term = expr.substr(term_start, pos - term_start);

    // Parse the term as a simple expression (number or symbol)
    // Reuse the simple parser logic
    if (term.empty()) {
        result.error_message = "Empty term";
        return result;
    }

    // Try hex ($xxxx)
    if (term[0] == '$') {
        auto val = parse_hex(term.substr(1));
        if (val.has_value()) {
            result.success = true;
            result.value = val.value();
            return result;
        }
        result.error_message = "Invalid hex literal";
        return result;
    }

    // Try binary (%nnnn)
    if (term[0] == '%') {
        auto val = parse_binary(term.substr(1));
        if (val.has_value()) {
            result.success = true;
            result.value = val.value();
            return result;
        }
        result.error_message = "Invalid binary literal";
        return result;
    }

    // Try decimal (starts with digit)
    if (std::isdigit(static_cast<unsigned char>(term[0]))) {
        auto val = parse_decimal(term);
        if (val.has_value()) {
            result.success = true;
            result.value = val.value();
            return result;
        }
        result.error_message = "Invalid decimal literal";
        return result;
    }

    // Must be a symbol
    if (is_symbol(term)) {
        auto sym = symbols_.lookup(term);
        if (!sym) {
            // Symbol not found
            if (pass == 1) {
                // Pass 1: Forward reference is OK
                result.success = true;
                result.value = 0; // Placeholder
                result.is_forward_ref = true;
            } else {
                // Pass 2: Undefined symbol is an error
                result.success = false;
                result.error_message = "Undefined symbol: " + term;
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

    result.error_message = "Invalid term: " + term;
    return result;
}

// Apply binary operator (from ASM2.S line 3029+)
// Operators: + - * / ! ^ |
// ! = XOR (EOR), ^ = AND, | = OR
uint16_t ExpressionEvaluator::apply_operator(char op, uint16_t left, uint16_t right) {
    switch (op) {
    case '+':
        return left + right;
    case '-':
        return left - right;
    case '*':
        return left * right;
    case '/':
        return (right != 0) ? left / right : 0; // Avoid division by zero
    case '!':
        return left ^ right; // XOR (EOR in EDASM)
    case '^':
        return left & right; // AND (in EDASM)
    case '|':
        return left | right; // OR
    default:
        return left; // Unknown operator, return left unchanged
    }
}

// Get operator precedence (higher = evaluated first)
// This is simplified; EDASM evaluates left-to-right for same precedence
int ExpressionEvaluator::get_precedence(char op) {
    switch (op) {
    case '*':
    case '/':
        return 3; // Highest
    case '+':
    case '-':
        return 2;
    case '&': // ^ in EDASM
    case '^': // ! in EDASM
    case '|':
        return 1; // Lowest
    default:
        return 0;
    }
}

// Check if character is a binary operator
bool ExpressionEvaluator::is_operator(char c) {
    return c == '+' || c == '-' || c == '*' || c == '/' || c == '&' || c == '|' || c == '^' ||
           c == '!';
}

} // namespace edasm
