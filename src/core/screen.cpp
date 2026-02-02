/**
 * @file screen.cpp
 * @brief Terminal screen management implementation using ncurses
 *
 * Provides simple ncurses wrapper for text-mode screen output.
 * Handles initialization, cleanup, and basic display operations.
 */

#include "edasm/screen.hpp"

#include <curses.h>

namespace edasm {

Screen::Screen() : initialized_(false) {}

Screen::~Screen() {
    shutdown();
}

void Screen::init() {
    if (initialized_) {
        return;
    }
    initscr();
    raw();
    keypad(stdscr, TRUE);
    noecho();
    curs_set(0);
    initialized_ = true;
}

void Screen::shutdown() {
    if (!initialized_) {
        return;
    }
    endwin();
    initialized_ = false;
}

void Screen::clear() {
    if (!initialized_) {
        return;
    }
    ::clear();
}

void Screen::refresh() {
    if (!initialized_) {
        return;
    }
    ::refresh();
}

void Screen::write_line(int row, const std::string &text) {
    if (!initialized_) {
        return;
    }
    if (row < 0 || row >= LINES) {
        return;
    }
    mvaddnstr(row, 0, text.c_str(), COLS - 1);
}

int Screen::get_key() {
    return initialized_ ? getch() : -1;
}

int Screen::rows() const {
    return initialized_ ? LINES : 0;
}

int Screen::cols() const {
    return initialized_ ? COLS : 0;
}

bool Screen::is_initialized() const {
    return initialized_;
}

} // namespace edasm
