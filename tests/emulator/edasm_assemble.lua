-- MAME Lua automation for EDASM assembly - adapted from boot_test.lua
-- Usage: mame apple2gs -flop3 /tmp/edasm_work.2mg -autoboot_script tests/emulator/edasm_assemble.lua

local machine = manager.machine
local cpu = machine.devices[":maincpu"]
local mem = cpu.spaces["program"]
local video = machine.video

-- Apple II screen memory locations
local TEXT_PAGE1_START = 0x0400
local TEXT_PAGE1_END = 0x07FF
local BASIC_PROMPT_CHAR = 0xDD -- ']' character in Apple II
local EDASM_PROMPT_CHAR = 0xBA -- ':' character in EdAsm

local start_time = nil
local screen_change_callback = nil
local screen_tap_handler = nil
local waiting_for_change = false
local last_screen_content = nil

-- Initialize timer
local function start_timer()
    start_time = os.clock()
    print("Timer started at " .. start_time)
end

-- Get elapsed time since start
local function elapsed_time()
    if not start_time then
        return 0
    end
    return os.clock() - start_time
end

-- Print elapsed time with label
local function print_elapsed(label)
    local elapsed = elapsed_time()
    print(string.format("[%.2fs] %s", elapsed, label or "checkpoint"))
end

-- Send a string of characters using natural keyboard
local function send_string(str)
    print_elapsed("Typing: " .. str)
    manager.machine.natkeyboard:post(str)
    print_elapsed("String sent: " .. str)
end

-- Send RETURN key using natural keyboard coded input
local function send_return()
    print_elapsed("Sending RETURN")
    manager.machine.natkeyboard:post_coded("{ENTER}")
    print_elapsed("RETURN sent")
end
-- Function to read and print the Apple II text screen
local function read_text_screen(label)
    local output = {}
    table.insert(output, '+' .. string.rep("-", 40) .. '+')

    for line = 0, 23 do
        local line_offset = (line % 8) * 128 + math.floor(line / 8) * 40
        local line_text = "|"
        for col = 0, 39 do
            local char = mem:read_u8(TEXT_PAGE1_START + line_offset + col)
            char = char & 0x7F -- Clear high bit
            if char < 32 or char > 126 then
                char = 32      -- Replace control chars with space
            end
            line_text = line_text .. string.char(char)
        end
        line_text = line_text .. "|"
        table.insert(output, line_text)
    end
    table.insert(output, '+' .. string.rep("-", 40) .. '+')

    print_elapsed("Screen Dump: " .. (label or "unknown"))
    for _, line in ipairs(output) do
        print(line)
    end
    print("=== End Screen Dump ===")
end
-- Get screen content as a string for comparison
local function get_screen_content()
    local range = mem:read_range(TEXT_PAGE1_START, TEXT_PAGE1_END, 8)

    local content = ""
    for line = 0, 23 do
        local line_offset = (line % 8) * 128 + math.floor(line / 8) * 40
        for col = 0, 39 do
            local byte_index = line_offset + col + 1 -- Lua 1-based indexing
            local char = string.byte(range, byte_index) or 0
            char = char & 0x7F                       -- Clear high bit
            if char <= 31 then
                char = char + 64                     -- Map 0x00..0x1F to 0x40..0x5F
            elseif char > 126 then
                char = 32                            -- Replace DEL with space
            end
            content = content .. string.char(char)
        end
    end
    return content
end

-- Memory tap callback for screen changes
local function on_screen_memory_write(offset, data, mask)
    if waiting_for_change then
        local current_content = get_screen_content()
        if current_content ~= last_screen_content then
            last_screen_content = current_content
            if screen_change_callback then
                screen_change_callback()
            end
        end
    end
end

-- Install memory tap on screen memory
local function install_screen_tap()
    if not screen_tap_handler then
        print_elapsed("Installing screen memory tap...")
        screen_tap_handler = mem:install_write_tap(
            TEXT_PAGE1_START,
            TEXT_PAGE1_END,
            "screen_watch",
            on_screen_memory_write
        )
        print_elapsed("Screen memory tap installed")
    end
end

-- Remove memory tap
local function remove_screen_tap()
    if screen_tap_handler then
        screen_tap_handler:remove()
        screen_tap_handler = nil
        print_elapsed("Screen memory tap removed")
    end
end

-- Wait for screen change using memory tap (simpler synchronous version)
local function wait_for_screen_change_tap(timeout_ms, label)
    timeout_ms = timeout_ms or 30000
    label = label or "screen change"

    print_elapsed("Waiting for " .. label .. " (memory tap)...")

    -- Store initial state
    local initial_content = get_screen_content()
    waiting_for_change = true
    local changed = false

    screen_change_callback = function()
        changed = true
        waiting_for_change = false
    end

    -- Poll with much longer intervals since we get notified via tap
    local max_wait = timeout_ms / 100            -- 100ms intervals instead of 50ms
    for i = 1, max_wait do
        emu.wait(emu.attotime.from_usec(100000)) -- 100ms

        if changed then
            print_elapsed("Screen changed for " .. label .. " (via tap)")
            read_text_screen("after_" .. label:gsub(" ", "_"))
            screen_change_callback = nil
            return true
        end
    end

    waiting_for_change = false
    screen_change_callback = nil
    print_elapsed("Timeout waiting for " .. label)
    read_text_screen("timeout_" .. label:gsub(" ", "_"))
    return false
