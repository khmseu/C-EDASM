#include <cassert>
#include <iostream>
#include <string>
#include <fstream>
#include <vector>

#include "edasm/assembler/assembler.hpp"

using namespace edasm;

// Helper to assemble source and return code bytes
Assembler::Result assemble_source(const std::string& source) {
    Assembler assembler;
    Assembler::Options opts;
    return assembler.assemble(source, opts);
}

void test_basic_instructions() {
    std::cout << "Testing basic 6502 instructions..." << std::endl;
    
    std::string source = R"(
        ORG $1000
START   LDA #$42
        STA $20
        RTS
        END
)";
    
    auto result = assemble_source(source);
    
    assert(result.success);
    assert(result.errors.empty());
    
    // Check generated code
    const auto& data = result.code;
    assert(data.size() == 5); // 5 bytes of code
    
    assert(data[0] == 0xA9); // LDA #
    assert(data[1] == 0x42); // $42
    assert(data[2] == 0x85); // STA ZP
    assert(data[3] == 0x20); // $20
    assert(data[4] == 0x60); // RTS
    
    std::cout << "  ✓ Basic instructions test passed" << std::endl;
}

void test_all_addressing_modes() {
    std::cout << "Testing all addressing modes..." << std::endl;
    
    std::string source = R"(
        ORG $1000
BASE    EQU $1000

        ; Implied
        RTS
        
        ; Immediate
        LDA #$00
        
        ; Zero Page
        LDA $10
        
        ; Zero Page,X
        LDA $10,X
        
        ; Absolute
        LDA BASE
        
        ; Absolute,X
        LDA BASE,X
        
        ; Absolute,Y
        LDA BASE,Y
        
        ; Indirect,X
        LDA ($10,X)
        
        ; Indirect,Y
        LDA ($10),Y
        
        END
)";
    
    auto result = assemble_source(source);
    
    assert(result.success);
    if (!result.errors.empty()) {
        for (const auto& err : result.errors) {
            std::cerr << "Error: " << err << std::endl;
        }
    }
    
    const auto& data = result.code;
    assert(data.size() >= 20); // Should have all instruction bytes
    
    std::cout << "  ✓ All addressing modes test passed" << std::endl;
}

void test_forward_references() {
    std::cout << "Testing forward references..." << std::endl;
    
    std::string source = R"(
        ORG $1000
START   JMP LATER
        NOP
LATER   RTS
        END
)";
    
    auto result = assemble_source(source);
    
    assert(result.success);
    
    const auto& data = result.code;
    // JMP absolute is 3 bytes: 4C lo hi
    assert(data[0] == 0x4C); // JMP
    // Should jump to address $1004 (after NOP at $1003)
    assert(data[1] == 0x04); // Low byte
    assert(data[2] == 0x10); // High byte
    
    std::cout << "  ✓ Forward references test passed" << std::endl;
}

void test_expressions() {
    std::cout << "Testing expression evaluation..." << std::endl;
    
    std::string source = R"(
        ORG $1000
BASE    EQU $1000
OFFSET  EQU $10

        ; Addition
        LDA #BASE+OFFSET
        
        ; Subtraction
        LDA #OFFSET-5
        
        ; Multiplication
        LDA #2*8
        
        ; Division
        LDA #16/2
        
        ; Low byte
        LDA #<BASE
        
        ; High byte
        LDA #>BASE
        
        END
)";
    
    auto result = assemble_source(source);
    
    assert(result.success);
    
    const auto& data = result.code;
    int idx = 0;
    
    // BASE+OFFSET = $1000+$10 = $1010 (only low byte in immediate)
    assert(data[idx] == 0xA9); idx++;
    assert(data[idx] == 0x10); idx++; // $1010 & $FF = $10
    
    // OFFSET-5 = $10-5 = $0B
    assert(data[idx] == 0xA9); idx++;
    assert(data[idx] == 0x0B); idx++;
    
    // 2*8 = 16 = $10
    assert(data[idx] == 0xA9); idx++;
    assert(data[idx] == 0x10); idx++;
    
    // 16/2 = 8
    assert(data[idx] == 0xA9); idx++;
    assert(data[idx] == 0x08); idx++;
    
    // <BASE = $00 (low byte of $1000)
    assert(data[idx] == 0xA9); idx++;
    assert(data[idx] == 0x00); idx++;
    
    // >BASE = $10 (high byte of $1000)
    assert(data[idx] == 0xA9); idx++;
    assert(data[idx] == 0x10); idx++;
    
    // TODO: Bitwise operations (&, |, ^) need implementation
    // Currently not parsed properly in expression evaluator
    
    std::cout << "  ✓ Expression evaluation test passed" << std::endl;
}

