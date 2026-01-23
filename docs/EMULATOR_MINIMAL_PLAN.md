# Minimal EDASM Emulator Plan

Goal: run original EDASM.SYSTEM with a tiny 65C02 emulator focused on trap-first discovery.

## Decisions (initial)

- CPU: 65C02 superset; reserve host-callback opcode byte $02 (unused/JAM on 65C02). All uninitialized fetches hit $02.
- Memory image at reset: entire address space prefilled with $02 (host-call trap). EDASM.SYSTEM loaded at $2000 (confirmed load and entrypoint). Vectors set to start EDASM entrypoint manually.
- Traps: address-indexed dispatch tables.
    - Opcode fetch trap keyed by PC.
    - $C000–$C0FF read/write traps keyed by address + access type.
    - Default handler: log PC/address/access, CPU registers, and a memory window; then halt.
- ROM/ProDOS: no full ProDOS; map only needed services to host shims as discovered. Use extracted binaries, not .2mg boot.
- Text screen: use Apple II text page layout ($0400–$07FF) per docs/apple2-text-screen-layout.md for optional visualization or scraping; 80-column not required initially.
- Host services: provide EXEC-like line feeder (monitor/GETLN stand-in) to push EDASM commands; adjust as traps reveal needs.

## Implementation Sketch

- Modules (under src/core with headers in include/edasm):
    - cpu: decode/execute 65C02, includes handler for opcode $02 -> host trap callback.
    - bus: address decode, RAM/ROM regions, load EDASM image at $2000, vector setup.
    - traps: dispatcher tables for opcode fetch and $C0xx read/write; default logger/halting stub; configurable handlers.
    - host_shims: optional ProDOS/monitor stand-ins hooked via traps or vectors; EXEC-like feeder utility.
    - screen: helper for text page addressing/render (optional for debug/UI).
- Loader: read EDASM.SYSTEM binary, place at $2000, set PC to entrypoint $2000.
- Run loop: single-step with tracing option; on trap, invoke dispatcher; on unknown, log+halt.

## Open Questions (to iterate by tracing)

- Which $C0xx soft switches EDASM actually touches.
- Minimal ProDOS/monitor vectors required (GETLN/RDKEY/etc.) and how to map to host.
- Memory dump size/format on unknown trap; where to write logs/artifacts.

## Next Actions

1. Stand up cpu + bus skeleton with opcode $02 trap path; prefills with $02 and loads EDASM at $2000.
2. Add trap dispatch tables and default logger/halting behavior.
3. Implement tiny runner that feeds a canned EXEC-like script to reach first trap; capture logs for analysis.
