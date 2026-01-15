#include "edasm/assembler/assembler.hpp"

#include <sstream>
#include <algorithm>

namespace edasm {

Assembler::Assembler() = default;

Assembler::Result Assembler::assemble(const std::string &source) {
    Result result;
    result.org_address = org_address_;
    
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
    
    // Pass 1: Build symbol table, track PC
    if (!pass1(lines, result)) {
        return result;
    }
    
    // Pass 2: Generate code
    if (!pass2(lines, result)) {
        return result;
    }
    
    result.code_length = static_cast<uint16_t>(result.code.size());
    result.success = result.errors.empty();
    return result;
}

void Assembler::reset() {
    symbols_.reset();
    program_counter_ = org_address_;
    current_line_ = 0;
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
    
    if (mnem == "ORG") {
        // TODO: Parse ORG address from operand
        // For now, just note that it's a directive
    } else if (mnem == "EQU") {
        // TODO: Parse EQU value and define symbol
        // For now, just note that it's a directive
    } else if (mnem == "DS") {
        // Define Storage - advance PC
        // TODO: Parse operand for count
        // program_counter_ += count;
    } else if (mnem == "DB" || mnem == "DFB") {
        // Define Byte - estimate size
        // TODO: Parse operand and count bytes
        program_counter_ += 1;  // Minimum 1 byte
    } else if (mnem == "DW" || mnem == "DA") {
        // Define Word - estimate size
        // TODO: Parse operand and count words
        program_counter_ += 2;  // Minimum 2 bytes
    } else if (mnem == "ASC" || mnem == "DCI") {
        // ASCII string
        // TODO: Parse string length
        // For now, assume operand length
        if (!line.operand.empty()) {
            program_counter_ += static_cast<uint16_t>(line.operand.length());
        }
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

bool Assembler::pass2(const std::vector<SourceLine>& lines, Result& result) {
    program_counter_ = org_address_;
    
    for (const auto& line : lines) {
        current_line_ = line.line_number;
        
        // Skip comment-only lines
        if (line.is_comment_only()) {
            continue;
        }
        
        // Process instruction or directive
        if (line.has_mnemonic()) {
            if (is_directive(line.mnemonic)) {
                if (!process_directive_pass2(line, result)) {
                    // Continue on error to find more errors
                }
            } else {
                if (!process_line_pass2(line, result)) {
                    // Continue on error
                }
            }
        }
    }
    
    return result.errors.empty();
}

bool Assembler::process_line_pass2(const SourceLine& line, Result& result) {
    // TODO: Implement instruction encoding
    // This is a placeholder - actual implementation needs opcode table
    add_warning(result, "Instruction encoding not yet implemented: " + line.mnemonic, 
                line.line_number);
    return true;
}

bool Assembler::process_directive_pass2(const SourceLine& line, Result& result) {
    // TODO: Implement directive processing
    // For now, just skip
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
        "REL", "ENT", "EXT", "END", "LST", "SBTL", "MSB"
    };
    
    return std::find(directives.begin(), directives.end(), mnemonic) != directives.end();
}

} // namespace edasm
