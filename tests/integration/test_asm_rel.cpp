#include "edasm/assembler/assembler.hpp"
#include <fstream>
#include <iostream>
#include <sstream>

int main(int argc, char **argv) {
    if (argc < 3) {
        std::cerr << "Usage: " << argv[0] << " <source_file> <output_rel_file>" << std::endl;
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

        if (result.is_rel_file) {
            std::cout << "REL file format: " << result.rel_file_data.size() << " bytes total\n";

            // Write REL file
            std::ofstream out(argv[2], std::ios::binary);
            if (!out) {
                std::cerr << "Error: Cannot write output file " << argv[2] << std::endl;
                return 1;
            }

            out.write(reinterpret_cast<const char *>(result.rel_file_data.data()),
                      result.rel_file_data.size());
            out.close();

            std::cout << "REL file written to: " << argv[2] << "\n";
        } else {
            std::cout << "Not a REL file (use REL directive)\n";
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
            if (sym.is_entry())
                std::cout << " (ENT)";
            if (sym.is_external())
                std::cout << " (EXT)";
            if (sym.is_undefined())
                std::cout << " (UNDEF)";
            std::cout << std::endl;
        }
    }

    return result.success ? 0 : 1;
}
