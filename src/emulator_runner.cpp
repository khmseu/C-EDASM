#include "edasm/cpu.hpp"
#include "edasm/bus.hpp"
#include "edasm/traps.hpp"
#include "edasm/host_shims.hpp"
#include <iostream>
#include <iomanip>
#include <string>

using namespace edasm;

int main(int argc, char* argv[]) {
    std::cout << "C-EDASM Minimal Emulator" << std::endl;
    std::cout << "========================" << std::endl << std::endl;
    
    // Parse command line
    std::string binary_path = "third_party/EdAsm/EDASM.SYSTEM";
    uint16_t load_addr = 0x2000;
    uint16_t entry_point = 0x2000;
    size_t max_instructions = 1000;
    bool trace = false;
    
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--binary" && i + 1 < argc) {
            binary_path = argv[++i];
        } else if (arg == "--load" && i + 1 < argc) {
            load_addr = static_cast<uint16_t>(std::stoul(argv[++i], nullptr, 16));
        } else if (arg == "--entry" && i + 1 < argc) {
            entry_point = static_cast<uint16_t>(std::stoul(argv[++i], nullptr, 16));
        } else if (arg == "--max" && i + 1 < argc) {
            max_instructions = std::stoul(argv[++i]);
        } else if (arg == "--trace") {
            trace = true;
        } else if (arg == "--help") {
            std::cout << "Usage: " << argv[0] << " [options]" << std::endl;
            std::cout << "Options:" << std::endl;
            std::cout << "  --binary <path>   Binary file to load (default: third_party/EdAsm/EDASM.SYSTEM)" << std::endl;
            std::cout << "  --load <addr>     Load address in hex (default: 2000)" << std::endl;
            std::cout << "  --entry <addr>    Entry point in hex (default: 2000)" << std::endl;
            std::cout << "  --max <n>         Max instructions to execute (default: 1000)" << std::endl;
            std::cout << "  --trace           Enable instruction tracing" << std::endl;
            std::cout << "  --help            Show this help" << std::endl;
            return 0;
        }
    }
    
    // Initialize emulator
    Bus bus;
    CPU cpu(bus);
    HostShims shims;
    
    std::cout << "Initializing emulator..." << std::endl;
    std::cout << "  Memory: 64KB filled with trap opcode ($02)" << std::endl;
    
    // Reset bus (fills with trap opcode)
    bus.reset();
    
    // Install host shims
    shims.install_io_traps(bus);
    std::cout << "  I/O traps installed at $C000 (KBD) and $C010 (KBDSTRB)" << std::endl;
    
    // Try to load binary
    std::cout << "  Loading binary: " << binary_path << std::endl;
    std::cout << "  Load address: $" << std::hex << std::uppercase << std::setw(4) 
              << std::setfill('0') << load_addr << std::endl;
    
    if (!bus.load_binary_from_file(load_addr, binary_path)) {
        std::cerr << "Error: Failed to load binary file: " << binary_path << std::endl;
        std::cerr << "Note: EDASM.SYSTEM binary should be extracted from EDASM_SRC.2mg" << std::endl;
        std::cerr << "      For now, this emulator will just demonstrate trap behavior." << std::endl;
    } else {
        std::cout << "  Binary loaded successfully" << std::endl;
    }
    
    // Set entry point
    cpu.state().PC = entry_point;
    std::cout << "  Entry point: $" << std::hex << std::uppercase << std::setw(4) 
              << std::setfill('0') << entry_point << std::endl;
    
    // Set trap handler
    cpu.set_trap_handler(TrapManager::default_trap_handler);
    std::cout << "  Trap handler installed" << std::endl;
    
    std::cout << std::endl << "Starting execution..." << std::endl;
    std::cout << "Maximum instructions: " << std::dec << max_instructions << std::endl;
    if (trace) {
        std::cout << "Tracing enabled" << std::endl;
    }
    std::cout << std::endl;
    
    // Run emulator
    size_t count = 0;
    bool running = true;
    
    while (running && count < max_instructions) {
        if (trace) {
            std::cout << "[" << std::dec << count << "] "
                      << TrapManager::dump_cpu_state(cpu.state()) << std::endl;
        }
        
        running = cpu.step();
        count++;
    }
    
    std::cout << std::endl;
    std::cout << "Execution stopped after " << std::dec << count << " instructions" << std::endl;
    std::cout << "Final CPU state:" << std::endl;
    std::cout << TrapManager::dump_cpu_state(cpu.state()) << std::endl;
    
    if (running) {
        std::cout << std::endl << "Reached maximum instruction limit" << std::endl;
        return 1;
    } else {
        std::cout << std::endl << "Halted by trap handler" << std::endl;
        return 0;
    }
}
