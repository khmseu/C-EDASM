-- MAME Lua automation for EDASM assembly - Event-driven version
-- Usage: mame apple2gs -flop3 /tmp/edasm_work.2mg -autoboot_script tests/emulator/edasm_assemble_clean.lua

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
local screen_tap_handler = nil
local last_screen_content = nil
local last_screen_raw_content = nil

-- State machine constants
local STATE_INIT = 0
local STATE_WAITING_FOR_PRODOS = 1
local STATE_STARTING_EDASM = 2
local STATE_WAITING_FOR_EDASM = 3
local STATE_LOADING_FILE = 4
local STATE_ASSEMBLING = 5
local STATE_SAVING = 6
local STATE_QUITTING = 7
local STATE_COMPLETE = 8

local current_state = STATE_INIT
local automation_complete = false

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

-- Get screen content as a string for comparison.
-- Returned chars are 0x20 <= char <= 0x7e.
local function get_screen_content()
    local range = mem:read_range(TEXT_PAGE1_START, TEXT_PAGE1_END, 8)

    local visible = {}
    local raw = {}
    for line = 0, 23 do
        local line_offset = (line % 8) * 128 + math.floor(line / 8) * 40
        for col = 0, 39 do
            local byte_index = line_offset + col + 1 -- Lua 1-based indexing
            local char = string.byte(range, byte_index) or 0
            char = char & 0x7F                       -- Clear high bit
            raw[#raw + 1] = string.char(char)
            local printable = char
            if printable <= 0x1f then
                printable = printable | 0x40
            elseif printable == 0x7f then
                printable = 0x20
            end
            visible[#visible + 1] = string.char(printable)
        end
    end
    last_screen_raw_content = table.concat(raw)
    return table.concat(visible)
end

-- Function to read and print the Apple II text screen
local function log_text_screen(label)
    local output = {}
    table.insert(output, '+' .. string.rep("-", 40) .. '+')
    local screen = last_screen_raw_content or ""
    for line = 0, 23 do
        local line_start = line * 40 + 1
        local line_text = "|" .. string.sub(screen, line_start, line_start + 39) .. "|"
        table.insert(output, line_text)
    end
    table.insert(output, '+' .. string.rep("-", 40) .. '+')

    print_elapsed("Screen Dump: " .. (label or "unknown"))
    for _, line in ipairs(output) do
        print(line)
    end
    print("=== End Screen Dump ===")
end

-- Check if a prompt is visible on screen (line starting with prompt_char and otherwise blank)
local function check_for_prompt(prompt_char)
    local screen = last_screen_raw_content or ""
    local prompt_masked = prompt_char & 0x7F
    local blanks = string.rep(" ", 39)
    for line = 0, 23 do
        local line_start = line * 40 + 1
        local line_text = "|" .. string.sub(screen, line_start, line_start + 39) .. "|"
        local first_char = string.byte(line_text, 1) or 0
        if first_char == prompt_char or first_char == prompt_masked then
            local rest = string.sub(line_text, 2, 40)
            if rest == blanks then
                return true
            end
        end
    end
    return false
end

-- Memory tap callback for screen changes - contains state machine
local function on_screen_memory_write(offset, data, mask)
    local current_content = get_screen_content()
    if current_content ~= last_screen_content then
        last_screen_content = current_content

        log_text_screen("Screen change detected in state " .. current_state .. "\n" .. current_content)

        -- State machine based on screen changes
        if current_state == STATE_WAITING_FOR_PRODOS then
            if check_for_prompt(BASIC_PROMPT_CHAR) then
                print_elapsed("✓ ProDOS prompt detected")
                current_state = STATE_STARTING_EDASM
                print_elapsed("Starting EDASM.SYSTEM...")
                send_string("RUN EDASM.SYSTEM,S5")
                send_return()
            end
        elseif current_state == STATE_STARTING_EDASM then
            current_state = STATE_WAITING_FOR_EDASM
        elseif current_state == STATE_WAITING_FOR_EDASM then
            if check_for_prompt(EDASM_PROMPT_CHAR) then
                print_elapsed("✓ EDASM prompt detected")
                current_state = STATE_LOADING_FILE
                print_elapsed("Loading SIMPLE.SRC...")
                send_string("L SIMPLE.SRC")
                send_return()
            end
        elseif current_state == STATE_LOADING_FILE then
            current_state = STATE_ASSEMBLING
            print_elapsed("Assembling...")
            send_string("A")
            send_return()
        elseif current_state == STATE_ASSEMBLING then
            current_state = STATE_SAVING
            print_elapsed("Saving binary...")
            send_string("S SIMPLE.BIN")
            send_return()
        elseif current_state == STATE_SAVING then
            current_state = STATE_QUITTING
            print_elapsed("Quitting EDASM...")
            send_string("Q")
            send_return()
        elseif current_state == STATE_QUITTING then
            current_state = STATE_COMPLETE
            automation_complete = true
            print_elapsed("EDASM assembly sequence complete")
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

-- Main execution - now purely event-driven
local function on_start()
    start_timer()
    print_elapsed("EDASM Assembly Automation Started")

    -- Enable natural keyboard mode for proper input handling
    manager.machine.natkeyboard.in_use = true
    print_elapsed("Natural keyboard mode enabled")

    -- Install screen memory tap and initialize
    last_screen_content = get_screen_content()
    install_screen_tap()
    log_text_screen("initial_boot")

    -- Wait for initial boot, then start state machine
    current_state = STATE_WAITING_FOR_PRODOS
    print_elapsed("Waiting for ProDOS prompt...")

    -- Single long wait - all work happens in callbacks now
    while not automation_complete do
        emu.wait(emu.attotime.from_usec(1000000)) -- 1 second intervals
    end

    -- Cleanup
    remove_screen_tap()
    print_elapsed("EDASM Assembly Automation Complete")
end

-- Run the automation
on_start()
