/**
 * @file main.cpp
 * @brief Main entry point for EDASM CLI application
 * 
 * Simple entry point that creates and runs the App instance.
 */

#include "edasm/app.hpp"

int main(int argc, char **argv) {
    edasm::App app;
    return app.run(argc, argv);
}
