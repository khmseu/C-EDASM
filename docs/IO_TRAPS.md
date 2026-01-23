# I/O Trap Implementation ($C000-$C7FF)

## Overview

The emulator now implements comprehensive read and write traps for the Apple II I/O address range $C000-$C7FF. This allows the emulator to intercept and handle I/O operations without requiring actual hardware.

## Text Screen Monitoring

### Automatic Screen Logging and Stop on 'E' Character

The emulator monitors writes to the text screen memory ($0400-$07FF) and implements special behavior:

- **Screen Change Detection**: Any write to text screen memory marks the screen as "dirty"
- **'E' Character Detection**: When the first screen character ($0400) is set to 'E' (ASCII 0x45 or 0x65), the emulator:
  1. Immediately logs the current text screen state to stdout
  2. Sets a stop flag that can be checked via `HostShims::should_stop()`
  3. Prints a message: "[HostShims] First screen character set to 'E' - logging and stopping"

This feature is useful for debugging and capturing specific program states during emulation.

**Example Usage:**
```cpp
Bus bus;
CPU cpu(bus);
HostShims shims;
shims.install_io_traps(bus);

while (running) {
    running = cpu.step();
    
    // Check if 'E' was written to first screen position
    if (shims.should_stop()) {
        std::cout << "Emulator stopped by screen monitor" << std::endl;
        break;
    }
}
```

## Architecture

### Address Range Coverage

The I/O trap system covers the complete $C000-$C7FF range (2KB), which includes:

- **$C000-$C0FF**: Main I/O page (keyboard, graphics, game controllers, speaker)
- **$C100-$C7FF**: Peripheral slot I/O space (slots 1-7, 256 bytes each)

### Implementation Components

#### 1. Bus Class (`bus.cpp`)

- Provides `set_read_trap_range()` and `set_write_trap_range()` methods
- Maintains trap handler arrays for all 64KB of address space
- Invokes handlers before accessing memory when traps are installed

#### 2. HostShims Class (`host_shims.cpp`)

- Implements the actual I/O device emulation
- Single entry point: `install_io_traps()` installs both read and write handlers
- Delegates to specific device handlers based on address

#### 3. Device Handlers

Individual handlers for different I/O regions:

- `handle_io_read()`: Main dispatcher for read operations
- `handle_io_write()`: Main dispatcher for write operations
- `handle_kbd_read()`: Keyboard input ($C000)
- `handle_kbdstrb_read()`: Keyboard strobe clear ($C010)
- `handle_speaker_toggle()`: Speaker I/O ($C030)
- `handle_graphics_switches()`: Display mode switches ($C050-$C057)

## Implemented Devices

### Keyboard I/O ($C000-$C01F)

#### $C000 (KBD) - Keyboard Data

- **Read**: Returns keyboard data with high bit set if key available
  - After strobe clear: Returns last key with high bit clear
  - Auto-fetches queued input when available
- **Write**: Ignored (read-only)

#### $C010 (KBDSTRB) - Keyboard Strobe

- **Read**: Clears keyboard strobe, returns 0
- **Write**: Also clears keyboard strobe

**Usage Pattern:**

```asm
READ_KEY:
    LDA $C000        ; Check keyboard
    BPL READ_KEY     ; Wait for key (high bit set)
    STA $C010        ; Clear strobe
    AND #$7F         ; Strip high bit
    RTS
```

### Graphics Soft Switches ($C050-$C057)

All graphics switches work the same for read or write (access triggers the action):

- **$C050 (TXTCLR)**: Switch to graphics mode
- **$C051 (TXTSET)**: Switch to text mode
- **$C052 (MIXCLR)**: Full screen graphics
- **$C053 (MIXSET)**: Mixed text/graphics
- **$C054 (LOWSCR)**: Display page 1
- **$C055 (HISCR)**: Display page 2
- **$C056 (LORES)**: Low-resolution graphics
- **$C057 (HIRES)**: High-resolution graphics

**State maintained:**

- `text_mode_`: true = text, false = graphics
- `mixed_mode_`: true = mixed, false = full screen
- `page2_`: true = page 2, false = page 1
- `hires_`: true = hi-res, false = lo-res

### Speaker ($C030)

- **Read/Write**: Any access toggles speaker output
- **Implementation**: No actual sound output, just acknowledges access

### Game I/O ($C060-$C07F)

#### $C061-$C063: Push Buttons

- **Read**: Returns button state in high bit (0 = pressed, 1 = not pressed)
- Currently returns 0 (not pressed)

#### $C070: Paddle Trigger

- **Read/Write**: Triggers paddle timer
- Currently returns 0

### Expansion Slots ($C100-$C7FF)

- **Read/Write**: Returns 0 for all undefined addresses
- Each slot occupies 16 bytes: $Cn00-$CnFF (n = 1-7)
- Allows for future expansion (disk controllers, serial cards, etc.)

## Usage Example

```cpp
#include "edasm/bus.hpp"
#include "edasm/cpu.hpp"
#include "edasm/host_shims.hpp"

// Initialize emulator
Bus bus;
CPU cpu(bus);
HostShims shims;

// Install I/O traps (covers $C000-$C7FF)
shims.install_io_traps(bus);

// Queue keyboard input
shims.queue_input_line("HELLO");

// Now CPU can read from $C000 and get keyboard input
cpu.step();  // Execute: LDA $C000
```

## Testing

The implementation includes comprehensive tests in `test_io_traps.cpp`:

- Trap installation verification
- Keyboard I/O (read, strobe clearing)
- Graphics switches (all 8 switches)
- Speaker toggle
- Game I/O (buttons, paddles)
- Full range coverage ($C000-$C7FF)

Run tests: `ctest -R test_io_traps`

## Apple II I/O Compatibility

This implementation follows Apple II hardware conventions:

1. **Soft switches**: Both read and write trigger the action
2. **Keyboard**: High bit indicates key available, strobe must be cleared
3. **Graphics**: State changes immediate on access
4. **Memory-mapped I/O**: All I/O accessed via memory reads/writes

## Future Enhancements

Potential additions:

- **$C080-$C08F**: Language card bank switching (currently stubbed)
- **Slot I/O**: Disk II controller, serial cards, etc.
- **80-column**: Extended soft switches for 80-column display
- **Mouse/joystick**: Game controller support

## References

- Apple II Reference Manual (I/O addresses)
- [docs/apple2-text-screen-layout.md](apple2-text-screen-layout.md)
- Original EDASM source: `third_party/EdAsm/`
