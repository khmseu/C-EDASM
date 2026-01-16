# Emulator Decision Matrix

Quick reference for choosing an emulator for EDASM testing automation.

## Decision Tree

```
Start: Need to automate EDASM testing
│
├─ Q1: Is CI/CD integration required?
│  ├─ Yes → Go to Q2
│  └─ No → Use any emulator interactively (GSPlus recommended for debugging)
│
├─ Q2: Can we accept ~500MB dependency for CI?
│  ├─ Yes → **MAME** (best automation, highest fidelity)
│  ├─ No → Go to Q3
│  └─ Maybe → Continue evaluation
│
├─ Q3: Is development time constrained (< 1 week)?
│  ├─ Yes → **LinApple** with Xvfb (quickest to set up)
│  └─ No → Go to Q4
│
├─ Q4: Is emulation fidelity critical (edge cases)?
│  ├─ Yes → **MAME** (accept dependency)
│  ├─ No → **LinApple** (good enough for most cases)
│  └─ Maybe → **GSPlus** (middle ground)
│
└─ Q5: Do we need interactive debugging?
   ├─ Yes → **GSPlus** (excellent built-in debugger)
   └─ No → See above recommendations
```

## Quick Comparison

| Criterion | MAME | GSPlus | LinApple | Custom |
|-----------|------|--------|----------|--------|
| **Best for** | CI/CD automation | Interactive debugging | Quick setup | Full control |
| **Setup time** | 1-2 hours | 30 mins | 15 mins | 3-4 weeks |
| **Automation time** | 4-6 days | 5-7 days | 4-6 days | 4-6 weeks |
| **Binary size** | ~150 MB | ~20 MB | ~10 MB | ~5 MB |
| **Scripting** | Lua (full API) | None (needs patching) | None (needs patching) | Built-in |
| **Headless mode** | Native | Needs work | Needs Xvfb | Native |
| **Apple IIe accuracy** | Excellent | Good | Adequate | Unknown |
| **Learning curve** | Moderate | Low | Low | High |
| **Maintenance** | External (upstream) | External (upstream) | External (upstream) | Internal |
| **Community support** | Excellent | Good | Good | None |
| **.2mg support** | Yes | Spotty | No | Up to us |
| **ProDOS fidelity** | Excellent | Good | Good | Unknown |
| **Language card** | Complete | Complete | May be incomplete | Up to us |
| **Documentation** | Extensive | Good | Basic | None |

## Use Case Recommendations

### Use Case: Automated CI/CD Testing
**Recommended:** MAME  
**Why:** Native headless mode, Lua automation, high fidelity, deterministic execution

**Alternative:** LinApple + Xvfb  
**Why:** If dependency size is a concern, LinApple can work with virtual display

### Use Case: Interactive Debugging of EDASM
**Recommended:** GSPlus  
**Why:** Built-in debugger with breakpoints, watchpoints, step-through execution

**Alternative:** MAME with debugger build  
**Why:** MAME also has a debugger, though Lua interface may be more awkward for interactive use

### Use Case: Quick Prototype Testing
**Recommended:** LinApple  
**Why:** Fastest to install and get running, good enough fidelity for basic tests

**Alternative:** GSPlus  
**Why:** Almost as quick, slightly better fidelity

### Use Case: Long-term Production Testing
**Recommended:** MAME  
**Why:** Best long-term maintainability, highest confidence in results, external maintenance

**Alternative:** Custom emulator  
**Why:** Only if MAME proves inadequate and we need very specific features

### Use Case: Lightweight Development Environment
**Recommended:** LinApple  
**Why:** Small footprint, quick startup, good for iterative development

**Alternative:** GSPlus  
**Why:** Still lightweight, better debugging when needed

## Effort Estimates

### MAME Path
```
Phase 1: Proof of concept
  - Install MAME: 1 hour
  - Boot EDASM: 2 hours
  - First Lua script: 4 hours
  Total: 1 day

Phase 2: Basic automation
  - Keyboard injection: 1 day
  - Screen monitoring: 1 day
  - File I/O workflow: 1 day
  Total: 3 days

Phase 3: Production ready
  - Error handling: 1 day
  - CI integration: 1 day
  - Documentation: 1 day
  Total: 3 days

Overall: 1 week (5-7 days)
```

### GSPlus Path
```
Phase 1: Setup & patching
  - Install GSPlus: 1 hour
  - Understand codebase: 4 hours
  - Design patches: 3 hours
  Total: 1 day

Phase 2: Implement automation
  - stdin/socket input: 2 days
  - Headless mode: 1 day
  - Output capture: 1 day
  Total: 4 days

Phase 3: Integration
  - Testing & debugging: 1 day
  - CI integration: 1 day
  - Documentation: 1 day
  Total: 3 days

Overall: 1.5 weeks (7-10 days)
```

