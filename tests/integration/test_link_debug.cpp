#include "edasm/assembler/rel_file.hpp"
#include <fstream>
#include <iostream>

int main() {
    // Read test_module1.rel
    std::ifstream file("test_module1.rel", std::ios::binary);
    std::vector<uint8_t> data((std::istreambuf_iterator<char>(file)),
                              std::istreambuf_iterator<char>());
    file.close();

    std::vector<uint8_t> code;
    std::vector<edasm::RLDEntry> rld;
    std::vector<edasm::ESDEntry> esd;

    if (edasm::RELFileBuilder::parse(data, code, rld, esd)) {
        std::cout << "Code: " << code.size() << " bytes\n";
        for (size_t i = 0; i < code.size(); ++i) {
            printf("%02X ", code[i]);
        }
        std::cout << "\n\nRLD entries: " << rld.size() << "\n";
        for (const auto &r : rld) {
            printf("  flags=%02X addr=%04X sym=%02X\n", r.flags, r.address, r.symbol_num);
        }
        std::cout << "\nESD entries: " << esd.size() << "\n";
        for (const auto &e : esd) {
            printf("  flags=%02X addr=%04X sym=%02X name=%s\n", e.flags, e.address, e.symbol_num,
                   e.name.c_str());
        }
    }

    return 0;
}
