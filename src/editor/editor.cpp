/**
 * @file editor.cpp
 * @brief Editor implementation for EDASM text editor
 *
 * Implements the text editor from EDASM.SRC/EDITOR/ modules.
 *
 * Primary references: EDITOR1.S, EDITOR2.S, EDITOR3.S
 *
 * Key routines from EDITOR modules:
 * - EDITOR1.S: File commands (LOCK/UNLOCK/DELETE/RENAME), line printing
 * - EDITOR2.S: Text buffer manipulation, cursor movement, search/replace
 * - EDITOR3.S: Insert/delete operations, command execution
 *
 * Original EDASM editor features:
 * - Split buffer text management (gap buffer style)
 * - Sweet16 pseudo-machine for 16-bit pointer operations (C++ uses native pointers)
 * - Text buffer from $0801 to $9900 (HiMem) - C++ uses dynamic allocation
 * - Swap mode support for multiple buffers
 *
 * Zero page variables from EDITOR/EQUATES.S mapped to C++ class members:
 * - Z60 (TabChar) -> tab_expand_mode_
 * - Z7A-Z82 (workspace) -> move_workspace_
 *
 * This C++ implementation preserves EDASM command semantics while using
 * modern data structures (std::string, std::vector) for memory management.
 */

#include "edasm/editor/editor.hpp"

#include <algorithm>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <stdexcept>

#include "edasm/screen.hpp"

