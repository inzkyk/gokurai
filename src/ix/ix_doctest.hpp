#pragma once

#include "ix.hpp"

#define DOCTEST_CONFIG_NO_SHORT_MACRO_NAMES
#define DOCTEST_CONFIG_SUPER_FAST_ASSERTS
#define DOCTEST_CONFIG_NO_EXCEPTIONS
#include <doctest.hpp>

#define ix_TEST_CASE(name) DOCTEST_TEST_CASE(name)

#if !ix_MEASURE_COVERAGE && ix_DO_TEST
#define ix_EXPECT(...) DOCTEST_CHECK((__VA_ARGS__))
#define ix_EXPECT_EQF(lhs, rhs) DOCTEST_CHECK_EQ((lhs), doctest::ApproxF(rhs))
#define ix_EXPECT_EQSTR(lhs, rhs) DOCTEST_CHECK_EQ(doctest::String(lhs), doctest::String(rhs))
#else
#define ix_EXPECT(...) ix_UNUSED((__VA_ARGS__))
#define ix_EXPECT_EQF(lhs, rhs) ix_UNUSED((lhs) - (rhs))
#define ix_EXPECT_EQSTR(lhs, rhs) ix_UNUSED((lhs) - (rhs))
#endif

int ix_do_doctest(int argc, const char *const *argv);
int ix_do_doctest_benchmark(int argc, const char **argv);

// Based on doctest.h (https://github.com/doctest/doctest, MIT License)
namespace doctest
{

struct ApproxF
{
    explicit ApproxF(float value);
    ApproxF operator()(float value) const;
    ApproxF &epsilon(float newEpsilon);
    ApproxF &scale(float newScale);

    // clang-format off
    friend bool operator==(float lhs, const ApproxF & rhs);
    friend bool operator==(const ApproxF & lhs, float rhs);
    friend bool operator!=(float lhs, const ApproxF & rhs);
    friend bool operator!=(const ApproxF & lhs, float rhs);
    friend bool operator<=(float lhs, const ApproxF & rhs);
    friend bool operator<=(const ApproxF & lhs, float rhs);
    friend bool operator>=(float lhs, const ApproxF & rhs);
    friend bool operator>=(const ApproxF & lhs, float rhs);
    friend bool operator< (float lhs, const ApproxF & rhs);
    friend bool operator< (const ApproxF & lhs, float rhs);
    friend bool operator> (float lhs, const ApproxF & rhs);
    friend bool operator> (const ApproxF & lhs, float rhs);
    // clang-format on

    float m_epsilon;
    float m_scale;
    float m_value;
};

String toString(const ApproxF &in);

} // namespace doctest
