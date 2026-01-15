#include <cassert>
#include <iostream>
#include <string>

#include "edasm/editor/editor.hpp"
#include "edasm/screen.hpp"

using namespace edasm;

void test_line_range_parsing() {
    std::cout << "Testing LineRange parsing..." << std::endl;
    
    // Test single line
    auto r1 = LineRange::parse("10");
    assert(r1.start == 10);
    assert(r1.end == 10);
    
    // Test range
    auto r2 = LineRange::parse("10,20");
    assert(r2.start == 10);
    assert(r2.end == 20);
    
    // Test open start
    auto r3 = LineRange::parse(",20");
    assert(!r3.start.has_value());
    assert(r3.end == 20);
    
    // Test open end
    auto r4 = LineRange::parse("10,");
    assert(r4.start == 10);
    assert(!r4.end.has_value());
    
    // Test empty (all)
    auto r5 = LineRange::parse("");
    assert(r5.is_all());
    
    std::cout << "  ✓ LineRange parsing tests passed" << std::endl;
}

void test_basic_editor_operations() {
    std::cout << "Testing basic editor operations..." << std::endl;
    
    // Create screen (won't actually initialize ncurses in test)
    Screen screen;
    Editor editor(screen);
    
    // Test insert
    editor.insert_line(0, "Line 1");
    editor.insert_line(1, "Line 2");
    editor.insert_line(2, "Line 3");
    assert(editor.line_count() == 3);
    
    // Test delete
    editor.delete_line(1);
    assert(editor.line_count() == 2);
    assert(editor.lines()[0] == "Line 1");
    assert(editor.lines()[1] == "Line 3");
    
    // Test replace
    editor.replace_line(0, "Modified Line 1");
    assert(editor.lines()[0] == "Modified Line 1");
    
    std::cout << "  ✓ Basic editor operations tests passed" << std::endl;
}

void test_find() {
    std::cout << "Testing FIND command..." << std::endl;
    
    Screen screen;
    Editor editor(screen);
    
    editor.insert_line(0, "Hello World");
    editor.insert_line(1, "Test Line");
    editor.insert_line(2, "Hello Again");
    
    // Find "Hello"
    auto result1 = editor.find("Hello", LineRange{});
    assert(result1.found);
    assert(result1.line_num == 0);
    assert(result1.pos == 0);
    
    // Find "Again"
    auto result2 = editor.find("Again", LineRange{});
    assert(result2.found);
    assert(result2.line_num == 2);
    
    // Find non-existent
    auto result3 = editor.find("NotFound", LineRange{});
    assert(!result3.found);
    
    std::cout << "  ✓ FIND tests passed" << std::endl;
}

void test_change() {
    std::cout << "Testing CHANGE command..." << std::endl;
    
    Screen screen;
    Editor editor(screen);
    
    editor.insert_line(0, "Hello World");
    editor.insert_line(1, "Hello Test");
    editor.insert_line(2, "Goodbye World");
    
    // Change all "Hello" to "Hi"
    int count = editor.change("Hello", "Hi", LineRange{}, true);
    assert(count == 2);
    assert(editor.lines()[0] == "Hi World");
    assert(editor.lines()[1] == "Hi Test");
    assert(editor.lines()[2] == "Goodbye World");
    
    std::cout << "  ✓ CHANGE tests passed" << std::endl;
}

void test_copy_move() {
    std::cout << "Testing COPY and MOVE commands..." << std::endl;
    
    Screen screen;
    Editor editor(screen);
    
    editor.insert_line(0, "Line 0");
    editor.insert_line(1, "Line 1");
    editor.insert_line(2, "Line 2");
    editor.insert_line(3, "Line 3");
    
    // Test COPY
    Screen screen_copy;
    Editor editor_copy(screen_copy);
    editor_copy.insert_line(0, "Line 0");
    editor_copy.insert_line(1, "Line 1");
    editor_copy.insert_line(2, "Line 2");
    editor_copy.insert_line(3, "Line 3");
    
    LineRange range{0, 1};
    editor_copy.copy_lines(range, 4);
    assert(editor_copy.line_count() == 6);
    assert(editor_copy.lines()[4] == "Line 0");
    assert(editor_copy.lines()[5] == "Line 1");
    
    // Test MOVE
    Screen screen_move;
    Editor editor_move(screen_move);
    editor_move.insert_line(0, "Line 0");
    editor_move.insert_line(1, "Line 1");
    editor_move.insert_line(2, "Line 2");
    editor_move.insert_line(3, "Line 3");
    
    // Move lines 0-1 to position 3
    // After extraction: ["Line 2", "Line 3"]
    // Insert at adjusted position: ["Line 2", "Line 0", "Line 1", "Line 3"]
    editor_move.move_lines(range, 3);
    assert(editor_move.line_count() == 4);
    assert(editor_move.lines()[0] == "Line 2");
    assert(editor_move.lines()[1] == "Line 0");
    assert(editor_move.lines()[2] == "Line 1");
    assert(editor_move.lines()[3] == "Line 3");
    
    std::cout << "  ✓ COPY and MOVE tests passed" << std::endl;
}

void test_join_split() {
    std::cout << "Testing JOIN and SPLIT commands..." << std::endl;
    
    Screen screen;
    Editor editor(screen);
    
    editor.insert_line(0, "Line");
    editor.insert_line(1, "One");
    editor.insert_line(2, "Two");
    
    // Test JOIN
    LineRange range{0, 1};
    editor.join_lines(range);
    assert(editor.line_count() == 2);
    assert(editor.lines()[0] == "Line One");
    
    // Test SPLIT
    editor.split_line(0, 4);
    assert(editor.line_count() == 3);
    assert(editor.lines()[0] == "Line");
    assert(editor.lines()[1] == " One");
    
    std::cout << "  ✓ JOIN and SPLIT tests passed" << std::endl;
}

int main() {
    std::cout << "Running Editor Tests" << std::endl;
    std::cout << "===================" << std::endl;
    
    try {
        test_line_range_parsing();
        test_basic_editor_operations();
        test_find();
        test_change();
        test_copy_move();
        test_join_split();
        
        std::cout << std::endl;
        std::cout << "✓ All editor tests passed!" << std::endl;
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "✗ Test failed: " << e.what() << std::endl;
        return 1;
    }
}
