#include "../../include/edasm/emulator/mli.hpp"
#include <cassert>
#include <chrono>
#include <iomanip>
#include <iostream>

using namespace edasm;

// Test that the lookup table provides correct results
void test_lookup_correctness() {
    // Test all valid call numbers
    const uint8_t valid_calls[] = {0x40, 0x41, 0x65, 0x80, 0x81, 0x82, 0xC0, 0xC1, 0xC2,
                                   0xC3, 0xC4, 0xC5, 0xC6, 0xC7, 0xC8, 0xC9, 0xCA, 0xCB,
                                   0xCC, 0xCD, 0xCE, 0xCF, 0xD0, 0xD1, 0xD2, 0xD3};

    for (uint8_t call_num : valid_calls) {
        const MLICallDescriptor *desc = MLIHandler::get_call_descriptor(call_num);
        assert(desc != nullptr);
        assert(desc->call_number == call_num);
    }

    // Test invalid call numbers
    const uint8_t invalid_calls[] = {0x00, 0x01, 0x42, 0x7F, 0xFF, 0xBF, 0xD4, 0xD5};

    std::cout << "Testing invalid call numbers..." << std::endl;
    for (uint8_t call_num : invalid_calls) {
        std::cout << "  Testing call 0x" << std::hex << std::uppercase << std::setw(2)
                  << std::setfill('0') << static_cast<int>(call_num) << "..." << std::flush;
        const MLICallDescriptor *desc = MLIHandler::get_call_descriptor(call_num);
        if (desc != nullptr) {
            std::cerr << std::endl
                      << "ERROR for call 0x" << std::hex << std::uppercase << std::setw(2)
                      << std::setfill('0') << static_cast<int>(call_num) << ": got descriptor!"
                      << std::endl;
            std::cerr << "  Descriptor address: " << static_cast<const void *>(desc) << std::endl;
            std::cerr << "  Descriptor call_number: 0x" << std::hex << std::uppercase
                      << std::setw(2) << std::setfill('0') << static_cast<int>(desc->call_number)
                      << std::dec << std::endl;
            std::cerr << "  Descriptor name: " << desc->name << std::endl;
            assert(false);
        } else {
            std::cout << " OK" << std::endl;
        }
    }

    std::cout << "✓ Lookup correctness test passed" << std::endl;
}

// Test edge cases
void test_edge_cases() {
    // Test boundary values
    const MLICallDescriptor *desc;

    // Minimum valid call (0x40)
    desc = MLIHandler::get_call_descriptor(0x40);
    assert(desc != nullptr);
    assert(desc->call_number == 0x40);

    // Maximum valid call (0xD3)
    desc = MLIHandler::get_call_descriptor(0xD3);
    assert(desc != nullptr);
    assert(desc->call_number == 0xD3);

    // Call 0x00 (invalid)
    desc = MLIHandler::get_call_descriptor(0x00);
    assert(desc == nullptr);

    // Call 0xFF (invalid)
    desc = MLIHandler::get_call_descriptor(0xFF);
    assert(desc == nullptr);

    // Gap in range (0x43-0x64 are invalid)
    desc = MLIHandler::get_call_descriptor(0x50);
    assert(desc == nullptr);

    // Gap in range (0x83-0xBF are invalid)
    desc = MLIHandler::get_call_descriptor(0xA0);
    assert(desc == nullptr);

    // Gap in range (0xD4-0xFF are invalid)
    desc = MLIHandler::get_call_descriptor(0xE0);
    assert(desc == nullptr);

    std::cout << "✓ Edge cases test passed" << std::endl;
}

// Performance benchmark (optional, for demonstration)
void benchmark_lookup() {
    constexpr int ITERATIONS = 1000000;

    // Test with a mix of valid and invalid call numbers
    const uint8_t test_calls[] = {0xC8, 0xCA, 0xCC, 0xFF, 0x82, 0xC4, 0x00, 0xC6,
                                  0xD1, 0xCF, 0x50, 0xC0, 0x40, 0xD3, 0xA0, 0x65};

    auto start = std::chrono::high_resolution_clock::now();

    int found_count = 0;
    for (int i = 0; i < ITERATIONS; ++i) {
        for (uint8_t call_num : test_calls) {
            const MLICallDescriptor *desc = MLIHandler::get_call_descriptor(call_num);
            if (desc != nullptr) {
                found_count++;
            }
        }
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);

    double avg_lookup_ns =
        static_cast<double>(duration.count() * 1000) / (ITERATIONS * std::size(test_calls));

    std::cout << "✓ Benchmark completed:" << std::endl;
    std::cout << "  Total lookups: " << (ITERATIONS * std::size(test_calls)) << std::endl;
    std::cout << "  Found: " << found_count << " descriptors" << std::endl;
    std::cout << "  Time: " << duration.count() << " µs" << std::endl;
    std::cout << "  Avg per lookup: " << std::fixed << std::setprecision(2) << avg_lookup_ns
              << " ns" << std::endl;
}

int main() {
    std::cout << "Running MLI lookup table tests..." << std::endl;
    std::cout << std::endl;

    test_lookup_correctness();
    test_edge_cases();
    benchmark_lookup();

    std::cout << std::endl;
    std::cout << "All lookup table tests passed!" << std::endl;
    return 0;
}
