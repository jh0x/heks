#include "fast_hex/fast_hex.hpp"
#include "fast_hex/fast_hex_arm_neon.hpp"
#include "fast_hex/fast_hex_x86_64_icelake.hpp"
#include "fast_hex/fast_hex_x86_64_v3.hpp"
#include "fast_hex/fast_hex_x86_64_v4.hpp"

#include <algorithm>
#include <cctype>
#include <iomanip>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

#include <doctest/doctest.h>

using namespace heks;

using namespace std::string_view_literals;

// Test data for encodeHex tests
constexpr std::string_view raw_data[] = {
    // Empty string
    ""sv,

    // 1
    "\x00"sv,
    "\x01"sv,
    "\x41"sv,
    "\xFF"sv,

    // 2
    "\x00\x00"sv,
    "\x01\x02"sv,
    "\x41\x42"sv,
    "\xFF\xFE"sv,

    // 3
    "\x00\x01\x02"sv,
    "\x41\x42\x43"sv,
    "\xFF\xFE\xFD"sv,

    // 4
    "\x00\x01\x02\x03"sv,
    "\x10\x20\x30\x40"sv,
    "\xAA\xBB\xCC\xDD"sv,

    // 5
    "\x00\x01\x02\x03\x04"sv,
    "Hello"sv,

    // 6
    "Hello!"sv,
    "\x00\x11\x22\x33\x44\x55"sv,

    // 7
    "Test123"sv,
    "\x01\x02\x03\x04\x05\x06\x07"sv,

    // 8
    "TestData"sv,
    "\xDE\xAD\xBE\xEF\xFE\xED\xFA\xCE"sv,

    // 9
    "Test12345"sv,
    "\x00\x11\x22\x33\x44\x55\x66\x77\x88"sv,

    // 10
    "Test123456"sv,
    "\x0A\x0B\x0C\x0D\x0E\x0F\x10\x11\x12\x13"sv,

    // 11
    "Test1234567"sv,
    "\x01\x02\x03\x04\x05\x06\x07\x08\x09\x0A\x0B"sv,

    // 12
    "Test12345678"sv,
    "\xF0\xE0\xD0\xC0\xB0\xA0\x90\x80\x70\x60\x50\x40"sv,

    // 13
    "Test123456789"sv,
    "\x01\x02\x03\x04\x05\x06\x07\x08\x09\x0A\x0B\x0C\x0D"sv,

    // 14
    "Test123456789A"sv,
    "\x10\x11\x12\x13\x14\x15\x16\x17\x18\x19\x1A\x1B\x1C\x1D"sv,

    // 15
    "Test123456789AB"sv,
    "\x00\x01\x02\x03\x04\x05\x06\x07\x08\x09\x0A\x0B\x0C\x0D\x0E"sv,

    // 16
    "Test123456789ABC"sv,
    "\x0F\x0E\x0D\x0C\x0B\x0A\x09\x08\x07\x06\x05\x04\x03\x02\x01\x00"sv,

    // 17
    "Test123456789ABCD"sv,
    "\x01\x02\x03\x04\x05\x06\x07\x08\x09\x0A\x0B\x0C\x0D\x0E\x0F\x10\x11"sv,

    // 18
    "Test123456789ABCDE"sv,
    "\x12\x13\x14\x15\x16\x17\x18\x19\x1A\x1B\x1C\x1D\x1E\x1F\x20\x21\x22\x23"sv,

    // 19
    "Test123456789ABCDEF"sv,
    "\x01\x02\x03\x04\x05\x06\x07\x08\x09\x0A\x0B\x0C\x0D\x0E\x0F\x10\x11\x12\x13"sv,

    // 20
    "Test123456789ABCDEF0"sv,
    "\x14\x15\x16\x17\x18\x19\x1A\x1B\x1C\x1D\x1E\x1F\x20\x21\x22\x23\x24\x25\x26\x27"sv,

    // Long
    "\x00\x01\x02\x03\x04\x05\x06\x07\x08\x09\x0a\x0b\x0c\x0d\x0e\x0f"
    "\x10\x11\x12\x13\x14\x15\x16\x17\x18\x19\x1a\x1b\x1c\x1d\x1e\x1f"
    "\x20\x21\x22\x23\x24\x25\x26\x27\x28\x29\x2a\x2b\x2c\x2d\x2e\x2f"
    "\x30\x31\x32\x33\x34\x35\x36\x37\x38\x39\x3a\x3b\x3c\x3d\x3e\x3f"
    "\x40\x41\x42\x43\x44\x45\x46\x47\x48\x49\x4a\x4b\x4c\x4d\x4e\x4f"
    "\x50\x51\x52\x53\x54\x55\x56\x57\x58\x59\x5a\x5b\x5c\x5d\x5e\x5f"
    "\x60\x61\x62\x63\x64\x65\x66\x67\x68\x69\x6a\x6b\x6c\x6d\x6e\x6f"
    "\x70\x71\x72\x73\x74\x75\x76\x77\x78\x79\x7a\x7b\x7c\x7d\x7e\x7f"
    "\x80\x81\x82\x83\x84\x85\x86\x87\x88\x89\x8a\x8b\x8c\x8d\x8e\x8f"
    "\x90\x91\x92\x93\x94\x95\x96\x97\x98\x99\x9a\x9b\x9c\x9d\x9e\x9f"
    "\xa0\xa1\xa2\xa3\xa4\xa5\xa6\xa7\xa8\xa9\xaa\xab\xac\xad\xae\xaf"
    "\xb0\xb1\xb2\xb3\xb4\xb5\xb6\xb7\xb8\xb9\xba\xbb\xbc\xbd\xbe\xbf"
    "\xc0\xc1\xc2\xc3\xc4\xc5\xc6\xc7\xc8\xc9\xca\xcb\xcc\xcd\xce\xcf"
    "\xd0\xd1\xd2\xd3\xd4\xd5\xd6\xd7\xd8\xd9\xda\xdb\xdc\xdd\xde\xdf"
    "\xe0\xe1\xe2\xe3\xe4\xe5\xe6\xe7\xe8\xe9\xea\xeb\xec\xed\xee\xef"
    "\xf0\xf1\xf2\xf3\xf4\xf5\xf6\xf7\xf8\xf9\xfa\xfb\xfc\xfd\xfe\xff"sv};

