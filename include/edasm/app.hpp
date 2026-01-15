#pragma once

#include <memory>
#include <string>
#include <vector>

namespace edasm {

class Screen;
class Editor;
class Assembler;

class App {
  public:
    App();
    ~App();

    int run(int argc, char **argv);

  private:
    void print_help() const;
    int run_editor();
    int assemble_current_buffer();

    std::unique_ptr<Screen> screen_;
    std::unique_ptr<Editor> editor_;
    std::unique_ptr<Assembler> assembler_;
};

} // namespace edasm
