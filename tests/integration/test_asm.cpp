#include "edasm/assembler/assembler.hpp"
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>

int main(int argc, char **argv) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <source_file>" << std::endl;
        return 1;
    }

    // Read source file
    std::ifstream file(argv[1]);
    if (!file) {
        std::cerr << "Error: Cannot open file " << argv[1] << std::endl;
        return 1;
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string source = buffer.str();

    // Assemble
    edasm::Assembler assembler;
    auto result = assembler.assemble(source);

    // Report results
    std::cout << "Assembly ";
    if (result.success) {
        std::cout << "SUCCEEDED" << std::endl;
        std::cout << "Code size: " << result.code_length << " bytes" << std::endl;
        std::cout << "ORG: $" << std::hex << result.org_address << std::endl;

        // Print hex dump
        std::cout << "\nHex dump:" << std::endl;
        for (size_t i = 0; i < result.code.size(); ++i) {
            if (i % 16 == 0) {
                std::cout << std::hex << (result.org_address + i) << ": ";
            }
            std::cout << std::hex << std::setw(2) << std::setfill('0')
                      << static_cast<int>(result.code[i]) << " ";
            if ((i + 1) % 16 == 0 || i + 1 == result.code.size()) {
                std::cout << std::endl;
            }
        }
    } else {
        std::cout << "FAILED" << std::endl;
    }

    // Print errors
    if (!result.errors.empty()) {
        std::cout << "\nErrors:" << std::endl;
        for (const auto &err : result.errors) {
            std::cout << "  " << err << std::endl;
        }
    }

    // Print warnings
    if (!result.warnings.empty()) {
        std::cout << "\nWarnings:" << std::endl;
        for (const auto &warn : result.warnings) {
            std::cout << "  " << warn << std::endl;
        }
    }

    // Print symbol table
    auto symbols = assembler.symbols().sorted_by_name();
    if (!symbols.empty()) {
        std::cout << "\nSymbol Table:" << std::endl;
        for (const auto &sym : symbols) {
            std::cout << "  " << sym.name << " = $" << std::hex << sym.value;
            if (sym.is_relative())
                std::cout << " (REL)";
            if (sym.is_undefined())
                std::cout << " (UNDEF)";
            std::cout << std::endl;
        }
    }

    return result.success ? 0 : 1;
}
