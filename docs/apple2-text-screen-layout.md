# Apple II 40×24 Text Screen — Memory Layout ✅

**Summary**

- The 40×24 text page uses 960 bytes for characters but occupies a **1 KB block** in memory (Page 1 = `$0400..$07FF`, Page 2 = `$0800..$0BFF`).
- The page is arranged as **8 blocks of 128 bytes**. Each 128‑byte block stores **three 40‑byte rows (120 bytes)** followed by **8 unused bytes** (the “screen holes”).

---

## Layout & Addressing 🔧

- Screen: **40 columns × 24 rows = 960 characters**.
- Physical arrangement: 8 × 128‑byte segments, each segment -> 3 rows (40 bytes each) + 8 bytes unused.

**Address formula** (row r = 0..23, column c = 0..39):

```
addr = base + (r % 8) * 128 + (r // 8) * 40 + c
```

- `base` = `$0400` for TEXT/LORES page 1, `$0800` for page 2.

**Examples (base = `$0400`):**

- Row 0: `$0400..$0427`
- Row 8: `$0428..$044F`
- Row 16: `$0450..$0477`
- The unused bytes for that 128‑byte block are `$0478..$047F` (a screen hole).
- Next block (rows 1,9,17): starts at `$0480`, with `$04F8..$04FF` as that block's hole.

## Screen Holes & Use ⚙️

- The eight 8‑byte holes (e.g. `$0478..$047F`, `$04F8..$04FF`, etc.) were intentionally left out of display memory.
- By convention they served as **scratchpad RAM** for peripheral cards and firmware (documented in the Apple II Reference Manual).

---

## Why this exists

- Wozniak’s design traded linear addressing for minimal hardware (fewer chips) and integrated DRAM refresh into video memory reads; the interleaved layout saved parts but made address math non-linear.

## Sources

- Retrocomputing StackExchange: “What are the ‘Screen Holes’ in Apple II graphics?” (detailed address table)
- Apple II Reference Manual (1979) — slot scratchpad RAM spec
- CiderPress / Hi‑Res Notes — explanation of 128‑byte grouping and holes
- Apple II history notes (overview of the non‑linear mapping)

---

If you want, I can add a tiny 6502 snippet or a short script to compute addresses for any (row,col) and print the resulting addresses. 🔧
