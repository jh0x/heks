#include "fast_hex/fast_hex_inline.hpp"

#include <cstdint>
#include <cstring>
#include <string>
#include <string_view>
#include <vector>

#include <doctest/doctest.h>

using namespace std::literals::string_view_literals;
#if FAST_HEX_USE_NAMESPACE
using namespace heks;
#endif

struct TestCase8
{
    uint64_t input;
    std::string_view expected_lower;
    std::string_view expected_upper;
};

static constexpr TestCase8 test_cases8[] = {{0x0011223344556677ULL, "0011223344556677"sv, "0011223344556677"sv},
                                            {0x00AABBCCDDEEFF99ULL, "00aabbccddeeff99"sv, "00AABBCCDDEEFF99"sv},
                                            {0x0000000000000000ULL, "0000000000000000"sv, "0000000000000000"sv},
                                            {0xFFFFFFFFFFFFFFFFULL, "ffffffffffffffff"sv, "FFFFFFFFFFFFFFFF"sv},
                                            {0x123456789ABCDEF0ULL, "123456789abcdef0"sv, "123456789ABCDEF0"sv},
                                            {0x0000000FF1CEFACEULL, "0000000ff1ceface"sv, "0000000FF1CEFACE"sv},
                                            {0x00BAB10CCAFEBABEULL, "00bab10ccafebabe"sv, "00BAB10CCAFEBABE"sv},
                                            {0x1BADB002DEADBEEFULL, "1badb002deadbeef"sv, "1BADB002DEADBEEF"sv},
                                            {0x4B1D0000FEEDFACEULL, "4b1d0000feedface"sv, "4B1D0000FEEDFACE"sv},
                                            {0x50FFC001BAADF00DULL, "50ffc001baadf00d"sv, "50FFC001BAADF00D"sv},
                                            {0xABADBABECAFEBABEULL, "abadbabecafebabe"sv, "ABADBABECAFEBABE"sv},
                                            {0xACE0FBA5FEEDC0DEULL, "ace0fba5feedc0de"sv, "ACE0FBA5FEEDC0DE"sv},
                                            {0xB105F00DDEADBEEFULL, "b105f00ddeadbeef"sv, "B105F00DDEADBEEF"sv},
                                            {0xB16B00B5BAADF00DULL, "b16b00b5baadf00d"sv, "B16B00B5BAADF00D"sv},
                                            {0x0B00B13500BAD00BULL, "0b00b13500bad00b"sv, "0B00B13500BAD00B"sv},
                                            {0xBAAAAAADDEADFEEDULL, "baaaaaaddeadfeed"sv, "BAAAAAADDEADFEED"sv},
                                            {0xBAADF00DFEEDBABEULL, "baadf00dfeedbabe"sv, "BAADF00DFEEDBABE"sv},
                                            {0xBAD22222CAFED00DULL, "bad22222cafed00d"sv, "BAD22222CAFED00D"sv},
                                            {0xBADDCAFEDECAFBADULL, "baddcafedecafbad"sv, "BADDCAFEDECAFBAD"sv},
                                            {0xBEEFBABEFACEFEEDULL, "beefbabefacefeed"sv, "BEEFBABEFACEFEED"sv},
                                            {0xCAFEBABE0D15EA5EULL, "cafebabe0d15ea5e"sv, "CAFEBABE0D15EA5E"sv},
                                            {0xCAFED00DDABBAD00ULL, "cafed00ddabbad00"sv, "CAFED00DDABBAD00"sv},
                                            {0x0D15EA5EDEADBAADULL, "0d15ea5edeadbaad"sv, "0D15EA5EDEADBAAD"sv},
                                            {0xDABBAD00DEADBABEULL, "dabbad00deadbabe"sv, "DABBAD00DEADBABE"sv},
                                            {0xDEAD2BADDEADBEAFULL, "dead2baddeadbeaf"sv, "DEAD2BADDEADBEAF"sv},
                                            {0xDEADBABEDEADC0DEULL, "deadbabedeadc0de"sv, "DEADBABEDEADC0DE"sv},
                                            {0xDEADBEAFDEADDEADULL, "deadbeafdeaddead"sv, "DEADBEAFDEADDEAD"sv},
                                            {0xDEADC0DEDEADFA11ULL, "deadc0dedeadfa11"sv, "DEADC0DEDEADFA11"sv},
                                            {0xDEADDEADDEAD10CCULL, "deaddeaddead10cc"sv, "DEADDEADDEAD10CC"sv},
                                            {0xDEADD00DDEADFEEDULL, "deadd00ddeadfeed"sv, "DEADD00DDEADFEED"sv},
                                            {0xDEADFA11DECAFBADULL, "deadfa11decafbad"sv, "DEADFA11DECAFBAD"sv},
                                            {0xDEAD10CCDEFEC8EDULL, "dead10ccdefec8ed"sv, "DEAD10CCDEFEC8ED"sv},
                                            {0xDEADFEEDD0D0CACAULL, "deadfeedd0d0caca"sv, "DEADFEEDD0D0CACA"sv},
                                            {0xDECAFBADE011CFD0ULL, "decafbade011cfd0"sv, "DECAFBADE011CFD0"sv},
                                            {0xDEFEC8EDF0CACC1AULL, "defec8edf0cacc1a"sv, "DEFEC8EDF0CACC1A"sv},
                                            {0xD0D0CACAFACEFEEDULL, "d0d0cacafacefeed"sv, "D0D0CACAFACEFEED"sv},
                                            {0xE011CFD0FBADBEEFULL, "e011cfd0fbadbeef"sv, "E011CFD0FBADBEEF"sv},
                                            {0xF0CACC1AFEE1DEADULL, "f0cacc1afee1dead"sv, "F0CACC1AFEE1DEAD"sv},
                                            {0xFACEFEEDFEEDBABEULL, "facefeedfeedbabe"sv, "FACEFEEDFEEDBABE"sv},
                                            {0xFBADBEEFFEEDC0DEULL, "fbadbeeffeedc0de"sv, "FBADBEEFFEEDC0DE"sv},
                                            {0xFEE1DEADFFBADD11ULL, "fee1deadffbadd11"sv, "FEE1DEADFFBADD11"sv},
                                            {0xFEEDBABEF00DBABEULL, "feedbabef00dbabe"sv, "FEEDBABEF00DBABE"sv},
                                            {0xFFBADD1100BAB10CULL, "ffbadd1100bab10c"sv, "FFBADD1100BAB10C"sv},
                                            {0xF00DBABE1BADB002ULL, "f00dbabe1badb002"sv, "F00DBABE1BADB002"sv}};

