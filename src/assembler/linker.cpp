#include "edasm/assembler/linker.hpp"

#include <fstream>
#include <sstream>
#include <algorithm>

namespace edasm {

Linker::Result Linker::link(const std::vector<std::string>& rel_files, const Options& opts) {
    Result result;
    options_ = opts;
    
    // Reset state
    modules_.clear();
    entry_table_.clear();
    extern_table_.clear();
    next_load_address_ = options_.origin;
    
    // Phase 1: Load and parse REL files
    if (!load_modules(rel_files, result)) {
        return result;
    }
    
    // Phase 2: Build symbol tables from ESD
    if (!build_symbol_tables(result)) {
        return result;
    }
    
    // Phase 3: Assign load addresses to modules
    assign_load_addresses();
    
    // Phase 4: Resolve external references
    if (!resolve_externals(result)) {
        return result;
    }
    
    // Phase 5: Relocate code using RLD entries
    if (!relocate_code(result)) {
        return result;
    }
    
    // Phase 6: Generate output based on output type
    switch (options_.output_type) {
        case Options::OutputType::BIN:
            result.output_data = generate_bin_output();
            break;
        case Options::OutputType::REL:
            result.output_data = generate_rel_output();
            break;
        case Options::OutputType::SYS:
            result.output_data = generate_sys_output();
            break;
    }
    
    result.load_address = options_.origin;
    result.code_length = static_cast<uint16_t>(result.output_data.size());
    
    // Generate load map if requested
    if (options_.generate_map) {
        result.load_map = generate_load_map();
    }
    
    result.success = result.errors.empty();
    return result;
}

// =========================================
// Phase 1: Load Modules
// =========================================

bool Linker::load_modules(const std::vector<std::string>& filenames, Result& result) {
    if (filenames.empty()) {
        add_error(result, "No input files specified");
        return false;
    }
    
    for (size_t i = 0; i < filenames.size(); ++i) {
        Module module;
        module.filename = filenames[i];
        
        if (!load_rel_file(filenames[i], module, result)) {
            return false;
        }
        
        modules_.push_back(std::move(module));
    }
    
    return true;
}

bool Linker::load_rel_file(const std::string& filename, Module& module, Result& result) {
    // Read file contents
    std::ifstream file(filename, std::ios::binary);
    if (!file) {
        add_error(result, "Cannot open file: " + filename);
        return false;
    }
    
    // Read entire file into vector
    std::vector<uint8_t> file_data((std::istreambuf_iterator<char>(file)),
                                    std::istreambuf_iterator<char>());
    file.close();
    
    if (file_data.empty()) {
        add_error(result, "Empty file: " + filename);
        return false;
    }
    
    // Parse REL file format
    if (!RELFileBuilder::parse(file_data, module.code, module.rld_entries, module.esd_entries)) {
        add_error(result, "Invalid REL file format: " + filename);
        return false;
    }
    
    module.code_length = static_cast<uint16_t>(module.code.size());
    return true;
}

// =========================================
// Phase 2: Build Symbol Tables
// =========================================

bool Linker::build_symbol_tables(Result& result) {
    // Process ESD entries from all modules
    for (size_t mod_num = 0; mod_num < modules_.size(); ++mod_num) {
        const auto& module = modules_[mod_num];
        
        // Count external symbols for this module (for symbol numbering)
        uint8_t ext_count = 0;
        
        for (const auto& esd : module.esd_entries) {
            if (esd.is_external()) {
                ext_count++;
            }
            process_esd_entry(esd, static_cast<uint8_t>(mod_num), ext_count, result);
        }
    }
    
    return result.errors.empty();
}

void Linker::process_esd_entry(const ESDEntry& esd, uint8_t module_num, uint8_t& ext_count, Result& result) {
    if (esd.is_entry()) {
        // ENTRY symbol - add to entry table
        if (entry_table_.find(esd.name) != entry_table_.end()) {
            // Duplicate ENTRY definition
            add_warning(result, "Duplicate ENTRY symbol: " + esd.name);
            return;
        }
        
        EntryRecord entry;
        entry.name = esd.name;
        entry.address = esd.address;  // Will be relocated later
        entry.flags = esd.flags;
        entry.module_number = module_num;
        
        entry_table_[esd.name] = entry;
        
    } else if (esd.is_external()) {
        // EXTERNAL reference - add to extern table
        // The symbol number in RLD will be the index of this external symbol (1-indexed)
        ExternRecord ext;
        ext.name = esd.name;
        ext.patch_address = esd.address;
        ext.flags = esd.flags;
        ext.module_number = module_num;
        ext.symbol_number = ext_count;  // Use the counter passed in
        ext.resolved = false;
        
        extern_table_.push_back(ext);
    }
}

// =========================================
// Phase 3: Assign Load Addresses
// =========================================

void Linker::assign_load_addresses() {
    uint16_t current_address = next_load_address_;
    
    for (auto& module : modules_) {
        module.load_address = current_address;
        current_address += module.code_length;
        
        // Optional alignment (align to page boundary)
        if (options_.align && (current_address & 0xFF) != 0) {
            current_address = (current_address + 0x100) & 0xFF00;
        }
    }
}

// =========================================
// Phase 4: Resolve External References
// =========================================

bool Linker::resolve_externals(Result& result) {
    for (auto& ext : extern_table_) {
        // Look up in entry table
        auto it = entry_table_.find(ext.name);
        if (it == entry_table_.end()) {
            // Unresolved external - error or warning depending on output type
            if (options_.output_type == Options::OutputType::REL) {
                // For REL output, unresolved externals are OK
                add_warning(result, "Unresolved external: " + ext.name);
            } else {
                add_error(result, "Unresolved external: " + ext.name);
            }
            continue;
        }
        
        // Resolve the external reference
        ext.resolved = true;
        ext.entry = &it->second;
        
        // Add this external to the entry's reference list
        it->second.extern_refs.push_back(extern_table_.size() - 1);
    }
    
    return result.errors.empty();
}

// =========================================
// Phase 5: Relocate Code
// =========================================

bool Linker::relocate_code(Result& result) {
    for (size_t mod_idx = 0; mod_idx < modules_.size(); ++mod_idx) {
        auto& module = modules_[mod_idx];
        // Relocate each symbol in the module
        for (const auto& rld : module.rld_entries) {
            apply_rld_entry(module, rld, mod_idx, result);
        }
    }
    
    return result.errors.empty();
}

void Linker::apply_rld_entry(Module& module, const RLDEntry& rld, size_t module_idx, Result& result) {
    // Get current address in code (little-endian)
    if (rld.address + 1 >= module.code.size()) {
        add_error(result, "RLD entry address out of range in " + module.filename);
        return;
    }
    
    uint16_t current_value = module.code[rld.address] | 
                            (static_cast<uint16_t>(module.code[rld.address + 1]) << 8);
    
    uint16_t relocated_value = current_value;
    
    if (rld.flags == RLDEntry::TYPE_RELATIVE) {
        // Relative address - add module's load address
        // The current value is relative to the module's origin
        relocated_value = current_value + module.load_address;
        
    } else if (rld.flags == RLDEntry::TYPE_EXTERNAL) {
        // External reference - find the external symbol by number for this module
        bool found = false;
        for (const auto& ext : extern_table_) {
            if (ext.module_number == module_idx && ext.symbol_number == rld.symbol_num) {
                if (ext.resolved && ext.entry) {
                    // Use the entry's relocated address (absolute address)
                    relocated_value = ext.entry->address + 
                                     modules_[ext.entry->module_number].load_address;
                    found = true;
                    break;
                }
            }
        }
        
        if (!found) {
            add_warning(result, "Could not resolve RLD external reference (sym=" + 
                       std::to_string(rld.symbol_num) + ") at offset " + 
                       std::to_string(rld.address) + " in " + module.filename);
        }
    }
    
    // Write back relocated value (little-endian)
    module.code[rld.address] = static_cast<uint8_t>(relocated_value & 0xFF);
    module.code[rld.address + 1] = static_cast<uint8_t>(relocated_value >> 8);
}

// =========================================
// Phase 6: Generate Output
// =========================================

std::vector<uint8_t> Linker::generate_bin_output() {
    std::vector<uint8_t> output;
    
    // Concatenate all module code
    for (const auto& module : modules_) {
        output.insert(output.end(), module.code.begin(), module.code.end());
    }
    
    return output;
}

std::vector<uint8_t> Linker::generate_rel_output() {
    // For REL output, we need to:
    // 1. Concatenate code from all modules
    // 2. Generate new RLD for remaining relocations
    // 3. Generate new ESD for unresolved externals and entries
    
    std::vector<uint8_t> combined_code;
    std::vector<RLDEntry> combined_rld;
    std::vector<ESDEntry> combined_esd;
    
    // Combine code and adjust RLD addresses
    uint16_t code_offset = 0;
    for (const auto& module : modules_) {
        combined_code.insert(combined_code.end(), module.code.begin(), module.code.end());
        
        // Adjust RLD addresses by code offset
        for (auto rld : module.rld_entries) {
            rld.address += code_offset;
            combined_rld.push_back(rld);
        }
        
        code_offset += module.code_length;
    }
    
    // Add unresolved externals to ESD
    for (const auto& ext : extern_table_) {
        if (!ext.resolved) {
            ESDEntry esd;
            esd.name = ext.name;
            esd.address = ext.patch_address;
            esd.flags = ext.flags;
            esd.symbol_num = ext.symbol_number;
            combined_esd.push_back(esd);
        }
    }
    
    // Add entries to ESD
    for (const auto& [name, entry] : entry_table_) {
        ESDEntry esd;
        esd.name = name;
        esd.address = entry.address + modules_[entry.module_number].load_address;
        esd.flags = entry.flags;
        combined_esd.push_back(esd);
    }
    
    // Build REL file format
    RELFileBuilder builder;
    for (const auto& rld : combined_rld) {
        builder.add_rld_entry(rld.address, rld.flags, rld.symbol_num);
    }
    for (const auto& esd : combined_esd) {
        builder.add_esd_entry(esd.name, esd.address, esd.flags, esd.symbol_num);
    }
    
    return builder.build(combined_code);
}

std::vector<uint8_t> Linker::generate_sys_output() {
    // SYS format is same as BIN for now
    // (In ProDOS, SYS files have different file type metadata)
    return generate_bin_output();
}

// =========================================
// Load Map Generation
// =========================================

std::string Linker::generate_load_map() const {
    std::ostringstream map;
    
    map << "EDASM Linker Load Map\n";
    map << "=====================\n\n";
    
    map << "Modules:\n";
    for (size_t i = 0; i < modules_.size(); ++i) {
        const auto& module = modules_[i];
        map << "  " << (i + 1) << ". " << module.filename << "\n";
        map << "     Load Address: $" << std::hex << std::uppercase << module.load_address << "\n";
        map << "     Code Length:  " << std::dec << module.code_length << " bytes\n";
    }
    
    map << "\nEntry Points:\n";
    for (const auto& [name, entry] : entry_table_) {
        uint16_t final_addr = entry.address + modules_[entry.module_number].load_address;
        map << "  " << name << " = $" << std::hex << std::uppercase << final_addr;
        map << " (module " << (entry.module_number + 1) << ")\n";
    }
    
    if (!extern_table_.empty()) {
        map << "\nExternal References:\n";
        for (const auto& ext : extern_table_) {
            map << "  " << ext.name << " (module " << (ext.module_number + 1) << ")";
            if (ext.resolved) {
                map << " -> RESOLVED\n";
            } else {
                map << " -> UNRESOLVED\n";
            }
        }
    }
    
    return map.str();
}

// =========================================
// Error Reporting
// =========================================

void Linker::add_error(Result& result, const std::string& msg) {
    result.errors.push_back("Linker error: " + msg);
}

void Linker::add_warning(Result& result, const std::string& msg) {
    result.warnings.push_back("Linker warning: " + msg);
}

} // namespace edasm
