#pragma once

#include <string>
#include <vector>
#include <cstdint>

#include "edasm/assembler/symbol_table.hpp"
#include "edasm/assembler/tokenizer.hpp"
#include "edasm/assembler/opcode_table.hpp"
#include "edasm/assembler/expression.hpp"
#include "edasm/assembler/listing.hpp"
#include "edasm/assembler/rel_file.hpp"

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
        
        // REL file format data (only populated if rel_mode is true)
        bool is_rel_file{false};
        std::vector<uint8_t> rel_file_data;  // Complete REL format with RLD/ESD
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
    RELFileBuilder rel_builder_;  // RLD/ESD builder for REL files
    uint8_t next_extern_symbol_num_{0};  // Counter for external symbol numbers
    
    // Listing control (from ASM3.S ListingF, msbF)
    bool listing_enabled_{true};  // LST ON/OFF
    bool msb_on_{false};  // MSB ON/OFF - sets high bit on chars
    
    // Include file tracking (from ASM3.S)
    bool in_include_file_{false};  // IDskSrcF - true when reading from INCLUDE file
    std::string base_path_;  // Base path for resolving relative include paths
    
    // Conditional assembly state (from ASM3.S CondAsmF at $BA)
    // Values: 0x00=assemble, 0x40=skip(false), 0x80=in ELSE, 0xC0=skip in ELSE
    uint8_t cond_asm_flag_{0x00};  // CondAsmF
    
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
    void emit_word_with_relocation(uint16_t word, const std::string& operand, Result& result);
    
    // Helpers
    void add_error(Result& result, const std::string& msg, int line_num = -1);
    void add_warning(Result& result, const std::string& msg, int line_num = -1);
    bool is_directive(const std::string& mnemonic) const;
    uint16_t evaluate_operand(const std::string& operand);
    
    // Include file preprocessing (from ASM3.S L9348)
    std::vector<SourceLine> preprocess_includes(const std::vector<SourceLine>& lines, 
                                                  Result& result, 
                                                  int nesting_level = 0);
    std::string resolve_include_path(const std::string& include_path) const;
    
    // Conditional assembly (from ASM3.S L90B7-L9122)
    bool should_assemble_line() const;  // Check if current line should be assembled
    bool process_conditional_directive_pass1(const SourceLine& line, Result& result);
    bool process_conditional_directive_pass2(const SourceLine& line, Result& result);
};

} // namespace edasm
