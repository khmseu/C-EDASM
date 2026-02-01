---@diagnostic disable: lowercase-global
-- boot_test.lua
-- MAME Lua script to boot ProDOS and launch EDASM with proper keyboard injection
-- Usage: mame apple2e -flop1 EDASM_SRC.2mg -video none -sound none -nothrottle -autoboot_script boot_test.lua

local machine = manager.machine
local cpu = machine.devices[":maincpu"]
local mem = cpu.spaces["program"]
local video = machine.video

-- Apple II screen memory locations
local TEXT_PAGE1_START = 0x0400
-- Take a snapshot to the default snap directory
local snap_counter = 0
-- function snap(label)
--     snap_counter = snap_counter + 1
--     local tag = string.format("%04d.png %s", snap_counter, label or "snap")
--     print("Snapshot: " .. tag)
--     if video and video:snapshot() then
--         -- MAME names files automatically; tag is just for log readability
--     end
-- end
function snap(label, comment)
    snap_counter = snap_counter + 1
    local tag = string.format("%04d_%s.log", snap_counter, label or "snap")
    print("Snapshot: " .. tag .. (comment and (" - " .. comment) or ""))
    read_text_screen(tag)
end

-- Wait with snapshots before and after
function wait_with_snap(usec, label)
    local atto = emu.attotime.from_usec(usec)
    snap("before_" .. (label or "wait"), string.format("(%gs)", atto:as_double()))
    emu.wait(atto)
    snap("after_" .. (label or "wait"))
end

local TEXT_PAGE1_END = 0x07FF
local PROMPT_CHAR = 0xDD -- ']' character in Apple II (high bit set)

-- Keyboard handling for Apple II
-- The Apple II reads keyboard from $C000 (KBD) and clears it by reading $C010 (KBDSTRB)
local KBD_ADDR = 0xC000
local KBDSTRB_ADDR = 0xC010

-- Convert ASCII character to Apple II keyboard code
function ascii_to_apple2(ch)
    local code = string.byte(ch)
    -- Apple II uses high bit set for all characters
    if code >= 97 and code <= 122 then -- lowercase a-z
        code = code - 32               -- Convert to uppercase
    end
    -- Use bitwise OR to set high bit (Lua 5.3+ or bit32 library)
    -- bit32 is available in MAME's Lua environment
    if bit32 then
        return bit32.bor(code, 0x80)
    else
        -- Fallback for Lua 5.3+ with native bitwise ops
        return code | 0x80
    end
end

-- Send a single character to Apple II keyboard
function send_char(ch)
    local apple_code = ascii_to_apple2(ch)
    -- Write to keyboard register
    mem:write_u8(KBD_ADDR, apple_code)
    -- Give it time to be read
    wait_with_snap(10000, "char") -- ~10ms at 1MHz
    -- Clear keyboard strobe
    mem:read_u8(KBDSTRB_ADDR)
    wait_with_snap(5000, "char_gap") -- Short delay between chars
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
    wait_with_snap(10000, "ret1")
    mem:read_u8(KBDSTRB_ADDR)
    wait_with_snap(50000, "ret2") -- Longer wait after RETURN
end

-- Send SPACE key
function send_space()
    print("Sending SPACE")
    mem:write_u8(KBD_ADDR, 0xA0) -- ' ' with high bit set
    wait_with_snap(10000, "sp1")
    mem:read_u8(KBDSTRB_ADDR)
    wait_with_snap(20000, "sp2")
end

-- Read a character from screen memory at given offset
function read_screen_char(offset)
    return mem:read_u8(TEXT_PAGE1_START + offset)
end

-- Store last screen content to avoid duplicate writes
local last_screen_content = nil

-- Function to read and print the Apple II text screen
function read_text_screen(label)
    local output = {}
    table.insert(output, '+' .. string.rep("-", 40) .. '+')
    -- Use Apple II memory layout: 8 blocks of 128 bytes, each maps to 3 rows of 40 bytes
    for line = 0, 23 do
        local line_text = "|"
        local line_offset = (line % 8) * 128 + math.floor(line / 8) * 40
        for col = 0, 39 do
            local char_addr = TEXT_PAGE1_START + line_offset + col
            local raw = mem:read_u8(char_addr)
            local char = raw % 128 -- lower 7 bits
            if char < 32 then
                char = raw|64;
            end
            line_text = line_text .. string.char(char)
        end
        line_text = line_text .. "|"
        table.insert(output, line_text)
    end
    table.insert(output, '+' .. string.rep("-", 40) .. '+')

    -- -- Print to console
    -- for _, line in ipairs(output) do
    --     print(line)
    -- end

    -- Write to log file if label is provided and content has changed
    if not label then
        label = 'dump.log'
    end

    -- Convert output to string for comparison
    local current_content = table.concat(output, "\n")

    -- Only write if content is different from last time
    if current_content ~= last_screen_content then
        local filename = "tmp/" .. label
        local file = io.open(filename, "a")
        if file then
            for _, line in ipairs(output) do
                file:write(line .. "\n")
            end
            file:close()
            last_screen_content = current_content
        end
    end
