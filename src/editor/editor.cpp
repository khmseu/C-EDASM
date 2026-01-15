#include "edasm/editor/editor.hpp"

#include <sstream>
#include <fstream>
#include <stdexcept>
#include <algorithm>

#include "edasm/screen.hpp"

namespace edasm {

// =========================================
// LineRange implementation
// =========================================

LineRange LineRange::parse(const std::string& range_str) {
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

void Editor::load_file(const std::string& path) {
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

void Editor::save_file(const std::string& path) {
    std::ofstream file(path);
    if (!file) {
        throw std::runtime_error("Cannot create file: " + path);
    }
    
    for (const auto& line : lines_) {
        file << line << '\n';
    }
}

void Editor::insert_line(int line_num, const std::string& text) {
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

void Editor::delete_range(const LineRange& range) {
    auto [start, end] = resolve_range(range);
    
    // Clamp to valid range
    start = std::max(0, std::min(start, static_cast<int>(lines_.size()) - 1));
    end = std::max(0, std::min(end, static_cast<int>(lines_.size()) - 1));
    
    if (start <= end && start < static_cast<int>(lines_.size())) {
        lines_.erase(lines_.begin() + start, lines_.begin() + end + 1);
    }
}

void Editor::replace_line(int line_num, const std::string& text) {
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

void Editor::list_lines(const LineRange& range) {
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
        char line_num_str[16];
        snprintf(line_num_str, sizeof(line_num_str), "%04d  ", line_num);
        screen_.write_line(screen_row++, std::string(line_num_str) + lines_[line_num]);
    }
    
    screen_.refresh();
}

std::pair<int, int> Editor::resolve_range(const LineRange& range) const {
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

} // namespace edasm
