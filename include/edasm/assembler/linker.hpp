#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "edasm/assembler/rel_file.hpp"

namespace edasm {

// Linker for EDASM relocatable (REL) object files
// Based on LINKER/LINK.S from EDASM.SRC
//
// Performs 6 phases:
// 1. Read command options (file list, output type, origin)
// 2. Build symbol tables from ESD records
// 3. Resolve external references
// 4. Relocate code segments
// 5. Generate output RLD/ESD (for REL output)
// 6. Create load maps (optional)

class Linker {
  public:
    // Linker output configuration
    struct Options {
        enum class OutputType {
            BIN, // Binary executable
            REL, // Relocatable object (for further linking)
            SYS  // System file
        };

        OutputType output_type = OutputType::BIN;
        uint16_t origin = 0x0800;  // Default origin for BIN/SYS
        bool generate_map = false; // Generate load map
        bool align = false;        // Align module boundaries
    };

    // Result of linking operation
    struct Result {
        bool success{false};
        std::vector<std::string> errors;
        std::vector<std::string> warnings;
        std::vector<uint8_t> output_data; // Linked code or REL file
        uint16_t load_address{0x0800};
        uint16_t code_length{0};
        std::string load_map; // Optional load map
    };

    // Entry table record (24 bytes in EDASM, simplified in C++)
    // Stores defined symbols (ENTRY points)
    struct EntryRecord {
        std::string name;
        uint16_t address;      // Final relocated address
        uint8_t flags;         // Symbol flags
        uint8_t module_number; // Which module defined this

        // Linked list of external references (for multi-ref symbols)
        std::vector<size_t> extern_refs; // Indices into extern table
    };

    // External reference record (8 bytes in EDASM, simplified in C++)
    // Stores undefined symbols (EXTERNAL references)
    struct ExternRecord {
        std::string name;
        uint16_t patch_address;            // Address in code to patch
        uint8_t flags;                     // Symbol flags
        uint8_t module_number;             // Which module references this
        uint8_t symbol_number;             // Symbol number in RLD
        bool resolved{false};              // Has this been resolved?
        const EntryRecord *entry{nullptr}; // Points to resolved entry
    };

    // Module information (one per REL file)
    struct Module {
        std::string filename;
        std::vector<uint8_t> code;
        std::vector<RLDEntry> rld_entries;
        std::vector<ESDEntry> esd_entries;
        uint16_t load_address{0}; // Assigned during link
        uint16_t code_length{0};
    };

    Linker() = default;

    // Link multiple REL files into output
    Result link(const std::vector<std::string> &rel_files, const Options &opts);

  private:
    Options options_;
    std::vector<Module> modules_;
    std::unordered_map<std::string, EntryRecord> entry_table_;
    std::vector<ExternRecord> extern_table_;
    uint16_t next_load_address_{0};

    // Phase 1: Load and parse REL files
    bool load_modules(const std::vector<std::string> &filenames, Result &result);
    bool load_rel_file(const std::string &filename, Module &module, Result &result);

    // Phase 2: Build symbol tables from ESD
    bool build_symbol_tables(Result &result);
    void process_esd_entry(const ESDEntry &esd, uint8_t module_num, uint8_t &ext_count,
                           Result &result);

    // Phase 3: Assign load addresses to modules
    void assign_load_addresses();

    // Phase 4: Resolve external references
    bool resolve_externals(Result &result);

    // Phase 5: Relocate code using RLD entries
    bool relocate_code(Result &result);
    void apply_rld_entry(Module &module, const RLDEntry &rld, size_t module_idx, Result &result);

    // Phase 6: Generate output
    std::vector<uint8_t> generate_bin_output();
    std::vector<uint8_t> generate_rel_output();
    std::vector<uint8_t> generate_sys_output();

    // Helper: Generate load map
    std::string generate_load_map() const;

    // Error reporting
    void add_error(Result &result, const std::string &msg);
    void add_warning(Result &result, const std::string &msg);
};

} // namespace edasm
