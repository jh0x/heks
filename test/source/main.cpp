#define DOCTEST_CONFIG_IMPLEMENT
#include <doctest/doctest.h>

#include "cpu_feature_tests.hpp"
#include "sve_vector_lengths.hpp"

#include <algorithm>
#include <array>
#include <charconv>
#include <cstddef>
#include <cstdlib>
#include <iostream>
#include <string>
#include <string_view>
#include <system_error>

// The SIMD tests skip themselves on a CPU without the instruction set, which would hide a CI machine that never
// runs them. Set FAST_HEX_TEST_REQUIRE to a comma-separated list of avx2, avx512, avx512vbmi, neon, sve and
// sve-vl to fail instead. sve-vl=16 also says how many SVE vector lengths the tests must run at.
//
// The rule for an instruction set: at least one of its test cases ran, and none was skipped for want of the CPU.
// So a filter on the command line may leave some of them out, but not all.
//
// A listener records which test cases ran and were skipped, so this checks what happened rather than a copy of
// the tests' skip conditions. The decorators of cpu_feature_tests.hpp say which instruction set a test case tests.

namespace
{

struct FeatureTests
{
    std::string_view feature;
    std::size_t seen = 0;
    std::size_t ran = 0;
    std::size_t skipped = 0; // Marked to skip for want of the instruction set: also if --no-skip ran it
};

// SVE test cases that ran without a forEachSveVectorLength call, so at the current vector length only
std::size_t sve_tests_at_one_length = 0;

// Not for --help, --count, the --list options and --no-run, which run no test
bool tests_were_run = false;

std::array<FeatureTests, 5> feature_tests{{{"avx2"}, {"avx512"}, {"avx512vbmi"}, {"neon"}, {"sve"}}};

FeatureTests * featureOf(const doctest::TestCaseData & test)
{
    const std::string_view feature = heks_test::cpuFeatureOf(test);
    for (FeatureTests & tests : feature_tests)
        if (tests.feature == feature)
            return &tests;
    return nullptr;
}

struct FeatureListener : doctest::IReporter
{
    explicit FeatureListener(const doctest::ContextOptions &) { }

    bool in_sve_test = false;
    std::size_t sve_runs_before = 0;

    void test_case_start(const doctest::TestCaseData & test) override
    {
        FeatureTests * tests = featureOf(test);
        if (!tests)
            return;
        ++tests->seen;
        ++tests->ran;
        tests->skipped += test.m_skip; // --no-skip ran it, on a CPU that lacks the instruction set
        in_sve_test = tests->feature == "sve";
        sve_runs_before = heks_test::sveVectorLengthRuns().tests;
    }
    void test_case_end(const doctest::CurrentTestCaseStats & stats) override
    {
        // Not a failed one: it may have stopped partway through its vector lengths, and fails the run anyway
        if (in_sve_test && stats.failure_flags == 0 && heks_test::sveVectorLengthRuns().tests == sve_runs_before)
            ++sve_tests_at_one_length;
        in_sve_test = false;
    }
    // By its skip decorator, or by a filter on the command line
    void test_case_skipped(const doctest::TestCaseData & test) override
    {
        if (FeatureTests * tests = featureOf(test))
        {
            ++tests->seen;
            tests->skipped += test.m_skip;
        }
    }

    void report_query(const doctest::QueryData &) override { }
    void test_run_start() override { tests_were_run = true; }
    void test_run_end(const doctest::TestRunStats &) override { }
    void test_case_reenter(const doctest::TestCaseData &) override { }
    void test_case_exception(const doctest::TestCaseException &) override { }
    void subcase_start(const doctest::SubcaseSignature &) override { }
    void subcase_end() override { }
    void log_assert(const doctest::AssertData &) override { }
    void log_message(const doctest::MessageData &) override { }
};

// For sve-vl and sve-vl=N: every SVE test case must have run at each vector length the tests can set, and at more
// than one, so not only at the length the process started with. With N, at exactly that many: wanted is N, or 0
std::string unmetSveVectorLengths(std::size_t wanted)
{
    const heks_test::SveVectorLengthRuns runs = heks_test::sveVectorLengthRuns();
    if (runs.tests == 0)
        return "no SVE test ran";
    if (sve_tests_at_one_length != 0)
        return std::to_string(sve_tests_at_one_length) + " SVE test case(s) did not run at each vector length";
    if (runs.length_not_set)
        return "an SVE test could not set a vector length that was settable before";
    if (runs.fewest_lengths < 2)
        return "an SVE test ran at " + std::to_string(runs.fewest_lengths) + " settable vector length(s), not several";
    if (wanted != 0 && runs.fewest_lengths != wanted)
        return "the SVE tests ran at " + std::to_string(runs.fewest_lengths) + " vector lengths, not " + std::to_string(wanted);
    return {};
}

// Empty if the requirement was met, or else why not
std::string unmetRequirement(std::string_view feature)
{
    constexpr std::string_view SveVl = "sve-vl";
    constexpr std::string_view SveVlCount = "sve-vl=";
    if (feature == SveVl)
        return unmetSveVectorLengths(0);
    if (feature.starts_with(SveVlCount))
    {
        const std::string_view count = feature.substr(SveVlCount.size());
        std::size_t wanted = 0;
        const auto [end, error] = std::from_chars(count.data(), count.data() + count.size(), wanted);
        if (error != std::errc{} || end != count.data() + count.size() || wanted < 2)
            return "the N of sve-vl=N must be a number above 1";
        return unmetSveVectorLengths(wanted);
    }
    for (const FeatureTests & tests : feature_tests)
    {
        if (tests.feature != feature)
            continue;
        if (tests.seen == 0)
            return "its tests are not compiled in";
        if (tests.skipped != 0)
            return std::to_string(tests.skipped) + " of its " + std::to_string(tests.seen) + " test cases are skipped on this CPU";
        if (tests.ran == 0)
            return "none of its test cases ran: a filter left them all out";
        return {};
    }
    return "unknown name";
}

} // namespace

REGISTER_LISTENER("required_cpu_features", 1, FeatureListener);

int main(int argc, char ** argv)
{
    doctest::Context context(argc, argv);
    const int result = context.run();
    // Not context.shouldExit(): --exit sets that too, and still runs the tests
    if (!tests_were_run)
        return result;

    const char * env = std::getenv("FAST_HEX_TEST_REQUIRE");
    const std::string_view required = env ? env : "";
    constexpr std::string_view Blanks = " \t";
    bool met = true;
    std::size_t begin = 0;
    while (begin < required.size())
    {
        const std::size_t end = std::min(required.find(',', begin), required.size());
        std::string_view feature = required.substr(begin, end - begin);
        begin = end + 1;
        // "avx2, neon" and "avx2,,neon" are fine
        feature.remove_prefix(std::min(feature.find_first_not_of(Blanks), feature.size()));
        feature = feature.substr(0, feature.find_last_not_of(Blanks) + 1);
        if (feature.empty())
            continue;
        const std::string unmet = unmetRequirement(feature);
        if (!unmet.empty())
        {
            std::cerr << "FAST_HEX_TEST_REQUIRE: " << feature << ": " << unmet << '\n';
            met = false;
        }
    }
    return met ? result : EXIT_FAILURE;
}