#if defined(FAST_HEX_HAS_INT128)

struct TestCase16
{
    __uint128_t input;
    std::string_view expected_lower;
    std::string_view expected_upper;
};

// clang-format off
static constexpr TestCase16 test_cases16[] = {
    {(__uint128_t{0x0011223344556677ULL} << 64) | 0x8899AABBCCDDEEFFULL, "00112233445566778899aabbccddeeff"sv, "00112233445566778899AABBCCDDEEFF"sv},
    {(__uint128_t{0xFFFFFFFFFFFFFFFFULL} << 64) | 0xFFFFFFFFFFFFFFFFULL, "ffffffffffffffffffffffffffffffff"sv, "FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF"sv},
    {(__uint128_t{0x123456789ABCDEF0ULL} << 64) | 0x0FEDCBA987654321ULL, "123456789abcdef00fedcba987654321"sv, "123456789ABCDEF00FEDCBA987654321"sv},
    {(__uint128_t{0xDEADBEEFDEADC0DEULL} << 64) | 0xFEEDFACECAFEBABEULL, "deadbeefdeadc0defeedfacecafebabe"sv, "DEADBEEFDEADC0DEFEEDFACECAFEBABE"sv},
    {(__uint128_t{0x0B00B13500BAD00BULL} << 64) | 0xBAADF00DFEEDBABEULL, "0b00b13500bad00bbaadf00dfeedbabe"sv, "0B00B13500BAD00BBAADF00DFEEDBABE"sv},
    {(__uint128_t{0x0102030405060708ULL} << 64) | 0x090A0B0C0D0E0F10ULL, "0102030405060708090a0b0c0d0e0f10"sv, "0102030405060708090A0B0C0D0E0F10"sv},
    {(__uint128_t{0xFFFFFFFF00000000ULL} << 64) | 0x00000000FFFFFFFFULL, "ffffffff0000000000000000ffffffff"sv, "FFFFFFFF0000000000000000FFFFFFFF"sv},
    {(__uint128_t{0x0000000000000001ULL} << 64) | 0x1000000000000000ULL, "00000000000000011000000000000000"sv, "00000000000000011000000000000000"sv},
};
// clang-format on

