/**
 * @file app.cpp
 * @brief Application main loop implementation for EDASM
 * 
 * Implements the main interpreter loop from EDASM.SRC/EI/EDASMINT.S.
 * Provides command parsing, dispatch, and module coordination for the
 * editor, assembler, and file operations.
 * 
 * Key routines from EDASMINT.S:
 * - Entry point at $B100: Command interpreter and main loop -> run()
 * - Module loader/unloader for Editor, Assembler, Linker
 * - ProDOS interface and file management (adapted for Linux)
 * - Command parsing and dispatch
 * - RESET vector ($03F2) and Ctrl-Y vector ($03F8) handlers
 * 
 * Global page at $BD00-$BEFF used for inter-module communication.
 * C++ equivalent uses class member variables for state sharing.
 * 
 * Zero page variables from EI/EQUATES.S mapped to C++ class members:
 * - TxtBgn/TxtEnd ($0A-$0F) -> text_begin_/text_end_
 * - StackP ($49) -> saved/restored by App
 * - FileType ($51) -> file_type_ (in assembler/file modules)
 * - ExecMode ($53) -> exec_mode_
 * - SwapMode ($74) -> swap_mode_ (in editor)
 * 
 * This C++ implementation adapts the original Apple II/ProDOS model to Linux
 * while maintaining command compatibility and semantic equivalence.
 * 
 * Reference: EI/EDASMINT.S - EDASM Interpreter main module
 */

#include "edasm/app.hpp"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string_view>

#if __has_include(<filesystem>)
#include <filesystem>
#endif

#include "edasm/assembler/assembler.hpp"
#include "edasm/editor/editor.hpp"
#include "edasm/screen.hpp"

namespace edasm {

App::App()
    : screen_(std::make_unique<Screen>()), editor_(std::make_unique<Editor>(*screen_)),
      assembler_(std::make_unique<Assembler>()), current_prefix_(".") {

    // Initialize command dispatch table (from EDASMINT.S command table)
    commands_["LOAD"] = [this](const auto &args) { cmd_load(args); };
    commands_["SAVE"] = [this](const auto &args) { cmd_save(args); };
    commands_["LIST"] = [this](const auto &args) { cmd_list(args); };
    commands_["L"] = [this](const auto &args) { cmd_list(args); }; // Shorthand
    commands_["INSERT"] = [this](const auto &args) { cmd_insert(args); };
    commands_["I"] = [this](const auto &args) { cmd_insert(args); }; // Shorthand
    commands_["DELETE"] = [this](const auto &args) { cmd_delete(args); };
    commands_["FIND"] = [this](const auto &args) { cmd_find(args); };
    commands_["CHANGE"] = [this](const auto &args) { cmd_change(args); };
    commands_["MOVE"] = [this](const auto &args) { cmd_move(args); };
    commands_["COPY"] = [this](const auto &args) { cmd_copy(args); };
    commands_["JOIN"] = [this](const auto &args) { cmd_join(args); };
    commands_["SPLIT"] = [this](const auto &args) { cmd_split(args); };
    commands_["CATALOG"] = [this](const auto &args) { cmd_catalog(args); };
    commands_["CAT"] = [this](const auto &args) { cmd_catalog(args); }; // Shorthand
    commands_["PREFIX"] = [this](const auto &args) { cmd_prefix(args); };
    commands_["ASM"] = [this](const auto &args) { cmd_asm(args); };
    commands_["BYE"] = [this](const auto &args) { cmd_bye(args); };
    commands_["QUIT"] = [this](const auto &args) { cmd_bye(args); };
    commands_["HELP"] = [this](const auto &args) { cmd_help(args); };
    commands_["?"] = [this](const auto &args) { cmd_help(args); };

    // File operations (from EDITOR1.S)
    commands_["RENAME"] = [this](const auto &args) { cmd_rename(args); };
    commands_["LOCK"] = [this](const auto &args) { cmd_lock(args); };
    commands_["UNLOCK"] = [this](const auto &args) { cmd_unlock(args); };
    commands_["DELETEFILE"] = [this](const auto &args) { cmd_delete_file(args); };

    // Command file execution (from EDASMINT.S)
    commands_["EXEC"] = [this](const auto &args) { cmd_exec(args); };
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
    // Read command line from user or EXEC file
    // In EXEC mode, read from file (from EDASMINT.S LB2BD-LB2FA)
    // Otherwise read from keyboard

    if (exec_mode_ && exec_file_ && exec_file_->is_open()) {
        std::string line;
        if (std::getline(*exec_file_, line)) {
            // Display the command with "+" prefix (from EDASMINT.S LB2D4)
            if (screen_ && screen_->is_initialized()) {
                screen_->write_line(1, "+" + line);
                screen_->refresh();
            } else {
                std::cout << "+" << line << std::endl;
            }
            return line;
        } else {
            // EOF or error - close file and exit EXEC mode
            // (from EDASMINT.S LB2E5)
            exec_file_->close();
            exec_file_.reset();
            exec_mode_ = false;

            if (screen_ && screen_->is_initialized()) {
                screen_->write_line(1, "EXEC complete");
                screen_->refresh();
            } else {
                std::cout << "EXEC complete" << std::endl;
            }

            // Fall through to read from keyboard
        }
    }

    // Read from keyboard (standard input)
    // In actual implementation, this would use ncurses line input
    // For now, use simple console input
    std::string line;
    std::getline(std::cin, line);
    return line;
}

void App::parse_and_execute_command(const std::string &cmd_line) {
    auto tokens = tokenize_command(cmd_line);
    if (tokens.empty()) {
        return;
    }

    // Convert command to uppercase for case-insensitive matching
    std::string cmd = tokens[0];
    std::transform(cmd.begin(), cmd.end(), cmd.begin(),
                   [](unsigned char c) { return std::toupper(c); });

    // Check if it's a line number (go to line command)
    if (std::all_of(cmd.begin(), cmd.end(), [](unsigned char c) { return ::isdigit(c); })) {
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
        } catch (const std::exception &e) {
            print_error(std::string("Error: ") + e.what());
        }
    } else {
        print_error("Unknown command: " + cmd);
    }
}

