-- assemble_test.lua
-- MAME Lua script to automate EDASM assembly of test files
-- Usage: mame apple2e -flop1 EDASM_SRC.2mg -flop2 test_disk.2mg -video none -sound none -nothrottle -autoboot_script assemble_test.lua

local emu = manager.machine
local cpu = emu.devices[":maincpu"]
local mem = cpu.spaces["program"]
local kbd = emu.ioport

-- Configuration
local TEST_FILE = "TEST.SIMPLE.SRC"  -- File to assemble (on flop2)
local OUTPUT_FILE = "TEST.SIMPLE.BIN" -- Output binary name

-- Helper function to wait for cycles
function wait_cycles(cycles)
    emu.wait(cycles)
end

-- Helper function to simulate keystroke
-- Note: This is a simplified version. Real implementation needs proper
-- keyboard matrix handling for Apple II
function send_key(key)
    -- In real implementation, this would:
    -- 1. Set appropriate keyboard matrix bits
    -- 2. Wait for EDASM to read the key
    -- 3. Clear the matrix
    print("Sending key: " .. key)
    wait_cycles(1000)
end

-- Send a string of keys
function send_string(str)
    print("Typing: " .. str)
    for i = 1, #str do
        local c = str:sub(i, i)
        send_key(c)
    end
end

-- Send return key
function send_return()
    print("Sending RETURN")
    wait_cycles(1000)
end

-- Wait for ProDOS boot
function wait_for_prodos()
    print("Waiting for ProDOS to boot...")
    -- Check for ProDOS prompt in screen memory
    -- Screen memory starts at $0400 for text page 1
    
    local max_wait = 500
    local count = 0
    
    while count < max_wait do
        wait_cycles(10000)
        count = count + 1
        
        -- Could check memory here for ProDOS prompt
        -- For now, just wait a fixed time
    end
    
    print("ProDOS boot complete (assumed)")
end

-- Launch EDASM
function launch_edasm()
    print("Launching EDASM...")
    send_string("EDASM.SYSTEM")
    send_return()
    
    -- Wait for EDASM to load
    wait_cycles(2000000)  -- ~2 seconds
    print("EDASM loaded (assumed)")
end

-- Load source file in EDASM
function load_source()
    print("Loading source file: " .. TEST_FILE)
    
    -- In EDASM, use 'L' command to load
    send_key("L")  -- Load command
    wait_cycles(50000)
    
    send_string(TEST_FILE)
    send_return()
    
    -- Wait for file to load
    wait_cycles(1000000)
    print("Source file loaded (assumed)")
end

-- Assemble the source
function assemble()
    print("Assembling source...")
    
    -- In EDASM, use 'A' command to assemble
    send_key("A")  -- Assemble command
    wait_cycles(50000)
    
    -- Wait for assembly to complete
    -- Real implementation would monitor memory or screen for completion
    wait_cycles(5000000)  -- ~5 seconds for assembly
    
    print("Assembly complete (assumed)")
end

-- Save binary output
function save_binary()
    print("Saving binary: " .. OUTPUT_FILE)
    
    -- In EDASM, use 'S' command to save
    send_key("S")  -- Save command
    wait_cycles(50000)
    
    send_string(OUTPUT_FILE)
    send_return()
    
    -- Wait for save to complete
    wait_cycles(1000000)
    print("Binary saved (assumed)")
end

-- Exit EDASM and ProDOS
function exit_edasm()
    print("Exiting EDASM...")
    send_key("Q")  -- Quit command (if supported)
    wait_cycles(500000)
    print("EDASM exited")
end

-- Main automation sequence
function run_automation()
    print("=== EDASM Automation Started ===")
    print("Test file: " .. TEST_FILE)
    print("Output file: " .. OUTPUT_FILE)
    print("")
    
    wait_for_prodos()
    launch_edasm()
    load_source()
    assemble()
    save_binary()
    exit_edasm()
    
    print("")
    print("=== Automation Complete ===")
    print("Binary should be saved to disk 2 as: " .. OUTPUT_FILE)
    print("Use diskm8 or AppleCommander to extract it")
    print("")
    print("Emulator will continue running. Press Ctrl+C to exit.")
end

-- Register start callback
emu.register_start(function()
    -- Give ProDOS a moment to initialize
    wait_cycles(100000)
    run_automation()
end)

print("EDASM assembly automation script loaded")
print("Note: This is a prototype. Keyboard handling needs proper implementation.")
print("See MAME Lua API documentation for emu.keypost() and ioport usage.")