#endif // defined(FAST_HEX_HAS_INT128)

struct TestCase2x8
{
    uint64_t input1;
    uint64_t input2;
    std::string_view expected_lower;
    std::string_view expected_upper;
};

#if defined(FAST_HEX_AVX2)
// clang-format off
static constexpr TestCase2x8 test_cases2x8[] = {
    {0x0011223344556677ULL, 0x8899AABBCCDDEEFFULL, "00112233445566778899aabbccddeeff"sv, "00112233445566778899AABBCCDDEEFF"sv},
    {0xFFFFFFFFFFFFFFFFULL, 0xFFFFFFFFFFFFFFFFULL, "ffffffffffffffffffffffffffffffff"sv, "FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF"sv},
    {0x123456789ABCDEF0ULL, 0x0FEDCBA987654321ULL, "123456789abcdef00fedcba987654321"sv, "123456789ABCDEF00FEDCBA987654321"sv},
    {0xDEADBEEFDEADC0DEULL, 0xFEEDFACECAFEBABEULL, "deadbeefdeadc0defeedfacecafebabe"sv, "DEADBEEFDEADC0DEFEEDFACECAFEBABE"sv},
    {0x0B00B13500BAD00BULL, 0xBAADF00DFEEDBABEULL, "0b00b13500bad00bbaadf00dfeedbabe"sv, "0B00B13500BAD00BBAADF00DFEEDBABE"sv},
    {0x0102030405060708ULL, 0x090A0B0C0D0E0F10ULL, "0102030405060708090a0b0c0d0e0f10"sv, "0102030405060708090A0B0C0D0E0F10"sv},
    {0xFFFFFFFF00000000ULL, 0x00000000FFFFFFFFULL, "ffffffff0000000000000000ffffffff"sv, "FFFFFFFF0000000000000000FFFFFFFF"sv},
    {0x0000000000000001ULL, 0x1000000000000000ULL, "00000000000000011000000000000000"sv, "00000000000000011000000000000000"sv},
};
// clang-format on
#endif

template <typename Fun, typename TC>
void run_encode_integral_test(Fun encode_func, size_t outlength, const TC & test_cases)
{
    for (size_t i = 0; i < std::size(test_cases); i++)
    {
        const auto & tc = test_cases[i];
        std::vector<uint8_t> dest_lower_vec(outlength);
        auto * dest_lower = dest_lower_vec.data();
        std::vector<uint8_t> dest_upper_vec(outlength);
        auto * dest_upper = dest_upper_vec.data();

        encode_func(dest_lower, tc.input, lower);
        encode_func(dest_upper, tc.input, upper);

        std::string_view result_lower(reinterpret_cast<char *>(dest_lower), outlength);
        std::string_view result_upper(reinterpret_cast<char *>(dest_upper), outlength);

        CAPTURE(i);
        CAPTURE(tc.input);
        REQUIRE(result_lower == tc.expected_lower);
        REQUIRE(result_upper == tc.expected_upper);
    }
}

template <typename Fun, typename TC>
void run_decode_integral_test(Fun decode_func, const TC & test_cases)
{
    for (size_t i = 0; i < std::size(test_cases); i++)
    {
        const auto & tc = test_cases[i];
        auto input_data = reinterpret_cast<const uint8_t *>(tc.expected_lower.data());

        using T = decltype(tc.input);
        T output_value = decode_func(input_data);

        CAPTURE(i);
        CAPTURE(tc.expected_lower);
        REQUIRE(output_value == tc.input);
    }
}

