/**
 * @file screen.hpp
 * @brief Terminal screen management using ncurses
 * 
 * Provides a simple wrapper around ncurses for text-mode screen output.
 * Handles screen initialization, cleanup, and basic text operations.
 */

#pragma once

#include <string>

namespace edasm {

/**
 * @brief Screen manager for terminal I/O using ncurses
 * 
 * Wraps ncurses functionality to provide simple screen operations
 * for the EDASM editor/assembler interface. Non-copyable.
 */
class Screen {
  public:
    /**
     * @brief Construct a new Screen object
     */
    Screen();
    
    /**
     * @brief Destroy the Screen object and clean up ncurses
     */
    ~Screen();

    Screen(const Screen &) = delete;
    Screen &operator=(const Screen &) = delete;

    /**
     * @brief Initialize ncurses and set up the screen
     */
    void init();
    
    /**
     * @brief Shutdown ncurses and restore terminal
     */
    void shutdown();

    /**
     * @brief Clear the screen
     */
    void clear();
    
    /**
     * @brief Refresh the screen display
     */
    void refresh();
    
    /**
     * @brief Write a line of text at the specified row
     * @param row Row number (0-based)
     * @param text Text to write
     */
    void write_line(int row, const std::string &text);
    
    /**
     * @brief Get a single keypress from the user
     * @return int Key code (character or special key)
     */
    int get_key();

    /**
     * @brief Get the number of rows in the terminal
     * @return int Number of rows
     */
    int rows() const;
    
    /**
     * @brief Get the number of columns in the terminal
     * @return int Number of columns
     */
    int cols() const;
    
    /**
     * @brief Check if screen is initialized
     * @return bool True if ncurses is initialized
     */
    bool is_initialized() const;

  private:
    bool initialized_; ///< Screen initialization status
};

} // namespace edasm
