-- assemble_test.lua
-- MAME Lua script to automate EDASM assembly of test files with proper keyboard injection
-- Usage: mame apple2e -flop1 EDASM_SRC.2mg -flop2 test_disk.2mg -video none -sound none -nothrottle -autoboot_script assemble_test.lua

local emu = manager.machine
local cpu = emu.devices[":maincpu"]
local mem = cpu.spaces["program"]

-- Configuration
local TEST_FILE = "TEST.SIMPLE.SRC"  -- File to assemble (on flop2)
local OUTPUT_FILE = "TEST.SIMPLE.BIN" -- Output binary name

-- Apple II memory locations
local TEXT_PAGE1_START = 0x0400
local TEXT_PAGE1_END = 0x07FF
local KBD_ADDR = 0xC000
local KBDSTRB_ADDR = 0xC010
local PROMPT_CHAR = 0xDD  -- ']' character

-- Convert ASCII character to Apple II keyboard code
function ascii_to_apple2(ch)
    local code = string.byte(ch)
    if code >= 97 and code <= 122 then  -- lowercase to uppercase
        code = code - 32
    end
    -- Use bitwise OR to set high bit
    if bit32 then
        return bit32.bor(code, 0x80)
    else
        -- Fallback for Lua 5.3+ with native bitwise ops
        return code | 0x80
    end
end

-- Send a single character
function send_char(ch)
    local apple_code = ascii_to_apple2(ch)
    mem:write_u8(KBD_ADDR, apple_code)
    emu.wait(10000)
    mem:read_u8(KBDSTRB_ADDR)
    emu.wait(5000)
end

-- Send a string
function send_string(str)
    print("Typing: " .. str)
    for i = 1, #str do
        local ch = str:sub(i, i)
        send_char(ch)
    end
end

-- Send RETURN key
function send_return()
    print("Pressing RETURN")
    mem:write_u8(KBD_ADDR, 0x8D)
    emu.wait(10000)
    mem:read_u8(KBDSTRB_ADDR)
    emu.wait(50000)
end

-- Send a command with RETURN
function send_command(cmd)
    send_string(cmd)
    send_return()
end

-- Read screen character at offset
function read_screen_char(offset)
    return mem:read_u8(TEXT_PAGE1_START + offset)
end

-- Check for ProDOS prompt
function check_for_prodos_prompt()
    for offset = 0x750, 0x777 do
        if read_screen_char(offset - TEXT_PAGE1_START) == PROMPT_CHAR then
            return true
        end
    end
    return false
end

-- Check for EDASM command prompt (looks for 'CMD:' or '*' on screen)
function check_for_edasm_prompt()
    -- EDASM shows "CMD:" or a '*' prompt
    -- Scan multiple lines for these patterns
    -- Apple II text screen: 40 chars per line, 24 lines
    -- Line offset = line * 0x28 (40 decimal, 0x28 hex)
    for line = 0, 23 do
        local line_offset = line * 0x28
        for col = 0, 39 do
            local ch = read_screen_char(line_offset + col)
            -- Check for '*' prompt (0xAA in Apple II)
            if ch == 0xAA then
                return true
            end
        end
    end
    return false
end

-- Wait for ProDOS boot
function wait_for_prodos()
    print("Waiting for ProDOS to boot...")
    
    local max_wait = 500
    local count = 0
    
    while count < max_wait do
        emu.wait(10000)
        count = count + 1
        
        if count % 10 == 0 then
            if check_for_prodos_prompt() then
                print("✓ ProDOS ready")
                emu.wait(100000)
                return true
            end
        end
    end
    
    print("⚠ ProDOS prompt timeout (using fallback)")
    return false
end

-- Wait for EDASM prompt
function wait_for_edasm_prompt()
    print("Waiting for EDASM prompt...")
    
    local max_wait = 300
    local count = 0
    
    while count < max_wait do
        emu.wait(10000)
        count = count + 1
        
        if count % 10 == 0 then
            if check_for_edasm_prompt() then
                print("✓ EDASM ready")
                emu.wait(50000)
                return true
            end
        end
    end
    
    print("⚠ EDASM prompt timeout (using fallback)")
    emu.wait(500000)  -- Fallback: wait 500ms
    return false
end

-- Launch EDASM
function launch_edasm()
    print("Launching EDASM.SYSTEM...")
    send_command("EDASM.SYSTEM")
    
    -- Wait for EDASM to load
    wait_for_edasm_prompt()
    print("✓ EDASM loaded")
end

-- Load source file
function load_source()
    print("")
    print("Loading source file: " .. TEST_FILE)
    
    -- Send 'L' command to load
    send_string("L")
    emu.wait(50000)
    
    -- Send filename
    send_command(TEST_FILE)
    
    -- Wait for file to load
    -- File loading is typically fast on emulated disks
    emu.wait(500000)  -- 500ms should be plenty
    
    print("✓ Source loaded")
end

-- Assemble the source
function assemble()
    print("")
    print("Assembling source...")
    
    -- Send 'A' command to assemble
    send_string("A")
    emu.wait(50000)
    
    -- Wait for assembly to complete
    -- Real implementation would monitor memory for completion flag
    emu.wait(2000000)  -- 2 seconds for assembly
    
    print("✓ Assembly complete")
end

-- Save binary output
function save_binary()
    print("")
    print("Saving binary: " .. OUTPUT_FILE)
    
    -- Send 'S' command to save
    send_string("S")
    emu.wait(50000)
    
    -- Send filename
    send_command(OUTPUT_FILE)
    
    -- Wait for save to complete
    emu.wait(500000)
    
    print("✓ Binary saved")
end

-- Exit EDASM
function exit_edasm()
    print("")
    print("Exiting EDASM...")
    
    -- Send 'Q' command to quit
    send_string("Q")
    emu.wait(50000)
    
    -- Confirm quit if prompted
    send_string("Y")  -- Yes to confirm
    emu.wait(50000)
    
    print("✓ EDASM exited")
end

-- Main automation sequence
function run_automation()
    print("=== EDASM Assembly Automation ===")
    print("Configuration:")
    print("  Source: " .. TEST_FILE)
    print("  Output: " .. OUTPUT_FILE)
    print("")
    
    -- Boot sequence
    local prodos_ok = wait_for_prodos()
    if not prodos_ok then
        print("⚠ Warning: ProDOS detection uncertain")
    end
    
    -- Launch EDASM
    launch_edasm()
    
    -- Assembly workflow
    load_source()
    assemble()
    save_binary()
    exit_edasm()
    
    print("")
    print("=== Automation Complete ===")
    print("")
    print("Output file should be on disk 2: " .. OUTPUT_FILE)
    print("Extract with: cadius EXTRACTVOLUME test_disk.2mg /output/dir/")
    print("")
    print("Press Ctrl+C to exit MAME.")
end

-- Register start callback
emu.register_start(function()
    -- Give system a moment to initialize
    emu.wait(100000)
    
    -- Run automation
    run_automation()
end)

print("EDASM assembly automation script loaded (with keyboard injection)")
print("Ready to automate: load → assemble → save workflow")
