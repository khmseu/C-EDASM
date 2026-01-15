#include "edasm/app.hpp"

#include <iostream>
#include <sstream>
#include <string_view>
#include <algorithm>
#include <cctype>

#include "edasm/assembler/assembler.hpp"
#include "edasm/editor/editor.hpp"
#include "edasm/screen.hpp"
#include "edasm/constants.hpp"

namespace edasm {

App::App()
    : screen_(std::make_unique<Screen>()), 
      editor_(std::make_unique<Editor>(*screen_)),
      assembler_(std::make_unique<Assembler>()),
      current_prefix_(".") {
    
    // Initialize command dispatch table (from EDASMINT.S command table)
    commands_["LOAD"] = [this](const auto& args) { cmd_load(args); };
    commands_["SAVE"] = [this](const auto& args) { cmd_save(args); };
    commands_["LIST"] = [this](const auto& args) { cmd_list(args); };
    commands_["L"] = [this](const auto& args) { cmd_list(args); }; // Shorthand
    commands_["INSERT"] = [this](const auto& args) { cmd_insert(args); };
    commands_["I"] = [this](const auto& args) { cmd_insert(args); }; // Shorthand
    commands_["DELETE"] = [this](const auto& args) { cmd_delete(args); };
    commands_["CATALOG"] = [this](const auto& args) { cmd_catalog(args); };
    commands_["CAT"] = [this](const auto& args) { cmd_catalog(args); }; // Shorthand
    commands_["PREFIX"] = [this](const auto& args) { cmd_prefix(args); };
    commands_["ASM"] = [this](const auto& args) { cmd_asm(args); };
    commands_["BYE"] = [this](const auto& args) { cmd_bye(args); };
    commands_["QUIT"] = [this](const auto& args) { cmd_bye(args); };
    commands_["HELP"] = [this](const auto& args) { cmd_help(args); };
    commands_["?"] = [this](const auto& args) { cmd_help(args); };
}

App::~App() {
    if (screen_ && screen_->is_initialized()) {
        screen_->shutdown();
    }
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
    command_loop();
    screen_->shutdown();
    return 0;
}

void App::command_loop() {
    // Main command loop (analogous to EDASMINT.S main loop at $B100)
    while (running_) {
        display_prompt();
        std::string cmd_line = read_command_line();
        
        // Skip empty lines and comments (lines starting with *)
        if (cmd_line.empty() || cmd_line[0] == '*') {
            continue;
        }
        
        parse_and_execute_command(cmd_line);
    }
}

void App::display_prompt() {
    // Display prompt similar to EDASM (with date/time in original)
    // For now, simple "]" prompt like EDASM
    screen_->clear();
    screen_->write_line(0, "]");
    screen_->refresh();
}

std::string App::read_command_line() {
    // Read command line from user
    // In actual implementation, this would use ncurses line input
    // For now, use simple console input
    std::string line;
    std::getline(std::cin, line);
    return line;
}

void App::parse_and_execute_command(const std::string& cmd_line) {
    auto tokens = tokenize_command(cmd_line);
    if (tokens.empty()) {
        return;
    }
    
    // Convert command to uppercase for case-insensitive matching
    std::string cmd = tokens[0];
    std::transform(cmd.begin(), cmd.end(), cmd.begin(),
                   [](unsigned char c) { return std::toupper(c); });
    
    // Check if it's a line number (go to line command)
    if (std::all_of(cmd.begin(), cmd.end(), ::isdigit)) {
        // TODO: Implement go-to-line
        print_error("Go to line not yet implemented");
        return;
    }
    
    // Look up command in dispatch table
    auto it = commands_.find(cmd);
    if (it != commands_.end()) {
        try {
            // Remove command from args, pass rest to handler
            std::vector<std::string> args(tokens.begin() + 1, tokens.end());
            it->second(args);
        } catch (const std::exception& e) {
            print_error(std::string("Error: ") + e.what());
        }
    } else {
        print_error("Unknown command: " + cmd);
    }
}

std::vector<std::string> App::tokenize_command(const std::string& line) {
    // Tokenize command line on spaces and commas (EDASM style)
    std::vector<std::string> tokens;
    std::stringstream ss(line);
    std::string token;
    
    while (ss >> token) {
        // Handle comma-separated tokens
        size_t pos = 0;
        while ((pos = token.find(',')) != std::string::npos) {
            if (pos > 0) {
                tokens.push_back(token.substr(0, pos));
            }
            token = token.substr(pos + 1);
        }
        if (!token.empty()) {
            tokens.push_back(token);
        }
    }
    
    return tokens;
}

void App::print_error(const std::string& msg) {
    // Print error message to screen
    if (screen_ && screen_->is_initialized()) {
        screen_->write_line(1, "ERROR: " + msg);
        screen_->refresh();
    } else {
        std::cerr << "ERROR: " << msg << std::endl;
    }
}

// =========================================
// Command Handlers (from EDASMINT.S)
// =========================================

void App::cmd_load(const std::vector<std::string>& args) {
    if (args.empty()) {
        print_error("LOAD requires filename");
        return;
    }
    
    std::string filename = args[0];
    // Add default extension if none provided
    if (filename.find('.') == std::string::npos) {
        filename += ".src";
    }
    
    try {
        editor_->load_file(filename);
        screen_->write_line(1, "Loaded: " + filename);
        screen_->refresh();
    } catch (const std::exception& e) {
        print_error(e.what());
    }
}

void App::cmd_save(const std::vector<std::string>& args) {
    if (args.empty()) {
        print_error("SAVE requires filename");
        return;
    }
    
    std::string filename = args[0];
    // Add default extension if none provided
    if (filename.find('.') == std::string::npos) {
        filename += ".src";
    }
    
    try {
        editor_->save_file(filename);
        screen_->write_line(1, "Saved: " + filename);
        screen_->refresh();
    } catch (const std::exception& e) {
        print_error(e.what());
    }
}

void App::cmd_list(const std::vector<std::string>& args) {
    // Parse range: LIST, LIST 10, LIST 10,20, LIST 10,, LIST ,20
    std::string range_str;
    if (!args.empty()) {
        // Join all args with commas (in case user typed "LIST 10 20")
        range_str = args[0];
        for (size_t i = 1; i < args.size(); ++i) {
            range_str += "," + args[i];
        }
    }
    
    try {
        auto range = LineRange::parse(range_str);
        editor_->list_lines(range);
        last_list_range_ = range_str; // Store for Ctrl-R repeat
    } catch (const std::exception& e) {
        print_error(e.what());
    }
}

void App::cmd_insert(const std::vector<std::string>& args) {
    // TODO: Implement INSERT mode
    // This would enter an interactive mode where user types lines
    // until an empty line is entered
    print_error("INSERT not yet implemented");
}

void App::cmd_delete(const std::vector<std::string>& args) {
    if (args.empty()) {
        print_error("DELETE requires line range");
        return;
    }
    
    std::string range_str = args[0];
    for (size_t i = 1; i < args.size(); ++i) {
        range_str += "," + args[i];
    }
    
    try {
        auto range = LineRange::parse(range_str);
        editor_->delete_range(range);
        screen_->write_line(1, "Lines deleted");
        screen_->refresh();
    } catch (const std::exception& e) {
        print_error(e.what());
    }
}

void App::cmd_catalog(const std::vector<std::string>& args) {
    // TODO: Implement directory listing
    print_error("CATALOG not yet implemented");
}

void App::cmd_prefix(const std::vector<std::string>& args) {
    if (args.empty()) {
        // Display current prefix
        screen_->write_line(1, "PREFIX: " + current_prefix_);
        screen_->refresh();
    } else {
        // Set new prefix
        current_prefix_ = args[0];
    }
}

void App::cmd_asm(const std::vector<std::string>& args) {
    // Assemble current buffer
    auto result = assembler_->assemble(editor_->joined_buffer());
    if (!result.success) {
        for (const auto& err : result.errors) {
            print_error(err);
        }
    } else {
        screen_->write_line(1, "Assembly successful");
        screen_->refresh();
    }
}

void App::cmd_bye(const std::vector<std::string>& args) {
    running_ = false;
}

void App::cmd_help(const std::vector<std::string>& args) {
    // Display help information
    screen_->clear();
    int row = 0;
    screen_->write_line(row++, "EDASM Commands:");
    screen_->write_line(row++, "  LOAD <file>    - Load source file");
    screen_->write_line(row++, "  SAVE <file>    - Save buffer to file");
    screen_->write_line(row++, "  LIST [range]   - List lines");
    screen_->write_line(row++, "  INSERT         - Enter insert mode");
    screen_->write_line(row++, "  DELETE <range> - Delete lines");
    screen_->write_line(row++, "  CATALOG [path] - List directory");
    screen_->write_line(row++, "  PREFIX [path]  - Set/show directory");
    screen_->write_line(row++, "  ASM [opts]     - Assemble buffer");
    screen_->write_line(row++, "  BYE/QUIT       - Exit EDASM");
    screen_->write_line(row++, "  HELP/?         - Show this help");
    screen_->write_line(row++, "");
    screen_->write_line(row++, "Press any key to continue...");
    screen_->refresh();
    screen_->get_key();
}

void App::print_help() const {
    std::cout << "EDASM (C++/ncurses) - 6502 Editor/Assembler" << std::endl;
    std::cout << "Usage: edasm_cli [options]" << std::endl;
    std::cout << "  -h, --help    Show this message" << std::endl;
    std::cout << std::endl;
    std::cout << "Port of Apple II EDASM to modern C++/ncurses" << std::endl;
    std::cout << "Based on EDASM.SRC from markpmlim/EdAsm" << std::endl;
}

} // namespace edasm
