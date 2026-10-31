// Header-only, so the code under test is compiled here with the sanitizer and coverage instrumentation
#include "fast_hex/fast_hex.hpp"
#include "fast_hex/fast_hex_arm_neon.hpp"
#include "fast_hex/fast_hex_x86_64_icelake.hpp"
#include "fast_hex/fast_hex_x86_64_v3.hpp"

#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <vector>

using namespace heks;


extern "C" int LLVMFuzzerTestOneInput(const uint8_t * Data, size_t Size)
{
    const std::size_t even_size = (Size % 2 == 0) ? Size : Size - 1;
    const std::size_t decoded_size = even_size / 2;

    // The same input mapped to valid hex, with the case of each letter chosen by the input
    constexpr char Digits[] = "0123456789abcdefABCDEF";
    std::vector<uint8_t> valid_hex(even_size);
    std::vector<uint8_t> expected(decoded_size);
    for (std::size_t i = 0; i < even_size; ++i)
    {
        const std::size_t digit = static_cast<std::size_t>(Data[i]) % (sizeof(Digits) - 1);
        const auto value = static_cast<uint8_t>(digit < 16 ? digit : digit - 6);
        valid_hex[i] = static_cast<uint8_t>(Digits[digit]);
        expected[i / 2] = static_cast<uint8_t>(i % 2 == 0 ? value << 4 : expected[i / 2] | value);
    }

    auto decode = [&](auto decode_func)
    {
        std::vector<uint8_t> decoded(decoded_size);
        // Arbitrary input: the output is unspecified, but nothing may crash or go out of bounds
        decode_func(decoded.data(), Data, RawLength{decoded_size});
        // Valid input: the output must match. Poison the buffer with the complement of the expected
        // output first, so a byte the decoder fails to write cannot pass with the previous call's value
        for (std::size_t i = 0; i < decoded_size; ++i)
        {
            decoded[i] = static_cast<uint8_t>(~expected[i]);
        }
        decode_func(decoded.data(), valid_hex.data(), RawLength{decoded_size});
        if (decoded != expected)
        {
            std::abort();
        }
    };

    decode(decodeHex);
    decode(decodeHexLUT);
    decode(decodeHexLUT4);
#if defined(FAST_HEX_X86_64)
    static const bool has_avx2 = cpuHasAvx2();
    if (has_avx2)
        decode(decodeHexAvx2);
    static const bool has_avx512_vbmi = cpuHasAvx512Vbmi();
    if (has_avx512_vbmi)
        decode(decodeHexAvx512Vbmi);
#endif
    return 0;
}
