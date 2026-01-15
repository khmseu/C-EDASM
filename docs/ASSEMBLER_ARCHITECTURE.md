# Assembler Architecture

This document describes the EDASM assembler architecture based on analysis of `EDASM.SRC/ASM/*.S` files.

## Overview

The EDASM assembler is a two-pass (optionally three-pass) 6502 assembler with these features:
- Full 6502 instruction set with all addressing modes
- Symbol table with hash-based lookup
- Forward reference resolution
- Expression evaluation with operators
- Multiple output formats (BIN, REL, SYS)
- Listing generation with optional symbol table
- Relocatable code support
- External/entry point declarations

## File Structure

- **ASM1.S** (~4,000 lines): Pass 3 - Symbol table sorting and printing
- **ASM2.S** (~11,000 lines): Pass 1 & 2 - Main assembly logic
- **ASM3.S** (~23,000 lines): Expression evaluation, opcode tables, directives
- **EQUATES.S**: Constants and zero page variable definitions
- **EXTERNALS.S**: External routine references

## Assembly Passes

### Pass 1: Symbol Table Building

**Goal**: Build symbol table, track program counter

**Process**:
1. Initialize PC to ORG value (default $0800)
2. For each source line:
   - Parse label (if present)
   - Add to symbol table with undefined value
   - Parse mnemonic/directive
   - Update PC based on instruction size
   - Mark forward references
3. Record final PC as code length

**Data Structures**:
- Symbol table: Hash table with 256 buckets
- Symbol nodes: Variable length records
- PC tracking: 16-bit program counter

**Symbol Definition**:
```
LABEL   LDA #$00    ; LABEL defined with current PC value
```

**Forward Reference**:
```
        JMP AHEAD   ; AHEAD marked as forward ref
        ...
AHEAD   RTS        ; AHEAD resolved in pass 2
```

### Pass 2: Code Generation

**Goal**: Generate machine code with all symbols resolved

**Process**:
1. Reset PC to ORG value
2. For each source line:
   - Parse label (verify matches pass 1)
   - Parse mnemonic
   - Determine addressing mode
   - Evaluate operand expression
   - Look up symbol values
   - Emit opcode and operand bytes
   - Write to output buffer/file
   - Generate listing line (if enabled)
3. Write output file(s)

**Addressing Mode Detection**:
```
LDA #$12      ; Immediate     ($A9 $12)
LDA $12       ; Zero Page     ($A5 $12)
LDA $1234     ; Absolute      ($AD $34 $12)
LDA $12,X     ; Zero Page,X   ($B5 $12)
LDA $1234,X   ; Absolute,X    ($BD $34 $12)
LDA $1234,Y   ; Absolute,Y    ($B9 $34 $12)
LDA ($12,X)   ; Indirect,X    ($A1 $12)
LDA ($12),Y   ; Indirect,Y    ($B1 $12)
LDA ($1234)   ; Indirect      ($6C $34 $12) - JMP only
```

**Error Detection**:
- Undefined symbols
- Invalid addressing mode
- Out of range values
- Duplicate labels
- Syntax errors

### Pass 3: Symbol Table Listing (Optional)

**Goal**: Sort and print symbol table

**Process**:
1. Compact symbol table (remove link pointers)
2. Sort by name or value (user option)
3. Format in 2, 4, or 6 columns
4. Print with flag indicators
5. Output to screen, printer, or file

**Symbol Display Format**:
```
LABEL    = $1234  R    (Relative)
EXTERN   = $0000  X    (External)
ENTRY    = $2000  E    (Entry point)
MACRO    = $0000  M    (Macro)
```

## Symbol Table Implementation

### Hash Table Structure

256-entry array where each entry points to a linked list of symbols:

```
HeaderT[0..255]  → Symbol node → Symbol node → NULL
```

Hash function: First character of symbol name (0-255)

### Symbol Node Layout

```
+0: Next pointer (2 bytes) - Link to next node in chain
+2: Symbol name (1-16 chars) - Last char has bit 7 set
+n: Flag byte (1 byte)
    bit 7: Undefined
    bit 6: Unreferenced
    bit 5: Relative
    bit 4: External
    bit 3: Entry
    bit 2: Macro
    bit 1: No such label
    bit 0: Forward referenced
+n+1: Value (2 bytes, little-endian)
```

### Symbol Operations

**Insert**:
1. Hash symbol name to bucket index
2. Check if already exists (error if duplicate)
3. Allocate node at end of symbol table
4. Link to bucket chain
5. Store name, flags, value

**Lookup**:
1. Hash symbol name to bucket index
2. Walk linked list comparing names
3. Return pointer to node or NULL

**Update**:
1. Lookup symbol
2. Modify value and/or flags in place

### C++ Port Mapping

Use `std::unordered_map<std::string, Symbol>`:

```cpp
struct Symbol {
    std::string name;
    uint16_t value;
    uint8_t flags;
    int line_defined;  // For error reporting
};

class SymbolTable {
    std::unordered_map<std::string, Symbol> symbols_;
public:
    void define(const std::string& name, uint16_t value, uint8_t flags);
    Symbol* lookup(const std::string& name);
    bool is_defined(const std::string& name) const;
    std::vector<Symbol> sorted_by_name() const;
    std::vector<Symbol> sorted_by_value() const;
};
```

