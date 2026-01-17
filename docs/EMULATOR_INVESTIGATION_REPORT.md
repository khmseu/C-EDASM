# Emulator Options Investigation Report

## Executive Summary

This document provides a detailed investigation of the emulator options outlined in DEBUGGER_EMULATOR_PLAN.md. Based on research conducted in January 2026, this report analyzes each option's suitability for hosting the original Apple II EDASM within our C-EDASM testing infrastructure.

**Recommendation:** Proceed with **MAME (apple2e driver)** as the primary emulator choice for CI/CD integration and automated testing.

## Research Methodology

Investigation included:

- Review of official documentation and community forums
- Analysis of automation capabilities and scripting APIs
- Assessment of ProDOS disk image support
- Evaluation of CI/CD integration feasibility
- Consideration of maintenance burden and learning curve

## Detailed Option Analysis

### Option 1: MAME (apple2e driver) — RECOMMENDED

#### Overview

MAME (Multiple Arcade Machine Emulator) is a mature, cross-platform emulator with extensive Apple IIe support. As of 2024-2026, MAME's Lua scripting capabilities have matured significantly for automation use cases.

#### Strengths

1. **High Fidelity Emulation**
   - Accurate language card emulation
   - Complete soft switch support ($C0xx range)
   - Full ProDOS MLI compatibility
   - Disk II controller accuracy

2. **Disk Image Support**
   - Native support for .2mg, .po, .dsk formats
   - Multiple disk mounting (e.g., `-flop1`, `-flop2`)
   - Read/write operations to mounted images

3. **Automation & Headless Operation**
   - Command-line flags for headless mode:

     ```bash
     mame apple2e -flop1 edasm_src.2mg -video none -sound none -nothrottle
     ```

   - `-autoboot_script` for Lua automation
   - `-autoboot_delay` for timing control
   - No X11/display server required for headless runs

4. **Lua Scripting API**
   - **Keystroke injection**: `emu.keypost()` for simulating keyboard input
   - **Memory access**: Read/write CPU memory and registers
   - **Breakpoints**: Set programmatic breakpoints for specific addresses
   - **State management**: Save/restore emulator state
   - **Event hooks**: Frame completion, pause, reset events
   - **Disk I/O hooks**: Intercept and manipulate disk operations

5. **CI/CD Friendliness**
   - Deterministic execution with `-nothrottle`
   - Log file generation
   - Exit codes for scripting
   - Well-documented CLI interface

6. **Community & Documentation**
   - Extensive official Lua API documentation
   - Active community with example scripts
   - Regular updates and maintenance
   - Cross-platform (Linux, macOS, Windows)

#### Weaknesses

1. **Dependency Weight**
   - Large binary (~100+ MB)
   - Multiple library dependencies
   - Longer installation time in CI environments

2. **Learning Curve**
   - Lua scripting knowledge required
   - Complex API for advanced automation
   - Documentation, while extensive, requires careful reading

3. **API Stability**
   - Lua API can change between MAME versions (though rarely breaks)
   - Scripts may need maintenance across MAME updates

#### Implementation Path

**Phase 1: Proof of Concept (1-2 days)**

```bash
# Install MAME
apt-get install mame  # or build from source

# Basic boot test
mame apple2e -flop1 third_party/EdAsm/EDASM_SRC.2mg \
     -video none -sound none -nothrottle \
     -autoboot_delay 2 -autoboot_script tests/emulator/boot_test.lua
```

Example `boot_test.lua`:

```lua
function wait_for_prompt()
    -- Wait for ProDOS prompt by checking memory or screen
    local cycles = 0
    while cycles < 1000000 do
        emu.wait(1000)  -- Wait 1000 cycles
        cycles = cycles + 1000
    end
end

function boot_edasm()
    wait_for_prompt()
    -- Type "EDASM.SYSTEM" and press return
    emu.keypost("EDASM.SYSTEM\n")
end

emu.register_start(function()
    boot_edasm()
end)
```

**Phase 2: File Injection & Extraction (1-2 days)**

- Create writable .2mg disk image with test sources
- Mount as second floppy (`-flop2`)
- Script EDASM to load, assemble, and save outputs
- Extract result files using ProDOS disk tools (see section below)

**Phase 3: CI Integration (1-2 days)**

- Containerize MAME with Dockerfile
- Create GitHub Actions workflow
- Implement timeout and retry logic
- Store golden artifacts for comparison

#### Resource Requirements

- Disk space: ~500 MB (MAME + dependencies + disk images)
- CI runtime: ~30-60 seconds per test run (with `-nothrottle`)
- Development time: 4-6 days total for full integration

---

### Option 2: GSPlus / KEGS

#### Overview

