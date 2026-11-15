#pragma once

// Decorators that mark a test case as testing one instruction set: TEST_CASE("name" * heks_test::avx2()).
// Each skips the test case on a CPU without the instruction set, and records which one it tests for
// FAST_HEX_TEST_REQUIRE in main.cpp. The test case stays in its test suite.
//
// Don't add doctest::skip to one: it replaces the decorator's skip if it comes after. Nor put one in a TEST_SUITE
// marked doctest::skip: the decorator's skip replaces the suite's. Give a further condition
// with andNeeds: heks_test::avx512().andNeeds(cpuHasAvx2()).

#include "fast_hex/fast_hex_arm_neon.hpp"
#include "fast_hex/fast_hex_arm_sve.hpp"
#include "fast_hex/fast_hex_x86_64_icelake.hpp"
#include "fast_hex/fast_hex_x86_64_v3.hpp"
#include "fast_hex/fast_hex_x86_64_v4.hpp"

#include <doctest/doctest.h>

#include <string>
#include <string_view>
#include <vector>

namespace heks_test
{

// A test case that a decorator marked, known by where it is defined
struct CpuFeatureTestCase
{
    std::string file;
    unsigned line;
    std::string_view feature;
};

inline std::vector<CpuFeatureTestCase> & cpuFeatureTestCases()
{
    static std::vector<CpuFeatureTestCase> test_cases;
    return test_cases;
}

// The instruction set the test case was marked with, or empty
inline std::string_view cpuFeatureOf(const doctest::TestCaseData & test)
{
    for (const CpuFeatureTestCase & test_case : cpuFeatureTestCases())
        if (test_case.line == test.m_line && test_case.file == test.m_file.c_str())
            return test_case.feature;
    return {};
}

struct CpuFeatureTest
{
    const char * feature; // One of the names FAST_HEX_TEST_REQUIRE takes
    bool cpu_has_it;

    // For a test case that needs more than its instruction set
    CpuFeatureTest andNeeds(bool condition) const { return {feature, cpu_has_it && condition}; }

    // Called by doctest, as for its own decorators
    void fill(doctest::detail::TestCase & test_case) const
    {
        cpuFeatureTestCases().push_back({test_case.m_file.c_str(), test_case.m_line, feature});
        test_case.m_skip = !cpu_has_it;
    }
};

#if defined(FAST_HEX_X86_64)
inline CpuFeatureTest avx2()
{
    return {"avx2", heks::cpuHasAvx2()};
}
inline CpuFeatureTest avx512()
{
    return {"avx512", heks::cpuHasAvx512()};
}
inline CpuFeatureTest avx512vbmi()
{
    return {"avx512vbmi", heks::cpuHasAvx512Vbmi()};
}
#endif

#if defined(FAST_HEX_NEON)
// Every CPU the NEON code is compiled for has NEON
inline CpuFeatureTest neon()
{
    return {"neon", true};
}
#endif

#if defined(FAST_HEX_ARM_SVE)
inline CpuFeatureTest sve()
{
    return {"sve", heks::cpuHasSve()};
}
#endif

} // namespace heks_test
