local machine = manager.machine
local system = machine and machine.system

local function section(title)
    print("\n" .. string.rep("=", 72))
    print(" " .. title)
    print(string.rep("=", 72))
end

local function format_value(value)
    if value == nil then
        return "<nil>"
    elseif type(value) == "boolean" then
        return value and "true" or "false"
    elseif type(value) == "string" and value == "" then
        return "<empty>"
    end
    return tostring(value)
end

local function summarize_list(items, limit)
    limit = limit or 6
    if #items == 0 then
        return "<none>"
    end
    if #items <= limit then
        return table.concat(items, ", ")
    end
    local trimmed = {}
    for i = 1, limit do
        trimmed[i] = items[i]
    end
    return table.concat(trimmed, ", ") .. " ...+" .. (#items - limit)
end

local function describe_state_entries(state_table)
    local entries = {}
    if not state_table then
        return "<none>"
    end
    for symbol in pairs(state_table) do
        entries[#entries + 1] = symbol
    end
    table.sort(entries)
    return summarize_list(entries, 8)
end

local function describe_spaces(space_table)
    local entries = {}
    if not space_table then
        return "<none>"
    end
    for name, space in pairs(space_table) do
        local label = string.format("%s(%d-bit %s)", name, space.data_width or 0, space.endianness or "?")
        entries[#entries + 1] = label
    end
    table.sort(entries)
    return summarize_list(entries, 6)
end

local function describe_device(tag, device)
    local owner_tag = device.owner and device.owner.tag or "<root>"
    print(string.format("  %s [%s/%s] owner=%s configured=%s started=%s",
        tag,
        format_value(device.name),
        format_value(device.shortname),
        owner_tag,
        format_value(device.configured),
        format_value(device.started)
    ))
    local spaces_text = describe_spaces(device.spaces)
    if spaces_text ~= "<none>" then
        print("    spaces: " .. spaces_text)
    end
    local states_text = describe_state_entries(device.state)
    if states_text ~= "<none>" then
        print("    states: " .. states_text)
    end
end

local function dump_enumerator(title, enumerator, describer)
    section(title)
    if not enumerator then
        print("  <enumerator missing>")
        return
    end
    local count = 0
    for tag, entry in pairs(enumerator) do
        count = count + 1
        describer(tag, entry)
    end
    if count == 0 then
        print("  <none>")
    end
end

local function describe_screen(tag, screen)
    local rotation, flipx, flipy = screen:orientation()
    local flips = {}
    if flipx then flips[#flips + 1] = "flipX" end
    if flipy then flips[#flips + 1] = "flipY" end
    local flip_label = (#flips > 0) and table.concat(flips, ", ") or "none"
    local rotation_label = (rotation ~= nil) and (tostring(rotation) .. "deg") or "<nil>"
    print(string.format("  %s [%dx%d] refresh=%s orientation=%s (%s)",
        tag,
        screen.width or 0,
        screen.height or 0,
        format_value(screen.refresh),
        rotation_label,
        flip_label
    ))
end

local function describe_palette(tag, palette)
    print(string.format("  %s entries=%s indirect=%s shadows=%s highlights=%s",
        tag,
        format_value(palette.entries),
        format_value(palette.indirect_entries),
        format_value(palette.shadows_enabled),
        format_value(palette.highlights_enabled)
    ))
end

local function describe_sound_device(tag, sound)
    print(string.format("  %s inputs=%s outputs=%s speaker=%s microphone=%s",
        tag,
        format_value(sound.inputs),
        format_value(sound.outputs),
        format_value(sound.speaker),
        format_value(sound.microphone)
    ))
end

local function describe_cassette(tag, cassette)
    print(string.format("  %s motor=%s playing=%s recording=%s length=%s position=%s",
        tag,
        format_value(cassette.motor_state),
        format_value(cassette.is_playing),
        format_value(cassette.is_recording),
        format_value(cassette.length),
        format_value(cassette.position)
    ))
end

local function describe_image_device(tag, image)
    print(string.format("  %s [%s] exists=%s type=%s file=%s",
        tag,
        format_value(image.instance_name),
        format_value(image.exists),
        format_value(image.image_type_name),
        format_value(image.filename)
    ))
end

local function describe_slot_device(tag, slot)
    local options = {}
    for name, option in pairs(slot.options or {}) do
        local suffix = option.selectable and "" or " (fixed)"
        options[#options + 1] = name .. suffix
    end
    table.sort(options)
    print(string.format("  %s fixed=%s selectable=%s options=%s",
        tag,
        format_value(slot.fixed),
        format_value(slot.has_selectable_options),
        summarize_list(options, 8)
    ))
end

local function dump_memory_collection(label, collection, describer)
    print("  " .. label)
    local count = 0
    for tag, entry in pairs(collection or {}) do
        count = count + 1
        print("    " .. describer(tag, entry))
    end
    if count == 0 then
        print("    <none>")
    end
end

local function describe_share(tag, share)
    return string.format("%s size=%s bytes bitwidth=%s endianness=%s",
        tag,
        format_value(share.size),
        format_value(share.bitwidth),
        format_value(share.endianness)
    )
end

local function describe_bank(tag, bank)
    return string.format("%s entry=%s", tag, format_value(bank.entry))
end

local function describe_region(tag, region)
    local device_tag = region.device and region.device.tag or "<none>"
    return string.format("%s size=%s bytes bitwidth=%s device=%s",
        tag,
        format_value(region.size),
        format_value(region.bitwidth),
        device_tag
    )
end

section("Session Overview")
print("  " .. (emu.app_name() or "mame") .. " " .. (emu.app_version() or "unknown"))
if system then
    print("  driver : " .. format_value(system.name) .. " (" .. format_value(system.description) .. ")")
    print("  year   : " .. format_value(system.year))
    print("  maker  : " .. format_value(system.manufacturer))
    print("  parent : " .. format_value(system.parent))
    print("  compat : " .. format_value(system.compatible_with))
    print("  source : " .. format_value(system.source_file))
    print("  rotation: " .. format_value(system.rotation))
    print("  statuses: not_working=" .. format_value(system.not_working) ..
        " save=" .. format_value(system.supports_save) ..
        " no_sound=" .. format_value(system.no_sound_hw) ..
        " no_cocktail=" .. format_value(system.no_cocktail))
end

section("Video & Sound Managers")
if machine.video then
    print(string.format("  video speed=%.1f%% throttled=%s frameskip=%s",
        machine.video.speed_percent or 0,
        format_value(machine.video.throttled),
        format_value(machine.video.frameskip)
    ))
end
if machine.sound then
    print("  sound muted=" .. format_value(machine.sound.muted) .. " ui_mute=" .. format_value(machine.sound.ui_mute))
end
if machine.output then
    print("  output manager ready")
end

local memory = machine.memory
section("Memory Overview")
if memory then
    dump_memory_collection("shares", memory.shares, describe_share)
    dump_memory_collection("banks", memory.banks, describe_bank)
    dump_memory_collection("regions", memory.regions, describe_region)
else
    print("  <memory manager missing>")
end

dump_enumerator("Device Tree", machine.devices, describe_device)
dump_enumerator("Screen Devices", machine.screens, describe_screen)
dump_enumerator("Palette Devices", machine.palettes, describe_palette)
dump_enumerator("Sound Devices", machine.sounds, describe_sound_device)
dump_enumerator("Slot Devices", machine.slots, describe_slot_device)
dump_enumerator("Media Image Devices", machine.images, describe_image_device)
dump_enumerator("Cassette Devices", machine.cassettes, describe_cassette)

section("Inventory Complete")
print("  Inventory dump complete.")
