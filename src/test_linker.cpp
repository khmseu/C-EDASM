#include <iostream>
#include <fstream>
#include <iomanip>
#include "edasm/assembler/linker.hpp"

using namespace edasm;

void print_hex_dump(const std::vector<uint8_t>& data, uint16_t base_addr) {
    for (size_t i = 0; i < data.size(); i += 16) {
        std::cout << std::hex << std::setw(4) << std::setfill('0') 
                  << (base_addr + i) << ": ";
        
        for (size_t j = i; j < i + 16 && j < data.size(); ++j) {
            std::cout << std::hex << std::setw(2) << std::setfill('0') 
                      << static_cast<int>(data[j]) << " ";
        }
        std::cout << "\n";
    }
}

int main(int argc, char* argv[]) {
    if (argc < 3) {
        std::cerr << "Usage: " << argv[0] << " <output.bin> <module1.rel> <module2.rel> ...\n";
        return 1;
    }
    
    std::string output_file = argv[1];
    std::vector<std::string> input_files;
    
    for (int i = 2; i < argc; ++i) {
        input_files.push_back(argv[i]);
    }
    
    // Set up linker options
    Linker linker;
    Linker::Options opts;
    opts.output_type = Linker::Options::OutputType::BIN;
    opts.origin = 0x0800;
    opts.generate_map = true;
    
    // Link the files
    std::cout << "Linking " << input_files.size() << " module(s)...\n";
    auto result = linker.link(input_files, opts);
    
    // Print errors/warnings
    for (const auto& warning : result.warnings) {
        std::cout << "WARNING: " << warning << "\n";
    }
    
    for (const auto& error : result.errors) {
        std::cerr << "ERROR: " << error << "\n";
    }
    
    if (!result.success) {
        std::cerr << "Linking FAILED\n";
        return 1;
    }
    
    std::cout << "Linking SUCCEEDED\n";
    std::cout << "Load address: $" << std::hex << std::uppercase << result.load_address << "\n";
    std::cout << "Code size: " << std::dec << result.code_length << " bytes\n\n";
    
    // Print load map
    if (!result.load_map.empty()) {
        std::cout << result.load_map << "\n";
    }
    
    // Print hex dump
    std::cout << "Code:\n";
    print_hex_dump(result.output_data, result.load_address);
    
    // Write output file
    std::ofstream out(output_file, std::ios::binary);
    if (!out) {
        std::cerr << "Cannot write output file: " << output_file << "\n";
        return 1;
    }
    
    // Write 4-byte header (load address, length) for BIN file
    uint16_t addr = result.load_address;
    uint16_t len = result.code_length;
    out.write(reinterpret_cast<const char*>(&addr), 2);
    out.write(reinterpret_cast<const char*>(&len), 2);
    out.write(reinterpret_cast<const char*>(result.output_data.data()), result.output_data.size());
    out.close();
    
    std::cout << "\nOutput written to: " << output_file << "\n";
    
    return 0;
}
