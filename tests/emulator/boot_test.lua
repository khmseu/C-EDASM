-- boot_test.lua
-- Simple MAME Lua script to boot ProDOS and launch EDASM
-- Usage: mame apple2e -flop1 EDASM_SRC.2mg -video none -sound none -nothrottle -autoboot_script boot_test.lua

local emu = manager.machine
local cpu = emu.devices[":maincpu"]
local mem = cpu.spaces["program"]

-- Wait for ProDOS to boot by checking for specific prompt or memory state
function wait_for_prodos()
    print("Waiting for ProDOS to boot...")
    
    -- Simple delay-based wait (can be improved with memory polling)
    -- ProDOS typically takes 2-3 seconds to boot
    for i = 1, 300 do  -- Wait ~3 seconds at full speed
        emu.wait(10000)  -- Wait 10000 CPU cycles
    end
    
    print("ProDOS should be booted")
end

-- Send keystrokes to launch EDASM
function launch_edasm()
    print("Launching EDASM.SYSTEM...")
    
    -- Type the command to run EDASM
    -- Note: This is a simplified approach. Real implementation would need
    -- to properly handle keyboard buffer and timing
    local command = "EDASM.SYSTEM"
    
    -- In real implementation, use emu.keypost() for each character
    -- For now, this is a placeholder showing the concept
    print("Would type: " .. command)
    
    -- Wait for EDASM to load
    for i = 1, 200 do
        emu.wait(10000)
    end
    
    print("EDASM should be running")
end

-- Main script entry point
function on_start()
    print("=== EDASM Boot Test Started ===")
    wait_for_prodos()
    launch_edasm()
    print("=== Boot test complete ===")
    print("Emulator will continue running. Use Ctrl+C to exit.")
end

-- Register the start callback
emu.register_start(on_start)

print("Boot test script loaded")
