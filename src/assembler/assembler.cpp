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
    // Encode instruction using opcode table
    return encode_instruction(line, result);
}

bool Assembler::encode_instruction(const SourceLine& line, Result& result) {
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
        emit_word(value, result);
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

uint16_t Assembler::evaluate_operand(const std::string& operand) {
    // TODO: Implement full expression evaluator
    // For now, handle simple cases
    
    std::string op = operand;
    
    // Strip addressing mode prefixes
    if (!op.empty() && op[0] == '#') {
        op = op.substr(1);  // Immediate mode
    }
    
    // Strip indexing suffixes
    size_t comma = op.find(',');
    if (comma != std::string::npos) {
        op = op.substr(0, comma);
    }
    
    // Strip parentheses for indirect modes
    if (!op.empty() && op[0] == '(') {
        op = op.substr(1);
        size_t close = op.find(')');
        if (close != std::string::npos) {
            op = op.substr(0, close);
        }
    }
    
    // Try to parse as hex ($nnnn)
    if (!op.empty() && op[0] == '$') {
        return static_cast<uint16_t>(std::stoul(op.substr(1), nullptr, 16));
    }
    
    // Try to parse as decimal
    if (!op.empty() && std::isdigit(static_cast<unsigned char>(op[0]))) {
        return static_cast<uint16_t>(std::stoul(op, nullptr, 10));
    }
    
    // Must be a symbol - look it up
    auto value = symbols_.get_value(op);
    if (value.has_value()) {
        return value.value();
    }
    
    // Undefined symbol - return 0 (error will be reported elsewhere)
    return 0;
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
