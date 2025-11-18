#include "fast_hex/fast_hex.hpp"

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <vector>


extern "C" int LLVMFuzzerTestOneInput(const uint8_t * Data, size_t Size)
{
    std::vector<uint8_t> encoded;
    std::vector<uint8_t> decoded;

    const std::size_t encodedSize = 2 * Size;
    const std::size_t decodedSize = Size;

    encoded.resize(encodedSize);
    decoded.resize(decodedSize);

    auto encodedPtr = reinterpret_cast<uint8_t *>(encoded.data());
    auto decodedPtr = reinterpret_cast<uint8_t *>(decoded.data());

    encodeHex(encodedPtr, Data, Size);
    decodeHexLUT(decodedPtr, encodedPtr, decodedSize);

    if (std::memcmp(Data, decodedPtr, decodedSize) != 0)
    {
        assert(false && "Decoded data does not match original data");
    }

    return 0;
}