TEST_SUITE("encode_integral")
{
    TEST_CASE("encode_integral 8 naive")
    {
        run_encode_integral_test([](auto dst, auto input, auto c) { encode_integral_naive(dst, input, c); }, 16, test_cases8);
    }
#if defined(FAST_HEX_HAS_INT128)
    TEST_CASE("encode_integral 16 naive")
    {
        run_encode_integral_test([](auto dst, auto input, auto c) { encode_integral_naive(dst, input, c); }, 32, test_cases16);
    }
#endif
#if defined(FAST_HEX_AVX) || defined(FAST_HEX_NEON)
    TEST_CASE("encode_integral 8")
    {
        run_encode_integral_test([](auto dst, auto input, auto c) { encode_integral8(dst, input, c); }, 16, test_cases8);
    }
#endif // defined(FAST_HEX_AVX)
#if defined(FAST_HEX_AVX2)
    TEST_CASE("encode_integral 16")
    {
        run_encode_integral_test([](auto dst, auto input, auto c) { encode_integral16(dst, input, c); }, 32, test_cases16);
    }
    TEST_CASE("encode_integral 2x8")
    {
        constexpr auto OutLength = 32;
        for (size_t i = 0; i < std::size(test_cases2x8); i++)
        {
            const auto & tc = test_cases2x8[i];
            uint8_t dest_lower[OutLength] = {};
            uint8_t dest_upper[OutLength] = {};
            encode_integral2x8(dest_lower, &tc.input1, lower);
            encode_integral2x8(dest_upper, &tc.input1, upper);

            std::string_view result_lower(reinterpret_cast<char *>(dest_lower), OutLength);
            std::string_view result_upper(reinterpret_cast<char *>(dest_upper), OutLength);

            CAPTURE(i);
            REQUIRE(result_lower == tc.expected_lower);
            REQUIRE(result_upper == tc.expected_upper);
        }
    }
#endif // defined(FAST_HEX_AVX2)
}

template <typename T>
struct TestCase
{
    T input;
    std::string_view expected_lower;
    std::string_view expected_upper;
};

static constexpr TestCase<uint8_t> test_cases1[] = {
    {0x00, "00"sv, "00"sv},
    {0x7F, "7f"sv, "7F"sv},
    {0x80, "80"sv, "80"sv},
    {0xFF, "ff"sv, "FF"sv},
};

static constexpr TestCase<uint16_t> test_cases2[] = {
    {0x0000, "0000"sv, "0000"sv},
    {0x0001, "0001"sv, "0001"sv},
    {0x0010, "0010"sv, "0010"sv},
    {0x0100, "0100"sv, "0100"sv},
    {0x1000, "1000"sv, "1000"sv},
    {0x1234, "1234"sv, "1234"sv},
    {0x7FFF, "7fff"sv, "7FFF"sv},
    {0x8000, "8000"sv, "8000"sv},
    {0xFFFF, "ffff"sv, "FFFF"sv},
};

static constexpr TestCase<uint32_t> test_cases4[] = {
    {0x00000000, "00000000"sv, "00000000"sv},
    {0x00000001, "00000001"sv, "00000001"sv},
    {0x00000010, "00000010"sv, "00000010"sv},
    {0x00000100, "00000100"sv, "00000100"sv},
    {0x00001000, "00001000"sv, "00001000"sv},
    {0x00010000, "00010000"sv, "00010000"sv},
    {0x00123456, "00123456"sv, "00123456"sv},
    {0x12345678, "12345678"sv, "12345678"sv},
    {0x7FFFFFFF, "7fffffff"sv, "7FFFFFFF"sv},
    {0x80000000, "80000000"sv, "80000000"sv},
    {0xFFFFFFFF, "ffffffff"sv, "FFFFFFFF"sv},
};

TEST_SUITE("decode_integral")
{
    TEST_CASE("decode_integral 1 naive")
    {
        run_decode_integral_test(decode_integral_naive<uint8_t>, test_cases1);
    }
    TEST_CASE("decode_integral 2 naive")
    {
        run_decode_integral_test(decode_integral_naive<uint16_t>, test_cases2);
    }
    TEST_CASE("decode_integral 4 naive")
    {
        run_decode_integral_test(decode_integral_naive<uint32_t>, test_cases4);
    }
    TEST_CASE("decode_integral 8 naive")
    {
        run_decode_integral_test(decode_integral_naive<uint64_t>, test_cases8);
    }
#if defined(FAST_HEX_AVX)
    TEST_CASE("decode_integral 8 naive")
    {
        run_decode_integral_test(decode_integral8, test_cases8);
    }
#endif // defined(FAST_HEX_AVX)
#if defined(FAST_HEX_HAS_INT128)
    TEST_CASE("decode_integral 16 naive")
    {
        run_decode_integral_test(decode_integral_naive<__uint128_t>, test_cases16);
    }
#endif // defined(FAST_HEX_HAS_INT128)
}