GSPlus is a modern fork of KEGS (Apple IIgs emulator) with improved cross-platform support. While primarily targeting the IIgs, it can emulate IIe environments.

#### Strengths

1. **Lighter Weight**
   - Smaller binary than MAME (~10-20 MB)
   - Fewer dependencies
   - Faster startup time

2. **CLI Configuration**
   - `-config <file>` for programmatic configuration
   - Predefined .gsp config files for different test scenarios
   - Command-line disk mounting

3. **Built-in Debugger**
   - Available with `WITH_DEBUGGER=ON` compile flag
   - Breakpoints and watchpoints
   - Memory inspection and modification
   - CPU state examination
   - Step-through execution

4. **ProDOS Support**
   - Decent ProDOS fidelity
   - Disk image mounting
   - Language card support

#### Weaknesses

1. **Limited Automation API**
   - No native scripting interface (no Lua/Python hooks)
   - Would require stdin/socket patching for automation
   - Screen scraping may be necessary for output verification

2. **Headless Mode Limitations**
   - No built-in headless mode
   - Would require SDL or display patching
   - Less suitable for CI without modifications

3. **Spotty .2mg Support**
   - May need conversion to .po/.dsk format
   - Disk image compatibility can be inconsistent

4. **Automation Gaps**
   - No programmatic keystroke injection
   - Limited state save/restore
   - Would need extensive patching for CI use

#### Potential Use Cases

- **Interactive debugging**: The built-in debugger is excellent for manual investigation
- **Development environment**: Good for developers needing step-through debugging
- **Fallback option**: If MAME proves too heavy for CI

#### Implementation Path (if MAME fails)

1. Build GSPlus with debugger enabled
2. Create config files for different test scenarios
3. Develop input injection mechanism (patching or wrapper)
4. Script output extraction via screen capture or memory dump
5. Estimated effort: 5-7 days (significant patching required)

---

### Option 3: LinApple / LinApple-Pie

#### Overview

LinApple is a Linux port of AppleWin, focusing on Apple II/IIe emulation. It's lightweight and has a smaller codebase.

#### Strengths

1. **Small Footprint**
   - Very small binary (~5-10 MB)
   - Minimal dependencies (SDL, libcurl)
   - Fast startup

2. **Easy to Patch**
   - Smaller, more approachable codebase
   - Fork-friendly for custom modifications
   - Active community

3. **CLI Flags**
   - `--d1 <disk>` and `--d2 <disk>` for disk mounting
   - `--autoboot` to skip splash screen
   - Basic automation possible

4. **ProDOS Support**
   - Supports .dsk and .po formats
   - Read/write operations work well
   - Compatible with ProDOS 8

#### Weaknesses

1. **No True Headless Mode**
   - Requires SDL window (though minimizable)
   - X11/display server needed (or Xvfb in CI)
   - Less elegant than MAME's headless option

2. **No .2mg Support**
   - Would need disk image conversion
   - May lose metadata in conversion

3. **Lower Fidelity**
   - Some soft switch edge cases may not be accurate
   - Language card support may be incomplete
   - Could affect EDASM behavior in edge cases

4. **No Scripting API**
   - No Lua or Python hooks
   - Would need socket/stdio patching for automation
   - Similar limitations to GSPlus

#### Potential Use Cases

- **Lightweight CI**: If disk space/time is critical
- **Quick testing**: For fast iteration during development
- **Fallback**: If MAME and GSPlus both fail

#### Implementation Path (if needed)

1. Fork LinApple for stdio/socket input
2. Add deterministic mode (no random delays)
3. Implement output capture mechanism
4. Use Xvfb in CI for display
5. Estimated effort: 4-6 days (moderate patching)

---

### Option 4: Build Custom Minimal Emulator

#### Overview

Create a purpose-built emulator focused solely on EDASM testing needs.

#### Strengths

1. **Full Control**
   - Tailored exactly to C-EDASM testing requirements
   - No unnecessary features
   - Tight integration with test infrastructure

2. **Minimal Dependencies**
   - Only implement what's needed
   - No GUI overhead
   - Optimized for CI execution

3. **Perfect Automation**
   - Built-in scripting from day one
   - Custom debugging interfaces
   - Ideal test hooks

#### Weaknesses

1. **High Development Cost**
   - Estimated 4-8 weeks of full-time development
   - Requires deep 6502 and Apple II knowledge
   - Significant testing required

2. **Complexity of Accurate Emulation**
   - **6502 CPU**: ~56 instructions, addressing modes (~1 week)
   - **Memory banking**: Main/aux RAM, language card (~1 week)
   - **ProDOS MLI**: File I/O, block device interface (~2 weeks)
   - **Disk II controller**: Sector timing, state machine (~1-2 weeks)
   - **Soft switches**: $C0xx range, peripheral slots (~1 week)
   - **Testing & debugging**: Edge cases, timing (~2 weeks)

