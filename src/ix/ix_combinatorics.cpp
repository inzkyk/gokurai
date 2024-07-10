#include "ix_combinatorics.hpp"
#include "ix_doctest.hpp"

ix_TEST_CASE("ix_is_unique")
{
    {
        uint64_t buf[] = {1};
        ix_EXPECT(ix_is_unique(buf, 0));
        ix_EXPECT(ix_is_unique(buf, 1));
    }
    {
        uint64_t buf[] = {1, 2, 3};
        ix_EXPECT(ix_is_unique(buf, 3));
    }
    {
        uint64_t buf[] = {1, 2, 1};
        ix_EXPECT(!ix_is_unique(buf, 3));
    }
}

ix_TEST_CASE("ix_shuffle")
{
    const size_t N = 100;
    uint64_t buf[N];
    for (size_t i = 0; i < N; i++)
    {
        buf[i] = i;
    }
    ix_shuffle(buf, N);
    for (size_t i = 0; i < N; i++)
    {
        ix_EXPECT(buf[i] < N);
    }
    ix_EXPECT(ix_is_unique(buf, N));
}
