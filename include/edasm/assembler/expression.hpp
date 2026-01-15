#pragma once

#include <string>
#include <cstdint>
#include <optional>

namespace edasm {

class SymbolTable;

// Expression evaluation result (from ASM2.S EvalExpr)
struct ExpressionResult {
    bool success{false};
    uint16_t value{0};
    bool is_relative{false};  // Relative expression (from RelExprF)
    bool is_external{false};  // External reference
    bool is_forward_ref{false};  // Forward reference (undefined symbol)
    std::string error_message;
};

// Expression evaluator for 6502 assembly operands
// Based on EvalExpr from ASM2.S (line 2561+)
class ExpressionEvaluator {
  public:
    explicit ExpressionEvaluator(const SymbolTable& symbols);
    
    // Evaluate an expression string
    // Returns value and flags (relative, external, forward ref)
    ExpressionResult evaluate(const std::string& expr, int pass);
    
  private:
    const SymbolTable& symbols_;
    
    // Parse numeric literals
    std::optional<uint16_t> parse_hex(const std::string& str);
    std::optional<uint16_t> parse_decimal(const std::string& str);
    std::optional<uint16_t> parse_binary(const std::string& str);
    
    // Check if string is a symbol
    bool is_symbol(const std::string& str);
    
    // Simple expression parsing (Phase 3 - no operators yet)
    ExpressionResult parse_simple(const std::string& expr, int pass);
    
    // Full expression parsing with operators (Phase 4)
    // TODO: Implement in Phase 4
    // - Addition/subtraction (+/-)
    // - Multiplication/division (*,/)
    // - Logical ops (AND &, OR |, XOR ^)
    // - Unary ops (< low byte, > high byte)
};

} // namespace edasm
