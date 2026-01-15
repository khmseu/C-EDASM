#pragma once

#include <string>

namespace edasm {

class Screen {
  public:
    Screen();
    ~Screen();

    Screen(const Screen &) = delete;
    Screen &operator=(const Screen &) = delete;

    void init();
    void shutdown();

    void clear();
    void refresh();
    void write_line(int row, const std::string &text);
    int get_key();

    int rows() const;
    int cols() const;
    bool is_initialized() const;

  private:
    bool initialized_;
};

} // namespace edasm
