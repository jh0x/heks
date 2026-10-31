// Header-only, so the code under test is compiled here with the sanitizer and coverage instrumentation
#include "fast_hex/fast_hex.hpp"
#include "fast_hex/fast_hex_arm_neon.hpp"
#include "fast_hex/fast_hex_x86_64_icelake.hpp"
#include "fast_hex/fast_hex_x86_64_v3.hpp"
#include "fast_hex/fast_hex_x86_64_v4.hpp"

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <vector>

using namespace heks;


extern "C" int LLVMFuzzerTestOneInput(const uint8_t * Data, size_t Size)
{
#if defined(FAST_HEX_X86_64)
    static const bool has_avx2 = cpuHasAvx2();
    static const bool has_avx512 = cpuHasAvx512();
    static const bool has_avx512_vbmi = cpuHasAvx512Vbmi();
#endif
    const std::size_t encoded_size = 2 * Size;
    const std::size_t decoded_size = Size;

    // Independent of the library, so encoders are checked byte for byte, including the case
    auto reference_encode = [&](const char * digits)
    {
        std::vector<uint8_t> encoded(encoded_size);
        for (std::size_t i = 0; i < Size; ++i)
        {
            encoded[2 * i] = static_cast<uint8_t>(digits[Data[i] >> 4]);
            encoded[2 * i + 1] = static_cast<uint8_t>(digits[Data[i] & 0x0F]);
        }
        return encoded;
    };
    const std::vector<uint8_t> expected_lower = reference_encode("0123456789abcdef");
    const std::vector<uint8_t> expected_upper = reference_encode("0123456789ABCDEF");

    auto decode = [&](const auto & source, auto decode_func)
    {
        std::vector<uint8_t> decoded(decoded_size);
        decode_func(decoded.data(), source.data(), RawLength{decoded_size});
        // Not memcmp: decoded.data() is null when Size is 0
        if (!std::equal(decoded.begin(), decoded.end(), Data))
        {
            std::abort();
        }
    };

    auto test_encode = [&](auto encode_func, auto hex_case, const std::vector<uint8_t> & expected)
    {
        std::vector<uint8_t> encoded(encoded_size);
        encode_func(encoded.data(), Data, RawLength{Size}, hex_case);
        if (encoded != expected)
        {
            std::abort();
        }

        decode(encoded, decodeHex);
        decode(encoded, decodeHexLUT);
        decode(encoded, decodeHexLUT4);
#if defined(FAST_HEX_X86_64)
        if (has_avx2)
            decode(encoded, decodeHexAvx2);
        if (has_avx512)
            decode(encoded, decodeHexAvx512);
        if (has_avx512_vbmi)
            decode(encoded, decodeHexAvx512Vbmi);
#endif
    };

    test_encode(encodeHex<lower_t>, lower, expected_lower);
    test_encode(encodeHex<upper_t>, upper, expected_upper);
#if defined(FAST_HEX_X86_64)
    if (has_avx2)
    {
        test_encode(encodeHexAvx2<lower_t>, lower, expected_lower);
        test_encode(encodeHexAvx2<upper_t>, upper, expected_upper);
    }
    if (has_avx512)
    {
        test_encode(encodeHexAvx512<lower_t>, lower, expected_lower);
        test_encode(encodeHexAvx512<upper_t>, upper, expected_upper);
    }
    if (has_avx512_vbmi)
    {
        test_encode(encodeHexAvx512Vbmi<lower_t>, lower, expected_lower);
        test_encode(encodeHexAvx512Vbmi<upper_t>, upper, expected_upper);
    }
#endif
#if defined(FAST_HEX_NEON)
    test_encode(encodeHexNeon<lower_t>, lower, expected_lower);
    test_encode(encodeHexNeon<upper_t>, upper, expected_upper);
#endif

    return 0;
}
