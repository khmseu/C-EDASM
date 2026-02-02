/**
 * @file editor.hpp
 * @brief Text editor module for EDASM
 *
 * Implements line-based text editing commands from EDITOR*.S modules.
 * Provides buffer management, line editing, search/replace, and display
 * functionality compatible with original EDASM editor commands.
 */

#pragma once

#include <optional>
#include <string>
#include <vector>

namespace edasm {

class Screen;

/**
 * @brief Line range specification for editor commands
 *
 * Represents a range of line numbers for commands like LIST, DELETE, etc.
 * Nullopt values indicate open-ended ranges (from beginning or to end).
 */
struct LineRange {
    std::optional<int> start; ///< Start line (nullopt = from beginning)
    std::optional<int> end;   ///< End line (nullopt = to end)

    /**
     * @brief Parse a line range string (e.g., "10", "10,20", ",20", "10,")
     * @param range_str Range string to parse
     * @return LineRange Parsed line range
     */
    static LineRange parse(const std::string &range_str);

    /**
     * @brief Check if range covers all lines
     * @return bool True if both start and end are unspecified
     */
    bool is_all() const {
        return !start.has_value() && !end.has_value();
    }
};

/**
 * @brief Text editor providing line-based editing operations
 *
 * Implements EDASM editor commands from EDITOR*.S modules including
 * buffer management, line insertion/deletion, search/replace, and
 * line manipulation operations (move, copy, join, split).
 */
class Editor {
  public:
    /**
     * @brief Construct a new Editor object
     * @param screen Screen reference for output
     */
    explicit Editor(Screen &screen);

    // Buffer management (from EDITOR*.S)

    /**
     * @brief Open buffer with text content
     * @param text Initial text content
     */
    void open_buffer(const std::string &text);

    /**
     * @brief Clear the entire buffer
     */
    void clear_buffer();

    /**
     * @brief Load a file into the buffer
     * @param path File path to load
     */
    void load_file(const std::string &path);

    /**
     * @brief Save buffer contents to a file
     * @param path File path to save to
     */
    void save_file(const std::string &path);

    // Line editing (from EDITOR2.S, EDITOR3.S)

    /**
     * @brief Insert a new line at specified position
     * @param line_num Line number (0-based)
     * @param text Line text content
     */
    void insert_line(int line_num, const std::string &text);

    /**
     * @brief Delete a single line
     * @param line_num Line number to delete
     */
    void delete_line(int line_num);

    /**
     * @brief Delete a range of lines
     * @param range Line range to delete
     */
    void delete_range(const LineRange &range);

    /**
     * @brief Replace a line with new text
     * @param line_num Line number to replace
     * @param text New text content
     */
    void replace_line(int line_num, const std::string &text);

    // Search and replace (from EDITOR2.S LD865, LD8C6)

    /**
     * @brief Result of a find operation
     */
    struct FindResult {
        bool found{false}; ///< True if pattern was found
        int line_num{-1};  ///< Line number where found
        size_t pos{0};     ///< Position within line
    };

    /**
     * @brief Find pattern in buffer
     * @param pattern Text pattern to find
     * @param range Line range to search
     * @param start_line Starting line for search
     * @return FindResult Search result with location
     */
    FindResult find(const std::string &pattern, const LineRange &range, int start_line = 0);

    /**
     * @brief Change (replace) text in buffer
     * @param old_text Text to find
     * @param new_text Replacement text
     * @param range Line range to search
     * @param all True to replace all occurrences
     * @return int Number of replacements made
     */
    int change(const std::string &old_text, const std::string &new_text, const LineRange &range,
               bool all = true);

    // Buffer manipulation (from EDITOR2.S LD819)

    /**
     * @brief Move lines to a new position
     * @param src_range Source line range
     * @param dest_line Destination line number
     */
    void move_lines(const LineRange &src_range, int dest_line);

    /**
     * @brief Copy lines to a new position
     * @param src_range Source line range
     * @param dest_line Destination line number
     */
    void copy_lines(const LineRange &src_range, int dest_line);

    // Line operations

    /**
     * @brief Join multiple lines into one
     * @param range Line range to join
     */
    void join_lines(const LineRange &range);

    /**
     * @brief Split a line at specified position
     * @param line_num Line number to split
     * @param pos Character position to split at
     */
    void split_line(int line_num, size_t pos);

    // Display

    /**
     * @brief Render buffer to screen
     */
    void render();

    /**
     * @brief List lines to screen
     * @param range Line range to list
     */
    void list_lines(const LineRange &range);

    // Access

    /**
     * @brief Get reference to line buffer
     * @return const std::vector<std::string>& Buffer lines
     */
    const std::vector<std::string> &lines() const;

    /**
     * @brief Get all lines joined as single string
     * @return std::string Complete buffer text
     */
    std::string joined_buffer() const;

    /**
     * @brief Get number of lines in buffer
     * @return int Line count
     */
    int line_count() const {
        return static_cast<int>(lines_.size());
    }

  private:
    Screen &screen_;                 ///< Screen reference for output
    std::vector<std::string> lines_; ///< Buffer lines

    /**
     * @brief Resolve a line range to actual indices
     * @param range Line range specification
     * @return std::pair<int, int> Start and end indices (inclusive)
     */
    std::pair<int, int> resolve_range(const LineRange &range) const;
};

} // namespace edasm
