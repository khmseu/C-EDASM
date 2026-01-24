-- MAME Lua automation for EDASM assembly with debounced screen updates
-- Usage: mame apple2gs -flop3 /tmp/edasm_work.2mg -autoboot_script tests/emulator/edasm_assemble_debounced.lua

local machine = manager.machine
local cpu = machine.devices[":maincpu"]
local mem = cpu.spaces["program"]
local video = machine.video
local memory_manager = machine.memory
local docram_share = memory_manager and memory_manager.shares and memory_manager.shares[":docram"]
local SEARCH_STRING = "PL OPTR"
local READ_CHUNK_SIZE = 0x10000
local global_memory_search_done = false

-- Apple II screen memory locations
local TEXT_PAGE1_START = 0x0400
local TEXT_PAGE1_END = 0x07FF
local BASIC_PROMPT_CHAR = ']' -- character in Apple II
local EDASM_PROMPT_CHAR = ':' -- character in EdAsm
local DEBOUNCE_INTERVAL_SEC = 0.1

local start_time = nil
local screen_tap_handler = nil
local last_screen_content = nil
local pending_screen_visible = nil
local last_screen_change_time = nil
-- State machine constants
local STATE_INIT = 0
local STATE_WAITING_FOR_PRODOS = 1
local STATE_PREFIX_COMMAND = 2
local STATE_RUN_EDASM_COMMAND = 3
local STATE_WAITING_FOR_EDASM = 4
local STATE_LOADING_FILE = 5
local STATE_ASSEMBLING = 6
local STATE_SAVING = 7
local STATE_QUITTING = 8
local STATE_COMPLETE = 9

local current_state = STATE_INIT
local automation_complete = false

-- Initialize timer
local function start_timer()
    start_time = os.time()
    print("Timer started at " .. start_time)
end

-- Get elapsed time since start
local function elapsed_time()
    if not start_time then
        return 0
    end
    return os.difftime(os.time(), start_time)
end

-- Print elapsed time with label
local function print_elapsed(label)
    local elapsed = elapsed_time()
    print(string.format("[%.2fs (%.2f)] %s", elapsed, os.clock(), label or "checkpoint"))
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

local function to_printable_char(value)
    local byte = (value or 0) & 0x7F
    if byte <= 0x1f then
        byte = byte | 0x40
    elseif byte == 0x7f then
        byte = 0x20
    end
    return byte
end