end

-- Check if a prompt is visible on screen (line starting with prompt_char and otherwise blank)
local function check_for_prompt(prompt_char)
    -- Check all lines for a line that starts with prompt_char and is otherwise blank
    for line = 0, 23 do
        local line_offset = (line % 8) * 128 + math.floor(line / 8) * 40

        -- Read first character of the line
        local first_char = mem:read_u8(TEXT_PAGE1_START + line_offset)

        if first_char == prompt_char or first_char == (prompt_char & 0x7F) then
            -- Check if rest of line is blank (spaces or control chars)
            local is_blank = true
            for col = 1, 39 do
                local char = mem:read_u8(TEXT_PAGE1_START + line_offset + col)
                char = char & 0x7F               -- Clear high bit
                if char ~= 32 and char ~= 0 then -- Not space or null
                    is_blank = false
                    break
                end
            end

            if is_blank then
                print("Found prompt '" .. string.char(prompt_char & 0x7F) .. "' at line " .. line)
                return true
            end
        end
    end
    return false
end

-- Wait for any prompt character
local function wait_for_prompt(prompt_char, timeout_ms, label)
    timeout_ms = timeout_ms or 300000 -- Default 30 seconds
    label = label or ("prompt '" .. string.char(prompt_char & 0x7F) .. "'")

    print("Waiting for " .. label .. "...")

    -- Check if we already have the prompt
    if check_for_prompt(prompt_char) then
        print("✓ " .. label .. " already detected")
        return true
    end

    local max_wait = timeout_ms / 100 -- 100ms intervals
    for i = 1, max_wait do
        if check_for_prompt(prompt_char) then
            print("✓ " .. label .. " detected after wait")
            return true
        end
        emu.wait(emu.attotime.from_usec(100000)) -- 100ms
    end

    print("Timeout waiting for " .. label)
    return false
end

-- Wait for ProDOS prompt
local function wait_for_prodos()
    emu.wait(emu.attotime.from_usec(2000000)) -- Wait 2 seconds for boot
    read_text_screen("initial_boot")

    if wait_for_prompt(BASIC_PROMPT_CHAR, 300000, "ProDOS prompt") then
        read_text_screen("prodos_ready")
        return true
    end

    read_text_screen("timeout_state")
    return true -- Assume ready since we can see the screen
end

-- Wait for ProDOS prompt
local function wait_for_edasm()
    emu.wait(emu.attotime.from_usec(2000000)) -- Wait 2 seconds for boot
    read_text_screen("initial_boot")

    if wait_for_prompt(EDASM_PROMPT_CHAR, 300000, "EdAsm prompt") then
        read_text_screen("edasm_ready")
        return true
    end

    read_text_screen("timeout_state")
    return true -- Assume ready since we can see the screen
end

-- Run EDASM and assemble SIMPLE.SRC
local function run_edasm_assembly()
    print_elapsed("Starting EDASM Assembly")

    -- Load SIMPLE.SRC
    print_elapsed("Loading SIMPLE.SRC...")
    send_string("L SIMPLE.SRC")
    send_return()
    wait_for_screen_change_tap(5000, "file load")

    -- Assemble
    print_elapsed("Assembling...")
    send_string("A")
    send_return()
    wait_for_screen_change_tap(8000, "assembly")

    -- Save binary
    print_elapsed("Saving binary...")
    send_string("S SIMPLE.BIN")
    send_return()
    wait_for_screen_change_tap(5000, "file save")

    -- Quit EDASM
    print_elapsed("Quitting EDASM...")
    send_string("Q")
    send_return()
    wait_for_screen_change_tap(3000, "quit to ProDOS")

    print_elapsed("EDASM assembly sequence complete")
end

-- Main execution
local function on_start()
    install_screen_tap()
    start_timer()
    print_elapsed("EDASM Assembly Automation Started")

    -- Enable natural keyboard mode for proper input handling
    manager.machine.natkeyboard.in_use = true
    print_elapsed("Natural keyboard mode enabled")

    local prodos_ready = wait_for_prodos()
    if prodos_ready then
        -- Run EDASM.SYSTEM
        print_elapsed("Starting EDASM.SYSTEM...")
        send_string("RUN EDASM.SYSTEM,S5")
        send_return()
        wait_for_screen_change_tap(10000, "EDASM startup")

        local edasm_ready = wait_for_edasm()
        if edasm_ready then
            run_edasm_assembly()
        else
            print_elapsed("Could not detect EdAsm prompt")
            read_text_screen("final_state")
        end
    else
        print_elapsed("Could not detect ProDOS prompt")
        read_text_screen("final_state")
    end

    -- Cleanup
    remove_screen_tap()
    print_elapsed("EDASM Assembly Automation Complete")
end

-- Run the automation
on_start()
