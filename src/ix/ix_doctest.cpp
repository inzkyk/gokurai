#include "ix_doctest.hpp"
#include "ix_Logger.hpp"
#include "ix_SystemManager.hpp"
#include "ix_assert.hpp"
#include "ix_limits.hpp"
#include "ix_min_max.hpp"
#include "ix_random.hpp"

#include <math.h>
#include <sokol_time.h>

static_assert(sizeof(int8_t) == 1);
static_assert(sizeof(int16_t) == 2);
static_assert(sizeof(int32_t) == 4);
static_assert(sizeof(int64_t) == 8);
static_assert(sizeof(uint8_t) == 1);
static_assert(sizeof(uint16_t) == 2);
static_assert(sizeof(uint32_t) == 4);
static_assert(sizeof(uint64_t) == 8);

#if ix_PLATFORM(WASM)
static_assert(sizeof(size_t) == 4);
static_assert(sizeof(ptrdiff_t) == 4);
#else
static_assert(sizeof(size_t) == 8);
static_assert(sizeof(ptrdiff_t) == 8);
#endif

int ix_do_doctest(int argc, const char *const *argv)
{
    const auto &sm = ix_SystemManager::get();
    ix_ASSERT_FATAL(sm.is_initialized("stdio"));
    ix_ASSERT_FATAL(sm.is_initialized("sokol_time"));
    ix_ASSERT_FATAL(sm.is_initialized("logger"));

    ix_rand_set_random_seed();

    doctest::Context context(argc, argv);
    context.setOption("no-intro", true);
    context.setOption("no-version", true);
    context.setOption("gnu-file-line", true);

#if ix_PLATFORM(WASM)
    context.setOption("no-colors", true);
#endif

    ix_log_verbose("initialization finished.");
    const int ret = context.run();
    ix_log_verbose("tests finished.");

    return ret;
}

// Based on doctest.h (https://github.com/doctest/doctest, MIT License)
namespace doctest
{

ApproxF::ApproxF(float value)
    : m_epsilon(static_cast<float>(ix_numeric_limits<float>::epsilon()) * 100),
      m_scale(1.0),
      m_value(value)
{
}

ApproxF ApproxF::operator()(float value) const
{
    ApproxF approx(value);
    approx.epsilon(m_epsilon);
    approx.scale(m_scale);
    return approx;
}

ApproxF &ApproxF::epsilon(float newEpsilon)
{
    m_epsilon = newEpsilon;
    return *this;
}

ApproxF &ApproxF::scale(float newScale)
{
    m_scale = newScale;
    return *this;
}

bool operator==(float lhs, const ApproxF &rhs)
{
    // Thanks to Richard Harris for his help refining this formula
    return fabsf(lhs - rhs.m_value) < rhs.m_epsilon * (rhs.m_scale + ix_max(fabsf(lhs), fabsf(rhs.m_value)));
}

// clang-format off
bool operator==(const ApproxF& lhs, float rhs) { return operator==(rhs, lhs); }
bool operator!=(float lhs, const ApproxF& rhs) { return !operator==(lhs, rhs); }
bool operator!=(const ApproxF& lhs, float rhs) { return !operator==(rhs, lhs); }
bool operator<=(float lhs, const ApproxF& rhs) { return lhs < rhs.m_value || lhs == rhs; }
bool operator<=(const ApproxF& lhs, float rhs) { return lhs.m_value < rhs || lhs == rhs; }
bool operator>=(float lhs, const ApproxF& rhs) { return lhs > rhs.m_value || lhs == rhs; }
bool operator>=(const ApproxF& lhs, float rhs) { return lhs.m_value > rhs || lhs == rhs; }
bool operator<(float lhs, const ApproxF& rhs) { return lhs < rhs.m_value && lhs != rhs; }
bool operator<(const ApproxF& lhs, float rhs) { return lhs.m_value < rhs && lhs != rhs; }
bool operator>(float lhs, const ApproxF& rhs) { return lhs > rhs.m_value && lhs != rhs; }
bool operator>(const ApproxF& lhs, float rhs) { return lhs.m_value > rhs && lhs != rhs; }
// clang-format on

String toString(const ApproxF &in)
{
    return "ApproxF( " + doctest::toString(in.m_value) + " )"; // NOLINT
}

} // namespace doctest

ix_TEST_CASE("ix_doctest: ApproxF")
{
    ix_EXPECT(0.0F == doctest::ApproxF(0.0F));
    ix_EXPECT(doctest::ApproxF(0.0F) == 0.0F);
    ix_EXPECT(0.0F != doctest::ApproxF(1.0F));
    ix_EXPECT(doctest::ApproxF(1.0F) != 0.0F);

    ix_EXPECT(0.0F < doctest::ApproxF(1.0F));
    ix_EXPECT(doctest::ApproxF(0.0F) < 1.0F);

    ix_EXPECT(0.0F <= doctest::ApproxF(1.0F));
    ix_EXPECT(doctest::ApproxF(0.0F) <= 1.0F);

    ix_EXPECT(0.0F <= doctest::ApproxF(0.0F));
    ix_EXPECT(doctest::ApproxF(0.0F) <= 0.0F);

    ix_EXPECT(doctest::ApproxF(1.0F) > 0.0F);
    ix_EXPECT(1.0F > doctest::ApproxF(0.0F));

    ix_EXPECT(doctest::ApproxF(1.0F) >= 0.0F);
    ix_EXPECT(1.0F >= doctest::ApproxF(0.0F));

    ix_EXPECT(doctest::ApproxF(0.0F) >= 0.0F);
    ix_EXPECT(0.0F >= doctest::ApproxF(0.0F));

    const doctest::ApproxF f(1.0);
    ix_EXPECT(f(10.0) == 10.0F);

    ix_EXPECT(toString(f) == "ApproxF( 1 )");
}