## Expression Evaluation

### Operators Supported

**Arithmetic**:
- `+` Addition
- `-` Subtraction
- `*` Multiplication
- `/` Division
- `MOD` Modulo

**Bitwise**:
- `&` AND
- `|` OR
- `^` XOR
- `.NOT.` Complement

**Shift**:
- `<<` Shift left
- `>>` Shift right

**Relational** (for conditionals):
- `=` Equal
- `<` Less than
- `>` Greater than
- `<=` Less or equal
- `>=` Greater or equal
- `<>` Not equal

### Expression Precedence

1. Parentheses `( )`
2. Unary: `-` `.NOT.`
3. Multiplicative: `*` `/` `MOD`
4. Additive: `+` `-`
5. Shift: `<<` `>>`
6. Relational: `=` `<` `>` `<=` `>=` `<>`
7. Bitwise: `&` `^` `|`

### Expression Syntax

```
        LDA #SYMBOL+10      ; Symbol plus offset
        LDA #(END-START)/2  ; Computed value
        LDA #>TABLE         ; High byte
        LDA #<TABLE         ; Low byte
```

### Evaluation Algorithm

Recursive descent parser with operator precedence:

1. Parse primary (number, symbol, unary)
2. Parse binary operators left-to-right
3. Apply precedence rules
4. Resolve symbols from table
5. Compute result
6. Track if result is absolute or relative

## Directives

### ORG - Set Origin
```
        ORG $8000
```
Sets program counter to specified address.

### EQU - Equate
```
CONST   EQU $1234
```
Defines symbol with constant value (not PC-relative).

### Data Definition

**DA** - Define Address (16-bit):
```
        DA LABEL1,LABEL2,$1234
```

**DW** - Define Word (16-bit):
```
        DW $1234,$5678
```

**DB/DFB** - Define Byte:
```
        DB $01,$02,$03
        DFB $FF
```

**ASC** - ASCII String:
```
        ASC "Hello, World!"
        ASC 'Text'
```
High bit cleared on all characters.

**DCI** - DCI String:
```
        DCI "FILENAME"
```
High bit set on last character (Apple II convention).

**DS** - Define Storage:
```
        DS 256      ; Reserve 256 bytes
```

### Relocatable Code Directives

**REL** - Start relocatable section:
```
        REL
```

**ENT** - Entry point:
```
        ENT START   ; Declare START as entry
```

**EXT** - External reference:
```
        EXT ROUTINE ; Declare ROUTINE as external
```

**END** - End of source:
```
        END
```

### Listing Control

**LST** - Listing on/off:
```
        LST ON      ; Enable listing
        LST OFF     ; Disable listing
```

**SBTL** - Subtitle:
```
        SBTL "Module Name"
```

### Conditional Assembly

**MSB** - High bit control:
```
        MSB ON      ; Set bit 7 on strings
        MSB OFF     ; Clear bit 7 on strings
```

## Output Formats

### BIN Format

Simple binary with 4-byte header:
```
+0: Load address low byte
+1: Load address high byte
+2: Length low byte
+3: Length high byte
+4: Code bytes...
```

### REL Format (Relocatable)

Complex format with:
- Header with code length
- Code image
- Relocation bitmap (which addresses need adjusting)
- Symbol table with entry/external declarations
- End marker

### SYS Format

System file with:
- Load address: $2000
- Entry point at $2000
- Code bytes

### LST Format (Listing)

Text file with:
```
Line# Addr  Bytes        Source
----- ----  ----------   ---------------------------
0010  8000  A9 00        START   LDA #$00
0020  8002  8D 00 04             STA $0400
0030  8005  60                   RTS
```

Symbol table appended at end.

## Error Messages

- Syntax error
- Undefined symbol
- Duplicate label
- Invalid addressing mode
- Out of range
- Disk full (I/O error)
- Bad expression
- Missing operand
- Phase error (PC mismatch between passes)

## Implementation Notes for C++ Port

### Tokenization

Split source line into components:
```cpp
struct SourceLine {
    std::string label;      // Optional
    std::string mnemonic;   // Required
    std::string operand;    // Optional
    std::string comment;    // Optional
};
```

### Opcode Table

Use map or array for opcode lookup:
```cpp
struct OpcodeEntry {
    std::string mnemonic;
    AddressingMode mode;
    uint8_t opcode;
    int cycles;
};

std::unordered_map<std::string, std::vector<OpcodeEntry>> opcodes_;
```

### Code Emission

Accumulate bytes in buffer:
```cpp
std::vector<uint8_t> code_buffer_;
uint16_t program_counter_ = 0x0800;

void emit_byte(uint8_t byte) {
    code_buffer_.push_back(byte);
    program_counter_++;
}

void emit_word(uint16_t word) {
    emit_byte(word & 0xFF);
    emit_byte(word >> 8);
}
```

### Listing Generation

Format each line:
```cpp
std::string format_listing_line(
    int line_number,
    uint16_t address,
    const std::vector<uint8_t>& bytes,
    const std::string& source) {
    // Format: "0010  8000  A9 00        LDA #$00"
}
```
