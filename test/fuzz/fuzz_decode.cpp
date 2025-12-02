#include "fast_hex/fast_hex.hpp"

#include <cstddef>
#include <cstdint>
#include <vector>

#if FAST_HEX_USE_NAMESPACE
using namespace heks;
#endif


extern "C" int LLVMFuzzerTestOneInput(const uint8_t * Data, size_t Size)
{
    const std::size_t even_size = (Size % 2 == 0) ? Size : Size - 1;
    const std::size_t decoded_size = even_size / 2;

    auto decode = [&](auto decode_func)
    {
        std::vector<uint8_t> decoded(decoded_size);
        decode_func(decoded.data(), Data, RawLength{decoded_size});
    };

    decode(decodeHexLUT);
    decode(decodeHexLUT4);
    decode(decodeHexBMI);
#if defined(__AVX2__)
    decode(decodeHexVec);
#endif
    return 0;
}
