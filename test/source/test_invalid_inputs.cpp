#include "fast_hex/fast_hex.hpp"
#include "fast_hex/fast_hex_arm_neon.hpp"
#include "fast_hex/fast_hex_x86_64_icelake.hpp"
#include "fast_hex/fast_hex_x86_64_v3.hpp"
#include "fast_hex/fast_hex_x86_64_v4.hpp"

#include <string>
#include <string_view>

#include <doctest/doctest.h>

using namespace heks;

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
        DecodingFunc(reinterpret_cast<uint8_t *>(output.data()), reinterpret_cast<const uint8_t *>(bad_hex_str.data()), RawLength{output.size()});

        CAPTURE(i);
        CAPTURE(bad_hex_str);
    }
}

TEST_CASE("decodeHex_invalid")
{
    testInvalidHexDecoding<decodeHex>();
}

TEST_CASE("decodeHexLUT_invalid")
{
    testInvalidHexDecoding<decodeHexLUT>();
    testInvalidHexDecoding<decodeHexLUT4>();
}

#if defined(FAST_HEX_X86_64)
TEST_CASE("decodeHexAvx2_invalid" * doctest::skip(!cpuHasAvx2()))
{
    testInvalidHexDecoding<decodeHexAvx2>();
}

TEST_CASE("decodeHexAvx512_invalid" * doctest::skip(!cpuHasAvx512()))
{
    testInvalidHexDecoding<decodeHexAvx512>();
}
TEST_CASE("decodeHexAvx512Vbmi_invalid" * doctest::skip(!cpuHasAvx512Vbmi()))
{
    testInvalidHexDecoding<decodeHexAvx512Vbmi>();
}

// The output for invalid input is unspecified, but the AVX2 and x86-64-v4 code compute it the same way, so
// they must agree. Covers the 64-byte loop, the 32-byte step, the overlapping tail and the 17-31 byte path.
TEST_CASE("decodeHexAvx512_matches_avx2_on_invalid" * doctest::skip(!cpuHasAvx2() || !cpuHasAvx512()))
{
    std::string src(2 * 300, '\0');
    for (size_t i = 0; i < src.size(); ++i)
        src[i] = static_cast<char>((i * 131 + 7) ^ (i >> 3)); // A mix of hex and non-hex bytes, including 0x80-0xFF
    for (const char fill : {'Z', '\xFF'})
    {
        const std::string uniform(src.size(), fill);
        for (const std::string_view input : {std::string_view{src}, std::string_view{uniform}})
        {
            for (size_t len = 0; len <= 300; ++len)
            {
                std::string avx2(len, '\0');
                std::string avx512(len, '\0');
                const auto * in = reinterpret_cast<const uint8_t *>(input.data());
                decodeHexAvx2(reinterpret_cast<uint8_t *>(avx2.data()), in, RawLength{len});
                decodeHexAvx512(reinterpret_cast<uint8_t *>(avx512.data()), in, RawLength{len});
                CAPTURE(fill);
                CAPTURE(len);
                CHECK(avx2 == avx512);
            }
        }
    }
}
#endif
