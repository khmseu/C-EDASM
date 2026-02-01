# Apple IIe Memory Map Reference

**Source:** Inside the Apple IIe.txt

This document extracts symbol definitions, hardware explanations, and bank switching behavior from the Apple IIe technical reference.

---

## Table of Contents

1. [I/O Memory Map ($C000-$C0FF)](#io-memory-map-c000-c0ff)
2. [Memory Management Soft Switches](#memory-management-soft-switches)
3. [Video Soft Switches](#video-soft-switches)
4. [Soft Switch Status Flags](#soft-switch-status-flags)
5. [Annunciator Switches](#annunciator-switches)
6. [Built-In Device I/O](#built-in-device-io)
7. [Bank-Switched RAM](#bank-switched-ram)
8. [Bank-Switched ROM](#bank-switched-rom)
9. [Peripheral Card I/O](#peripheral-card-io)
10. [Auxiliary Memory](#auxiliary-memory)

---

## I/O Memory Map ($C000-$C0FF)

The Apple IIe reserves addresses $C000...$C0FF for I/O operations and "soft switch" memory locations. A soft switch is a memory location that, when accessed (read or written), activates or deactivates specific hardware features.

**Key Ranges:**

- $C000-$C00F: Memory management and video control
- $C010-$C06F: Built-in device I/O (keyboard, speaker, game controllers)
- $C050-$C05F: Video mode and annunciator soft switches
- $C080-$C08F: Bank-switched RAM control
- $C090-$C0FF: Peripheral card I/O (16 bytes per slot)

---

## Memory Management Soft Switches

These switches control which memory banks are active for read/write operations across different memory ranges.

### Zero Page and Stack Switching

| Address | Decimal | Access | Symbol   | Description                                 | Notes |
| ------- | ------- | ------ | -------- | ------------------------------------------- | ----- |
| $C008   | 49160   | W      | ALTZPOFF | Enable main memory $0000-$01FF and main BSR | 5     |
| $C009   | 49161   | W      | ALTZPON  | Enable aux. memory $0000-$01FF and aux. BSR | 5     |

**Note 5:** Affects zero page ($00-$FF) and stack ($100-$1FF), plus which bank-switched RAM (BSR) is available.

### Main/Auxiliary Memory Switching ($200-$BFFF)

| Address | Decimal | Access | Symbol     | Description                           | Notes |
| ------- | ------- | ------ | ---------- | ------------------------------------- | ----- |
| $C000   | 49154   | W      | 80STOREOFF | PAGE2 switches video pages            | 1     |
| $C001   | 49153   | W      | 80STOREON  | PAGE2 switches main/aux. video memory | 1     |
| $C002   | 49154   | W      | RAMRDOFF   | Read from main memory $200-$BFFF      | 4     |
| $C003   | 49155   | W      | RAMRDON    | Read from aux. memory $200-$BFFF      | 4     |
| $C004   | 49156   | W      | RAMWRTOFF  | Write to main memory $200-$BFFF       | 4     |
| $C005   | 49157   | W      | RAMWRTON   | Write to aux. memory $200-$BFFF       | 4     |

**Note 1:** If 80STORE is ON, PAGE2OFF/PAGE2ON control video RAM selection. If OFF, they control text page selection.

**Note 4:** RAMRD and RAMWRT do not affect video RAM ($400-$7FF) if 80STORE is ON, or hi-res RAM ($2000-$3FFF) if both 80STORE and HIRES are ON. In these cases, PAGE2 controls those areas.

### ROM Switching ($C100-$CFFF)

| Address | Decimal | Access | Symbol       | Description                     | Notes |
| ------- | ------- | ------ | ------------ | ------------------------------- | ----- |
| $C006   | 49158   | W      | INTCXROMOFF  | Enable slot ROM $C100-$CFFF     | 5     |
| $C007   | 49159   | W      | INTCXROMON   | Enable internal ROM $C100-$CFFF | 5     |
| $C00A   | 49162   | W      | SLOTC3ROMOFF | Enable internal ROM $C300-$C3FF | 5     |
| $C00B   | 49163   | W      | SLOTC3ROMON  | Enable slot 3 ROM $C300-$C3FF   | 5     |

**Note 5:** SLOTC3ROM switches only affect $C300-$C3FF when INTCXROM is OFF.

**ROM Switching Behavior:**

- Internal ROM contains: monitor extensions, self-test, 80-column firmware
- Peripheral cards have ROM at $C100-$C7FF (256 bytes per slot) and shared expansion ROM at $C800-$CFFF
- INTCXROMON ($C007): Selects all internal ROM $C100-$CFFF
- INTCXROMOFF ($C006): Allows peripheral card ROMs to be active
- When INTCXROM is OFF, SLOTC3ROM controls whether $C300-$C3FF uses slot 3 ROM or internal 80-column firmware

---

## Video Soft Switches

### Video Control ($C00C-$C00F, $C050-$C057)

| Address | Decimal | Access | Symbol        | Description                         | Notes |
| ------- | ------- | ------ | ------------- | ----------------------------------- | ----- |
| $C00C   | 49164   | W      | 80COLOFF      | Turn off 80-column display          |       |
| $C00D   | 49165   | W      | 80COLON       | Turn on 80-column display           |       |
| $C00E   | 49166   | W      | ALTCHARSETOFF | Turn off alternate characters       |       |
| $C00F   | 49167   | W      | ALTCHARSETON  | Turn on alternate characters        |       |
| $C050   | 49232   | RW     | TEXTOFF       | Select graphics mode                |       |
| $C051   | 49233   | RW     | TEXTON        | Select text mode                    |       |
| $C052   | 49234   | RW     | MIXEDOFF      | Full screen graphics                | 2     |
| $C053   | 49235   | RW     | MIXEDON       | Graphics with 4 lines of text       | 2     |
| $C054   | 49236   | RW     | PAGE20FF      | Select page1 (or main video memory) | 1     |
| $C055   | 49237   | RW     | PAGE20N       | Select page2 (or aux. video memory) | 1     |
| $C056   | 49238   | RW     | HIRESOFF      | Select low-resolution graphics      | 1,2   |
| $C057   | 49239   | RW     | HIRESON       | Select high-resolution graphics     | 1,2   |

**Note 1:** Behavior depends on 80STORE switch state. See Memory Management section.

**Note 2:** HIRES and MIXED switches only meaningful when TEXT is OFF (graphics mode active).

**Activation:** Switches $C050-$C057 can be activated by either reading OR writing. Other switches require writing.

---

## Soft Switch Status Flags

These read-only locations report the current state of various soft switches.

| Address | Decimal | Access | Symbol     | Status Description                                 | Notes |
| ------- | ------- | ------ | ---------- | -------------------------------------------------- | ----- |
| $C010   | 49168   | R7     | AKD        | 1=key pressed, 0=all keys released                 | 3     |
| $C011   | 49169   | R7     | BSRBANK2   | 1=bank2 BSR available, 0=bank1 available           |       |
| $C012   | 49170   | R7     | BSRREADRAM | 1=BSR active for reads, 0=ROM active               |       |
| $C013   | 49171   | R7     | RAMRD      | 0=main $200-$BFFF active, 1=aux. active (reads)    | 4     |
| $C014   | 49172   | R7     | RAMWRT     | 0=main $200-$BFFF active, 1=aux. active (writes)   | 4     |
| $C015   | 49173   | R7     | INTCXROM   | 1=internal $C100-$CFFF active, 0=slot ROM active   | 5     |
| $C016   | 49174   | R7     | ALTZP      | 1=aux. ZP/stack/BSR, 0=main ZP/stack/BSR           |       |
| $C017   | 49175   | R7     | SLOTC3ROM  | 1=slot 3 ROM active, 0=internal $C3 ROM active     | 5     |
| $C018   | 49176   | R7     | 80STORE    | 1=PAGE2 switches main/aux., 0=PAGE2 switches pages | 1     |
| $C019   | 49177   | R7     | VERTBLANK  | 1=vertical retrace on, 0=off                       |       |
| $C01A   | 49178   | R7     | TEXT       | 1=text mode, 0=graphics mode                       |       |
| $C01B   | 49179   | R7     | MIXED      | 1=mixed graphics/text, 0=full screen               | 2     |
| $C01C   | 49180   | R7     | PAGE2      | 1=page2 or aux. video, 0=page1 or main video       | 1     |
| $C01D   | 49181   | R7     | HIRES      | 1=high-res graphics, 0=low-res graphics            | 1,2   |
| $C01E   | 49182   | R7     | ALTCHARSET | 1=alternate character set on, 0=primary set on     |       |
| $C01F   | 49183   | R7     | 80COL      | 1=80-column display on, 0=40-column on             |       |

**Access R7:** Read bit 7 to get status (>=$80 means ON, <$80 means OFF)

**Note 3:** Reading $C010 (KBDSTRB) clears the keyboard strobe (bit 7 of $C000).

---

## Annunciator Switches

Game I/O connector annunciators. Can be turned on/off independently.

| Address | Decimal | Access | Symbol | Description            |
| ------- | ------- | ------ | ------ | ---------------------- |
| $C058   | 49240   | RW     | CLRAN0 | Turn off annunciator 0 |
| $C059   | 49241   | RW     | SETAN0 | Turn on annunciator 0  |
| $C05A   | 49242   | RW     | CLRAN1 | Turn off annunciator 1 |
| $C05B   | 49243   | RW     | SETAN1 | Turn on annunciator 1  |
| $C05C   | 49244   | RW     | CLRAN2 | Turn off annunciator 2 |
| $C05D   | 49245   | RW     | SETAN2 | Turn on annunciator 2  |
| $C05E   | 49246   | RW     | CLRAN3 | Turn off annunciator 3 |
| $C05F   | 49247   | RW     | SETAN3 | Turn on annunciator 3  |

**Usage:** Annunciator 3 is used for double-width graphics modes. Others available for custom hardware.

---

## Built-In Device I/O

### Keyboard and Speaker

| Address | Decimal | Access | Symbol   | Description                                   |
| ------- | ------- | ------ | -------- | --------------------------------------------- |
| $C000   | 49152   | R      | KBD      | Keyboard data (bits 0-6: ASCII code)          |
| $C000   | 49152   | R7     | KBD      | 1=keyboard stroke available, 0=no key pressed |
| $C010   | 49168   | RW     | KBDSTRB  | Clear keyboard strobe (read or write)         |
| $C010   | 49168   | R7     | AKD      | 1=key being pressed, 0=all keys released      |
| $C020   | 49184   | R      | CASSOUT  | Toggle cassette output port state             |
| $C030   | 49200   | R      | SPEAKER  | Toggle speaker state (click)                  |
| $C040   | 49216   | R      | GCSTROBE | Generate game I/O connector strobe signal     |

**Keyboard Usage:**

1. Check $C000 bit 7 to see if key pressed
2. If set, read $C000 bits 0-6 for ASCII code
3. Access $C010 to clear strobe (allows next key to be detected)

### Game Controllers

| Address | Decimal | Access | Symbol  | Description                          |
| ------- | ------- | ------ | ------- | ------------------------------------ |
| $C060   | 49248   | R7     | CASSIN  | 1=cassette input on                  |
| $C061   | 49249   | R7     | PB0     | 1=push button 0 pressed              |
| $C062   | 49250   | R7     | PB1     | 1=push button 1 pressed              |
| $C063   | 49251   | R7     | PB2     | 1=push button 2 pressed (OPEN-APPLE) |
| $C064   | 49252   | R7     | GC0     | 0=game controller 0 timed out        |
| $C065   | 49253   | R7     | GC1     | 0=game controller 1 timed out        |
| $C066   | 49254   | R7     | GC2     | 0=game controller 2 timed out        |
| $C067   | 49255   | R7     | GC3     | 0=game controller 3 timed out        |
| $C070   | 49264   | R      | GCRESET | Reset game controller timers         |

**Note:** PB2 ($C063) also reads the OPEN-APPLE key state.

---

## Bank-Switched RAM

### Overview

The Apple IIe contains 16K of bank-switched RAM (BSR) at addresses normally occupied by ROM:

- **$D000-$DFFF:** Two 4K banks (Bank 1 and Bank 2)
- **$E000-$FFFF:** One 8K area (shared by both banks)

This allows 64K total RAM (48K main + 16K bank-switched) even though ROM also exists at $D000-$FFFF.

### Bank-Switched RAM Control ($C080-$C08F)

The switches control three independent aspects:

1. **Bank selection:** Bank 1 or Bank 2 for $D000-$DFFF
2. **Read selection:** Read from RAM or ROM at $D000-$FFFF
3. **Write selection:** Write to RAM or ROM (write-protect)

| Address | Decimal | Access | Symbol    | Bank | Read From | Write Enable | Notes |
| ------- | ------- | ------ | --------- | ---- | --------- | ------------ | ----- |
| $C080   | 49280   | R      | READBSR2  | 2    | RAM       | No           |       |
| $C081   | 49281   | RR     | WRITEBSR2 | 2    | ROM       | Yes          | \*    |
| $C082   | 49282   | R      | OFFBSR2   | 2    | ROM       | No           |       |
| $C083   | 49283   | RR     | RDWRBSR2  | 2    | RAM       | Yes          | \*    |
| $C088   | 49288   | R      | READBSR1  | 1    | RAM       | No           |       |
| $C089   | 49289   | RR     | WRITEBSR1 | 1    | ROM       | Yes          | \*    |
| $C08A   | 49290   | R      | OFFBSR1   | 1    | ROM       | No           |       |
| $C08B   | 49291   | RR     | RDWRBSR1  | 1    | RAM       | Yes          | \*    |

**Duplicates:** $C084-$C087 duplicate $C080-$C083, $C08C-$C08F duplicate $C088-$C08B

**Access RR (\*):** Must read TWICE in succession to enable write. Single read only affects read/bank selection.

### Control Bits ($C08X)

```text
Bit 3: BANK-SELECT    (1=Bank 1, 0=Bank 2)
Bit 2: (unused)
Bit 1: READ-SELECT    (if equals write-select bit, read from RAM; else ROM)
Bit 0: WRITE-SELECT   (1 + double-read = write to RAM; else ROM)
```

### Bank-Switched RAM Status

| Address | Decimal | Access | Symbol     | Description                             |
| ------- | ------- | ------ | ---------- | --------------------------------------- |
| $C011   | 49169   | R7     | BSRBANK2   | >=$80: Bank2 active, <$80: Bank1 active |
| $C012   | 49170   | R7     | BSRREADRAM | >=$80: RAM enabled for reads, <$80: ROM |

#### Example: Preserve and Restore BSR State

```assembly
        LDA BSRBANK2        ; Save bank status
        STA BANKSAVE
        LDA BSRREADRAM      ; Save read-enable status
        STA READSAVE
        ; ... fiddle with BSR ...
        LDA BANKSAVE        ; Restore original state
        BPL SETBANK1
        LDA READSAVE
        BPL SETROM
        LDA $C0B3           ; Read RAM, Bank2, write-enable
        LDA $C0B3           ; Second read to enable write
        JMP DONE
SETROM: LDA $C082           ; ROM read, Bank2, write-protect
        JMP DONE
SETBANK1:
        ; ... similar for Bank1
DONE:
```

### Common Usage Patterns

**Read from RAM, write-protect:**

```assembly
        LDA $C080           ; Bank2 read RAM, write-protect
```

**Read from ROM, write-enable RAM (e.g., for ProDOS):**

```assembly
        LDA $C081           ; Bank2 read ROM, write-enable
        LDA $C081           ; Second read required
```

**Read from RAM, write-enable RAM:**

```assembly
        LDA $C083           ; Bank2 read/write RAM
        LDA $C083           ; Second read required
```

---

## Bank-Switched ROM

### Internal ROM vs. Peripheral Card ROM

The Apple IIe contains multiple ROM areas at overlapping addresses:

**$C100-$CFFF Areas:**

1. **Internal ROM:** Monitor extensions, self-test, 80-column firmware
2. **Peripheral Card ROM:** Up to 7 slots × 256 bytes ($C100-$C7FF)
3. **Expansion ROM:** Shared $C800-$CFFF space (peripheral cards)

### ROM Switching Summary

| Switch        | Address | $C100-$C2FF  | $C300-$C3FF  | $C400-$C7FF  | $C800-$CFFF   |
| ------------- | ------- | ------------ | ------------ | ------------ | ------------- |
| INTCXROMON    | $C007   | Internal ROM | Internal ROM | Internal ROM | Internal ROM  |
| INTCXROMOFF + | $C006   | Slot ROM     | (SLOTC3ROM)  | Slot ROM     | Expansion ROM |
| SLOTC3ROMOFF  | $C00A   |              | Internal ROM |              |               |
| INTCXROMOFF + | $C006   | Slot ROM     | (SLOTC3ROM)  | Slot ROM     | Expansion ROM |
| SLOTC3ROMON   | $C00B   |              | Slot 3 ROM   |              |               |

**Expansion ROM Behavior:**

- $C800-$CFFF is shared among all peripheral cards
- Access to $CFFF turns off all expansion ROMs
- Access to any $C100-$C7FF slot page turns on that slot's expansion ROM
- Access to $C300-$C3FF (when internal ROM selected) selects internal expansion ROM

---

## Peripheral Card I/O

### Slot I/O Ranges ($C090-$C0FF)

Each expansion slot has 16 bytes reserved for device-specific I/O:

| Slot | Address Range | Description          |
| ---- | ------------- | -------------------- |
| 1    | $C090-$C09F   | Slot 1 I/O           |
| 2    | $C0A0-$C0AF   | Slot 2 I/O           |
| 3    | $C0B0-$C0BF   | Slot 3 I/O           |
| 4    | $C0C0-$C0CF   | Slot 4 I/O           |
| 5    | $C0D0-$C0DF   | Slot 5 I/O           |
| 6    | $C0E0-$C0EF   | Slot 6 I/O (Disk II) |
| 7    | $C0F0-$C0FF   | Slot 7 I/O           |

**Formula:** Slot `s` I/O base = $C080 + $10\*s

**Additional Ranges:**

- Slot ROM: $C100 + $100*s to $C1FF + $100*s (256 bytes each, $C100-$C7FF)
- Expansion ROM: $C800-$CFFF (shared, controlled by accessing slot pages)

---

## Auxiliary Memory

### Extended 80-Column Card

The extended 80-column text card adds 64K of auxiliary RAM, giving the Apple IIe a total of 128K RAM.

### Auxiliary Memory Ranges

**Available with Extended 80-Column Card:**

- $0000-$01FF: Auxiliary zero page and stack
- $0200-$BFFF: 47.5K auxiliary main memory
- $D000-$FFFF: 16K auxiliary bank-switched RAM (same structure as main BSR)

### Accessing Auxiliary Memory

**Zero Page/Stack ($0000-$01FF):**

- ALTZPOFF ($C008): Use main ZP/stack
- ALTZPON ($C009): Use auxiliary ZP/stack

**Main Memory ($0200-$BFFF):**

- RAMRDOFF ($C002), RAMRDON ($C003): Control read source
- RAMWRTOFF ($C004), RAMWRTON ($C005): Control write destination

**Bank-Switched RAM ($D000-$FFFF):**

- ALTZP switch also controls which BSR (main or aux.) is available
- Same $C080-$C08F switches control the currently-selected BSR

### Video Memory and 80STORE

The 80STORE switch ($C000/$C001) changes how PAGE2 works:

**80STORE OFF (default):**

- PAGE20FF ($C054): Select text page1 / hi-res page1
- PAGE20N ($C055): Select text page2 / hi-res page2

**80STORE ON (80-column mode):**

- PAGE20FF ($C054): Select main video RAM ($400-$7FF, $2000-$3FFF if HIRES on)
- PAGE20N ($C055): Select aux. video RAM ($400-$7FF, $2000-$3FFF if HIRES on)

**Special Note:** When 80STORE is ON, the RAMRD/RAMWRT switches do NOT affect:

- Text/lo-res video RAM: $400-$7FF (controlled by PAGE2)
- Hi-res video RAM: $2000-$3FFF (controlled by PAGE2 when HIRES is also ON)

### 80-Column Display

**Text Display:**

- 80COLON ($C00D): Enable 80-column display
- Even columns (0,2,4...78): Stored in auxiliary memory
- Odd columns (1,3,5...79): Stored in main memory
- Data interleaved: requires alternating main/aux. writes

**Double-Width Graphics:**

- Similar interleaving for double-width lo-res and hi-res
- Requires setting 80COLON and SETAN3 ($C05E)
- Even pixels from auxiliary memory, odd from main

---

## Memory Management Interactions

### Complex Interactions Summary

1. **Video RAM ($400-$7FF, $2000-$3FFF):**
    - If 80STORE ON: Controlled by PAGE2 (main vs. aux.)
    - If 80STORE OFF: Controlled by RAMRD/RAMWRT like other memory

2. **ROM Selection ($C100-$CFFF):**
    - INTCXROM: Main switch (internal vs. slot ROM)
    - SLOTC3ROM: Sub-switch for $C300-$C3FF only (effective when INTCXROM OFF)

3. **Bank-Switched RAM ($D000-$FFFF):**
    - ALTZP selects main or auxiliary BSR
    - $C080-$C08F control the selected BSR's bank/read/write state
    - Can read from ROM while writing to RAM (common pattern)

4. **Zero Page and Stack ($0000-$01FF):**
    - ALTZP controls which is active
    - Switching changes where:
        - Zero-page variables live
        - Stack operations occur
        - Which BSR is accessible via $C080-$C08F

---

## Quick Reference: Common Symbols

### Most Frequently Used Symbols

**Keyboard:**

- KBD ($C000): Read key data + strobe status
- KBDSTRB ($C010): Clear keyboard strobe

**Video:**

- TEXTOFF ($C050), TEXTON ($C051)
- MIXEDOFF ($C052), MIXEDON ($C053)
- PAGE20FF ($C054), PAGE20N ($C055)
- HIRESOFF ($C056), HIRESON ($C057)

**Memory Management:**

- RAMRDOFF ($C002), RAMRDON ($C003)
- RAMWRTOFF ($C004), RAMWRTON ($C005)
- INTCXROMOFF ($C006), INTCXROMON ($C007)

**Bank-Switched RAM:**

- READBSR2 ($C080), WRITEBSR2 ($C081)
- OFFBSR2 ($C082), RDWRBSR2 ($C083)
- READBSR1 ($C088), WRITEBSR1 ($C089)
- OFFBSR1 ($C08A), RDWRBSR1 ($C08B)

**Status Flags:**

- BSRBANK2 ($C011): BSR bank status
- BSRREADRAM ($C012): BSR read enable status
- RAMRD ($C013), RAMWRT ($C014): Main/aux. status
- INTCXROM ($C015): ROM selection status

**80-Column:**

- 80COLOFF ($C00C), 80COLON ($C00D)
- 80STORE ($C018): Status flag

---

## Notes and Conventions

### Access Codes

- **R:** Read only
- **W:** Write only
- **RW:** Read or Write
- **R7:** Read bit 7 for status (>=$80 = ON/true, <$80 = OFF/false)
- **RR:** Must read twice in succession for full effect

### Naming Conventions

- Switches typically have ON/OFF pairs (e.g., TEXTON/TEXTOFF)
- Status locations use same name as switch or descriptive name
- Memory ranges: "main" (motherboard) vs. "aux." (80-column card)
- BSR: Bank-Switched RAM
- ZP: Zero Page

### Reset Defaults

- Most switches return to default state on RESET
- 80-column switches default based on whether extended card installed
- Bank-switched RAM defaults to ROM read, write-protected

---

## References

This document was extracted from:

- **Inside the Apple IIe** (book)
- Chapter 8: Memory Management
- Appendix III: Memory Map and I/O Assignments

For complete details on using these features, refer to the full text.

---

**Document Version:** 1.0  
**Extracted:** 2026-01-31  
**Source File:** docs/Inside the Apple IIe.txt
