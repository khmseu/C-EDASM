#pragma once

#include <string>
#include <vector>
#include <cstdint>

#include "edasm/assembler/symbol_table.hpp"
#include "edasm/assembler/tokenizer.hpp"
#include "edasm/assembler/opcode_table.hpp"
#include "edasm/assembler/expression.hpp"
#include "edasm/assembler/listing.hpp"

namespace edasm {

class Assembler {
  public:
    struct Result {
        bool success{false};
        std::vector<std::string> errors;
        std::vector<std::string> warnings;
        std::vector<uint8_t> code;  // Generated machine code
        uint16_t org_address{0x0800};  // ORG address (default $0800)
        uint16_t code_length{0};
        std::string listing;  // Listing output (if enabled)
    };
    
    struct Options {
        bool generate_listing = false;
        bool list_symbols = true;
        bool sort_symbols_by_value = false;
        int symbol_columns = 4;  // 2, 4, or 6
    };

    Assembler();
    Result assemble(const std::string &source) {
        return assemble(source, Options{});
    }
    Result assemble(const std::string &source, const Options& opts);
    void reset();
    
    // Access to symbol table for debugging/listing
    const SymbolTable& symbols() const { return symbols_; }

  private:
    SymbolTable symbols_;
    OpcodeTable opcodes_;
    uint16_t program_counter_{0x0800};  // PC tracking
    uint16_t org_address_{0x0800};      // ORG directive value
    int current_line_{0};
    Options options_;
    
    // REL file state (from ASM3.S RelCodeF)
    bool rel_mode_{false};  // True when REL directive is used
    uint8_t file_type_{0x06};  // Default BIN ($06), REL is $FE
    
    // Assembly passes (from ASM2.S and ASM3.S)
    bool pass1(const std::vector<SourceLine>& lines, Result& result);
    bool pass2(const std::vector<SourceLine>& lines, Result& result, ListingGenerator* listing);
    
    // Pass 1: Build symbol table
    void process_label_pass1(const SourceLine& line);
    void process_directive_pass1(const SourceLine& line, Result& result);
    void update_pc_pass1(const SourceLine& line);
    
    // Pass 2: Generate code
    bool process_line_pass2(const SourceLine& line, Result& result, ListingGenerator* listing);
    bool process_directive_pass2(const SourceLine& line, Result& result, ListingGenerator* listing);
    bool encode_instruction(const SourceLine& line, Result& result, ListingGenerator* listing);
    
    // Code emission
    void emit_byte(uint8_t byte, Result& result);
    void emit_word(uint16_t word, Result& result);
    
    // Helpers
    void add_error(Result& result, const std::string& msg, int line_num = -1);
    void add_warning(Result& result, const std::string& msg, int line_num = -1);
    bool is_directive(const std::string& mnemonic) const;
    uint16_t evaluate_operand(const std::string& operand);
};

} // namespace edasm