3. **Maintenance Burden**
   - Becomes a project of its own
   - Bug fixes and updates needed
   - Diverts resources from C-EDASM development

4. **Risk**
   - May miss edge cases that EDASM depends on
   - Could give false positives/negatives in testing
   - Harder to verify correctness (what validates the validator?)

#### Recommendation

**NOT RECOMMENDED** unless all other options fail. The effort-to-benefit ratio is poor compared to leveraging existing, well-tested emulators.

#### Fallback Consideration

If pursued, start with an existing 6502 core (e.g., lib6502, fake6502) and add minimal Apple II glue logic. Estimated effort: 3-4 weeks minimum.

---

## ProDOS Disk Image Tooling

Regardless of emulator choice, we need tools to inject test sources and extract results from ProDOS disk images.

### Recommended Tools

#### 1. DiskM8 (Primary Recommendation)

- **Website**: <https://github.com/paleotronic/diskm8>
- **Platforms**: Windows, macOS, Linux, Raspberry Pi, FreeBSD
- **Formats**: DSK, PO, 2MG, NIB (DOS and ProDOS order)
- **Features**:
  - Extract files: `diskm8 extract <image> <output-dir>`
  - Inject files: `diskm8 inject <image> <file>`
  - List contents: `diskm8 list <image>`
  - Create images: `diskm8 create <image> <size>`
  - BASIC detokenization
  - Disk comparison

**Example Workflow:**

```bash
# Create writable test disk
diskm8 create test_sources.2mg 140KB

# Inject test sources
diskm8 inject test_sources.2mg tests/test_simple.src
diskm8 inject test_sources.2mg tests/test_expressions.src

# After EDASM run, extract outputs
diskm8 extract edasm_output.2mg ./outputs/
```

#### 2. AppleCommander (Alternative)

- **Website**: <https://applecommander.github.io/>
- **Platforms**: Cross-platform (Java)
- **Formats**: DO, DSK, PO, 2MG, HDV, NIB
- **Features**: Similar to DiskM8, with GUI option

**Example Usage:**

```bash
# List contents
ac -l mydisk.2mg

# Extract file
ac -e mydisk.2mg FILENAME > output.bin

# Inject file
ac -p mydisk.2mg newfile.bin PRODOS
```

#### 3. apple2_prodos_utils (Lightweight C++ Option)

- **GitHub**: <https://github.com/Michaelangel007/apple2_prodos_utils>
- **Best for**: Lightweight CLI integration, builds from source
- **Limitation**: Only supports seed/sapling files for writing

### Integration Strategy

**In CI Pipeline:**

1. **Pre-test**: Use DiskM8/AppleCommander to prepare test disk with sources
2. **Emulator run**: Mount test disk, run EDASM via Lua script
3. **Post-test**: Extract output files from result disk
4. **Comparison**: Byte-for-byte diff against C-EDASM outputs

---

## Comparative Summary

| Feature | MAME | GSPlus | LinApple | Custom |
|---------|------|--------|----------|--------|
| **Fidelity** | ★★★★★ | ★★★★☆ | ★★★☆☆ | ★★★☆☆ |
| **Automation** | ★★★★★ | ★★☆☆☆ | ★★☆☆☆ | ★★★★★ |
| **Headless** | ★★★★★ | ★☆☆☆☆ | ★★☆☆☆ | ★★★★★ |
| **Setup Time** | ★★★☆☆ | ★★★★☆ | ★★★★★ | ★☆☆☆☆ |
| **Learning Curve** | ★★☆☆☆ | ★★★☆☆ | ★★★★☆ | ★☆☆☆☆ |
| **CI Friendly** | ★★★★★ | ★★☆☆☆ | ★★★☆☆ | ★★★★☆ |
| **Disk Support** | ★★★★★ | ★★★☆☆ | ★★★☆☆ | ★★★☆☆ |
| **Maintenance** | ★★★★★ | ★★★☆☆ | ★★★☆☆ | ★★☆☆☆ |
| **Debugger** | ★★★★☆ | ★★★★★ | ★★☆☆☆ | ★★★★★ |
| **Effort** | 4-6 days | 5-7 days | 4-6 days | 20-40 days |

**Legend:** ★★★★★ Excellent | ★★★★☆ Good | ★★★☆☆ Adequate | ★★☆☆☆ Poor | ★☆☆☆☆ Very Poor

---

## Final Recommendation

### Primary Path: MAME + DiskM8

**Reasoning:**