// Expected hex encodings for the raw data
constexpr std::string_view encoded_data[] = {
    // Empty string
    ""sv,

    // 1
    "00"sv,
    "01"sv,
    "41"sv,
    "ff"sv,

    // 2
    "0000"sv,
    "0102"sv,
    "4142"sv,
    "fffe"sv,

    // 3
    "000102"sv,
    "414243"sv,
    "fffefd"sv,

    // 4
    "00010203"sv,
    "10203040"sv,
    "aabbccdd"sv,

    // 5
    "0001020304"sv,
    "48656c6c6f"sv,

    // 6
    "48656c6c6f21"sv,
    "001122334455"sv,

    // 7
    "54657374313233"sv,
    "01020304050607"sv,

    // 8
    "5465737444617461"sv,
    "deadbeeffeedface"sv,

    // 9
    "546573743132333435"sv,
    "001122334455667788"sv,

    // 10
    "54657374313233343536"sv,
    "0a0b0c0d0e0f10111213"sv,

    // 11
    "5465737431323334353637"sv,
    "0102030405060708090a0b"sv,

    // 12
    "546573743132333435363738"sv,
    "f0e0d0c0b0a0908070605040"sv,

    // 13
    "54657374313233343536373839"sv,
    "0102030405060708090a0b0c0d"sv,

    // 14
    "5465737431323334353637383941"sv,
    "101112131415161718191a1b1c1d"sv,

    // 15
    "546573743132333435363738394142"sv,
    "000102030405060708090a0b0c0d0e"sv,

    // 16
    "54657374313233343536373839414243"sv,
    "0f0e0d0c0b0a09080706050403020100"sv,

    // 17
    "5465737431323334353637383941424344"sv,
    "0102030405060708090a0b0c0d0e0f1011"sv,

    // 18
    "546573743132333435363738394142434445"sv,
    "12131415161718191a1b1c1d1e1f20212223"sv,

    // 19
    "54657374313233343536373839414243444546"sv,
    "0102030405060708090a0b0c0d0e0f10111213"sv,

    // 20
    "5465737431323334353637383941424344454630"sv,
    "1415161718191a1b1c1d1e1f2021222324252627"sv,

    // Long
    "000102030405060708090a0b0c0d0e0f"
    "101112131415161718191a1b1c1d1e1f"
    "202122232425262728292a2b2c2d2e2f"
    "303132333435363738393a3b3c3d3e3f"
    "404142434445464748494a4b4c4d4e4f"
    "505152535455565758595a5b5c5d5e5f"
    "606162636465666768696a6b6c6d6e6f"
    "707172737475767778797a7b7c7d7e7f"
    "808182838485868788898a8b8c8d8e8f"
    "909192939495969798999a9b9c9d9e9f"
    "a0a1a2a3a4a5a6a7a8a9aaabacadaeaf"
    "b0b1b2b3b4b5b6b7b8b9babbbcbdbebf"
    "c0c1c2c3c4c5c6c7c8c9cacbcccdcecf"
    "d0d1d2d3d4d5d6d7d8d9dadbdcdddedf"
    "e0e1e2e3e4e5e6e7e8e9eaebecedeeef"
    "f0f1f2f3f4f5f6f7f8f9fafbfcfdfeff"sv};

