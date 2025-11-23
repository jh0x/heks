#include "fast_hex/fast_hex.hpp"

#include <string>
#include <string_view>

#include <doctest/doctest.h>

#if FAST_HEX_USE_NAMESPACE
using namespace heks;
#endif

// clang-format off
constexpr std::string_view invalid_hex_data[] = {
    "GG",           // Both invalid
    "HH",           // Both invalid
    "0G",           // One invalid
    "G0",           // One invalid
    "XX",           // Both invalid
    "XY",           // Both invalid
    "1!",           // Special character
    "@#",           // Special characters
    "  ",           // Whitespace
    "0 ",           // Whitespace
    " 0",           // Whitespace
    "[]",           // Brackets
    "{}",           // Braces
    "||",           // Pipes
    "~~",           // Tilde
    "??",           // Question marks
    "0z",           // Invalid hex digit
    "xy",           // Invalid lowercase hex digit
    "1g",           // Invalid hex digit
    "GGHH",         // Multiple invalid pairs
    "0G1H",         // Mixed valid/invalid
    "000G",         // Valid followed by invalid
    "G0H1",         // Invalid followed by invalid
    "012X",         // Valid followed by invalid
    "XXYYZZ",       // Longer invalid string
    "001122GG",     // Valid hex followed by invalid
    "A@B#C!",       // Mixed valid and invalid special chars
    "0\t",          // Tab character
    "0\n",          // Newline character
    "0\r",          // Carriage return
    "0\xFF",        // Non-ASCII character
    "\x01\x02",     // Control characters
    "FFFFFFFFFFFXXF0G",      // Length 16, valid hex followed by invalid
    "FFFFFFFFFFFFFFFF0G",    // Length 18, valid hex followed by invalid
    "GGGGGGGGGGGGGGGGGGGG",  // Length 20, all invalid
    "0123456789ABCDEFGHIJ",  // Length 20, mixed valid/invalid
};
//clang-format on

template <auto DecodingFunc>
void testInvalidHexDecoding()
{
    for (size_t i = 0; i < sizeof(invalid_hex_data) / sizeof(invalid_hex_data[0]); ++i)
    {
        const auto & bad_hex_str = invalid_hex_data[i];

        std::string output;
        output.resize(bad_hex_str.size() / 2);

        // The key test: just calling the function without sanitizer/crash/etc
        // For more - should do fuzzing
        DecodingFunc(reinterpret_cast<uint8_t *>(output.data()), reinterpret_cast<const uint8_t *>(bad_hex_str.data()), output.size());

        CAPTURE(i);
        CAPTURE(bad_hex_str);
    }
}

TEST_CASE("decodeHexLUT_invalid")
{
    testInvalidHexDecoding<decodeHexLUT>();
}

TEST_CASE("decodeHexLUT4_invalid")
{
    testInvalidHexDecoding<decodeHexLUT4>();
}

#if defined(__AVX2__)
TEST_CASE("decodeHexVec_invalid")
{
    testInvalidHexDecoding<decodeHexVec>();
}
#endif
