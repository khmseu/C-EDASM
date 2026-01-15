#include <iostream>
#include "edasm/assembler/expression.hpp"
#include "edasm/assembler/symbol_table.hpp"

int main() {
    edasm::SymbolTable symbols;
    symbols.define("BASE", 0x1000, 0, 1);
    
    edasm::ExpressionEvaluator eval(symbols);
    
    // Test high byte extraction
    std::string expr = "#>BASE";
    auto result = eval.evaluate(expr, 2);
    
    std::cout << "Expression: " << expr << std::endl;
    std::cout << "Success: " << result.success << std::endl;
    std::cout << "Value: $" << std::hex << result.value << std::endl;
    std::cout << "Error: " << result.error_message << std::endl;
    
    return 0;
}