template <auto EncodingFuncLower, auto EncodingFuncUpper>
void testHexEncoding()
{
    for (size_t i = 0; i < sizeof(raw_data) / sizeof(raw_data[0]); ++i)
    {
        const auto & raw_str = raw_data[i];
        const auto & expected_lower = encoded_data[i];
        const auto expected_upper = [&]()
        {
            auto upper_str = std::string(expected_lower);
            for (auto & ch : upper_str)
            {
                ch = static_cast<char>(std::toupper(static_cast<unsigned char>(ch)));
            }
            return upper_str;
        }();

        std::string output_lower;
        std::string output_upper;
        output_lower.resize(raw_str.size() * 2);
        output_upper.resize(raw_str.size() * 2);

        EncodingFuncLower(
            reinterpret_cast<uint8_t *>(output_lower.data()),
            reinterpret_cast<const uint8_t *>(raw_str.data()),
            RawLength{raw_str.size()},
            lower);
        EncodingFuncUpper(
            reinterpret_cast<uint8_t *>(output_upper.data()),
            reinterpret_cast<const uint8_t *>(raw_str.data()),
            RawLength{raw_str.size()},
            upper);

        CAPTURE(i);
        REQUIRE(output_lower == expected_lower);
        REQUIRE(output_upper == expected_upper);
    }
}

template <auto DecodingFunc>
void testHexDecoding()
{
    for (size_t i = 0; i < sizeof(raw_data) / sizeof(raw_data[0]); ++i)
    {
        const auto & hex_str_lower = encoded_data[i];
        const auto hex_str_upper = [&]()
        {
            auto upper_str = std::string(hex_str_lower);
            for (auto & ch : upper_str)
            {
                ch = static_cast<char>(std::toupper(static_cast<unsigned char>(ch)));
            }
            return upper_str;
        }();
        const auto & expected = raw_data[i];

        std::string output_from_lower;
        std::string output_from_upper;
        output_from_lower.resize(hex_str_lower.size() / 2);
        output_from_upper.resize(hex_str_upper.size() / 2);

        DecodingFunc(
            reinterpret_cast<uint8_t *>(output_from_lower.data()),
            reinterpret_cast<const uint8_t *>(hex_str_lower.data()),
            RawLength{output_from_lower.size()});
        DecodingFunc(
            reinterpret_cast<uint8_t *>(output_from_upper.data()),
            reinterpret_cast<const uint8_t *>(hex_str_upper.data()),
            RawLength{output_from_upper.size()});

        CAPTURE(i);
        CAPTURE(hex_str_lower);
        CAPTURE(hex_str_upper);
        std::ostringstream oss;
        for (size_t j = 0; j < output_from_lower.size(); ++j)
        {
            oss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(static_cast<unsigned char>(output_from_lower[j]));
        }
        CAPTURE(oss.str());
        REQUIRE(output_from_lower == expected);
        REQUIRE(output_from_upper == expected);
    }
}

