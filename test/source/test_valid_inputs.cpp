#include "fast-hex/fast-hex.hpp"

#include <string>
#include <string_view>

#include <doctest/doctest.h>

// Test data for encodeHex tests
constexpr std::string_view raw_data[] = {
    // Empty string
    std::string_view{"", 0},

    // 1
    std::string_view{"\x00", 1},
    std::string_view{"\x01", 1},
    std::string_view{"\x41", 1},
    std::string_view{"\xFF", 1},

    // 2
    std::string_view{"\x00\x00", 2},
    std::string_view{"\x01\x02", 2},
    std::string_view{"\x41\x42", 2},
    std::string_view{"\xFF\xFE", 2},

    // 3
    std::string_view{"\x00\x01\x02", 3},
    std::string_view{"\x41\x42\x43", 3},
    std::string_view{"\xFF\xFE\xFD", 3},

    // 4
    std::string_view{"\x00\x01\x02\x03", 4},
    std::string_view{"\x10\x20\x30\x40", 4},
    std::string_view{"\xAA\xBB\xCC\xDD", 4},

    // 5
    std::string_view{"\x00\x01\x02\x03\x04", 5},
    std::string_view{"Hello", 5},

    // 6
    std::string_view{"Hello!", 6},
    std::string_view{"\x00\x11\x22\x33\x44\x55", 6},

    // 7
    std::string_view{"Test123", 7},
    std::string_view{"\x01\x02\x03\x04\x05\x06\x07", 7},

    // 8
    std::string_view{"TestData", 8},
    std::string_view{"\xDE\xAD\xBE\xEF\xFE\xED\xFA\xCE", 8},

    // 9
    std::string_view{"Test12345", 9},
    std::string_view{"\x00\x11\x22\x33\x44\x55\x66\x77\x88", 9},

    // 10
    std::string_view{"Test123456", 10},
    std::string_view{"\x0A\x0B\x0C\x0D\x0E\x0F\x10\x11\x12\x13", 10},

    // 11
    std::string_view{"Test1234567", 11},
    std::string_view{"\x01\x02\x03\x04\x05\x06\x07\x08\x09\x0A\x0B", 11},

    // 12
    std::string_view{"Test12345678", 12},
    std::string_view{"\xF0\xE0\xD0\xC0\xB0\xA0\x90\x80\x70\x60\x50\x40", 12},

    // 13
    std::string_view{"Test123456789", 13},
    std::string_view{"\x01\x02\x03\x04\x05\x06\x07\x08\x09\x0A\x0B\x0C\x0D", 13},

    // 14
    std::string_view{"Test123456789A", 14},
    std::string_view{"\x10\x11\x12\x13\x14\x15\x16\x17\x18\x19\x1A\x1B\x1C\x1D", 14},

    // 15
    std::string_view{"Test123456789AB", 15},
    std::string_view{"\x00\x01\x02\x03\x04\x05\x06\x07\x08\x09\x0A\x0B\x0C\x0D\x0E", 15},

    // 16
    std::string_view{"Test123456789ABC", 16},
    std::string_view{"\x0F\x0E\x0D\x0C\x0B\x0A\x09\x08\x07\x06\x05\x04\x03\x02\x01\x00", 16},

    // 17
    std::string_view{"Test123456789ABCD", 17},
    std::string_view{"\x01\x02\x03\x04\x05\x06\x07\x08\x09\x0A\x0B\x0C\x0D\x0E\x0F\x10\x11", 17},

    // 18
    std::string_view{"Test123456789ABCDE", 18},
    std::string_view{"\x12\x13\x14\x15\x16\x17\x18\x19\x1A\x1B\x1C\x1D\x1E\x1F\x20\x21\x22\x23", 18},

    // 19
    std::string_view{"Test123456789ABCDEF", 19},
    std::string_view{"\x01\x02\x03\x04\x05\x06\x07\x08\x09\x0A\x0B\x0C\x0D\x0E\x0F\x10\x11\x12\x13", 19},

    // 20
    std::string_view{"Test123456789ABCDEF0", 20},
    std::string_view{"\x14\x15\x16\x17\x18\x19\x1A\x1B\x1C\x1D\x1E\x1F\x20\x21\x22\x23\x24\x25\x26\x27", 20}};

// Expected hex encodings for the raw data
using namespace std::string_view_literals;
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
    "1415161718191a1b1c1d1e1f2021222324252627"sv};

template <auto EncodingFunc>
void testHexEncoding()
{
    for (size_t i = 0; i < sizeof(raw_data) / sizeof(raw_data[0]); ++i)
    {
        const auto & raw_str = raw_data[i];
        const auto & expected = encoded_data[i];

        std::string output;
        output.resize(raw_str.size() * 2);

        EncodingFunc(reinterpret_cast<uint8_t *>(output.data()), reinterpret_cast<const uint8_t *>(raw_str.data()), raw_str.size());

        CAPTURE(i);
        REQUIRE(output == expected);
    }
}

template <auto DecodingFunc>
void testHexDecoding()
{
    for (size_t i = 0; i < sizeof(raw_data) / sizeof(raw_data[0]); ++i)
    {
        const auto & hex_str = encoded_data[i];
        const auto & expected = raw_data[i];

        std::string output;
        output.resize(hex_str.size() / 2);

        DecodingFunc(reinterpret_cast<uint8_t *>(output.data()), reinterpret_cast<const uint8_t *>(hex_str.data()), output.size());

        CAPTURE(i);
        CAPTURE(hex_str);
        REQUIRE(output == expected);
    }
}

TEST_CASE("encodeHex")
{
    testHexEncoding<encodeHex>();
}

#if defined(__AVX2__)
TEST_CASE("encodeHexVec")
{
    testHexEncoding<encodeHexVec>();
}
TEST_CASE("decodeHexVec_valid")
{
    testHexDecoding<decodeHexVec>();
}
#endif

TEST_CASE("decodeHexLUT_valid")
{
    testHexDecoding<decodeHexLUT>();
}

TEST_CASE("decodeHexLUT4_valid")
{
    testHexDecoding<decodeHexLUT4>();
}
