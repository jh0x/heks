#pragma once

// Runs a test at each SVE vector length the CPU supports, so that the code for 128-bit vectors (Graviton 4, Neoverse
// V2 and N2) is tested on CPUs with longer ones too. Linux only, and not when the tests are built for SVE
// (fast_hex_ENABLE_MARCH_NATIVE on an SVE CPU), where code inlined here could keep values or stack slots sized by
// the old length across the change: otherwise, at the current length.

#include "fast_hex/fast_hex_arm_sve.hpp"

#if defined(FAST_HEX_ARM_SVE)

#    include <algorithm>
#    include <vector>

#    include <doctest/doctest.h>

#    if defined(__linux__)
#        include <sys/prctl.h>
#    endif

namespace heks_test
{

template <class F>
void forEachSveVectorLength(F test)
{
#    if defined(__linux__) && defined(PR_SVE_GET_VL) && !defined(__ARM_FEATURE_SVE)
    const int original = prctl(PR_SVE_GET_VL);
    if (original >= 0)
    {
        // Back to the original length however the test ends: doctest's REQUIRE throws
        struct Restore
        {
            int config;
            ~Restore() { static_cast<void>(prctl(PR_SVE_SET_VL, static_cast<unsigned long>(config))); }
        } restore{original};

        std::vector<int> tested;
        for (int bytes = 16; bytes <= 256; bytes += 16)
        {
            // The kernel takes the longest length the CPU supports up to the one asked for
            const int config = prctl(PR_SVE_SET_VL, static_cast<unsigned long>(bytes));
            if (config < 0)
                continue;
            const int vector_bytes = config & PR_SVE_VL_LEN_MASK;
            if (std::find(tested.begin(), tested.end(), vector_bytes) != tested.end())
                continue;
            tested.push_back(vector_bytes);
            CAPTURE(vector_bytes);
            test();
        }
        if (!tested.empty())
            return;
        // Every length was refused, by a seccomp filter say
        MESSAGE("Couldn't set the SVE vector length: testing at the current one only");
    }
#    endif
    test();
}

} // namespace heks_test

#endif // defined(FAST_HEX_ARM_SVE)
