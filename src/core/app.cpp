#include "edasm/app.hpp"

#include <iostream>
#include <string_view>

#include "edasm/assembler/assembler.hpp"
#include "edasm/editor/editor.hpp"
#include "edasm/screen.hpp"

namespace edasm {

App::App()
    : screen_(std::make_unique<Screen>()), editor_(std::make_unique<Editor>(*screen_)),
      assembler_(std::make_unique<Assembler>()) {}

App::~App() {
    screen_->shutdown();
}

int App::run(int argc, char **argv) {
    if (argc > 1) {
        std::string_view arg{argv[1]};
        if (arg == "-h" || arg == "--help") {
            print_help();
            return 0;
        }
    }

    screen_->init();
    editor_->open_buffer("; EDASM port stub\n; TODO: load source file\n");
    editor_->render();

    if (screen_->rows() > 0) {
        screen_->write_line(screen_->rows() - 1,
                            "Press 'a' to assemble, 'q' to quit (placeholder)");
        screen_->refresh();
    }

    for (;;) {
        int ch = screen_->get_key();
        if (ch == 'q' || ch == 'Q') {
            break;
        }
        if (ch == 'a' || ch == 'A') {
            assemble_current_buffer();
            editor_->render();
            if (screen_->rows() > 0) {
                screen_->write_line(screen_->rows() - 1, "Assemble invoked");
                screen_->refresh();
            }
        }
    }

    screen_->shutdown();
    return 0;
}

void App::print_help() const {
    std::cout << "EDASM (C++/ncurses)" << std::endl;
    std::cout << "Usage: edasm_cli" << std::endl;
    std::cout << "-h, --help    Show this message" << std::endl;
    std::cout << "Keys: q quit, a assemble (placeholder)" << std::endl;
}

int App::run_editor() {
    return 0;
}

int App::assemble_current_buffer() {
    auto result = assembler_->assemble(editor_->joined_buffer());
    if (!result.success) {
        for (const auto &err : result.errors) {
            std::cerr << err << std::endl;
        }
        return 1;
    }
    return 0;
}

} // namespace edasm
