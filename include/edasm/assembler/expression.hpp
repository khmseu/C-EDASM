/**
 * @file expression.hpp
 * @brief Expression evaluation for 6502 assembly operands
 *
 * Implements EDASM-specific expression parser with operator precedence
 * from ASM2.S ($8561-$8829). Supports arithmetic, bitwise operations,
 * and byte extraction operators.
 *
 * Operators (EDASM-specific precedence):
 * - *, / (multiplication, division)
 * - +, - (addition, subtraction)
 * - <, > (low byte, high byte)
 * - ^, |, ! (bitwise AND, OR, XOR - NON-STANDARD!)
 *
 * Note: ^ is AND in EDASM, not XOR!
 *
 * Reference: ASM2.S EvalExpr ($8561), ASM3.S Operators table ($8829)
 */

#pragma once

#include <cstdint>
#include <optional>
#include <string>

namespace edasm {

class SymbolTable;

/**
 * @brief Result of expression evaluation
 *
 * Contains the computed value and metadata flags indicating
 * whether the expression involves relative, external, or forward-referenced symbols.
 */
struct ExpressionResult {
    bool success{false};        ///< True if evaluation succeeded
    uint16_t value{0};          ///< Computed value
    bool is_relative{false};    ///< True if relative expression (from RelExprF)
    bool is_external{false};    ///< True if external reference
    bool is_forward_ref{false}; ///< True if forward reference (undefined symbol)
    std::string error_message;  ///< Error description if success=false
};

/**
 * @brief Expression evaluator for 6502 assembly operands
 *
 * Parses and evaluates expressions with EDASM-specific operators.
 * Handles numeric literals (hex, decimal, binary), symbol references,
 * and complex expressions with operator precedence.
 *
 * Based on EvalExpr from ASM2.S (line 2561+)
 */
class ExpressionEvaluator {
  public:
    /**
     * @brief Construct a new Expression Evaluator
     * @param symbols Reference to symbol table for lookups
     */
    explicit ExpressionEvaluator(const SymbolTable &symbols);

    /**
     * @brief Evaluate an expression string
     * @param expr Expression to evaluate
     * @param pass Assembly pass (1 or 2)
     * @return ExpressionResult Value and metadata flags
     */
    ExpressionResult evaluate(const std::string &expr, int pass);

  private:
    const SymbolTable &symbols_; ///< Symbol table reference

    /**
     * @brief Parse hexadecimal literal (e.g., "$1234", "1234H")
     * @param str String to parse
     * @return std::optional<uint16_t> Parsed value or nullopt
     */
    std::optional<uint16_t> parse_hex(const std::string &str);

    /**
     * @brief Parse decimal literal
     * @param str String to parse
     * @return std::optional<uint16_t> Parsed value or nullopt
     */
    std::optional<uint16_t> parse_decimal(const std::string &str);

    /**
     * @brief Parse binary literal (e.g., "%10101010")
     * @param str String to parse
     * @return std::optional<uint16_t> Parsed value or nullopt
     */
    std::optional<uint16_t> parse_binary(const std::string &str);

    /**
     * @brief Check if string is a valid symbol name
     * @param str String to check
     * @return bool True if valid symbol
     */
    bool is_symbol(const std::string &str);

    /**
     * @brief Simple expression parsing (single term, no operators)
     * @param expr Expression string
     * @param pass Assembly pass
     * @return ExpressionResult Evaluation result
     */
    ExpressionResult parse_simple(const std::string &expr, int pass);

    /**
     * @brief Full expression parsing with operators
     * @param expr Expression string
     * @param pass Assembly pass
     * @return ExpressionResult Evaluation result
     */
    ExpressionResult parse_full(const std::string &expr, int pass);

    /**
     * @brief Parse a single term from expression
     * @param expr Expression string
     * @param pos Current position (modified)
     * @param pass Assembly pass
     * @return ExpressionResult Term value
     */
    ExpressionResult parse_term(const std::string &expr, size_t &pos, int pass);

    /**
     * @brief Apply binary operator to two operands
     * @param op Operator character
     * @param left Left operand
     * @param right Right operand
     * @return uint16_t Result value
     */
    uint16_t apply_operator(char op, uint16_t left, uint16_t right);

    /**
     * @brief Get operator precedence level
     * @param op Operator character
     * @return int Precedence (higher = tighter binding)
     */
    int get_precedence(char op);

    /**
     * @brief Check if character is an operator
     * @param c Character to check
     * @return bool True if operator
     */
    bool is_operator(char c);
};

} // namespace edasm
