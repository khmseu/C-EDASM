#include "edasm/assembler/linker.hpp"
#include <iostream>

int main() {
    std::vector<std::string> files = {"test_module1.rel", "test_module2.rel"};

    edasm::Linker linker;
    edasm::Linker::Options opts;
    opts.output_type = edasm::Linker::Options::OutputType::BIN;
    opts.origin = 0x0800;
    opts.generate_map = true;

    auto result = linker.link(files, opts);

    std::cout << "Success: " << result.success << "\n";
    std::cout << "Code:\n";
    for (size_t i = 0; i < result.output_data.size(); ++i) {
        printf("%02X ", result.output_data[i]);
        if ((i + 1) % 16 == 0)
            printf("\n");
    }
    printf("\n");

    return 0;
}