1. **Best automation**: Lua scripting provides complete control
2. **Highest fidelity**: Most accurate emulation reduces false test failures
3. **CI-ready**: Headless mode, deterministic execution, good exit codes
4. **Well-documented**: Extensive community resources and examples
5. **Future-proof**: Active development, regular updates

**Trade-offs Accepted:**

- Larger dependency (mitigated by Docker container)
- Lua learning curve (one-time investment, 1-2 days)
- Slightly slower CI runs (acceptable with `-nothrottle`)

**Estimated Timeline:**

- Week 1: Proof of concept (boot EDASM, run simple test)
- Week 2: Full automation (inject sources, extract outputs, compare)
- Week 3: CI integration (Docker, GitHub Actions, documentation)

### Contingency Plans

**If MAME proves problematic:**

1. **First fallback**: LinApple with Xvfb (simpler patching, good enough fidelity)
2. **Second fallback**: GSPlus with patching (better fidelity, more patching effort)
3. **Last resort**: Custom emulator (only if absolutely necessary)

---

## Open Questions & Next Steps

### Questions for Project Decision

1. **CI Budget**: Is 30-60 seconds per test run acceptable?
2. **Disk Image Strategy**: Commit golden outputs, or regenerate per run?
3. **Test Coverage**: Full EDASM feature set, or subset initially?
4. **80-Column Support**: Required for tests, or 40-column sufficient?
5. **Printer Emulation**: Needed for LST file tests?

### Immediate Next Steps

1. **Install MAME**: Verify `apple2e` driver works with EDASM_SRC.2mg
2. **Create boot script**: Simple Lua script to boot ProDOS and launch EDASM
3. **Test file I/O**: Inject a test source, assemble, extract output
4. **Prototype comparison**: Run same source through C-EDASM and EDASM, diff results
5. **Document findings**: Update plan with any discovered issues

### Proposed Testing Workflow

```bash
# 1. Prepare test disk
diskm8 create /tmp/test_disk.2mg 140KB
diskm8 inject /tmp/test_disk.2mg tests/test_simple.src

# 2. Run MAME with automation script
mame apple2e \
  -flop1 third_party/EdAsm/EDASM_SRC.2mg \
  -flop2 /tmp/test_disk.2mg \
  -video none -sound none -nothrottle \
  -autoboot_delay 2 \
  -autoboot_script tests/emulator/assemble_test.lua

# 3. Extract results
diskm8 extract /tmp/test_disk.2mg /tmp/edasm_output/

# 4. Compare with C-EDASM
./build/edasm_cli tests/test_simple.src -o /tmp/cedasm_output.bin
diff /tmp/edasm_output/TEST_SIMPLE.BIN /tmp/cedasm_output.bin
```

---

## References

### MAME Resources

- Official Lua API: <https://docs.mamedev.org/luascript/index.html>
- Autoboot examples: <https://forums.launchbox-app.com/topic/78092-autoboot-lua-scripts-in-mame/>
- Apple II community: <https://forums.atariage.com/forum/179-apple-ii/>

### Disk Tools

- DiskM8: <https://github.com/paleotronic/diskm8>
- AppleCommander: <https://applecommander.github.io/>
- ProDOS utilities: <https://github.com/Michaelangel007/apple2_prodos_utils>

### Alternative Emulators

- GSPlus: <https://github.com/digarok/gsplus>
- LinApple: <https://github.com/linappleii/linapple>
- KEGS: <https://kegs.sourceforge.net/>

### Apple II Technical References

- ProDOS Technical Reference: <https://prodos8.com/>
- Apple II Documentation Project: <https://www.a2central.com/>
- Language Card Guide: <http://www.easy68k.com/paulrsm/6502/LANGUAGE.html>

---

## Conclusion

After thorough investigation, **MAME with Lua automation** is the clear best choice for hosting the original EDASM in our testing infrastructure. While it has a steeper initial learning curve and larger dependency footprint, its superior automation capabilities, accuracy, and CI-friendliness make it the most sustainable long-term solution.

The alternative emulators (GSPlus, LinApple) are viable fallbacks but would require significant patching to achieve the same level of automation. Building a custom emulator is not recommended due to the high effort and risk involved.

With MAME + DiskM8 + Lua scripting, we can achieve:

- Automated test source injection
- Scripted EDASM assembly and linking
- Artifact extraction and comparison
- Headless CI execution
- Reproducible, deterministic test runs

**Recommended Action:** Proceed with MAME prototype (4-6 day investment) to validate the approach before committing to full CI integration.

---

*Report compiled: January 2026*  
*Based on: DEBUGGER_EMULATOR_PLAN.md + external research*  
*Status: Ready for review and decision*
