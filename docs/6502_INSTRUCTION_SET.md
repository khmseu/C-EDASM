# 6502 Instruction Set Reference

This document provides a complete reference for the 6502 instruction set that the EDASM assembler must support.

## Addressing Modes

| Mode | Syntax | Example | Bytes | Description |
|------|--------|---------|-------|-------------|
| Implied | - | `RTS` | 1 | No operand |
| Accumulator | A | `ASL A` | 1 | Operates on accumulator |
| Immediate | #$nn | `LDA #$42` | 2 | Literal value |
| Zero Page | $nn | `LDA $42` | 2 | Address $00-$FF |
| Zero Page,X | $nn,X | `LDA $42,X` | 2 | ZP address + X |
| Zero Page,Y | $nn,Y | `LDX $42,Y` | 2 | ZP address + Y |
| Absolute | $nnnn | `LDA $1234` | 3 | 16-bit address |
| Absolute,X | $nnnn,X | `LDA $1234,X` | 3 | Address + X |
| Absolute,Y | $nnnn,Y | `LDA $1234,Y` | 3 | Address + Y |
| Indirect | ($nnnn) | `JMP ($1234)` | 3 | Jump indirect |
| Indexed Indirect | ($nn,X) | `LDA ($42,X)` | 2 | (ZP+X) pointer |
| Indirect Indexed | ($nn),Y | `LDA ($42),Y` | 2 | (ZP)+Y pointer |
| Relative | $nn | `BEQ $42` | 2 | PC-relative branch |

## Complete Opcode Table

### Load/Store Operations

| Mnemonic | Description | Implied | Accum | Imm | ZP | ZP,X | ZP,Y | Abs | Abs,X | Abs,Y | (ZP,X) | (ZP),Y |
|----------|-------------|---------|-------|-----|-------|-------|-------|-------|-------|-------|--------|--------|
| LDA | Load A | | | A9/2 | A5/3 | B5/4 | | AD/4 | BD/4+ | B9/4+ | A1/6 | B1/5+ |
| LDX | Load X | | | A2/2 | A6/3 | | B6/4 | AE/4 | | BE/4+ | | |
| LDY | Load Y | | | A0/2 | A4/3 | B4/4 | | AC/4 | BC/4+ | | | |
| STA | Store A | | | | 85/3 | 95/4 | | 8D/4 | 9D/5 | 99/5 | 81/6 | 91/6 |
| STX | Store X | | | | 86/3 | | 96/4 | 8E/4 | | | | |
| STY | Store Y | | | | 84/3 | 94/4 | | 8C/4 | | | | |

Format: Opcode/Cycles (+ means extra cycle if page boundary crossed)

### Arithmetic Operations

| Mnemonic | Description | Imm | ZP | ZP,X | Abs | Abs,X | Abs,Y | (ZP,X) | (ZP),Y |
|----------|-------------|-----|-----|------|-----|-------|-------|--------|--------|
| ADC | Add with Carry | 69/2 | 65/3 | 75/4 | 6D/4 | 7D/4+ | 79/4+ | 61/6 | 71/5+ |
| SBC | Subtract with Carry | E9/2 | E5/3 | F5/4 | ED/4 | FD/4+ | F9/4+ | E1/6 | F1/5+ |

### Increment/Decrement

| Mnemonic | Description | Implied | ZP | ZP,X | Abs | Abs,X |
|----------|-------------|---------|-----|------|-----|-------|
| INC | Increment Memory | | E6/5 | F6/6 | EE/6 | FE/7 |
| DEC | Decrement Memory | | C6/5 | D6/6 | CE/6 | DE/7 |
| INX | Increment X | E8/2 | | | | |
| DEX | Decrement X | CA/2 | | | | |
| INY | Increment Y | C8/2 | | | | |
| DEY | Decrement Y | 88/2 | | | | |

### Logical Operations

| Mnemonic | Description | Imm | ZP | ZP,X | Abs | Abs,X | Abs,Y | (ZP,X) | (ZP),Y |
|----------|-------------|-----|-----|------|-----|-------|-------|--------|--------|
| AND | Logical AND | 29/2 | 25/3 | 35/4 | 2D/4 | 3D/4+ | 39/4+ | 21/6 | 31/5+ |
| ORA | Logical OR | 09/2 | 05/3 | 15/4 | 0D/4 | 1D/4+ | 19/4+ | 01/6 | 11/5+ |
| EOR | Exclusive OR | 49/2 | 45/3 | 55/4 | 4D/4 | 5D/4+ | 59/4+ | 41/6 | 51/5+ |

