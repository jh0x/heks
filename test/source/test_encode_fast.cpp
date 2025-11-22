#include "fast_hex/fast_hex.hpp"

#include <string>
#include <string_view>
#include <vector>

#include <doctest/doctest.h>

using namespace std::literals::string_view_literals;
using namespace heks;

struct TestCase8
{
    std::string_view input;
    std::string_view expected_lower;
    std::string_view expected_upper;
};

// clang-format off
std::vector<TestCase8> test_cases_8 = {
    {"\x00\x11\x22\x33\x44\x55\x66\x77"sv, "0011223344556677"sv, "0011223344556677"sv},
    {"\x00\xAA\xBB\xCC\xDD\xEE\xFF\x99"sv, "00aabbccddeeff99"sv, "00AABBCCDDEEFF99"sv},
    {"\x00\x00\x00\x00\x00\x00\x00\x00"sv, "0000000000000000"sv, "0000000000000000"sv},
    {"\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF"sv, "ffffffffffffffff"sv, "FFFFFFFFFFFFFFFF"sv},
    {"\x12\x34\x56\x78\x9A\xBC\xDE\xF0"sv, "123456789abcdef0"sv, "123456789ABCDEF0"sv},
};
// clang-format on

struct TestCase16
{
    std::string_view input;
    std::string_view expected_lower;
    std::string_view expected_upper;
};

// clang-format off
std::vector<TestCase16> test_cases_16 = {
    {"\x00\x11\x22\x33\x44\x55\x66\x77\x88\x99\xAA\xBB\xCC\xDD\xEE\xFF"sv,
     "00112233445566778899aabbccddeeff"sv,
     "00112233445566778899AABBCCDDEEFF"sv},
    {"\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"sv,
     "00000000000000000000000000000000"sv,
     "00000000000000000000000000000000"sv},
    {"\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF"sv,
     "ffffffffffffffffffffffffffffffff"sv,
     "FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF"sv},
    {"\x55\x0e\x84\x00\xe2\x9b\x41\xd4\xa7\x16\x44\x66\x55\x44\x00\x00"sv,
     "550e8400e29b41d4a716446655440000"sv,
     "550E8400E29B41D4A716446655440000"sv},
    {"\x12\x34\x56\x78\x9A\xBC\xDE\xF0\x11\x22\x33\x44\x55\x66\x77\x88"sv,
     "123456789abcdef01122334455667788"sv,
     "123456789ABCDEF01122334455667788"sv},
};
// clang-format on

template <typename LowerFunc, typename UpperFunc>
void run_tests_8(const std::vector<TestCase8> & test_cases, LowerFunc lower_func, UpperFunc upper_func)
{
    for (const auto & tc : test_cases)
    {
        uint8_t dest_lower[16] = {};
        uint8_t dest_upper[16] = {};

        lower_func(dest_lower, reinterpret_cast<const uint8_t *>(tc.input.data()));
        upper_func(dest_upper, reinterpret_cast<const uint8_t *>(tc.input.data()));

        std::string_view result_lower(reinterpret_cast<char *>(dest_lower), 16);
        std::string_view result_upper(reinterpret_cast<char *>(dest_upper), 16);

        REQUIRE(result_lower == tc.expected_lower);
        REQUIRE(result_upper == tc.expected_upper);
    }
}

template <typename LowerFunc, typename UpperFunc>
void run_tests_16(const std::vector<TestCase16> & test_cases, LowerFunc lower_func, UpperFunc upper_func)
{
    for (const auto & tc : test_cases)
    {
        uint8_t dest_lower[32] = {};
        uint8_t dest_upper[32] = {};

        lower_func(dest_lower, reinterpret_cast<const uint8_t *>(tc.input.data()));
        upper_func(dest_upper, reinterpret_cast<const uint8_t *>(tc.input.data()));

        std::string_view result_lower(reinterpret_cast<char *>(dest_lower), 32);
        std::string_view result_upper(reinterpret_cast<char *>(dest_upper), 32);

        REQUIRE(result_lower == tc.expected_lower);
        REQUIRE(result_upper == tc.expected_upper);
    }
}

TEST_SUITE("encodeHexFast")
{
#if defined(__AVX2__)
    TEST_CASE("encodeHex8 AVX2 Fast")
    {
        run_tests_8(test_cases_8, encodeHex8LowerFast, encodeHex8UpperFast);
    }

    TEST_CASE("encodeHex16 AVX2 Fast")
    {
        run_tests_16(test_cases_16, encodeHex16LowerFast, encodeHex16UpperFast);
    }
#endif
}
