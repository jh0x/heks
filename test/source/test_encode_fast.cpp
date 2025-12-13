#include "fast_hex/fast_hex.hpp"

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
    std::string_view input;
    std::string_view expected_lower;
    std::string_view expected_upper;
};

// clang-format off
alignas(8) constexpr const char test_case_8_1[] = "\x00\x11\x22\x33\x44\x55\x66\x77";
alignas(8) constexpr const char test_case_8_2[] = "\x00\xAA\xBB\xCC\xDD\xEE\xFF\x99";
alignas(8) constexpr const char test_case_8_3[] = "\x00\x00\x00\x00\x00\x00\x00\x00";
alignas(8) constexpr const char test_case_8_4[] = "\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF";
alignas(8) constexpr const char test_case_8_5[] = "\x12\x34\x56\x78\x9A\xBC\xDE\xF0";

std::vector<TestCase8> test_cases_8 = {
    {std::string_view(test_case_8_1, 8), "0011223344556677"sv, "0011223344556677"sv},
    {std::string_view(test_case_8_2, 8), "00aabbccddeeff99"sv, "00AABBCCDDEEFF99"sv},
    {std::string_view(test_case_8_3, 8), "0000000000000000"sv, "0000000000000000"sv},
    {std::string_view(test_case_8_4, 8), "ffffffffffffffff"sv, "FFFFFFFFFFFFFFFF"sv},
    {std::string_view(test_case_8_5, 8), "123456789abcdef0"sv, "123456789ABCDEF0"sv},
};
// clang-format on

struct TestCase16
{
    std::string_view input;
    std::string_view expected_lower;
    std::string_view expected_upper;
};

// clang-format off
alignas(8) constexpr const char test_case_16_1[] = "\x00\x11\x22\x33\x44\x55\x66\x77\x88\x99\xAA\xBB\xCC\xDD\xEE\xFF";
alignas(8) constexpr const char test_case_16_2[] = "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00";
alignas(8) constexpr const char test_case_16_3[] = "\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF";
alignas(8) constexpr const char test_case_16_4[] = "\x55\x0e\x84\x00\xe2\x9b\x41\xd4\xa7\x16\x44\x66\x55\x44\x00\x00";
alignas(8) constexpr const char test_case_16_5[] = "\x12\x34\x56\x78\x9A\xBC\xDE\xF0\x11\x22\x33\x44\x55\x66\x77\x88";

std::vector<TestCase16> test_cases_16 = {
    {std::string_view(test_case_16_1, 16), "00112233445566778899aabbccddeeff"sv, "00112233445566778899AABBCCDDEEFF"sv},
    {std::string_view(test_case_16_2, 16), "00000000000000000000000000000000"sv, "00000000000000000000000000000000"sv},
    {std::string_view(test_case_16_3, 16), "ffffffffffffffffffffffffffffffff"sv, "FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF"sv},
    {std::string_view(test_case_16_4, 16), "550e8400e29b41d4a716446655440000"sv, "550E8400E29B41D4A716446655440000"sv},
    {std::string_view(test_case_16_5, 16), "123456789abcdef01122334455667788"sv, "123456789ABCDEF01122334455667788"sv},
};
// clang-format on

template <size_t InLength, typename TestCase, typename LowerFunc, typename UpperFunc>
void run_tests_aligned(const std::vector<TestCase> & test_cases, LowerFunc lower_func, UpperFunc upper_func)
{
    constexpr auto OutLength = InLength * 2;
    for (const auto & tc : test_cases)
    {
        uint8_t dest_lower[OutLength] = {};
        uint8_t dest_upper[OutLength] = {};

        lower_func(dest_lower, reinterpret_cast<const uint8_t *>(tc.input.data()));
        upper_func(dest_upper, reinterpret_cast<const uint8_t *>(tc.input.data()));

        std::string_view result_lower(reinterpret_cast<char *>(dest_lower), OutLength);
        std::string_view result_upper(reinterpret_cast<char *>(dest_upper), OutLength);

        REQUIRE(result_lower == tc.expected_lower);
        REQUIRE(result_upper == tc.expected_upper);
    }
}

template <size_t Offset, size_t Size>
struct Unaligned
{
    uint8_t pad[Offset];
    uint8_t data[Size];
};

template <size_t InLength, size_t Offset, typename TestCase, typename LowerFunc, typename UpperFunc>
void run_tests_unaligned(const std::vector<TestCase> & test_cases, LowerFunc lower_func, UpperFunc upper_func)
{
    constexpr auto OutLength = InLength * 2;
    for (const auto & tc : test_cases)
    {
        Unaligned<Offset, OutLength> dest_lower{};
        Unaligned<Offset, OutLength> dest_upper{};

        Unaligned<Offset, InLength> src{};
        std::memcpy(src.data, tc.input.data(), InLength);

        lower_func(dest_lower.data, reinterpret_cast<const uint8_t *>(src.data));
        upper_func(dest_upper.data, reinterpret_cast<const uint8_t *>(src.data));

        std::string_view result_lower(reinterpret_cast<char *>(dest_lower.data), OutLength);
        std::string_view result_upper(reinterpret_cast<char *>(dest_upper.data), OutLength);

        REQUIRE(result_lower == tc.expected_lower);
        REQUIRE(result_upper == tc.expected_upper);
    }
}