namespace edasm {

// =========================================
// LineRange implementation
// =========================================

LineRange LineRange::parse(const std::string &range_str) {
    LineRange range;

    if (range_str.empty()) {
        // No range means all lines
        return range;
    }

    auto comma_pos = range_str.find(',');
    if (comma_pos == std::string::npos) {
        // Single line number: "10"
        range.start = std::stoi(range_str);
        range.end = range.start;
    } else {
        // Range: "10,20" or "10," or ",20"
        std::string start_str = range_str.substr(0, comma_pos);
        std::string end_str = range_str.substr(comma_pos + 1);

        if (!start_str.empty()) {
            range.start = std::stoi(start_str);
        }
        if (!end_str.empty()) {
            range.end = std::stoi(end_str);
        }
    }

    // Validate that start <= end if both are specified
    if (range.start > 0 && range.end > 0 && range.start > range.end) {
        // Swap them to be forgiving (like original EDASM)
        std::swap(range.start, range.end);
    }

    return range;
}

// =========================================
// Editor implementation
// =========================================

Editor::Editor(Screen &screen) : screen_(screen) {}

void Editor::open_buffer(const std::string &text) {
    lines_.clear();
    std::stringstream ss(text);
    std::string line;
    while (std::getline(ss, line)) {
        lines_.push_back(line);
    }
}

void Editor::clear_buffer() {
    lines_.clear();
}

void Editor::load_file(const std::string &path) {
    std::ifstream file(path);
    if (!file) {
        throw std::runtime_error("Cannot open file: " + path);
    }

    lines_.clear();
    std::string line;
    while (std::getline(file, line)) {
        lines_.push_back(line);
    }
}

void Editor::save_file(const std::string &path) {
    std::ofstream file(path);
    if (!file) {
        throw std::runtime_error("Cannot create file: " + path);
    }

    for (const auto &line : lines_) {
        file << line << '\n';
    }
}

void Editor::insert_line(int line_num, const std::string &text) {
    if (line_num < 0 || line_num > static_cast<int>(lines_.size())) {
        throw std::out_of_range("Line number out of range");
    }
    lines_.insert(lines_.begin() + line_num, text);
}

void Editor::delete_line(int line_num) {
    if (line_num < 0 || line_num >= static_cast<int>(lines_.size())) {
        throw std::out_of_range("Line number out of range");
    }
    lines_.erase(lines_.begin() + line_num);
}

void Editor::delete_range(const LineRange &range) {
    auto [start, end] = resolve_range(range);

    // Clamp to valid range
    start = std::max(0, std::min(start, static_cast<int>(lines_.size()) - 1));
    end = std::max(0, std::min(end, static_cast<int>(lines_.size()) - 1));

    if (start <= end && start < static_cast<int>(lines_.size())) {
        lines_.erase(lines_.begin() + start, lines_.begin() + end + 1);
    }
}

void Editor::replace_line(int line_num, const std::string &text) {
    if (line_num < 0 || line_num >= static_cast<int>(lines_.size())) {
        throw std::out_of_range("Line number out of range");
    }
    lines_[line_num] = text;
}

void Editor::render() {
    if (!screen_.is_initialized()) {
        return;
    }
    screen_.clear();

    int max_lines = std::min(screen_.rows(), static_cast<int>(lines_.size()));
    for (int row = 0; row < max_lines; ++row) {
        screen_.write_line(row, lines_[row]);
    }
    screen_.refresh();
}

void Editor::list_lines(const LineRange &range) {
    if (!screen_.is_initialized()) {
        return;
    }

    auto [start, end] = resolve_range(range);

    // Clamp to valid range
    start = std::max(0, std::min(start, static_cast<int>(lines_.size()) - 1));
    end = std::max(0, std::min(end, static_cast<int>(lines_.size()) - 1));

    screen_.clear();
    int screen_row = 0;
    int max_screen_rows = screen_.rows();

    for (int line_num = start; line_num <= end && screen_row < max_screen_rows; ++line_num) {
        // Format with line number like EDASM: "0010  LDA #$00"
        std::stringstream line_str;
        line_str << std::setw(4) << std::setfill('0') << line_num << "  " << lines_[line_num];
        screen_.write_line(screen_row++, line_str.str());
    }

    screen_.refresh();
}

std::pair<int, int> Editor::resolve_range(const LineRange &range) const {
    int start = range.start.value_or(0);
    int end = range.end.value_or(static_cast<int>(lines_.size()) - 1);
    return {start, end};
}

const std::vector<std::string> &Editor::lines() const {
    return lines_;
}

std::string Editor::joined_buffer() const {
    std::string out;
    for (size_t i = 0; i < lines_.size(); ++i) {
        out.append(lines_[i]);
        if (i + 1 < lines_.size()) {
            out.push_back('\n');
        }
    }
    return out;
}

// =========================================
// Search and Replace (from EDITOR2.S)
// =========================================

// FIND command (from EDITOR2.S LD865/LD87A)
// Searches for pattern string in specified line range
// CTRLA ($01) is used as wildcard character
Editor::FindResult Editor::find(const std::string &pattern, const LineRange &range,
                                int start_line) {
    FindResult result;

    if (pattern.empty() || lines_.empty()) {
        return result;
    }

    auto [start, end] = resolve_range(range);
    start = std::max(start, start_line);

    // Clamp to valid range
    start = std::max(0, std::min(start, static_cast<int>(lines_.size()) - 1));
    end = std::max(0, std::min(end, static_cast<int>(lines_.size()) - 1));

    // Search through lines
    for (int line_num = start; line_num <= end; ++line_num) {
        const auto &line = lines_[line_num];

        // Simple substring search (EDASM uses CTRLA as wildcard)
        // For now, implement simple search without wildcard
        size_t pos = line.find(pattern);
        if (pos != std::string::npos) {
            result.found = true;
            result.line_num = line_num;
            result.pos = pos;
            return result;
        }
    }

    return result;
}

// CHANGE command (from EDITOR2.S LD8C6)
// Find and replace text in specified range
// Returns number of replacements made
int Editor::change(const std::string &old_text, const std::string &new_text, const LineRange &range,
                   bool all) {
    if (old_text.empty() || lines_.empty()) {
        return 0;
    }

    int count = 0;
    auto [start, end] = resolve_range(range);

    // Clamp to valid range
    start = std::max(0, std::min(start, static_cast<int>(lines_.size()) - 1));
    end = std::max(0, std::min(end, static_cast<int>(lines_.size()) - 1));

    // Search and replace in each line
    for (int line_num = start; line_num <= end; ++line_num) {
        std::string &line = lines_[line_num];
        size_t pos = 0;

        while ((pos = line.find(old_text, pos)) != std::string::npos) {
            line.replace(pos, old_text.length(), new_text);
            pos += new_text.length();
            count++;

            if (!all) {
                // "Some" mode - only replace first occurrence per line
                break;
            }
        }
    }

    return count;
}

// =========================================
// Buffer Manipulation (from EDITOR2.S)
// =========================================

// MOVE command (from EDITOR2.S LD819)
// Move lines from src_range to dest_line
void Editor::move_lines(const LineRange &src_range, int dest_line) {
    auto [start, end] = resolve_range(src_range);

    // Validate range
    if (start < 0 || end >= static_cast<int>(lines_.size()) || start > end) {
        throw std::out_of_range("Invalid source range for MOVE");
    }

    if (dest_line < 0 || dest_line > static_cast<int>(lines_.size())) {
        throw std::out_of_range("Invalid destination for MOVE");
    }

    // Can't move to within the source range
    if (dest_line > start && dest_line <= end + 1) {
        throw std::invalid_argument("Destination within source range");
    }

    // Extract lines to move
    std::vector<std::string> moved_lines;
    for (int i = start; i <= end; ++i) {
        moved_lines.push_back(lines_[i]);
    }

    // Delete from source
    lines_.erase(lines_.begin() + start, lines_.begin() + end + 1);

    // Adjust destination if it's after the source
    int insert_pos = dest_line;
    if (dest_line > end) {
        insert_pos -= (end - start + 1);
    }

    // Insert at destination
    lines_.insert(lines_.begin() + insert_pos, moved_lines.begin(), moved_lines.end());
}

// COPY command (from EDITOR2.S LD819)
// Copy lines from src_range to dest_line
void Editor::copy_lines(const LineRange &src_range, int dest_line) {
    auto [start, end] = resolve_range(src_range);

    // Validate range
    if (start < 0 || end >= static_cast<int>(lines_.size()) || start > end) {
        throw std::out_of_range("Invalid source range for COPY");
    }

    if (dest_line < 0 || dest_line > static_cast<int>(lines_.size())) {
        throw std::out_of_range("Invalid destination for COPY");
    }

    // Extract lines to copy
    std::vector<std::string> copied_lines;
    for (int i = start; i <= end; ++i) {
        copied_lines.push_back(lines_[i]);
    }

    // Insert at destination
    lines_.insert(lines_.begin() + dest_line, copied_lines.begin(), copied_lines.end());
}

// JOIN command
// Join multiple lines into one
void Editor::join_lines(const LineRange &range) {
    auto [start, end] = resolve_range(range);

    // Validate range
    if (start < 0 || end >= static_cast<int>(lines_.size()) || start > end) {
        throw std::out_of_range("Invalid range for JOIN");
    }

    if (start == end) {
        return; // Nothing to join
    }

    // Join all lines in range
    std::string joined;
    for (int i = start; i <= end; ++i) {
        if (i > start) {
            joined += " "; // Add space between joined lines
        }
        joined += lines_[i];
    }

    // Replace first line with joined text, delete rest
    lines_[start] = joined;
    lines_.erase(lines_.begin() + start + 1, lines_.begin() + end + 1);
}

// SPLIT command
// Split line at specified position
void Editor::split_line(int line_num, size_t pos) {
    if (line_num < 0 || line_num >= static_cast<int>(lines_.size())) {
        throw std::out_of_range("Line number out of range");
    }

    std::string &line = lines_[line_num];
    if (pos > line.length()) {
        pos = line.length();
    }

    // Split line at position
    std::string second_part = line.substr(pos);
    line = line.substr(0, pos);

    // Insert second part as new line
    lines_.insert(lines_.begin() + line_num + 1, second_part);
}

} // namespace edasm
