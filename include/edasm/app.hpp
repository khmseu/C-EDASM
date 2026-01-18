#pragma once

#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
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

    // Main command loop (EDASMINT.S main loop)
    void command_loop();

    // Command parsing and dispatch
    void parse_and_execute_command(const std::string &cmd_line);
    std::vector<std::string> tokenize_command(const std::string &line);

    // Command handlers (from EDASMINT.S command dispatch)
    void cmd_load(const std::vector<std::string> &args);
    void cmd_save(const std::vector<std::string> &args);
    void cmd_list(const std::vector<std::string> &args);
    void cmd_insert(const std::vector<std::string> &args);
    void cmd_delete(const std::vector<std::string> &args);
    void cmd_find(const std::vector<std::string> &args);
    void cmd_change(const std::vector<std::string> &args);
    void cmd_move(const std::vector<std::string> &args);
    void cmd_copy(const std::vector<std::string> &args);
    void cmd_join(const std::vector<std::string> &args);
    void cmd_split(const std::vector<std::string> &args);
    void cmd_catalog(const std::vector<std::string> &args);
    void cmd_prefix(const std::vector<std::string> &args);
    void cmd_asm(const std::vector<std::string> &args);
    void cmd_bye(const std::vector<std::string> &args);
    void cmd_help(const std::vector<std::string> &args);

    // File operations (from EDITOR1.S)
    void cmd_rename(const std::vector<std::string> &args);
    void cmd_lock(const std::vector<std::string> &args);
    void cmd_unlock(const std::vector<std::string> &args);
    void cmd_delete_file(const std::vector<std::string> &args);

    // Command file execution (from EDASMINT.S LB9B0)
    void cmd_exec(const std::vector<std::string> &args);

    // Helper functions
    void display_prompt();
    std::string read_command_line();
    void print_error(const std::string &msg);

    // State
    std::unique_ptr<Screen> screen_;
    std::unique_ptr<Editor> editor_;
    std::unique_ptr<Assembler> assembler_;

    // Command dispatch table
    using CommandHandler = std::function<void(const std::vector<std::string> &)>;
    std::unordered_map<std::string, CommandHandler> commands_;

    bool running_{true};
    std::string current_prefix_;  // Current directory (PREFIX command)
    std::string last_list_range_; // For Ctrl-R repeat

    // EXEC command state (from EDASMINT.S ExecMode, RdExeRN)
    std::unique_ptr<std::ifstream> exec_file_; // File handle for EXEC file
    bool exec_mode_{false};                    // True when reading commands from file
};

} // namespace edasm