TEST_SUITE("encodeHexFast")
{
#if defined(FAST_HEX_AVX)
    TEST_CASE("encodeHex8 AVX2 Fast")
    {
        run_tests_aligned<8>(test_cases_8, encodeHex8LowerFast, encodeHex8UpperFast);
        run_tests_unaligned<8, 1>(test_cases_8, encodeHex8LowerFast, encodeHex8UpperFast);
        run_tests_unaligned<8, 2>(test_cases_8, encodeHex8LowerFast, encodeHex8UpperFast);
        run_tests_unaligned<8, 3>(test_cases_8, encodeHex8LowerFast, encodeHex8UpperFast);
        run_tests_unaligned<8, 4>(test_cases_8, encodeHex8LowerFast, encodeHex8UpperFast);
        run_tests_unaligned<8, 5>(test_cases_8, encodeHex8LowerFast, encodeHex8UpperFast);
        run_tests_unaligned<8, 6>(test_cases_8, encodeHex8LowerFast, encodeHex8UpperFast);
        run_tests_unaligned<8, 7>(test_cases_8, encodeHex8LowerFast, encodeHex8UpperFast);
    }
#endif
#if defined(FAST_HEX_AVX2)
    TEST_CASE("encodeHex16 AVX2 Fast")
    {
        run_tests_aligned<16>(test_cases_16, encodeHex16LowerFast, encodeHex16UpperFast);
        run_tests_unaligned<16, 1>(test_cases_16, encodeHex16LowerFast, encodeHex16UpperFast);
        run_tests_unaligned<16, 2>(test_cases_16, encodeHex16LowerFast, encodeHex16UpperFast);
        run_tests_unaligned<16, 3>(test_cases_16, encodeHex16LowerFast, encodeHex16UpperFast);
        run_tests_unaligned<16, 4>(test_cases_16, encodeHex16LowerFast, encodeHex16UpperFast);
        run_tests_unaligned<16, 5>(test_cases_16, encodeHex16LowerFast, encodeHex16UpperFast);
        run_tests_unaligned<16, 6>(test_cases_16, encodeHex16LowerFast, encodeHex16UpperFast);
        run_tests_unaligned<16, 7>(test_cases_16, encodeHex16LowerFast, encodeHex16UpperFast);
        run_tests_unaligned<16, 8>(test_cases_16, encodeHex16LowerFast, encodeHex16UpperFast);
        run_tests_unaligned<16, 9>(test_cases_16, encodeHex16LowerFast, encodeHex16UpperFast);
        run_tests_unaligned<16, 10>(test_cases_16, encodeHex16LowerFast, encodeHex16UpperFast);
        run_tests_unaligned<16, 11>(test_cases_16, encodeHex16LowerFast, encodeHex16UpperFast);
        run_tests_unaligned<16, 12>(test_cases_16, encodeHex16LowerFast, encodeHex16UpperFast);
        run_tests_unaligned<16, 13>(test_cases_16, encodeHex16LowerFast, encodeHex16UpperFast);
        run_tests_unaligned<16, 14>(test_cases_16, encodeHex16LowerFast, encodeHex16UpperFast);
        run_tests_unaligned<16, 15>(test_cases_16, encodeHex16LowerFast, encodeHex16UpperFast);
    }
#endif
#if defined(FAST_HEX_NEON)
    TEST_CASE("encodeHex8 NEON Fast")
    {
        run_tests_aligned<8>(test_cases_8, encodeHex8LowerNeon, encodeHex8UpperNeon);
        run_tests_unaligned<8, 1>(test_cases_8, encodeHex8LowerNeon, encodeHex8UpperNeon);
        run_tests_unaligned<8, 2>(test_cases_8, encodeHex8LowerNeon, encodeHex8UpperNeon);
        run_tests_unaligned<8, 3>(test_cases_8, encodeHex8LowerNeon, encodeHex8UpperNeon);
        run_tests_unaligned<8, 4>(test_cases_8, encodeHex8LowerNeon, encodeHex8UpperNeon);
        run_tests_unaligned<8, 5>(test_cases_8, encodeHex8LowerNeon, encodeHex8UpperNeon);
        run_tests_unaligned<8, 6>(test_cases_8, encodeHex8LowerNeon, encodeHex8UpperNeon);
        run_tests_unaligned<8, 7>(test_cases_8, encodeHex8LowerNeon, encodeHex8UpperNeon);
    }
#endif
}