std::vector<std::string> App::tokenize_command(const std::string &line) {
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

void App::print_error(const std::string &msg) {
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

void App::cmd_load(const std::vector<std::string> &args) {
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
    } catch (const std::exception &e) {
        print_error(e.what());
    }
}

void App::cmd_save(const std::vector<std::string> &args) {
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
    } catch (const std::exception &e) {
        print_error(e.what());
    }
}

void App::cmd_list(const std::vector<std::string> &args) {
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
    } catch (const std::exception &e) {
        print_error(e.what());
    }
}

void App::cmd_insert(const std::vector<std::string> &args) {
    // INSERT mode (from EDITOR1.S LD657)
    // Reads lines interactively and inserts them at current position
    // Empty line (just RETURN) exits INSERT mode

    int insert_line = editor_->line_count(); // Default: append at end

    // If line number specified, insert at that position
    if (!args.empty()) {
        try {
            insert_line = std::stoi(args[0]);
            if (insert_line < 0 || insert_line > editor_->line_count()) {
                print_error("Invalid line number");
                return;
            }
        } catch (...) {
            print_error("Invalid line number");
            return;
        }
    }

    screen_->clear();
    screen_->write_line(0, "INSERT mode - Empty line to exit");
    screen_->refresh();

    int current_line = insert_line;
    bool inserting = true;

    while (inserting && running_) {
        // Display line number prompt (like EDASM does)
        std::string prompt = std::to_string(current_line) + ": ";
        screen_->write_line(2, prompt);
        screen_->refresh();

        // Read line from user
        std::string line;
        std::getline(std::cin, line);

        // Empty line exits INSERT mode
        if (line.empty()) {
            inserting = false;
            break;
        }

        // Insert the line at current position
        editor_->insert_line(current_line, line);
        current_line++;
    }

    screen_->clear();
    std::string msg = "Inserted " + std::to_string(current_line - insert_line) + " line(s)";
    screen_->write_line(0, msg);
    screen_->refresh();
}

void App::cmd_delete(const std::vector<std::string> &args) {
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
    } catch (const std::exception &e) {
        print_error(e.what());
    }
}