end

-- Check if BitsyBye menu is visible on screen
function check_for_bitsybye_menu()
    -- BitsyBye menu shows first line like "S6,D1:/PRODOS.2.4.3" (ignore version)
    local first_line_offset = (0 % 8) * 128 + math.floor(0 / 8) * 40
    local first_line_text = ""
    for col = 0, 39 do
        local ch = read_screen_char(first_line_offset + col)
        local char = ch % 128       -- lower 7 bits
        if char < 32 then
            char = string.byte(" ") -- Convert control chars to space
        end
        first_line_text = first_line_text .. string.char(char)
    end

    -- Look for pattern like "S6,D1:/PRODOS" (ignore version number)
    if string.match(first_line_text, "S[0-9],D[0-9]:/PRODOS") then
        print("Found BitsyBye menu: " .. string.gsub(first_line_text, "%s+$", "")) -- trim trailing spaces
        return true
    end

    return false
end

-- Check if ProDOS prompt (']') is visible on screen
function check_for_prodos_prompt()
    -- read_text_screen()
    -- Scan last text line (line 23) using the 0x28/128 mapping
    local last_line = 23
    local last_line_offset = (last_line % 8) * 128 + math.floor(last_line / 8) * 40
    for col = 0, 39 do
        local ch = read_screen_char(last_line_offset + col)
        if ch == PROMPT_CHAR then
            print(string.format("Found ProDOS prompt at line %d, col %d (addr 0x%03X)", last_line, col,
                TEXT_PAGE1_START + last_line_offset + col))
            return true
        end
    end
    -- print("Did not find prodos prompt")
    return false
end

-- Wait for ProDOS; if BitsyBye menu is present, type BASIC.SYSTEM then wait for prompt
function wait_for_prodos()
    print("Waiting for ProDOS (BitsyBye or BASIC prompt)...")

    -- First, if a "Press any key" prompt is up, send SPACE early
    -- wait_with_snap(500000, "pre_space") -- ~5s to reach the prompt
    -- send_space()
    wait_with_snap(200000, "post_space")

    -- local max_wait = 500  -- ~5s initial wait for prompt
    local max_wait = 200 -- ~20s initial wait for prompt
    for i = 1, max_wait do
        wait_with_snap(100000, "wait_prompt" .. string.format("_%03d", i))
        if i % 10 == 0 then
            if check_for_prodos_prompt() then
                print("✓ ProDOS prompt detected (no menu)")
                wait_with_snap(100000, "post_prompt")
                return true
            elseif check_for_bitsybye_menu() then
                print("✓ BitsyBye menu detected, navigating to BASIC.SYSTEM")
                break -- Exit loop to proceed with menu navigation
            end
        end
    end

    -- Likely still in BitsyBye menu; navigate via hotkeys: 'b' (Bitsy.boot), 'b' (BASIC.SYSTEM), then RETURN
    print("No prompt; assuming BitsyBye menu, selecting BASIC.SYSTEM via hotkeys (b, b, RETURN)...")
    send_char('b')
    wait_with_snap(200000, "bitsy_b1")
    send_char('b')
    wait_with_snap(200000, "bitsy_b2")
    send_return()

    local max_wait_prompt = 600 -- ~6s to reach BASIC prompt
    for i = 1, max_wait_prompt do
        wait_with_snap(10000, "wait_basic_prompt" .. string.format("_%03d", i))
        if i % 10 == 0 and check_for_prodos_prompt() then
            print("✓ ProDOS prompt detected after BASIC.SYSTEM selection")
            wait_with_snap(100000, "post_basic_prompt")
            return true
        end
    end

    print("⚠ Timed out waiting for ProDOS prompt (after BASIC.SYSTEM selection)")
    return false
end

-- Launch EDASM by typing the command
function launch_edasm()
    print("Launching EDASM from S5,D1:EI/EDASM.SYSTEM via BRUN...")

    -- Type the command (ProDOS BRUN with explicit slot 5 drive 1 path)
    send_string("BRUN S5,D1:EI/EDASM.SYSTEM")
    send_return()

    -- Wait for EDASM to load
    -- EDASM shows its banner and command prompt
    print("Waiting for EDASM to load...")
    wait_with_snap(3000000, "wait_edasm") -- ~3 seconds for EDASM to load

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

function on_stop()
    snap("final_state", "Final screen state at emulator stop")
    print("=== EDASM Boot Test Ended ===")
end

emu.add_machine_stop_notifier(on_stop)

-- Run immediately (register_start is not available on this MAME build)
on_start()

print("Boot test script loaded (with keyboard injection)")
print("Target: Apple IIgs with ProDOS (flop1) + EDASM_SRC.2mg (flop3)")
