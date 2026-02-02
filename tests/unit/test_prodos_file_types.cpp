#include "../../include/edasm/files/file_types.hpp"
#include <cassert>
#include <iostream>

using namespace edasm;

void test_type_from_extension_and_code() {
    assert(type_from_extension(".src") == ProdosFileType::Source);
    assert(prodos_type_code(ProdosFileType::Source) == 0x04);

    assert(type_from_extension(".txt") == ProdosFileType::Text);
    assert(prodos_type_code(ProdosFileType::Text) == 0x04);

    assert(type_from_extension(".lst") == ProdosFileType::Listing);
    assert(prodos_type_code(ProdosFileType::Listing) == 0x04);

    assert(type_from_extension(".obj") == ProdosFileType::Object);
    assert(prodos_type_code(ProdosFileType::Object) == 0xFE);

    assert(type_from_extension(".sys") == ProdosFileType::System);
    assert(prodos_type_code(ProdosFileType::System) == 0xFF);

    assert(type_from_extension(".bin") == ProdosFileType::Binary);
    assert(prodos_type_code(ProdosFileType::Binary) == 0x06);

    // Unknown extension maps to Unknown and numeric code defaults to BIN
    assert(type_from_extension(".weird") == ProdosFileType::Unknown);
    assert(prodos_type_code(ProdosFileType::Unknown) == 0x06);

    std::cout << "✓ test_type_from_extension_and_code passed" << std::endl;
}

int main() {
    test_type_from_extension_and_code();
    return 0;
}
