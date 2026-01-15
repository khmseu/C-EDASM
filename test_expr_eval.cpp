#include <iostream>
#include "edasm/assembler/expression.hpp"
#include "edasm/assembler/symbol_table.hpp"

int main() {
    edasm::SymbolTable symbols;
    edasm::ExpressionEvaluator eval(symbols);
    
    // Test: $1000+$10
    std::string expr = "#$1000+$10";
    auto result = eval.evaluate(expr, 2);
    
    std::cout << "Expression: " << expr << std::endl;
    std::cout << "Success: " << result.success << std::endl;
    std::cout << "Value: $" << std::hex << result.value << std::endl;
    std::cout << "Error: " << result.error_message << std::endl;
    
    return 0;
}
