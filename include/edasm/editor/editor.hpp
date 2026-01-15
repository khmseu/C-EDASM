#pragma once

#include <string>
#include <vector>

namespace edasm {

class Screen;

class Editor {
  public:
    explicit Editor(Screen &screen);

    void open_buffer(const std::string &text);
    void render();
    const std::vector<std::string> &lines() const;
    std::string joined_buffer() const;

  private:
    Screen &screen_;
    std::vector<std::string> lines_;
};

} // namespace edasm
