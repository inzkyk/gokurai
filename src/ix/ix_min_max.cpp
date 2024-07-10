#include "ix_min_max.hpp"
#include "ix_doctest.hpp"

ix_TEST_CASE("ix_min")
{
    ix_EXPECT(ix_min(0, 1) == 0);
    ix_EXPECT(ix_min(1, 0) == 0);
}

ix_TEST_CASE("ix_max")
{
    ix_EXPECT(ix_max(0, 1) == 1);
    ix_EXPECT(ix_max(1, 0) == 1);
}
