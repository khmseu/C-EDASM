/**
 * @file app.hpp
 * @brief Main application class for EDASM editor/assembler
 *
 * Implements the main command interpreter loop from EDASM.SRC/EI/EDASMINT.S.
 * Provides command parsing, dispatch, and module coordination for the editor,
 * assembler, and file operations. Adapts the original Apple II/ProDOS model
 * to Linux while maintaining command compatibility and semantic equivalence.
 */

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

/**
 * @brief Main application class coordinating editor, assembler, and file operations
 *
 * The App class implements the main command loop and provides a command-line interface
 * for text editing and 6502 assembly. It dispatches commands to the editor and assembler
 * modules and handles file I/O operations compatible with ProDOS semantics.
 *
 * Reference: EDASMINT.S - EDASM Interpreter main module
 */
class App {
  public:
    /**
     * @brief Construct a new App object
     *
     * Initializes screen, editor, assembler, and command dispatch table
     */
    App();

    /**
     * @brief Destroy the App object
     *
     * Ensures screen is properly shut down if initialized
     */
    ~App();

    /**
     * @brief Run the main application loop
     *
     * @param argc Command line argument count
     * @param argv Command line argument values
     * @return int Exit code (0 for success)
     */
    int run(int argc, char **argv);

  private:
    /**
     * @brief Print command-line help message
     */
    void print_help() const;

    /**
     * @brief Main command loop from EDASMINT.S main loop
     */
    void command_loop();

    /**
     * @brief Parse and execute a command line
     * @param cmd_line Command string to parse and execute
     */
    void parse_and_execute_command(const std::string &cmd_line);

    /**
     * @brief Tokenize command line on spaces and commas
     * @param line Command line to tokenize
     * @return std::vector<std::string> Tokens extracted from the line
     */
    std::vector<std::string> tokenize_command(const std::string &line);

    // Command handlers (from EDASMINT.S command dispatch)

    /**
     * @brief Load a source file into the editor buffer
     * @param args Command arguments (filename required)
     */
    void cmd_load(const std::vector<std::string> &args);

    /**
     * @brief Save the editor buffer to a file
     * @param args Command arguments (filename required)
     */
    void cmd_save(const std::vector<std::string> &args);

    /**
     * @brief List lines from the editor buffer
     * @param args Command arguments (optional range)
     */
    void cmd_list(const std::vector<std::string> &args);

    /**
     * @brief Enter insert mode to add new lines
     * @param args Command arguments (optional line number)
     */
    void cmd_insert(const std::vector<std::string> &args);

    /**
     * @brief Delete a range of lines
     * @param args Command arguments (line range required)
     */
    void cmd_delete(const std::vector<std::string> &args);

    /**
     * @brief Find text in the buffer
     * @param args Command arguments (search pattern required)
     */
    void cmd_find(const std::vector<std::string> &args);

    /**
     * @brief Change (replace) text in the buffer
     * @param args Command arguments (old and new text required)
     */
    void cmd_change(const std::vector<std::string> &args);

    /**
     * @brief Move lines to a new position
     * @param args Command arguments (source range and destination required)
     */
    void cmd_move(const std::vector<std::string> &args);

    /**
     * @brief Copy lines to a new position
     * @param args Command arguments (source range and destination required)
     */
    void cmd_copy(const std::vector<std::string> &args);

    /**
     * @brief Join multiple lines into one
     * @param args Command arguments (line range required)
     */
    void cmd_join(const std::vector<std::string> &args);

    /**
     * @brief Split a line at a specified position
     * @param args Command arguments (line number and position required)
     */
    void cmd_split(const std::vector<std::string> &args);

    /**
     * @brief List directory contents (CATALOG command)
     * @param args Command arguments (optional path)
     */
    void cmd_catalog(const std::vector<std::string> &args);

    /**
     * @brief Set or display current directory prefix
     * @param args Command arguments (optional path)
     */
    void cmd_prefix(const std::vector<std::string> &args);

    /**
     * @brief Assemble the current buffer
     * @param args Command arguments (optional assembly options)
     */
    void cmd_asm(const std::vector<std::string> &args);

    /**
     * @brief Exit the application
     * @param args Command arguments (unused)
     */
    void cmd_bye(const std::vector<std::string> &args);

    /**
     * @brief Display help information
     * @param args Command arguments (unused)
     */
    void cmd_help(const std::vector<std::string> &args);

    // File operations (from EDITOR1.S)

    /**
     * @brief Rename a file
     * @param args Command arguments (old and new filenames required)
     */
    void cmd_rename(const std::vector<std::string> &args);

    /**
     * @brief Lock a file (make read-only)
     * @param args Command arguments (filename required)
     */
    void cmd_lock(const std::vector<std::string> &args);

    /**
     * @brief Unlock a file (remove read-only)
     * @param args Command arguments (filename required)
     */
    void cmd_unlock(const std::vector<std::string> &args);

    /**
     * @brief Delete a file from disk
     * @param args Command arguments (filename required)
     */
    void cmd_delete_file(const std::vector<std::string> &args);

    // Command file execution (from EDASMINT.S LB9B0)

    /**
     * @brief Execute commands from a text file
     * @param args Command arguments (filename required)
     */
    void cmd_exec(const std::vector<std::string> &args);

    // Helper functions

    /**
     * @brief Display the command prompt
     */
    void display_prompt();

    /**
     * @brief Read a command line from user or EXEC file
     * @return std::string The command line read
     */
    std::string read_command_line();

    /**
     * @brief Print an error message to screen
     * @param msg Error message to display
     */
    void print_error(const std::string &msg);

    // State
    std::unique_ptr<Screen> screen_;       ///< Screen/terminal interface
    std::unique_ptr<Editor> editor_;       ///< Editor module
    std::unique_ptr<Assembler> assembler_; ///< Assembler module

    // Command dispatch table
    using CommandHandler = std::function<void(const std::vector<std::string> &)>;
    std::unordered_map<std::string, CommandHandler> commands_; ///< Command name to handler mapping

    bool running_{true};          ///< Application running state
    std::string current_prefix_;  ///< Current directory (PREFIX command)
    std::string last_list_range_; ///< Last LIST range for Ctrl-R repeat

    // EXEC command state (from EDASMINT.S ExecMode, RdExeRN)
    std::unique_ptr<std::ifstream> exec_file_; ///< File handle for EXEC file
    bool exec_mode_{false};                    ///< True when reading commands from file
};

} // namespace edasm
