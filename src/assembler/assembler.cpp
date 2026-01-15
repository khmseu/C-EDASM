#include "edasm/assembler/assembler.hpp"

#include <sstream>
#include <fstream>
#include <algorithm>
#include <memory>

namespace edasm {

Assembler::Assembler() = default;

Assembler::Result Assembler::assemble(const std::string &source, const Options& opts) {
    Result result;
    result.org_address = org_address_;
    options_ = opts;
    
    // Reset state
    reset();
    
    // Tokenize source into lines
    std::vector<SourceLine> lines;
    std::stringstream ss(source);
    std::string line;
    int line_num = 1;
    
    while (std::getline(ss, line)) {
        auto parsed = Tokenizer::parse_line(line, line_num++);
        lines.push_back(parsed);
    }
    
    // Preprocess INCLUDE directives (from ASM3.S L9348)
    lines = preprocess_includes(lines, result, 0);
    if (!result.errors.empty()) {
        result.success = false;
        return result;
    }
    
    // Pass 1: Build symbol table, track PC
    if (!pass1(lines, result)) {
        return result;
    }
    
    // Pass 2: Generate code
    ListingGenerator* listing = nullptr;
    std::unique_ptr<ListingGenerator> listing_ptr;
    
    if (options_.generate_listing) {
        ListingGenerator::Options list_opts;
        list_opts.include_symbols = options_.list_symbols;
        list_opts.sort_by_value = options_.sort_symbols_by_value;
        list_opts.symbol_columns = options_.symbol_columns;
        listing_ptr = std::make_unique<ListingGenerator>(list_opts);
        listing = listing_ptr.get();
        listing->set_symbol_table(symbols_);
    }
    
    if (!pass2(lines, result, listing)) {
        return result;
    }
    
    // Generate REL file format if in REL mode
    if (rel_mode_) {
        // Build ESD entries from symbol table
        for (const auto& [name, symbol] : symbols_.get_all()) {
            // Add ENTRY symbols (defined in this module)
            if (symbol.flags & SYM_ENTRY) {
                uint8_t esd_flags = ESDEntry::FLAG_ENTRY;
                if (symbol.flags & SYM_RELATIVE) {
                    esd_flags |= ESDEntry::FLAG_RELATIVE;
                }
                rel_builder_.add_esd_entry(name, symbol.value, esd_flags);
            }
            
            // Add EXTERNAL symbols (referenced but not defined)
            if (symbol.flags & SYM_EXTERNAL) {
                uint8_t esd_flags = ESDEntry::FLAG_EXTERNAL;
                if (symbol.flags & SYM_UNDEFINED) {
                    esd_flags |= ESDEntry::FLAG_UNDEFINED;
                }
                if (symbol.flags & SYM_RELATIVE) {
                    esd_flags |= ESDEntry::FLAG_RELATIVE;
                }
                // Use the symbol number assigned during pass 2
                uint8_t sym_num = symbol.symbol_number;
                rel_builder_.add_esd_entry(name, symbol.value, esd_flags, sym_num);
            }
        }
        
        // Build complete REL file format
        result.rel_file_data = rel_builder_.build(result.code);
        result.is_rel_file = true;
    }
    
    // Generate listing if requested
    if (listing) {
        result.listing = listing->to_string();
    }
    
    result.code_length = static_cast<uint16_t>(result.code.size());
    result.success = result.errors.empty();
    return result;
}

void Assembler::reset() {
    symbols_.reset();
    program_counter_ = org_address_;
    current_line_ = 0;
    rel_mode_ = false;
    file_type_ = 0x06;  // Default to BIN type
    listing_enabled_ = true;  // Default LST ON
    msb_on_ = false;  // Default MSB OFF
    in_include_file_ = false;  // Not in include file
    base_path_ = ".";  // Default to current directory
    rel_builder_.reset();
    next_extern_symbol_num_ = 0;
}

// =========================================
// Pass 1: Build Symbol Table
// =========================================

bool Assembler::pass1(const std::vector<SourceLine>& lines, Result& result) {
    program_counter_ = org_address_;
    
    for (const auto& line : lines) {
        current_line_ = line.line_number;
        
        // Skip comment-only lines
        if (line.is_comment_only()) {
            continue;
        }
        
        // Process label definition
        if (line.has_label()) {
            process_label_pass1(line);
        }
        
        // Process directives that affect PC or symbol table
        if (line.has_mnemonic() && is_directive(line.mnemonic)) {
            process_directive_pass1(line, result);
        } else if (line.has_mnemonic()) {
            // Regular instruction - update PC
            update_pc_pass1(line);
        }
    }
    
    return result.errors.empty();
}

void Assembler::process_label_pass1(const SourceLine& line) {
    // Define label with current PC value
    // Mark as relative (code label) by default
    symbols_.define(line.label, program_counter_, SYM_RELATIVE, line.line_number);
}

void Assembler::process_directive_pass1(const SourceLine& line, Result& result) {
    const auto& mnem = line.mnemonic;
    
    // Create expression evaluator for this pass
    ExpressionEvaluator eval(symbols_);
    
    if (mnem == "ORG") {
        // ORG directive - set program counter (from ASM3.S L8A82)
        auto expr_result = eval.evaluate(line.operand, 1);
        if (expr_result.success) {
            org_address_ = expr_result.value;
            program_counter_ = expr_result.value;
        } else {
            add_error(result, "ORG: " + expr_result.error_message, line.line_number);
        }
    } else if (mnem == "EQU") {
        // EQU directive - define symbol with value (from ASM3.S L8A31)
        if (!line.has_label()) {
            add_error(result, "EQU requires a label", line.line_number);
            return;
        }
        auto expr_result = eval.evaluate(line.operand, 1);
        if (expr_result.success) {
            // Define symbol with evaluated value
            uint8_t flags = 0;
            if (expr_result.is_relative) flags |= SYM_RELATIVE;
            if (expr_result.is_external) flags |= SYM_EXTERNAL;
            symbols_.define(line.label, expr_result.value, flags, line.line_number);
        } else {
            add_error(result, "EQU: " + expr_result.error_message, line.line_number);
        }
    } else if (mnem == "REL") {
        // REL directive - enable relocatable mode (from ASM3.S L9126)
        // Sets RelCodeF flag and changes file type to REL ($FE)
        rel_mode_ = true;
        file_type_ = 0xFE;  // REL file type
    } else if (mnem == "ENT" || mnem == "ENTRY") {
        // ENT/ENTRY directive - mark symbol as entry point (from ASM3.S L9144)
        // Entry points are symbols that can be referenced by other modules
        if (line.operand.empty()) {
            add_error(result, "ENT requires a symbol name", line.line_number);
            return;
        }
        
        // Look up or define the symbol
        Symbol* sym = symbols_.lookup(line.operand);
        if (sym) {
            // Symbol exists - add ENTRY flag
            sym->flags |= SYM_ENTRY;
            if (rel_mode_) {
                sym->flags |= SYM_RELATIVE;
            }
        } else {
            // Symbol doesn't exist yet - define it with ENTRY flag
            // It will be resolved when the label is encountered
            uint8_t flags = SYM_ENTRY | SYM_UNDEFINED;
            if (rel_mode_) {
                flags |= SYM_RELATIVE;
            }
            symbols_.define(line.operand, 0, flags, line.line_number);
        }
    } else if (mnem == "EXT" || mnem == "EXTRN") {
        // EXT/EXTRN directive - mark symbol as external (from ASM3.S L91A8)
        // External symbols are defined in other modules
        if (line.operand.empty()) {
            add_error(result, "EXT requires a symbol name", line.line_number);
            return;
        }
        
        // Define symbol as external
        Symbol* sym = symbols_.lookup(line.operand);
        if (sym) {
            // Symbol already exists - add EXTERNAL flag
            sym->flags |= SYM_EXTERNAL;
            // In REL mode, external symbols are also relative
            if (rel_mode_) {
                sym->flags |= SYM_RELATIVE;
            }
            // Assign symbol number if not already assigned
            if (sym->symbol_number == 0) {
                sym->symbol_number = ++next_extern_symbol_num_;
            }
        } else {
            // Define as external with undefined value
            uint8_t flags = SYM_EXTERNAL | SYM_UNDEFINED;
            if (rel_mode_) {
                flags |= SYM_RELATIVE;
            }
            symbols_.define(line.operand, 0, flags, line.line_number);
            // Assign symbol number to newly created external symbol
            Symbol* new_sym = symbols_.lookup(line.operand);
            if (new_sym) {
                new_sym->symbol_number = ++next_extern_symbol_num_;
            }
        }
    } else if (mnem == "LST") {
        // LST directive - control listing output (from ASM3.S L8ECA)
        // LST ON or LST OFF
        std::string operand = line.operand;
        std::transform(operand.begin(), operand.end(), operand.begin(), ::toupper);
        
        if (operand.find("ON") != std::string::npos) {
            listing_enabled_ = true;
        } else if (operand.find("OFF") != std::string::npos) {
            listing_enabled_ = false;
        } else {
            add_error(result, "LST requires ON or OFF", line.line_number);
        }
    } else if (mnem == "MSB") {
        // MSB directive - control high bit on ASCII chars (from ASM3.S L8E66)
        // MSB ON or MSB OFF
        std::string operand = line.operand;
        std::transform(operand.begin(), operand.end(), operand.begin(), ::toupper);
        
        if (operand.find("ON") != std::string::npos) {
            msb_on_ = true;
        } else if (operand.find("OFF") != std::string::npos) {
            msb_on_ = false;
        } else {
            add_error(result, "MSB requires ON or OFF", line.line_number);
        }
    } else if (mnem == "SBTL") {
        // SBTL directive - subtitle for listing (from ASM3.S)
        // Note: Currently not stored; could be used by listing generator for section headers
        // in future enhancement. For now, accepted but ignored in pass 1.
    } else if (mnem == "DS") {
        // Define Storage - advance PC (from ASM3.S L8C0E)
        auto expr_result = eval.evaluate(line.operand, 1);
        if (expr_result.success) {
            program_counter_ += expr_result.value;
        } else {
            add_error(result, "DS: " + expr_result.error_message, line.line_number);
        }
    } else if (mnem == "DB" || mnem == "DFB") {
        // Define Byte - count bytes in operand
        // For now, simple count (TODO: handle expressions in operand list)
        program_counter_ += 1;
    } else if (mnem == "DW" || mnem == "DA") {
        // Define Word - count words in operand
        program_counter_ += 2;
    } else if (mnem == "ASC" || mnem == "DCI") {
        // ASCII string - estimate length
        if (!line.operand.empty()) {
            // Count characters in string (between quotes)
            size_t len = 0;
            bool in_string = false;
            for (char c : line.operand) {
                if (c == '"' || c == '\'') {
                    in_string = !in_string;
                } else if (in_string) {
                    len++;
                }
            }
            program_counter_ += static_cast<uint16_t>(len);
        }
    } else if (mnem == "END") {
        // END directive - stop assembly
        // Nothing to do in pass 1
    }
}

void Assembler::update_pc_pass1(const SourceLine& line) {
    // Estimate instruction size based on addressing mode
    // For Pass 1, we make a simple estimate
    // TODO: Proper addressing mode detection
    
    if (line.operand.empty()) {
        // Implied/Accumulator mode - 1 byte
        program_counter_ += 1;
    } else if (line.operand[0] == '#') {
        // Immediate mode - 2 bytes
        program_counter_ += 2;
    } else {
        // Assume absolute mode for now - 3 bytes
        // TODO: Detect zero page vs absolute
        program_counter_ += 3;
    }
}

// =========================================
// Pass 2: Generate Code
// =========================================

bool Assembler::pass2(const std::vector<SourceLine>& lines, Result& result, ListingGenerator* listing) {
    program_counter_ = org_address_;
    
    for (const auto& line : lines) {
        current_line_ = line.line_number;
        uint16_t line_start_pc = program_counter_;
        size_t code_start = result.code.size();
        
        // Skip comment-only lines
        if (line.is_comment_only()) {
            if (listing) {
                ListingGenerator::ListingLine list_line;
                list_line.line_number = line.line_number;
                list_line.source_line = line.raw_line;
                list_line.has_address = false;
                listing->add_line(list_line);
            }
            continue;
        }
        
        // Process instruction or directive
        if (line.has_mnemonic()) {
            if (is_directive(line.mnemonic)) {
                if (!process_directive_pass2(line, result, listing)) {
                    // Continue on error to find more errors
                }
            } else {
                if (!process_line_pass2(line, result, listing)) {
                    // Continue on error
                }
            }
        }
        
        // Add to listing if enabled
        if (listing && (line.has_mnemonic() || line.has_label())) {
            ListingGenerator::ListingLine list_line;
            list_line.line_number = line.line_number;
            list_line.address = line_start_pc;
            list_line.source_line = line.raw_line;
            list_line.has_address = (result.code.size() > code_start);
            
            // Copy generated bytes for this line
            for (size_t i = code_start; i < result.code.size(); ++i) {
                list_line.bytes.push_back(result.code[i]);
            }
            
            listing->add_line(list_line);
        }
    }
    
    return result.errors.empty();
}

bool Assembler::process_line_pass2(const SourceLine& line, Result& result, ListingGenerator* listing) {
    // Encode instruction using opcode table
    return encode_instruction(line, result, listing);
}

bool Assembler::encode_instruction(const SourceLine& line, Result& result, ListingGenerator* listing) {
    // Detect addressing mode from operand
    AddressingMode mode = AddressingModeDetector::detect(line.operand, line.mnemonic);
    
    // Look up opcode
    const Opcode* opcode = opcodes_.lookup(line.mnemonic, mode);
    if (!opcode) {
        // Try alternate addressing modes if needed
        // For example, if we detected Absolute but ZeroPage would work
        add_error(result, "Invalid addressing mode for " + line.mnemonic + ": " + line.operand,
                  line.line_number);
        return false;
    }
    
    // Emit opcode byte
    emit_byte(opcode->code, result);
    
    // Emit operand bytes based on addressing mode
    if (mode == AddressingMode::Relative) {
        // Branch instructions: calculate PC-relative offset
        uint16_t target = evaluate_operand(line.operand);
        // PC after this instruction (PC + 2 since branch is 2 bytes: opcode + offset)
        // Note: program_counter_ has been incremented by 1 from emit_byte above
        uint16_t next_pc = program_counter_ + 1;  // +1 for the offset byte we're about to emit
        int16_t offset = static_cast<int16_t>(target - next_pc);
        
        // Check range
        if (offset < -128 || offset > 127) {
            add_error(result, "Branch out of range: " + std::to_string(offset), line.line_number);
        }
        
        emit_byte(static_cast<uint8_t>(offset & 0xFF), result);
    } else if (mode == AddressingMode::Immediate ||
               mode == AddressingMode::ZeroPage ||
               mode == AddressingMode::ZeroPageX ||
               mode == AddressingMode::ZeroPageY ||
               mode == AddressingMode::IndexedIndirect ||
               mode == AddressingMode::IndirectIndexed) {
        // 1-byte operand
        uint16_t value = evaluate_operand(line.operand);
        emit_byte(static_cast<uint8_t>(value & 0xFF), result);
    } else if (mode == AddressingMode::Absolute ||
               mode == AddressingMode::AbsoluteX ||
               mode == AddressingMode::AbsoluteY ||
               mode == AddressingMode::Indirect) {
        // 2-byte operand (little-endian)
        uint16_t value = evaluate_operand(line.operand);
        emit_word_with_relocation(value, line.operand, result);
    }
    // Implied and Accumulator modes have no operand bytes
    
    return true;
}

void Assembler::emit_byte(uint8_t byte, Result& result) {
    result.code.push_back(byte);
    program_counter_++;
}

void Assembler::emit_word(uint16_t word, Result& result) {
    // Little-endian
    emit_byte(static_cast<uint8_t>(word & 0xFF), result);
    emit_byte(static_cast<uint8_t>((word >> 8) & 0xFF), result);
}

// Emit word with relocation tracking for REL mode
void Assembler::emit_word_with_relocation(uint16_t word, const std::string& operand, Result& result) {
    if (rel_mode_) {
        // Evaluate to get relocation info
        ExpressionEvaluator eval(symbols_);
        auto expr_result = eval.evaluate(operand, 2);
        
        if (expr_result.success) {
            // Check if this address needs relocation
            if (expr_result.is_relative || expr_result.is_external) {
                // Add RLD entry at current code position
                uint16_t rld_address = static_cast<uint16_t>(result.code.size());
                uint8_t rld_flags = RLDEntry::TYPE_RELATIVE;
                
                uint8_t symbol_num = 0;
                if (expr_result.is_external) {
                    // Find the external symbol to get its symbol number
                    // Extract symbol name from operand (simplified - may need better parsing)
                    std::string sym_name = operand;
                    // Remove addressing mode prefixes
                    if (!sym_name.empty() && sym_name[0] == '#') sym_name = sym_name.substr(1);
                    if (!sym_name.empty() && sym_name[0] == '<') sym_name = sym_name.substr(1);
                    if (!sym_name.empty() && sym_name[0] == '>') sym_name = sym_name.substr(1);
                    
                    Symbol* sym = symbols_.lookup(sym_name);
                    if (sym && sym->is_external()) {
                        symbol_num = sym->symbol_number;
                        rld_flags = RLDEntry::TYPE_EXTERNAL;
                    }
                }
                
                rel_builder_.add_rld_entry(rld_address, rld_flags, symbol_num);
            }
        }
    }
    
    emit_word(word, result);
}

uint16_t Assembler::evaluate_operand(const std::string& operand) {
    // Use the full ExpressionEvaluator (from ASM2.S EvalExpr line 2561+)
    ExpressionEvaluator eval(symbols_);
    
    // Pass 2 evaluation (all symbols should be defined)
    auto result = eval.evaluate(operand, 2);
    
    if (result.success) {
        return result.value;
    }
    
    // Error - return 0 (error will be reported elsewhere)
    return 0;
}

bool Assembler::process_directive_pass2(const SourceLine& line, Result& result, ListingGenerator* listing) {
    const auto& mnem = line.mnemonic;
    ExpressionEvaluator eval(symbols_);
    
    if (mnem == "ORG") {
        // ORG - set program counter (from ASM3.S L8A82)
        auto expr_result = eval.evaluate(line.operand, 2);
        if (expr_result.success) {
            program_counter_ = expr_result.value;
        } else {
            add_error(result, "ORG: " + expr_result.error_message, line.line_number);
            return false;
        }
    } else if (mnem == "EQU") {
        // EQU - symbol definition, already handled in pass 1
        // Nothing to do in pass 2
    } else if (mnem == "REL") {
        // REL - relocatable mode, already set in pass 1
        // Nothing to emit in pass 2
    } else if (mnem == "ENT" || mnem == "ENTRY") {
        // ENT/ENTRY - entry point declaration, already handled in pass 1
        // Nothing to emit in pass 2
    } else if (mnem == "EXT" || mnem == "EXTRN") {
        // EXT/EXTRN - external reference, already handled in pass 1
        // Nothing to emit in pass 2
    } else if (mnem == "LST") {
        // LST - listing control (from ASM3.S L8ECA)
        // LST ON or LST OFF
        std::string operand = line.operand;
        std::transform(operand.begin(), operand.end(), operand.begin(), ::toupper);
        
        if (operand.find("ON") != std::string::npos) {
            listing_enabled_ = true;
        } else if (operand.find("OFF") != std::string::npos) {
            listing_enabled_ = false;
        } else {
            add_error(result, "LST requires ON or OFF", line.line_number);
            return false;
        }
    } else if (mnem == "MSB") {
        // MSB - high bit control (from ASM3.S L8E66)
        // MSB ON or MSB OFF - must be processed in pass2 for code generation
        std::string operand = line.operand;
        std::transform(operand.begin(), operand.end(), operand.begin(), ::toupper);
        
        if (operand.find("ON") != std::string::npos) {
            msb_on_ = true;
        } else if (operand.find("OFF") != std::string::npos) {
            msb_on_ = false;
        } else {
            add_error(result, "MSB requires ON or OFF", line.line_number);
            return false;
        }
    } else if (mnem == "SBTL") {
        // SBTL directive - subtitle for listing (from ASM3.S)
        // Note: Currently not stored; could be used by listing generator for section headers
        // in future enhancement. For now, accepted but ignored in pass 2.
    } else if (mnem == "DS") {
        // DS - define storage (from ASM3.S L8C0E)
        auto expr_result = eval.evaluate(line.operand, 2);
        if (expr_result.success) {
            // Emit zeros for defined storage
            for (uint16_t i = 0; i < expr_result.value; ++i) {
                emit_byte(0, result);
            }
        } else {
            add_error(result, "DS: " + expr_result.error_message, line.line_number);
            return false;
        }
    } else if (mnem == "DB" || mnem == "DFB") {
        // DB/DFB - define byte(s)
        // Parse operand list: $12,$34,$56 or LABEL,#$00
        // Split on commas
        std::string operand = line.operand;
        size_t pos = 0;
        while (pos < operand.length()) {
            // Find next comma or end
            size_t comma = operand.find(',', pos);
            if (comma == std::string::npos) {
                comma = operand.length();
            }
            
            // Extract this value
            std::string value_str = operand.substr(pos, comma - pos);
            // Trim whitespace
            value_str.erase(0, value_str.find_first_not_of(" \t"));
            value_str.erase(value_str.find_last_not_of(" \t") + 1);
            
            if (!value_str.empty()) {
                auto expr_result = eval.evaluate(value_str, 2);
                if (expr_result.success) {
                    emit_byte(static_cast<uint8_t>(expr_result.value & 0xFF), result);
                } else {
                    add_error(result, "DB: " + expr_result.error_message, line.line_number);
                    return false;
                }
            }
            
            pos = comma + 1;
        }
    } else if (mnem == "DW" || mnem == "DA") {
        // DW/DA - define word(s)
        // Parse operand list similar to DB
        std::string operand = line.operand;
        size_t pos = 0;
        while (pos < operand.length()) {
            size_t comma = operand.find(',', pos);
            if (comma == std::string::npos) {
                comma = operand.length();
            }
            
            std::string value_str = operand.substr(pos, comma - pos);
            value_str.erase(0, value_str.find_first_not_of(" \t"));
            value_str.erase(value_str.find_last_not_of(" \t") + 1);
            
            if (!value_str.empty()) {
                auto expr_result = eval.evaluate(value_str, 2);
                if (expr_result.success) {
                    emit_word(expr_result.value, result);
                } else {
                    add_error(result, "DW: " + expr_result.error_message, line.line_number);
                    return false;
                }
            }
            
            pos = comma + 1;
        }
    } else if (mnem == "ASC") {
        // ASC - ASCII string (from ASM3.S)
        // Extract string from quotes
        // If MSB ON, set high bit on all characters
        std::string str = line.operand;
        bool in_string = false;
        for (char c : str) {
            if (c == '"' || c == '\'') {
                in_string = !in_string;
            } else if (in_string) {
                uint8_t byte = static_cast<uint8_t>(c);
                if (msb_on_) {
                    byte |= HIGH_BIT_MASK;  // Set high bit if MSB ON
                }
                emit_byte(byte, result);
            }
        }
    } else if (mnem == "DCI") {
        // DCI - DCI string (last char inverted/high bit set)
        std::string str = line.operand;
        std::vector<uint8_t> chars;
        bool in_string = false;
        for (char c : str) {
            if (c == '"' || c == '\'') {
                in_string = !in_string;
            } else if (in_string) {
                chars.push_back(static_cast<uint8_t>(c));
            }
        }
        // Emit all but last with high bit clear
        for (size_t i = 0; i < chars.size(); ++i) {
            if (i == chars.size() - 1) {
                // Last character - invert high bit
                emit_byte(chars[i] ^ HIGH_BIT_MASK, result);
            } else {
                emit_byte(chars[i], result);
            }
        }
    } else if (mnem == "END") {
        // END - stop assembly
        // Nothing to emit, but could set a flag to stop
    } else {
        add_error(result, "Unknown directive: " + mnem, line.line_number);
        return false;
    }
    
    return true;
}

// =========================================
// Helpers
// =========================================

void Assembler::add_error(Result& result, const std::string& msg, int line_num) {
    if (line_num < 0) {
        line_num = current_line_;
    }
    result.errors.push_back("Line " + std::to_string(line_num) + ": " + msg);
}

void Assembler::add_warning(Result& result, const std::string& msg, int line_num) {
    if (line_num < 0) {
        line_num = current_line_;
    }
    result.warnings.push_back("Line " + std::to_string(line_num) + ": " + msg);
}

bool Assembler::is_directive(const std::string& mnemonic) const {
    // List of assembler directives (from ASM3.S)
    static const std::vector<std::string> directives = {
        "ORG", "EQU", "DA", "DW", "DB", "DFB", "ASC", "DCI", "DS",
        "REL", "ENT", "EXT", "END", "LST", "SBTL", "MSB", "INCLUDE"
    };
    
    return std::find(directives.begin(), directives.end(), mnemonic) != directives.end();
}

// =========================================
// Include File Preprocessing (from ASM3.S L9348)
// =========================================

std::string Assembler::resolve_include_path(const std::string& include_path) const {
    // Remove quotes from include path
    std::string path = include_path;
    if (!path.empty() && (path.front() == '"' || path.front() == '\'')) {
        path = path.substr(1);
    }
    if (!path.empty() && (path.back() == '"' || path.back() == '\'')) {
        path = path.substr(0, path.size() - 1);
    }
    
    // If path is relative and we have a base path, resolve relative to base
    if (!path.empty() && path[0] != '/' && !base_path_.empty()) {
        return base_path_ + "/" + path;
    }
    
    return path;
}

std::vector<SourceLine> Assembler::preprocess_includes(const std::vector<SourceLine>& lines,
                                                         Result& result,
                                                         int nesting_level) {
    std::vector<SourceLine> expanded;
    
    // Check for nesting limit (original EDASM doesn't allow nested INCLUDEs)
    if (nesting_level > 0) {
        add_error(result, "INCLUDE/CHN NESTING", -1);
        return expanded;
    }
    
    for (const auto& line : lines) {
        // Check if this is an INCLUDE directive
        if (line.has_mnemonic() && line.mnemonic == "INCLUDE") {
            // Validate that INCLUDE is not called from within an include file
            if (in_include_file_) {
                add_error(result, "INCLUDE/CHN NESTING", line.line_number);
                continue;
            }
            
            // Get the include file path
            std::string include_path = resolve_include_path(line.operand);
            
            // Try to read the include file
            std::ifstream include_file(include_path);
            if (!include_file.is_open()) {
                add_error(result, "INCLUDE FILE NOT FOUND: " + include_path, line.line_number);
                continue;
            }
            
            // Read all lines from include file
            std::vector<SourceLine> include_lines;
            std::string include_line;
            int include_line_num = 1;
            
            // Set flag that we're in an include file
            bool saved_include_state = in_include_file_;
            const_cast<Assembler*>(this)->in_include_file_ = true;
            
            while (std::getline(include_file, include_line)) {
                auto parsed = Tokenizer::parse_line(include_line, include_line_num++);
                
                // Check for directives that are invalid from include files
                if (parsed.has_mnemonic()) {
                    if (parsed.mnemonic == "INCLUDE") {
                        add_error(result, "INCLUDE/CHN NESTING", parsed.line_number);
                        continue;
                    }
                    // According to EDASM.SRC, CHN is also invalid from INCLUDE
                    if (parsed.mnemonic == "CHN") {
                        add_error(result, "INVALID FROM INCLUDE", parsed.line_number);
                        continue;
                    }
                }
                
                include_lines.push_back(parsed);
            }
            
            // Restore include state
            const_cast<Assembler*>(this)->in_include_file_ = saved_include_state;
            
            include_file.close();
            
            // Add all lines from include file to expanded lines
            expanded.insert(expanded.end(), include_lines.begin(), include_lines.end());
            
        } else {
            // Not an INCLUDE directive, just copy the line
            expanded.push_back(line);
        }
    }
    
    return expanded;
}

} // namespace edasm