void App::cmd_catalog(const std::vector<std::string> &args) {
    // CATALOG command: list directory contents (from EDASMINT.S)
    std::string path = args.empty() ? current_prefix_ : args[0];

    // Use std::filesystem to list directory
    try {
        screen_->clear();
        int row = 0;
        screen_->write_line(row++, "Directory: " + path);
        screen_->write_line(row++, "");

#if __has_include(<filesystem>)
        namespace fs = std::filesystem;
        if (!fs::exists(path)) {
            print_error("Path not found: " + path);
            return;
        }

        if (!fs::is_directory(path)) {
            print_error("Not a directory: " + path);
            return;
        }

        // List directory entries
        for (const auto &entry : fs::directory_iterator(path)) {
            if (row >= screen_->rows() - 1) {
                screen_->write_line(row++, "Press any key for more...");
                screen_->refresh();
                screen_->get_key();
                screen_->clear();
                row = 0;
            }

            std::string name = entry.path().filename().string();
            if (entry.is_directory()) {
                name = "<DIR> " + name;
            }
            screen_->write_line(row++, name);
        }
#else
        screen_->write_line(row++, "CATALOG not available (no filesystem support)");
#endif

        screen_->refresh();
    } catch (const std::exception &e) {
        print_error(std::string("CATALOG error: ") + e.what());
    }
}

void App::cmd_prefix(const std::vector<std::string> &args) {
    if (args.empty()) {
        // Display current prefix
        screen_->write_line(1, "PREFIX: " + current_prefix_);
        screen_->refresh();
    } else {
        // Set new prefix
        current_prefix_ = args[0];
    }
}

void App::cmd_asm(const std::vector<std::string> &args) {
    // Assemble current buffer
    auto result = assembler_->assemble(editor_->joined_buffer());
    if (!result.success) {
        for (const auto &err : result.errors) {
            print_error(err);
        }
    } else {
        screen_->write_line(1, "Assembly successful");
        screen_->refresh();
    }
}

void App::cmd_bye(const std::vector<std::string> &args) {
    running_ = false;
}

void App::cmd_find(const std::vector<std::string> &args) {
    if (args.empty()) {
        print_error("FIND requires search text");
        return;
    }

    std::string pattern = args[0];

    // Parse optional range (default: all lines)
    LineRange range;
    if (args.size() > 1) {
        std::string range_str;
        for (size_t i = 1; i < args.size(); ++i) {
            range_str += args[i];
        }
        range = LineRange::parse(range_str);
    }

    auto result = editor_->find(pattern, range);
    if (result.found) {
        screen_->write_line(1, "Found at line " + std::to_string(result.line_num) + ", position " +
                                   std::to_string(result.pos));
        screen_->refresh();
    } else {
        print_error("Pattern not found");
    }
}

void App::cmd_change(const std::vector<std::string> &args) {
    if (args.size() < 2) {
        print_error("CHANGE requires old/new text");
        return;
    }

    std::string old_text = args[0];
    std::string new_text = args[1];

    // Parse optional range (default: all lines)
    LineRange range;
    if (args.size() > 2) {
        std::string range_str;
        for (size_t i = 2; i < args.size(); ++i) {
            range_str += args[i];
        }
        range = LineRange::parse(range_str);
    }

    int count = editor_->change(old_text, new_text, range, true);
    screen_->write_line(1, "Changed " + std::to_string(count) + " occurrence(s)");
    screen_->refresh();
}

void App::cmd_move(const std::vector<std::string> &args) {
    if (args.size() < 2) {
        print_error("MOVE requires range,dest");
        return;
    }

    try {
        auto range = LineRange::parse(args[0]);
        int dest = std::stoi(args[1]);
        editor_->move_lines(range, dest);
        screen_->write_line(1, "Lines moved");
        screen_->refresh();
    } catch (const std::exception &e) {
        print_error(e.what());
    }
}

void App::cmd_copy(const std::vector<std::string> &args) {
    if (args.size() < 2) {
        print_error("COPY requires range,dest");
        return;
    }

    try {
        auto range = LineRange::parse(args[0]);
        int dest = std::stoi(args[1]);
        editor_->copy_lines(range, dest);
        screen_->write_line(1, "Lines copied");
        screen_->refresh();
    } catch (const std::exception &e) {
        print_error(e.what());
    }
}

void App::cmd_join(const std::vector<std::string> &args) {
    if (args.empty()) {
        print_error("JOIN requires line range");
        return;
    }

    std::string range_str = args[0];
    for (size_t i = 1; i < args.size(); ++i) {
        range_str += "," + args[i];
    }

    try {
        auto range = LineRange::parse(range_str);
        editor_->join_lines(range);
        screen_->write_line(1, "Lines joined");
        screen_->refresh();
    } catch (const std::exception &e) {
        print_error(e.what());
    }
}

