#include "cpu_feature_tests.hpp"
#include "fast_hex/fast_hex.hpp"
#include "fast_hex/fast_hex_arm_neon.hpp"
#include "fast_hex/fast_hex_arm_sve.hpp"
#include "fast_hex/fast_hex_x86_64_icelake.hpp"
#include "fast_hex/fast_hex_x86_64_v3.hpp"
#include "fast_hex/fast_hex_x86_64_v4.hpp"
#include "sve_vector_lengths.hpp"

#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

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

constexpr size_t SweepMaxLength = 300; // Past the loops, steps and tails of every decoder

// 2 * SweepMaxLength chars each
std::vector<std::pair<const char *, std::string>> invalidSweepInputs()
{
    std::string mix(2 * SweepMaxLength, '\0');
    for (size_t i = 0; i < mix.size(); ++i)
        mix[i] = static_cast<char>((i * 131 + 7) ^ (i >> 3)); // A mix of hex and non-hex bytes, including 0x80-0xFF
    // Valid hex with one bad char every 13 chars. The odd step makes it alternately the first and second char of
    // its pair, and moves it through the positions of a vector
    std::string sparse(mix.size(), '\0');
    for (size_t i = 0; i < sparse.size(); ++i)
        sparse[i] = "0123456789abcdefABCDEF"[(i * 7 + 3) % 22];
    for (size_t i = 0; i < sparse.size(); i += 13)
        sparse[i] = static_cast<char>("G@`/:g \xFF"[(i / 13) % 8]);
    return {{"mix", mix},
            {"sparse", sparse},
            {"all 'Z'", std::string(mix.size(), 'Z')},
            {"all 'G'", std::string(mix.size(), 'G')},
            {"all 0xFF", std::string(mix.size(), '\xFF')}};
}

// Invalid input at every length up to SweepMaxLength, so that it goes through every path of the decoder: the
// fixed strings above stop at 10 bytes. One char that is not hex leaves the whole output unspecified (see the
// README), so all there is to check is that nothing is written past it, or read past the input (for ASan: the
// buffers end where they should)
template <auto Decode>
void testInvalidLengthSweep()
{
    constexpr uint8_t Canary = 0xA5;
    constexpr size_t Tail = 64;
    for (const auto & [name, input] : invalidSweepInputs())
    {
        CAPTURE(name);
        for (size_t len = 0; len <= SweepMaxLength; ++len)
        {
            CAPTURE(len);
            const std::vector<uint8_t> src(input.begin(), input.begin() + static_cast<std::ptrdiff_t>(2 * len));
            std::vector<uint8_t> dest(len + Tail, Canary);
            Decode(dest.data(), src.data(), RawLength{len});

            bool tail_untouched = true;
            for (size_t i = len; i < dest.size(); ++i)
                tail_untouched = tail_untouched && dest[i] == Canary;
            CHECK(tail_untouched);
        }
    }
}

// The output for invalid input is unspecified, but two decoders that compute it the same way must agree
template <auto DecodeA, auto DecodeB>
void testDecodersAgreeOnInvalid()
{
    for (const auto & [name, input] : invalidSweepInputs())
    {
        const auto * in = reinterpret_cast<const uint8_t *>(input.data());
        for (size_t len = 0; len <= SweepMaxLength; ++len)
        {
            std::string a(len, '\0');
            std::string b(len, '\0');
            DecodeA(reinterpret_cast<uint8_t *>(a.data()), in, RawLength{len});
            DecodeB(reinterpret_cast<uint8_t *>(b.data()), in, RawLength{len});
            CAPTURE(name);
            CAPTURE(len);
            CHECK(a == b);
        }
    }
}

TEST_CASE("decodeHex_invalid")
{
    testInvalidHexDecoding<decodeHex>();
    testInvalidLengthSweep<decodeHex>();
}

TEST_CASE("decodeHexLUT_invalid")
{
    testInvalidHexDecoding<decodeHexLUT>();
    testInvalidHexDecoding<decodeHexLUT4>();
    testInvalidLengthSweep<decodeHexLUT>();
    testInvalidLengthSweep<decodeHexLUT4>();
}

#if defined(FAST_HEX_X86_64)
TEST_CASE("decodeHexAvx2_invalid" * heks_test::avx2())
{
    testInvalidHexDecoding<decodeHexAvx2>();
    testInvalidLengthSweep<decodeHexAvx2>();
}

TEST_CASE("decodeHexAvx512_invalid" * heks_test::avx512())
{
    testInvalidHexDecoding<decodeHexAvx512>();
    testInvalidLengthSweep<decodeHexAvx512>();
}
TEST_CASE("decodeHexAvx512Vbmi_invalid" * heks_test::avx512vbmi())
{
    testInvalidHexDecoding<decodeHexAvx512Vbmi>();
    testInvalidLengthSweep<decodeHexAvx512Vbmi>();
}

// The output for invalid input is unspecified, but the AVX2 and x86-64-v4 code compute it the same way, so
// they must agree. Covers the 64-byte loop, the 32-byte step, the overlapping tail and the 17-31 byte path.
TEST_CASE("decodeHexAvx512_matches_avx2_on_invalid" * heks_test::avx512().andNeeds(cpuHasAvx2()))
{
    testDecodersAgreeOnInvalid<decodeHexAvx2, decodeHexAvx512>();
}
#endif

#if defined(FAST_HEX_NEON)
TEST_CASE("decodeHexNeon_invalid" * heks_test::neon())
{
    testInvalidHexDecoding<decodeHexNeon>();
    testInvalidLengthSweep<decodeHexNeon>();
}
#endif

#if defined(FAST_HEX_ARM_SVE)
TEST_CASE("decodeHexSve_invalid" * heks_test::sve())
{
    heks_test::forEachSveVectorLength(
        []
        {
            testInvalidHexDecoding<decodeHexSve>();
            testInvalidLengthSweep<decodeHexSve>();
        });
}

// The output for invalid input is unspecified, but decodeHexSve computes it as decodeHexNeon does, so they must
// agree at any length. Covers the NEON paths, the SVE loop and its predicated tail.
TEST_CASE("decodeHexSve_matches_neon_on_invalid" * heks_test::sve())
{
    heks_test::forEachSveVectorLength([] { testDecodersAgreeOnInvalid<decodeHexNeon, decodeHexSve>(); });
}
#endif
