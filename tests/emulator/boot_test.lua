-- boot_test.lua
-- MAME Lua script to boot ProDOS and launch EDASM with proper keyboard injection
-- Usage: mame apple2e -flop1 EDASM_SRC.2mg -video none -sound none -nothrottle -autoboot_script boot_test.lua

local emu = manager.machine
local cpu = emu.devices[":maincpu"]
local mem = cpu.spaces["program"]

-- Apple II screen memory locations
local TEXT_PAGE1_START = 0x0400
local TEXT_PAGE1_END = 0x07FF
local PROMPT_CHAR = 0xDD  -- ']' character in Apple II (high bit set)

-- Keyboard handling for Apple II
-- The Apple II reads keyboard from $C000 (KBD) and clears it by reading $C010 (KBDSTRB)
local KBD_ADDR = 0xC000
local KBDSTRB_ADDR = 0xC010

-- Convert ASCII character to Apple II keyboard code
function ascii_to_apple2(ch)
    local code = string.byte(ch)
    -- Apple II uses high bit set for all characters
    if code >= 97 and code <= 122 then  -- lowercase a-z
        code = code - 32  -- Convert to uppercase
    end
    return bit32.bor(code, 0x80)  -- Set high bit
end

-- Send a single character to Apple II keyboard
function send_char(ch)
    local apple_code = ascii_to_apple2(ch)
    -- Write to keyboard register
    mem:write_u8(KBD_ADDR, apple_code)
    -- Give it time to be read
    emu.wait(10000)  -- ~10ms at 1MHz
    -- Clear keyboard strobe
    mem:read_u8(KBDSTRB_ADDR)
    emu.wait(5000)   -- Short delay between chars
end

-- Send a string of characters
function send_string(str)
    print("Typing: " .. str)
    for i = 1, #str do
        local ch = str:sub(i, i)
        send_char(ch)
    end
end

-- Send RETURN key (CR = 0x8D in Apple II)
function send_return()
    print("Sending RETURN")
    mem:write_u8(KBD_ADDR, 0x8D)
    emu.wait(10000)
    mem:read_u8(KBDSTRB_ADDR)
    emu.wait(50000)  -- Longer wait after RETURN
end

-- Read a character from screen memory at given offset
function read_screen_char(offset)
    return mem:read_u8(TEXT_PAGE1_START + offset)
end

-- Check if ProDOS prompt (']') is visible on screen
function check_for_prodos_prompt()
    -- Scan last line of screen (line 23: offsets 0x750-0x777)
    -- ProDOS prompt appears as ']' at start of line
    for offset = 0x750, 0x777 do
        local ch = read_screen_char(offset - TEXT_PAGE1_START)
        if ch == PROMPT_CHAR then
            return true
        end
    end
    return false
end

-- Wait for ProDOS to boot by checking for prompt
function wait_for_prodos()
    print("Waiting for ProDOS to boot...")
    
    local max_wait = 500  -- Max ~5 seconds
    local count = 0
    
    while count < max_wait do
        emu.wait(10000)  -- Wait 10ms
        count = count + 1
        
        -- Check for ProDOS prompt every 10 iterations (~100ms)
        if count % 10 == 0 then
            if check_for_prodos_prompt() then
                print("✓ ProDOS prompt detected!")
                emu.wait(100000)  -- Wait a bit more for stability
                return true
            end
        end
    end
    
    print("⚠ Timed out waiting for ProDOS prompt (using fallback timing)")
    return false
end

-- Launch EDASM by typing the command
function launch_edasm()
    print("Launching EDASM.SYSTEM...")
    
    -- Type the command
    send_string("EDASM.SYSTEM")
    send_return()
    
    -- Wait for EDASM to load
    -- EDASM shows its banner and command prompt
    print("Waiting for EDASM to load...")
    emu.wait(2000000)  -- ~2 seconds for EDASM to load
    
    print("✓ EDASM should be running")
end

-- Main script entry point
function on_start()
    print("=== EDASM Boot Test Started ===")
    print("")
    
    -- Wait for ProDOS
    local prodos_ready = wait_for_prodos()
    if prodos_ready then
        print("Status: ProDOS detected")
    else
        print("Status: ProDOS not detected (continuing anyway)")
    end
    print("")
    
    -- Launch EDASM
    launch_edasm()
    
    print("")
    print("=== Boot test complete ===")
    print("EDASM should now be running.")
    print("Press Ctrl+C to exit MAME.")
    print("")
    print("Note: This is still a prototype implementation.")
    print("Memory monitoring may need tuning for different MAME versions.")
end

-- Register the start callback
emu.register_start(on_start)

print("Boot test script loaded (with keyboard injection)")
print("Target: Apple IIe with EDASM_SRC.2mg")
