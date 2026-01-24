#include "edasm/assembler/assembler.hpp"
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>

int main(int argc, char **argv) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <source_file> [listing_file]" << std::endl;
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

    // Set up assembler options
    edasm::Assembler::Options opts;
    opts.generate_listing = true;
    opts.list_symbols = true;
    opts.sort_symbols_by_value = false;
    opts.symbol_columns = 4;

    // Assemble
    edasm::Assembler assembler;
    auto result = assembler.assemble(source, opts);

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

        // Write listing to file if specified
        if (argc >= 3 && !result.listing.empty()) {
            std::ofstream list_file(argv[2]);
            if (list_file) {
                list_file << result.listing;
                std::cout << "\nListing written to: " << argv[2] << std::endl;
            } else {
                std::cerr << "Warning: Could not write listing to " << argv[2] << std::endl;
            }
        }

        // Print listing to console if no file specified
        if (argc < 3 && !result.listing.empty()) {
            std::cout << "\n" << result.listing << std::endl;
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

    return result.success ? 0 : 1;
}
