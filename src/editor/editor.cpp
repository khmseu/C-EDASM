#include "edasm/editor/editor.hpp"

#include <sstream>

#include "edasm/screen.hpp"

namespace edasm {

Editor::Editor(Screen &screen) : screen_(screen) {}

void Editor::open_buffer(const std::string &text) {
    lines_.clear();
    std::stringstream ss(text);
    std::string line;
    while (std::getline(ss, line)) {
        lines_.push_back(line);
    }
}

void Editor::render() {
    if (!screen_.is_initialized()) {
        return;
    }
    screen_.clear();
    for (int row = 0; row < static_cast<int>(lines_.size()); ++row) {
        screen_.write_line(row, lines_[row]);
    }
    screen_.refresh();
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