void test_all_directives() {
    std::cout << "Testing all directives..." << std::endl;
    
    std::string source = R"(
        ORG $1000
        
CONST   EQU $42

        ; DB - Define Byte
        DB $01
        
        ; DW/DA - Define Word
        DW $1234
        
        ; ASC - ASCII string
        ASC "HI"
        
        ; DCI - DCI string (last char inverted)
        DCI "OK"
        
        ; DS - Define Storage
        DS 5
        
        ; Actual code
        LDA #CONST
        RTS
        
        END
)";
    
    auto result = assemble_source(source);
    
    assert(result.success);
    
    const auto& data = result.code;
    int idx = 0;
    
    // DB $01
    assert(data[idx] == 0x01); idx++;
    
    // DW $1234 (little-endian)
    assert(data[idx] == 0x34); idx++;
    assert(data[idx] == 0x12); idx++;
    
    // ASC "HI"
    assert(data[idx] == 'H'); idx++;
    assert(data[idx] == 'I'); idx++;
    
    // DCI "OK" - last char has bit 7 set
    assert(data[idx] == 'O'); idx++;
    assert(data[idx] == ('K' | 0x80)); idx++;
    
    // DS 5 - 5 zero bytes
    for (int i = 0; i < 5; i++) {
        assert(data[idx] == 0x00); idx++;
    }
    
    // LDA #$42
    assert(data[idx] == 0xA9); idx++;
    assert(data[idx] == 0x42); idx++;
    
    // RTS
    assert(data[idx] == 0x60); idx++;
    
    std::cout << "  ✓ All directives test passed" << std::endl;
}

void test_conditional_assembly() {
    std::cout << "Testing conditional assembly..." << std::endl;
    
    std::string source = R"(
        ORG $1000

DEBUG   EQU 1
RELEASE EQU 0

        ; This should be assembled
        DO DEBUG
        LDA #$FF
        FIN
        
        ; This should be skipped
        DO RELEASE
        LDA #$00
        FIN
        
        ; ELSE clause
        DO RELEASE
        LDX #$00
        ELSE
        LDX #$FF
        FIN
        
        RTS
        END
)";
    
    auto result = assemble_source(source);
    
    assert(result.success);
    
    const auto& data = result.code;
    int idx = 0;
    
    // LDA #$FF (from DO DEBUG)
    assert(data[idx] == 0xA9); idx++;
    assert(data[idx] == 0xFF); idx++;
    
    // LDA #$00 should NOT be here
    
    // LDX #$FF (from ELSE clause, since RELEASE=0)
    assert(data[idx] == 0xA2); idx++;
    assert(data[idx] == 0xFF); idx++;
    
    // RTS
    assert(data[idx] == 0x60); idx++;
    
    std::cout << "  ✓ Conditional assembly test passed" << std::endl;
}

void test_msb_directive() {
    std::cout << "Testing MSB directive..." << std::endl;
    
    std::string source = R"(
        ORG $1000
        
        ; Normal ASCII
        ASC "AB"
        
        ; High bit set
        MSB ON
        ASC "AB"
        MSB OFF
        
        ; Normal again
        ASC "AB"
        
        END
)";
    
    auto result = assemble_source(source);
    
    assert(result.success);
    
    const auto& data = result.code;
    int idx = 0;
    
    // Normal "AB"
    assert(data[idx] == 'A'); idx++;
    assert(data[idx] == 'B'); idx++;
    
    // MSB ON "AB"
    assert(data[idx] == ('A' | 0x80)); idx++;
    assert(data[idx] == ('B' | 0x80)); idx++;
    
    // Normal "AB" again
    assert(data[idx] == 'A'); idx++;
    assert(data[idx] == 'B'); idx++;
    
    std::cout << "  ✓ MSB directive test passed" << std::endl;
}

int main() {
    std::cout << "Running Assembler Integration Tests\n";
    std::cout << "====================================\n\n";
    
    try {
        test_basic_instructions();
        test_all_addressing_modes();
        test_forward_references();
        test_expressions();
        test_all_directives();
        test_conditional_assembly();
        test_msb_directive();
        
        std::cout << "\n====================================\n";
        std::cout << "All tests PASSED! ✓\n";
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "\n✗ Test FAILED with exception: " << e.what() << std::endl;
        return 1;
    }
}
