#include "ix_Function.hpp"
#include "ix_Array.hpp"
#include "ix_Vector.hpp"
#include "ix_doctest.hpp"

template <typename F>
using ix_Function = ix_FunctionN<24, F>;

ix_TEST_CASE("ix_Function")
{
    {
        const ix_Function<int(int, int)> f([](int x, int y) { return x + y; });
        ix_EXPECT(f(1, 2) == 3);
    }

    {
        const int a = 1;
        const int b = 2;
        const ix_Function<int(void)> f([&]() { return a + b; });
        ix_EXPECT(f() == 3);
    }

    {
        const int a = 1;
        const int b = 2;
        const ix_Function<int(void)> f([=]() { return a + b; });
        ix_EXPECT(f() == 3);
    }

    {
        ix_Vector<uint64_t> v{1, 2};
        const ix_Function<uint64_t(void)> f([&]() { return v[0] + v[1]; });
        ix_EXPECT(f() == 3);
        v[0] = 100;
        ix_EXPECT(f() == 102);
    }

    {
        ix_Array<uint64_t, 2> v{1, 2};
        const ix_Function<uint64_t(void)> f([=]() { return v[0] + v[1]; });
        ix_EXPECT(f() == 3);
        v[0] = 100;
        ix_EXPECT(f() == 3);
    }

    {
        struct S
        {
            uint64_t a;
            uint64_t b;
            uint64_t c;
        };

        const S X{10, 20, 30};
        const ix_Function<uint64_t(void)> f([=]() { return X.a + X.b + X.c; });
        ix_EXPECT(f() == 60);
    }

    {
        ix_Function<void(void)> f([]() {});
        ix_EXPECT(!f.is_dead());
        f();
        ix_EXPECT(!f.is_dead());
        f.mark_dead();
        ix_EXPECT(f.is_dead());
    }
}

[[maybe_unused]] static int twice(int i)
{
    return 2 * i;
}

[[maybe_unused]] static unsigned int square(unsigned int f)
{
    return f * f;
}

ix_TEST_CASE("ix_Function: from function pointer")
{
    const ix_Function<int(int)> f(&twice);
    ix_EXPECT(f(10) == 20);

    const ix_Function<unsigned int(unsigned int)> g(&square);
    ix_EXPECT(g(2) == 4);
}
