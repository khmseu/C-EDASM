#pragma once

#include <string>
#include <vector>
#include <optional>

namespace edasm {

class Screen;

// Line range for LIST, DELETE, etc. (from EDASM editor commands)
struct LineRange {
    std::optional<int> start;  // nullopt means "from beginning"
    std::optional<int> end;    // nullopt means "to end"
    
    static LineRange parse(const std::string& range_str);
    bool is_all() const { return !start.has_value() && !end.has_value(); }
};

class Editor {
  public:
    explicit Editor(Screen &screen);

    // Buffer management (from EDITOR*.S)
    void open_buffer(const std::string &text);
    void clear_buffer();
    void load_file(const std::string& path);
    void save_file(const std::string& path);
    
    // Line editing
    void insert_line(int line_num, const std::string& text);
    void delete_line(int line_num);
    void delete_range(const LineRange& range);
    void replace_line(int line_num, const std::string& text);
    
    // Display
    void render();
    void list_lines(const LineRange& range);
    
    // Access
    const std::vector<std::string> &lines() const;
    std::string joined_buffer() const;
    int line_count() const { return static_cast<int>(lines_.size()); }

  private:
    Screen &screen_;
    std::vector<std::string> lines_;
    
    // Helper to convert range to actual line indices
    std::pair<int, int> resolve_range(const LineRange& range) const;
};

} // namespace edasm
