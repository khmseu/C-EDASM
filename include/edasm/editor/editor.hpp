#pragma once

#include <optional>
#include <string>
#include <vector>

namespace edasm {

class Screen;

// Line range for LIST, DELETE, etc. (from EDASM editor commands)
struct LineRange {
    std::optional<int> start; // nullopt means "from beginning"
    std::optional<int> end;   // nullopt means "to end"

    static LineRange parse(const std::string &range_str);
    bool is_all() const {
        return !start.has_value() && !end.has_value();
    }
};

class Editor {
  public:
    explicit Editor(Screen &screen);

    // Buffer management (from EDITOR*.S)
    void open_buffer(const std::string &text);
    void clear_buffer();
    void load_file(const std::string &path);
    void save_file(const std::string &path);

    // Line editing (from EDITOR2.S, EDITOR3.S)
    void insert_line(int line_num, const std::string &text);
    void delete_line(int line_num);
    void delete_range(const LineRange &range);
    void replace_line(int line_num, const std::string &text);

    // Search and replace (from EDITOR2.S LD865, LD8C6)
    struct FindResult {
        bool found{false};
        int line_num{-1};
        size_t pos{0};
    };
    FindResult find(const std::string &pattern, const LineRange &range, int start_line = 0);
    int change(const std::string &old_text, const std::string &new_text, const LineRange &range,
               bool all = true);

    // Buffer manipulation (from EDITOR2.S LD819)
    void move_lines(const LineRange &src_range, int dest_line);
    void copy_lines(const LineRange &src_range, int dest_line);

    // Line operations
    void join_lines(const LineRange &range);
    void split_line(int line_num, size_t pos);

    // Display
    void render();
    void list_lines(const LineRange &range);

    // Access
    const std::vector<std::string> &lines() const;
    std::string joined_buffer() const;
    int line_count() const {
        return static_cast<int>(lines_.size());
    }

  private:
    Screen &screen_;
    std::vector<std::string> lines_;

    // Helper to convert range to actual line indices
    std::pair<int, int> resolve_range(const LineRange &range) const;
};

} // namespace edasm
