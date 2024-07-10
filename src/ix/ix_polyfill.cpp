#include "ix_polyfill.hpp"
#include "ix_doctest.hpp"
#include "ix_string.hpp"

[[maybe_unused]] static constexpr size_t f(size_t x)
{
#if !ix_MEASURE_COVERAGE
    if (!ix_is_constant_evaluated())
    {
        const char *s = "hello";
        return ix_strlen(s); // not a comptime call.
    }
#endif

    return x;
}

ix_TEST_CASE("ix_is_constant_evaluated")
{
    constexpr size_t i = f(1);
    ix_EXPECT(i == 1);
#if ix_MEASURE_COVERAGE
    ix_EXPECT(f(1) == 1);
#else
    size_t x = i;
    x += 1;
    const size_t j = f(x);
    ix_EXPECT(j == 5);
#endif
}
