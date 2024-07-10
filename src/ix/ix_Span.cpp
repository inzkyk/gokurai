#include "ix_Span.hpp"
#include "ix_Vector.hpp"
#include "ix_doctest.hpp"

[[maybe_unused]] static bool f(const ix_Span<int> &span)
{
    ix_UNUSED(span);
    return true;
}

ix_TEST_CASE("ix_Span: from pointer + length")
{
    const char *buf = "hello world";
    const ix_Span<const char> span(buf, 5);
    ix_EXPECT(span.data() == buf);
    ix_EXPECT(span.size() == 5);
}

ix_TEST_CASE("ix_Span: from array")
{
    int buf[] = {0, 1, 2, 3, 4};
    const ix_Span<int> span(buf);
    ix_EXPECT(span.data() == static_cast<int *>(buf));
    ix_EXPECT(span.size() == 5);
}

ix_TEST_CASE("ix_Span: range-based for")
{
    int v[] = {0, 10, 20, 30, 40};
    int w[5] = {};
    size_t i = 0;
    for (const int x : ix_Span<int>(v, 5))
    {
        w[i] = x;
        i += 1;
    }
    ix_EXPECT(i == 5);
    for (size_t j = 0; j < 5; j++)
    {
        ix_EXPECT(v[j] == w[j]);
    }
}

ix_TEST_CASE("ix_Span: initializer list")
{
    ix_EXPECT(f({}));
    ix_EXPECT(f({0, 1, 2, 3, 4, 5}));
}

ix_TEST_CASE("ix_Span: contains")
{
    ix_EXPECT(ix_Span<int>({0, 1, 2, 3}).contains(0));
    ix_EXPECT(ix_Span<int>({0, 1, 2, 3}).contains(1));
    ix_EXPECT(ix_Span<int>({0, 1, 2, 3}).contains(2));
    ix_EXPECT(ix_Span<int>({0, 1, 2, 3}).contains(3));
    ix_EXPECT(!ix_Span<int>({0, 1, 2, 3}).contains(4));
    ix_EXPECT(!ix_Span<int>({0, 1, 2, 3}).contains(5));
}

ix_TEST_CASE("ix_Span: operator[]")
{
    ix_EXPECT(ix_Span<int>({0, 1, 2, 3})[0] == 0);
    ix_EXPECT(ix_Span<int>({0, 1, 2, 3})[1] == 1);
    ix_EXPECT(ix_Span<int>({0, 1, 2, 3})[2] == 2);
    ix_EXPECT(ix_Span<int>({0, 1, 2, 3})[3] == 3);

    ix_Vector<int> v{0, 1, 2, 3};
    ix_Span<int> span(v.data(), v.size());
    span[0] = 10;
    ix_ASSERT(span[0] == 10);
    ix_EXPECT(v == ix_Vector<int>{10, 1, 2, 3});
}

ix_TEST_CASE("ix_Span: equality")
{
    ix_Vector<int> v{0, 1, 2, 3};
    const ix_Span<int> span(v.data(), v.size());
    ix_EXPECT(span == ix_Span<int>{0, 1, 2, 3});
    ix_EXPECT(span != ix_Span<int>{0, 1, 2 != 4});
    ix_EXPECT(span != ix_Span<int>{0 != 1});
}