// Compare against the scalar reference for every length up to a few vector widths, so that each
// main loop / tail combination is exercised. Also checks nothing is written past the end of dest.
template <auto EncodingFuncLower, auto EncodingFuncUpper, auto DecodingFunc>
void testHexLengthSweep()
{
    constexpr size_t MaxLen = 300;
    constexpr uint8_t Canary = 0xA5;
    std::vector<uint8_t> raw(MaxLen);
    for (size_t i = 0; i < MaxLen; ++i)
    {
        raw[i] = static_cast<uint8_t>(i * 131 + 7);
    }

    for (size_t len = 0; len <= MaxLen; ++len)
    {
        CAPTURE(len);
        std::vector<uint8_t> expected_lower(len * 2);
        std::vector<uint8_t> expected_upper(len * 2);
        encodeHex(expected_lower.data(), raw.data(), RawLength{len}, lower);
        encodeHex(expected_upper.data(), raw.data(), RawLength{len}, upper);

        std::vector<uint8_t> out(len * 2 + 1, Canary);
        EncodingFuncLower(out.data(), raw.data(), RawLength{len}, lower);
        REQUIRE(std::equal(expected_lower.begin(), expected_lower.end(), out.begin()));
        REQUIRE(out[len * 2] == Canary);

        std::fill(out.begin(), out.end(), Canary);
        EncodingFuncUpper(out.data(), raw.data(), RawLength{len}, upper);
        REQUIRE(std::equal(expected_upper.begin(), expected_upper.end(), out.begin()));
        REQUIRE(out[len * 2] == Canary);

        // Mixed case input
        std::vector<uint8_t> mixed(expected_lower);
        for (size_t i = 0; i < mixed.size(); i += 3)
        {
            mixed[i] = expected_upper[i];
        }
        std::vector<uint8_t> decoded(len + 1, Canary);
        DecodingFunc(decoded.data(), mixed.data(), RawLength{len});
        REQUIRE(std::equal(raw.begin(), raw.begin() + static_cast<std::ptrdiff_t>(len), decoded.begin()));
        REQUIRE(decoded[len] == Canary);
    }
}

TEST_CASE("encodeHex")
{
    testHexEncoding<encodeHex<lower_t>, encodeHex<upper_t>>();
}
TEST_CASE("decodeHex_valid")
{
    testHexDecoding<decodeHex>();
}
TEST_CASE("decodeHexLUT_valid")
{
    testHexDecoding<decodeHexLUT>();
}
TEST_CASE("decodeHexLUT4_valid")
{
    testHexDecoding<decodeHexLUT4>();
}
TEST_CASE("hex_length_sweep")
{
    testHexLengthSweep<encodeHex<lower_t>, encodeHex<upper_t>, decodeHex>();
    testHexLengthSweep<encodeHex<lower_t>, encodeHex<upper_t>, decodeHexLUT>();
    testHexLengthSweep<encodeHex<lower_t>, encodeHex<upper_t>, decodeHexLUT4>();
}

#if defined(FAST_HEX_X86_64)
TEST_CASE("encodeHexAvx2" * doctest::skip(!cpuHasAvx2()))
{
    testHexEncoding<encodeHexAvx2<lower_t>, encodeHexAvx2<upper_t>>();
}
TEST_CASE("decodeHexAvx2_valid" * doctest::skip(!cpuHasAvx2()))
{
    testHexDecoding<decodeHexAvx2>();
}
TEST_CASE("hexAvx2_length_sweep" * doctest::skip(!cpuHasAvx2()))
{
    testHexLengthSweep<encodeHexAvx2<lower_t>, encodeHexAvx2<upper_t>, decodeHexAvx2>();
}

TEST_CASE("encodeHexAvx512" * doctest::skip(!cpuHasAvx512()))
{
    testHexEncoding<encodeHexAvx512<lower_t>, encodeHexAvx512<upper_t>>();
}
TEST_CASE("decodeHexAvx512_valid" * doctest::skip(!cpuHasAvx512()))
{
    testHexDecoding<decodeHexAvx512>();
}
TEST_CASE("hexAvx512_length_sweep" * doctest::skip(!cpuHasAvx512()))
{
    testHexLengthSweep<encodeHexAvx512<lower_t>, encodeHexAvx512<upper_t>, decodeHexAvx512>();
}

TEST_CASE("encodeHexAvx512Vbmi" * doctest::skip(!cpuHasAvx512Vbmi()))
{
    testHexEncoding<encodeHexAvx512Vbmi<lower_t>, encodeHexAvx512Vbmi<upper_t>>();
}
TEST_CASE("decodeHexAvx512Vbmi_valid" * doctest::skip(!cpuHasAvx512Vbmi()))
{
    testHexDecoding<decodeHexAvx512Vbmi>();
}
TEST_CASE("hexAvx512Vbmi_length_sweep" * doctest::skip(!cpuHasAvx512Vbmi()))
{
    testHexLengthSweep<encodeHexAvx512Vbmi<lower_t>, encodeHexAvx512Vbmi<upper_t>, decodeHexAvx512Vbmi>();
}
#endif

#if defined(FAST_HEX_NEON)
TEST_CASE("encodeHexNeon")
{
    testHexEncoding<encodeHexNeon<lower_t>, encodeHexNeon<upper_t>>();
}
TEST_CASE("hexNeon_length_sweep")
{
    testHexLengthSweep<encodeHexNeon<lower_t>, encodeHexNeon<upper_t>, decodeHex>();
}
#endif