void App::cmd_split(const std::vector<std::string> &args) {
    if (args.size() < 2) {
        print_error("SPLIT requires line,position");
        return;
    }

    try {
        int line_num = std::stoi(args[0]);
        size_t pos = std::stoul(args[1]);
        editor_->split_line(line_num, pos);
        screen_->write_line(1, "Line split");
        screen_->refresh();
    } catch (const std::exception &e) {
        print_error(e.what());
    }
}

void App::cmd_help(const std::vector<std::string> &args) {
    // Display help information
    screen_->clear();
    int row = 0;
    screen_->write_line(row++, "EDASM Commands:");
    screen_->write_line(row++, "  LOAD <file>       - Load source file");
    screen_->write_line(row++, "  SAVE <file>       - Save buffer to file");
    screen_->write_line(row++, "  LIST [range]      - List lines");
    screen_->write_line(row++, "  INSERT            - Enter insert mode");
    screen_->write_line(row++, "  DELETE <range>    - Delete lines");
    screen_->write_line(row++, "  FIND <text>       - Find text");
    screen_->write_line(row++, "  CHANGE <old> <new> - Replace text");
    screen_->write_line(row++, "  MOVE <range> <dest> - Move lines");
    screen_->write_line(row++, "  COPY <range> <dest> - Copy lines");
    screen_->write_line(row++, "  JOIN <range>      - Join lines");
    screen_->write_line(row++, "  SPLIT <line> <pos> - Split line");
    screen_->write_line(row++, "  CATALOG [path]    - List directory");
    screen_->write_line(row++, "  PREFIX [path]     - Set/show directory");
    screen_->write_line(row++, "  RENAME <old> <new> - Rename file");
    screen_->write_line(row++, "  LOCK <file>       - Make file read-only");
    screen_->write_line(row++, "  UNLOCK <file>     - Remove read-only");
    screen_->write_line(row++, "  DELETEFILE <file> - Delete a file");
    screen_->write_line(row++, "  EXEC <file>       - Execute commands from file");
    screen_->write_line(row++, "  ASM [opts]        - Assemble buffer");
    screen_->write_line(row++, "  BYE/QUIT          - Exit EDASM");
    screen_->write_line(row++, "  HELP/?            - Show this help");
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

// =========================================
// File Operations (from EDITOR1.S)
// =========================================

void App::cmd_rename(const std::vector<std::string> &args) {
    // RENAME <old> <new> - Rename a file
    // From EDITOR1.S LD308
    if (args.size() < 2) {
        print_error("RENAME requires old and new filenames");
        return;
    }

    std::string old_path = args[0];
    std::string new_path = args[1];

    // Add default extension if none provided
    if (old_path.find('.') == std::string::npos) {
        old_path += ".src";
    }
    if (new_path.find('.') == std::string::npos) {
        new_path += ".src";
    }

    try {
#if __has_include(<filesystem>)
        namespace fs = std::filesystem;
        if (!fs::exists(old_path)) {
            print_error("File not found: " + old_path);
            return;
        }

        // Check if destination already exists
        if (fs::exists(new_path)) {
            print_error("Destination already exists: " + new_path);
            return;
        }

        // Rename the file
        fs::rename(old_path, new_path);
        screen_->write_line(1, "Renamed: " + old_path + " -> " + new_path);
        screen_->refresh();
#else
        print_error("RENAME not available (no filesystem support)");
#endif
    } catch (const std::exception &e) {
        print_error(std::string("RENAME error: ") + e.what());
    }
}

void App::cmd_lock(const std::vector<std::string> &args) {
    // LOCK <file> - Set file to read-only
    // From EDITOR1.S LD287
    if (args.empty()) {
        print_error("LOCK requires a filename");
        return;
    }

    std::string path = args[0];
    if (path.find('.') == std::string::npos) {
        path += ".src";
    }

    try {
#if __has_include(<filesystem>)
        namespace fs = std::filesystem;
        if (!fs::exists(path)) {
            print_error("File not found: " + path);
            return;
        }

        // Set file to read-only (remove write permissions)
        auto perms = fs::status(path).permissions();
        fs::permissions(path,
                        perms & ~fs::perms::owner_write & ~fs::perms::group_write &
                            ~fs::perms::others_write,
                        fs::perm_options::replace);

        screen_->write_line(1, "Locked: " + path);
        screen_->refresh();
#else
        print_error("LOCK not available (no filesystem support)");
#endif
    } catch (const std::exception &e) {
        print_error(std::string("LOCK error: ") + e.what());
    }
}

void App::cmd_unlock(const std::vector<std::string> &args) {
    // UNLOCK <file> - Remove read-only attribute
    // From EDITOR1.S LD282
    if (args.empty()) {
        print_error("UNLOCK requires a filename");
        return;
    }

    std::string path = args[0];
    if (path.find('.') == std::string::npos) {
        path += ".src";
    }

    try {
#if __has_include(<filesystem>)
        namespace fs = std::filesystem;
        if (!fs::exists(path)) {
            print_error("File not found: " + path);
            return;
        }

        // Restore write permissions
        auto perms = fs::status(path).permissions();
        fs::permissions(path, perms | fs::perms::owner_write, fs::perm_options::replace);

        screen_->write_line(1, "Unlocked: " + path);
        screen_->refresh();
#else
        print_error("UNLOCK not available (no filesystem support)");
#endif
    } catch (const std::exception &e) {
        print_error(std::string("UNLOCK error: ") + e.what());
    }
}

void App::cmd_delete_file(const std::vector<std::string> &args) {
    // DELETEFILE <file> - Delete a file
    // From EDITOR1.S LD2C4
    // Note: Named DELETEFILE to avoid confusion with DELETE line command
    if (args.empty()) {
        print_error("DELETEFILE requires a filename");
        return;
    }

    std::string path = args[0];
    if (path.find('.') == std::string::npos) {
        path += ".src";
    }

    try {
#if __has_include(<filesystem>)
        namespace fs = std::filesystem;
        if (!fs::exists(path)) {
            print_error("File not found: " + path);
            return;
        }

        // Check if file is locked (read-only)
        auto perms = fs::status(path).permissions();
        bool is_readonly = (perms & fs::perms::owner_write) == fs::perms::none;

        if (is_readonly) {
            // Ask for confirmation (from EDITOR1.S LD2C4-LD2F4)
            screen_->write_line(1, "File is locked. Delete anyway? (Y/N)");
            screen_->refresh();
            int key = screen_->get_key();
            if (key != 'Y' && key != 'y') {
                screen_->write_line(1, "Delete cancelled");
                screen_->refresh();
                return;
            }
        }

        // Delete the file
        fs::remove(path);
        screen_->write_line(1, "Deleted: " + path);
        screen_->refresh();
#else
        print_error("DELETEFILE not available (no filesystem support)");
#endif
    } catch (const std::exception &e) {
        print_error(std::string("DELETEFILE error: ") + e.what());
    }
}

void App::cmd_exec(const std::vector<std::string> &args) {
    // EXEC <pathname> - Execute commands from a text file
    // From EDASMINT.S LB9B0-LB9E9
    // Opens a text file and reads commands line-by-line
    // Commands are displayed with "+" prefix as they execute

    if (args.empty()) {
        print_error("EXEC requires a filename");
        return;
    }

    std::string filename = args[0];
    // Add default extension if none provided (TXT type from EDASMINT.S)
    if (filename.find('.') == std::string::npos) {
        filename += ".txt";
    }

    // If already in EXEC mode, close the current file first
    // (from EDASMINT.S LB9B4)
    if (exec_mode_ && exec_file_) {
        exec_file_->close();
        exec_file_.reset();
        exec_mode_ = false;
    }

    // Try to open the file
    try {
        exec_file_ = std::make_unique<std::ifstream>(filename);

        if (!exec_file_->is_open() || !exec_file_->good()) {
            print_error("Cannot open EXEC file: " + filename);
            exec_file_.reset();
            return;
        }

        // Set EXEC mode flag (from EDASMINT.S LB9EA: LDA #$80 / STA ExecMode)
        exec_mode_ = true;

        if (screen_ && screen_->is_initialized()) {
            screen_->write_line(1, "Executing: " + filename);
            screen_->refresh();
        } else {
            std::cout << "Executing: " << filename << std::endl;
        }

    } catch (const std::exception &e) {
        print_error(std::string("EXEC error: ") + e.what());
        exec_file_.reset();
        exec_mode_ = false;
    }
}

} // namespace edasm
