#include "ix_Array.hpp"
#include "ix_doctest.hpp"

ix_TEST_CASE("ix_Array")
{
    ix_Array<int, 4> x{};
    ix_EXPECT(x.size() == 4);
    x[0] = 0;
    x[1] = 11;
    x[2] = 22;
    x[3] = 33;
    ix_EXPECT(x[0] == 0);
    ix_EXPECT(x[1] == 11);
    ix_EXPECT(x[2] == 22);
    ix_EXPECT(x[3] == 33);

    x = {0, 111, 222, 333};
    ix_EXPECT(x[0] == 0);
    ix_EXPECT(x[1] == 111);
    ix_EXPECT(x[2] == 222);
    ix_EXPECT(x[3] == 333);

    size_t num = 0;
    int w[4] = {};
    for (const int &i : x)
    {
        w[num] = i;
        num += 1;
    }
    ix_EXPECT(num == x.size());
    ix_EXPECT(w[0] == 0);
    ix_EXPECT(w[1] == 111);
    ix_EXPECT(w[2] == 222);
    ix_EXPECT(w[3] == 333);

    for (int &i : x)
    {
        i *= 10;
    }
    ix_EXPECT(x.size() == 4);
    ix_EXPECT(x[0] == 0);
    ix_EXPECT(x[1] == 1110);
    ix_EXPECT(x[2] == 2220);
    ix_EXPECT(x[3] == 3330);
}

ix_TEST_CASE("ix_Array: zero")
{
    const ix_Array<int, 0> x;
    ix_EXPECT(x.size() == 0);
}
