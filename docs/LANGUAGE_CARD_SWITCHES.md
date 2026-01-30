# Language Card Soft-Switches — Extracted Reference 🛠️

This document extracts the language‑card soft‑switch control addresses and behavior from the Apple "Language Card — Installation and Operation Manual" (Internet Archive OCR) and canonical symbol/equates used by EDASM and AppleWin.

Sources:

- Apple Language Card — Installation and Operation Manual (Internet Archive): <https://archive.org/details/APPLE_Language_Card_Installation_Operation_Manual>
- EDASM original equates: `third_party/EdAsm/EDASM.SRC/COMMONEQUS.S`
- AppleWin symbol table: <https://raw.githubusercontent.com/AppleWin/AppleWin/master/bin/APPLE2E.SYM>

---

## Canonical control addresses ($C080–$C08F)

| Address | Symbol (common names) | Behavior / Notes                                                                                                                |
| ------: | :-------------------- | :------------------------------------------------------------------------------------------------------------------------------ |
|   $C080 | `RDBANK2`, `LCRAMIN2` | Reads select **RAM bank 2** into the $D000–$DFFF window (read behavior). Writes may be ignored depending on the exact mode.     |
|   $C081 | `ROMIN2`, `ROMIN2`    | **Read ROM / Write RAM** for bank 2 (common mode used to copy ROM → RAM; reads return ROM bytes, writes update underlying RAM). |
|   $C082 | `RDROM2`, `LCROMIN2`  | **Read ROM only** (writes ignored, ROM active).                                                                                 |
|   $C083 | `LCBANK2`, `LCBANK2`  | **Read/Write RAM** bank 2 (both reads and writes target language‑card RAM in the $D000 window).                                 |
|   $C084 | `LCRAMIN2_`           | Variant/alternate encoding for bank2 RAM/read modes (underscore variants appear in symbol tables).                              |
|   $C085 | `ROMIN2_`             | Variant/alternate encoding (see manual for exact bit semantics).                                                                |
|   $C086 | `LCROMIN2_`           | Variant/alternate encoding.                                                                                                     |
|   $C087 | `LCBANK2_`            | Variant/alternate encoding.                                                                                                     |
|   $C088 | `LCRAMIN1`            | Bank 1 equivalents (same semantics as bank 2 but for the alternate 4K bank window).                                             |
|   $C089 | `ROMIN1`              | Read ROM / Write RAM — bank 1.                                                                                                  |
|   $C08A | `LCROMIN1`            | Read ROM (bank1 variant).                                                                                                       |
|   $C08B | `LCBANK1`             | Read/Write RAM — bank 1.                                                                                                        |
|   $C08C | `LCRAMIN1_`           | Variant/alternate encoding.                                                                                                     |
|   $C08D | `ROMIN1_`             | Variant/alternate encoding.                                                                                                     |
|   $C08E | `LCROMIN1_`           | Variant/alternate encoding.                                                                                                     |
|   $C08F | `LCBANK1_`            | Variant/alternate encoding.                                                                                                     |

Notes:

- The underscore (`_`) variants appear in symbol tables (AppleWin) and vendor manuals as alternate encodings of the same logical switches (often because certain bits in the address are ignored by hardware). The Apple manual describes that bit 2 is ignored for some ranges and that consecutive reads toggle selection when appropriate.
- The language card typically provides two selectable 4K banks of RAM and a small area that can be mapped without bank switching; the soft‑switches select which 4K bank is visible in $D000–$DFFF and whether reads come from ROM or RAM and whether writes are allowed.

---

## Practical monitor/test sequence (from manual and community sources)

Example sequence used to copy ROM onto the language card RAM and verify:

1. Enter monitor (CALL -151).
2. Put the card into read‑ROM/write‑RAM mode: `C081 C081` (Return).
3. Copy ROM into main memory and the card: `2000<D000.FF FFM  D000<D000.FFFFM` (example copy commands shown in the manual).
4. Put the card into read‑RAM/write‑RAM mode: `C083 C083`.
5. Compare language card RAM against main memory RAM: `2000<D000.FFFFV` — if nothing prints, copy was successful.

(See Applefritter thread and Microsoft RAMCard manual for practical examples.)

---

## Implementation guidelines for emulator

- Implement I/O traps for `$C080..$C08F` to update internal per‑bank mode state.
- On reads in banked window(s), consult state to decide whether to return ROM data or the language‑card RAM image.
- On writes in banked window(s), update language‑card RAM only when allowed by the current mode (e.g., `ROMIN` allows writes to RAM but reads still come from ROM; `RDROM` disallows writes).
- Preserve/restore bank registers across MLI calls as the ProDOS monitor/MLI expects (see ProDOS manual and MLI code references in repo).

---

## Appendix D — Cleaned excerpt (verbatim, cleaned)

> "The control codes independently do three things: write‑protect or write‑enable the RAM of the Language Card; select or deselect RAM read; and select the first or second 4K bank of RAM for the address space $D000–$DFFF. This bank selection is necessary because the address space $D000–$DFFF overlaps with I/O and other routines, so two 4K banks are switched into that window. Bit 3 in a control code selects which 4K block is located in $D000–$DFFF: if bit 3 = 0 the first 4K bank is mapped into $D000–$DFFF; if bit 3 = 1 the second 4K bank is mapped into $D000–$DFFF. (Appendix D, Apple Language Card — Installation and Operation Manual, cleaned OCR excerpt.)"

_Notes:_ the original OCR is noisy in places; this excerpt preserves the manual's wording while correcting obvious OCR artifacts (e.g., character misrecognition). For the full context, see the manual (pages in Appendix D) at: <https://archive.org/details/APPLE_Language_Card_Installation_Operation_Manual>

---

If you'd like, I can also add the short assembly example or implement the soft‑switch handlers in the emulator next.
