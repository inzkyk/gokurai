#include "ix_assert.hpp"
#include "ix_doctest.hpp"

#include <stdlib.h>

#if ix_MEASURE_COVERAGE

void ix_abort(void)
{
}

ix_TEST_CASE("ix_abort")
{
    ix_abort();
}

#else

[[noreturn]] void ix_abort()
{
#if ix_COMPILER(MSVC)
    __debugbreak();
#endif
    abort();
}

#endif