### Shift/Rotate Operations

| Mnemonic | Description | Accum | ZP | ZP,X | Abs | Abs,X |
|----------|-------------|-------|-----|------|-----|-------|
| ASL | Arithmetic Shift Left | 0A/2 | 06/5 | 16/6 | 0E/6 | 1E/7 |
| LSR | Logical Shift Right | 4A/2 | 46/5 | 56/6 | 4E/6 | 5E/7 |
| ROL | Rotate Left | 2A/2 | 26/5 | 36/6 | 2E/6 | 3E/7 |
| ROR | Rotate Right | 6A/2 | 66/5 | 76/6 | 6E/6 | 7E/7 |

### Compare Operations

| Mnemonic | Description | Imm | ZP | ZP,X | Abs | Abs,X | Abs,Y | (ZP,X) | (ZP),Y |
|----------|-------------|-----|-----|------|-----|-------|-------|--------|--------|
| CMP | Compare A | C9/2 | C5/3 | D5/4 | CD/4 | DD/4+ | D9/4+ | C1/6 | D1/5+ |
| CPX | Compare X | E0/2 | E4/3 | | EC/4 | | | | |
| CPY | Compare Y | C0/2 | C4/3 | | CC/4 | | | | |

### Branch Operations (All Relative)

| Mnemonic | Description | Opcode | Cycles |
|----------|-------------|--------|--------|
| BCC | Branch if Carry Clear | 90 | 2+ |
| BCS | Branch if Carry Set | B0 | 2+ |
| BEQ | Branch if Equal | F0 | 2+ |
| BNE | Branch if Not Equal | D0 | 2+ |
| BMI | Branch if Minus | 30 | 2+ |
| BPL | Branch if Plus | 10 | 2+ |
| BVC | Branch if Overflow Clear | 50 | 2+ |
| BVS | Branch if Overflow Set | 70 | 2+ |

Note: +1 cycle if branch taken, +2 if page boundary crossed

### Jump/Subroutine Operations

| Mnemonic | Description | Abs | Indirect |
|----------|-------------|-----|----------|
| JMP | Jump | 4C/3 | 6C/5 |
| JSR | Jump to Subroutine | 20/6 | |
| RTS | Return from Subroutine | 60/6 | |

### Test Operations

| Mnemonic | Description | ZP | Abs |
|----------|-------------|-----|-----|
| BIT | Bit Test | 24/3 | 2C/4 |

### Transfer Operations (All Implied)

| Mnemonic | Description | Opcode | Cycles |
|----------|-------------|--------|--------|
| TAX | Transfer A to X | AA | 2 |
| TAY | Transfer A to Y | A8 | 2 |
| TXA | Transfer X to A | 8A | 2 |
| TYA | Transfer Y to A | 98 | 2 |
| TSX | Transfer SP to X | BA | 2 |
| TXS | Transfer X to SP | 9A | 2 |

### Stack Operations (All Implied)

| Mnemonic | Description | Opcode | Cycles |
|----------|-------------|--------|--------|
| PHA | Push A | 48 | 3 |
| PHP | Push Status | 08 | 3 |
| PLA | Pull A | 68 | 4 |
| PLP | Pull Status | 28 | 4 |

### Status Flag Operations (All Implied)

| Mnemonic | Description | Opcode | Cycles |
|----------|-------------|--------|--------|
| CLC | Clear Carry | 18 | 2 |
| CLD | Clear Decimal | D8 | 2 |
| CLI | Clear Interrupt | 58 | 2 |
| CLV | Clear Overflow | B8 | 2 |
| SEC | Set Carry | 38 | 2 |
| SED | Set Decimal | F8 | 2 |
| SEI | Set Interrupt | 78 | 2 |

### System Operations (All Implied)

| Mnemonic | Description | Opcode | Cycles |
|----------|-------------|--------|--------|
| BRK | Break | 00 | 7 |
| NOP | No Operation | EA | 2 |
| RTI | Return from Interrupt | 40 | 6 |

## Addressing Mode Detection Algorithm

For the assembler, use this logic to determine addressing mode:

1. **No operand** → Implied or Accumulator
   - Check if mnemonic allows these modes
   - If operand is "A", use Accumulator mode

2. **#** prefix → Immediate
   - Example: `#$42`, `#<LABEL`, `#>LABEL`

3. **( )** parentheses → Indirect addressing
   - `(nnnn)` → Indirect (JMP only)
   - `(nn,X)` → Indexed Indirect
   - `(nn),Y` → Indirect Indexed