-- Read the Apple II text screen and return printable strings
local function fetch_screen_content()
    local range = mem:read_range(TEXT_PAGE1_START, TEXT_PAGE1_END, 8)
    local visible = {}
    for line = 0, 23 do
        local line_offset = (line % 8) * 128 + math.floor(line / 8) * 40
        for col = 0, 39 do
            local byte_index = line_offset + col + 1
            local char = string.byte(range, byte_index) or 0
            visible[#visible + 1] = string.char(to_printable_char(char))
        end
    end
    return table.concat(visible)
end

local function fetch_docram_content()
    if not docram_share then
        return nil
    end
    local share_size = docram_share.size or 0
    if share_size == 0 then
        return nil
    end
    local visible = {}
    for line = 0, 23 do
        local line_offset = (line % 8) * 128 + math.floor(line / 8) * 40
        for col = 0, 39 do
            local address = TEXT_PAGE1_START + line_offset + col
            local byte = 0
            if address >= 0 and address < share_size then
                byte = docram_share:read_u8(address)
            end
            visible[#visible + 1] = string.char(to_printable_char(byte))
        end
    end
    return table.concat(visible)
end

-- Get screen content while updating the stored buffer
local function get_screen_content()
    local visible = fetch_screen_content()
    return visible
end

local function dump_text_block(prefix, label, content)
    if not content then
        return
    end
    local output = {}
    table.insert(output, '+' .. string.rep("-", 40) .. '+')
    for line = 0, 23 do
        local line_start = line * 40 + 1
        local line_text = "|" .. string.sub(content, line_start, line_start + 39) .. "|"
        table.insert(output, line_text)
    end
    table.insert(output, '+' .. string.rep("-", 40) .. '+')

    print_elapsed(prefix .. ": " .. (label or "unknown"))
    for _, line in ipairs(output) do
        print(line)
    end
    print("=== End " .. prefix .. " Dump ===")
end

local function log_docram_snapshot(label)
    local docram = fetch_docram_content()
    if not docram then
        return
    end
    dump_text_block("DocRAM Dump", (label and (label .. " [:docram]")) or "[:docram]", docram)
end

local function safe_read(func, ...)
    if not func then
        return nil
    end
    local ok, data_or_err = pcall(func, ...)
    if not ok then
        print_elapsed("Memory read failed: " .. tostring(data_or_err))
        return nil
    end
    return data_or_err
end

local function search_blob(label, data, base_offset)
    if not data or data == "" then
        return false
    end
    local pos = string.find(data, SEARCH_STRING, 1, true)
    if pos then
        local absolute = base_offset or 0
        absolute = absolute + pos - 1
        print_elapsed(string.format("'%s' found in %s at offset 0x%X", SEARCH_STRING, label, absolute))
        return true
    end
    return false
end

local function search_share(tag, share)
    local size = share.size or 0
    if size <= 0 then
        return false
    end
    if not share.read then
        return false
    end
    local data = safe_read(share.read, share, 0, size)
    return search_blob("share " .. tag, data)
end

local function search_region(tag, region)
    local size = region.size or 0
    if size <= 0 then
        return false
    end
    if not region.read then
        return false
    end
    local data = safe_read(region.read, region, 0, size)
    return search_blob("region " .. tag, data)
end

local function search_space(tag, name, space)
    local mask = space.address_mask or 0
    if mask < 0 then
        return false
    end
    if not space.read_range then
        return false
    end
    local start = 0
    while start <= mask do
        local chunk_end = math.min(start + READ_CHUNK_SIZE - 1, mask)
        local data = safe_read(space.read_range, space, start, chunk_end, 8)
        if search_blob(string.format("space %s:%s [0x%X-0x%X]", tag, name, start, chunk_end), data, start) then
            return true
        end
        if chunk_end == mask then
            break
        end
        start = chunk_end + 1
    end
    return false
end

local function handler_summary(handler)
    if not handler then
        return nil
    end
    local parts = {}
    for _, field in ipairs({"name", "tag", "handlertype", "share", "region"}) do
        local value = handler[field]
        if value then
            parts[#parts + 1] = tostring(value)
        end
    end
    return table.concat(parts, " ")
end

local function search_map_entry_data(label, handler)
    local summary = handler_summary(handler)
    if not summary or summary == "" then
        return false
    end
    return search_blob(label, summary)
end

local function perform_global_memory_search()
    if global_memory_search_done then
        return
    end
    global_memory_search_done = true
    local found = false
    print_elapsed("Beginning global memory scan for '" .. SEARCH_STRING .. "'")

    if memory_manager then
        for tag, share in pairs(memory_manager.shares or {}) do
            if search_share(tag, share) then
                found = true
            end
        end
        if memory_manager.banks then
            for tag, bank in pairs(memory_manager.banks) do
                local text = string.format("bank %s entry=%s", tag, tostring(bank.entry))
                if search_blob(text, text) then
                    found = true
                end
            end
        end
        for tag, region in pairs(memory_manager.regions or {}) do
            if search_region(tag, region) then
                found = true
            end
        end
    end

    for device_tag, device in pairs(machine.devices or {}) do
        for space_name, space in pairs(device.spaces or {}) do
            if search_space(device_tag, space_name, space) then
                found = true
            end
            local map = space.map
            if map and map.entries then
                for idx, entry in ipairs(map.entries) do
                    local base = string.format("%s:%s entry %d read", device_tag, space_name, idx)
                    if search_map_entry_data(base, entry.read) then
                        found = true
                    end
                    local base_w = string.format("%s:%s entry %d write", device_tag, space_name, idx)
                    if search_map_entry_data(base_w, entry.write) then
                        found = true
                    end
                end
            end
        end
    end

    if not found then
        print_elapsed("Global scan complete; '" .. SEARCH_STRING .. "' not found")
    end
end

-- Function to read and print the Apple II text screen
local function log_text_screen(label)
    dump_text_block("Screen Dump", label, last_screen_content or "")
    log_docram_snapshot(label)
end

-- Check if a prompt is visible on screen (line starting with prompt_char and otherwise blank)
local function check_for_prompt(prompt_char)
    local screen = last_screen_content or ""
    local blanks = string.rep(" ", 39)
    local want = prompt_char .. blanks
    for line = 0, 23 do
        local line_start = line * 40 + 1
        local line_text = string.sub(screen, line_start, line_start + 39)
        if line_text == want then
            return true
        end
    end
    return false
end

-- Schedule the latest screen buffer for debounced processing
local function schedule_screen_update()
    local visible = fetch_screen_content()
    pending_screen_visible = visible
    last_screen_change_time = os.time()
    -- print_elapsed("Screen change detected; waiting for debounce window")
end

-- State transition logic is identical to the clean script
local function handle_state_transition()
    log_text_screen("Screen change detected in state " .. current_state)
    if current_state == STATE_WAITING_FOR_PRODOS then
        if check_for_prompt(BASIC_PROMPT_CHAR) then
            print_elapsed("✓ ProDOS prompt detected")
            current_state = STATE_PREFIX_COMMAND
            print_elapsed("Issuing EDASM prefix...")
            send_string("PREFIX /EDASM/EDASM")
            send_return()
        end
    elseif current_state == STATE_PREFIX_COMMAND then
        if check_for_prompt(BASIC_PROMPT_CHAR) then
            current_state = STATE_RUN_EDASM_COMMAND
            print_elapsed("Issuing EDASM load command...")
            send_string("- EDASM.SYSTEM")
            send_return()
        end
    elseif current_state == STATE_RUN_EDASM_COMMAND then
        if not global_memory_search_done and string.find(last_screen_content or "", "APECMUE", 1, true) then
            print_elapsed("APECMUE detected on screen; scanning memory for '" .. SEARCH_STRING .. "'")
            perform_global_memory_search()
        end
        current_state = STATE_WAITING_FOR_EDASM
    elseif current_state == STATE_WAITING_FOR_EDASM then
        if not global_memory_search_done and string.find(last_screen_content or "", "APECMUE", 1, true) then
            print_elapsed("APECMUE detected on screen; scanning memory for '" .. SEARCH_STRING .. "'")
            perform_global_memory_search()
        end
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

-- Process pending updates once they have been stable long enough
local function commit_pending_screen()
    if pending_screen_visible and last_screen_change_time and os.difftime(os.time(), last_screen_change_time) >= DEBOUNCE_INTERVAL_SEC then
        if pending_screen_visible ~= last_screen_content then
            last_screen_content = pending_screen_visible
            handle_state_transition()
        end
        pending_screen_visible = nil
        last_screen_change_time = nil
    end
end

-- Memory tap callback for screen changes - now only schedules updates
local function on_screen_memory_write(offset, data, mask)
    schedule_screen_update()
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

-- Main execution loop
local function on_start()
    start_timer()
    print_elapsed("EDASM Assembly Automation Started")

    manager.machine.natkeyboard.in_use = true
    print_elapsed("Natural keyboard mode enabled")

    last_screen_content = get_screen_content()
    install_screen_tap()
    log_text_screen("initial_boot")

    current_state = STATE_WAITING_FOR_PRODOS
    print_elapsed("Waiting for ProDOS prompt...")

    while not automation_complete do
        emu.wait(emu.attotime.from_usec(50000))
        commit_pending_screen()
    end

    remove_screen_tap()
    print_elapsed("EDASM Assembly Automation Complete")
end

on_start()