### LinApple Path
```
Phase 1: Setup
  - Install LinApple: 30 mins
  - Install Xvfb: 30 mins
  - Test basic workflow: 2 hours
  Total: 4 hours

Phase 2: Automation
  - Input injection: 1.5 days
  - Output capture: 1 day
  - Script integration: 1 day
  Total: 3.5 days

Phase 3: Production
  - Error handling: 1 day
  - CI integration: 1 day
  - Documentation: 1 day
  Total: 3 days

Overall: 1 week (6-8 days)
```

### Custom Emulator Path
```
Phase 1: Core emulation
  - 6502 CPU core: 5 days
  - Memory banking: 5 days
  - Apple II glue logic: 3 days
  Total: 2.5 weeks

Phase 2: Peripherals
  - ProDOS support: 10 days
  - Disk II controller: 7 days
  - I/O and soft switches: 3 days
  Total: 4 weeks

Phase 3: Testing & integration
  - Validation: 5 days
  - Debugging: 5 days
  - CI integration: 3 days
  - Documentation: 2 days
  Total: 3 weeks

Overall: 9-10 weeks
```

## Risk Assessment

### MAME Risks
- **Low Risk:** Dependency size in CI (mitigated by Docker layer caching)
- **Low Risk:** Lua learning curve (one-time investment, good documentation)
- **Low Risk:** API stability (rarely breaking changes)

**Overall Risk: LOW** ✓

### GSPlus Risks
- **Medium Risk:** Patching effort may exceed estimate
- **Medium Risk:** Headless mode may not work cleanly
- **Medium Risk:** .2mg support issues

**Overall Risk: MEDIUM** ⚠

### LinApple Risks
- **Medium Risk:** Xvfb in CI may have issues
- **Medium Risk:** Fidelity may cause false test failures
- **Low Risk:** No .2mg support (can convert images)

**Overall Risk: MEDIUM** ⚠

### Custom Emulator Risks
- **High Risk:** Development time significantly longer than estimate
- **High Risk:** Edge cases may be missed, causing false positives/negatives
- **High Risk:** Becomes a project of its own, diverting resources
- **Critical Risk:** May give incorrect results that undermine trust in tests

**Overall Risk: HIGH** ⛔

## Final Recommendation

```
Primary:    MAME + Lua automation
Fallback 1: LinApple + Xvfb  
Fallback 2: GSPlus + patches
Last resort: Custom emulator (avoid if possible)
```

### Decision Rationale

MAME wins on nearly all important criteria:
1. **Best automation capabilities** (Lua API)
2. **Highest emulation fidelity** (fewer false failures)
3. **Native headless mode** (no display hacks)
4. **Active maintenance** (upstream support)
5. **Excellent documentation** (faster development)
6. **Proven in production** (used by many projects)

The main downside (dependency size) is easily mitigated with Docker and layer caching. The Lua learning curve is a one-time investment that pays off in maintainability.

### When to Reconsider

Reconsider MAME only if:
- CI runners have severe space constraints (< 1 GB available)
- MAME proves incompatible with CI environment (unlikely)
- Lua automation proves more difficult than expected (unlikely)
- Project timeline cannot accommodate 1 week for implementation (rare)

In any of these cases, fall back to LinApple + Xvfb as the next best option.

## Useful Commands

### MAME
```bash
# Install
sudo apt-get install mame  # or build from source

# Basic run
mame apple2e -flop1 disk.2mg

# Headless automation
mame apple2e -flop1 disk.2mg -video none -sound none -nothrottle \
  -autoboot_script script.lua

# List available systems
mame -listfull | grep -i apple
```

### GSPlus
```bash
# Build
git clone https://github.com/digarok/gsplus.git
cd gsplus
cmake -DWITH_DEBUGGER=ON .
make

# Run with config
./gsplus -config myconfig.gsp
```

### LinApple
```bash
# Install
sudo apt-get install linapple  # if packaged
# or build from source

# Run with disk
linapple --d1 disk.po --autoboot

# With Xvfb (headless)
xvfb-run -a linapple --d1 disk.po --autoboot
```

### DiskM8 (disk manipulation)
```bash
# Install
wget https://github.com/paleotronic/diskm8/releases/download/vX.Y.Z/diskm8-linux-amd64
chmod +x diskm8-linux-amd64

# List contents
./diskm8-linux-amd64 list disk.2mg

# Extract files
./diskm8-linux-amd64 extract disk.2mg ./output/

# Inject file
./diskm8-linux-amd64 inject disk.2mg myfile.src
```

---

*Decision matrix compiled: January 2026*  
*Based on: EMULATOR_INVESTIGATION_REPORT.md*  
*Use this for quick reference when making emulator choices*