4. **No prefix, no parens**:
   - If value < $100 → Zero Page
   - If value >= $100 → Absolute
   - If operand has `,X` or `,Y`:
     - Value < $100 → Zero Page indexed
     - Value >= $100 → Absolute indexed

5. **Branch instructions** → Always Relative
   - Convert label to PC-relative offset
   - Range: -128 to +127 bytes

## C++ Implementation Structure

### Opcode Table Entry

```cpp
enum class AddressingMode {
    Implied,
    Accumulator,
    Immediate,
    ZeroPage,
    ZeroPageX,
    ZeroPageY,
    Absolute,
    AbsoluteX,
    AbsoluteY,
    Indirect,
    IndexedIndirect,  // (ZP,X)
    IndirectIndexed,  // (ZP),Y
    Relative
};

struct Opcode {
    std::string mnemonic;
    AddressingMode mode;
    uint8_t code;
    int bytes;
    int cycles;
    bool extra_cycle_on_page_cross;
};
```

### Opcode Lookup

```cpp
class OpcodeTable {
public:
    OpcodeTable();
    
    const Opcode* lookup(
        const std::string& mnemonic,
        AddressingMode mode) const;
    
    std::vector<AddressingMode> valid_modes(
        const std::string& mnemonic) const;
        
private:
    std::unordered_map<
        std::string,
        std::unordered_map<AddressingMode, Opcode>
    > table_;
};
```

### Initialization Example

```cpp
OpcodeTable::OpcodeTable() {
    // Load/Store A
    add("LDA", AddressingMode::Immediate,    0xA9, 2, 2, false);
    add("LDA", AddressingMode::ZeroPage,     0xA5, 2, 3, false);
    add("LDA", AddressingMode::ZeroPageX,    0xB5, 2, 4, false);
    add("LDA", AddressingMode::Absolute,     0xAD, 3, 4, false);
    add("LDA", AddressingMode::AbsoluteX,    0xBD, 3, 4, true);
    add("LDA", AddressingMode::AbsoluteY,    0xB9, 3, 4, true);
    add("LDA", AddressingMode::IndexedIndirect, 0xA1, 2, 6, false);
    add("LDA", AddressingMode::IndirectIndexed, 0xB1, 2, 5, true);
    
    // ... repeat for all instructions
}
```

## Operand Parsing Examples

```
LDA #$42       → Immediate, operand = $42
LDA $42        → ZeroPage (if $42 < $100), operand = $42
LDA $1234      → Absolute (if $1234 >= $100), operand = $1234
LDA $42,X      → ZeroPageX, operand = $42
LDA $1234,X    → AbsoluteX, operand = $1234
LDA ($42,X)    → IndexedIndirect, operand = $42
LDA ($42),Y    → IndirectIndexed, operand = $42
JMP ($1234)    → Indirect, operand = $1234
BEQ LABEL      → Relative, compute offset from PC
```

## Special Cases

### Branch Range Check

Branches use signed 8-bit offset (-128 to +127):
```cpp
int16_t offset = target_address - (current_pc + 2);
if (offset < -128 || offset > 127) {
    error("Branch out of range");
}
emit_byte(static_cast<uint8_t>(offset & 0xFF));
```

### Zero Page Optimization

If value fits in 8 bits and instruction supports ZP mode:
```cpp
if (value <= 0xFF && supports_zero_page(mnemonic)) {
    use_zero_page_mode();
} else {
    use_absolute_mode();
}
```

### High/Low Byte Operators

```
LDA #<LABEL    ; Low byte of LABEL
LDA #>LABEL    ; High byte of LABEL
```

Implementation:
```cpp
if (operand.starts_with('#')) {
    operand = operand.substr(1);  // Remove #
    if (operand.starts_with('<')) {
        value = evaluate_expression(operand.substr(1)) & 0xFF;
    } else if (operand.starts_with('>')) {
        value = (evaluate_expression(operand.substr(1)) >> 8) & 0xFF;
    } else {
        value = evaluate_expression(operand) & 0xFF;
    }
}
```

## Pseudo-Instructions (Not Real 6502 Opcodes)

Some assemblers provide these convenience mnemonics:

| Pseudo | Expands To | Description |
|--------|-----------|-------------|
| BLT | BMI | Branch if less than (signed) |
| BGE | BPL | Branch if greater or equal (signed) |
| BLO | BCC | Branch if lower (unsigned) |
| BHS | BCS | Branch if higher or same (unsigned) |

EDASM does not appear to support these in the original source, so omit from initial implementation.
