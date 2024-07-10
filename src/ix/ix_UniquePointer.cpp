#include "ix_UniquePointer.hpp"
#include "ix_Vector.hpp"
#include "ix_doctest.hpp"
#include "ix_random.hpp"
#include "ix_utility.hpp"

ix_TEST_CASE("ix_UniquePointer: basic")
{
    ix_UniquePointer<int> null(nullptr);
    ix_EXPECT(null.get() == nullptr);

    auto p = ix_make_unique<int>(100);
    ix_EXPECT(p.get() != nullptr);
    ix_EXPECT(*p == 100);
    *p = 200;
    ix_EXPECT(*p == 200);

    ix_UniquePointer<int> q(ix_move(p));
    ix_EXPECT(q.get() != nullptr);
    ix_EXPECT(*q == 200);

    auto r = ix_make_unique<int>(300);
    ix_EXPECT(*r == 300);
    r = ix_move(q);
    ix_EXPECT(*r == 200);

    auto s = ix_make_unique<int>(400);
    ix_EXPECT(*s == 400);
    int *raw_p = ix_MALLOC(int *, sizeof(int));
    *raw_p = 500;
    s.swap(ix_move(raw_p));
    ix_EXPECT(*s == 500);

    s = ix_move(s);
    ix_EXPECT(s.get() != nullptr);

    int *ptr = s.detach();
    ix_FREE(ptr);
}

ix_TEST_CASE("ix_UniquePointer: const")
{
    const auto p = ix_make_unique<int>(100);
    ix_EXPECT(p.get() != nullptr);
    ix_EXPECT(*p == 100);
}

ix_TEST_CASE("ix_UniquePointer: calling member function")
{
    auto v = ix_make_unique<ix_Vector<int>>(size_t{10});
    ix_EXPECT(v->size() == 10);
    ix_EXPECT((*v).size() == 10);

    const ix_UniquePointer<ix_Vector<int>> w(ix_move(v));
    ix_EXPECT(w->size() == 10);
    ix_EXPECT((*w).size() == 10);
}

ix_TEST_CASE("ix_UniquePointer: array")
{
    const size_t n = ix_rand_range<size_t>(10, 20);

    auto p = ix_make_unique_array<size_t>(n);
    ix_EXPECT(p.get() != nullptr);
    for (size_t i = 0; i < n; i++)
    {
        p[i] = i * i;
    }

    for (size_t i = 0; i < n; i++)
    {
        ix_EXPECT(p[i] == i * i);
    }

    ix_UniquePointer<size_t[]> q(ix_move(p));
    ix_EXPECT(q.get() != nullptr);
    for (size_t i = 0; i < n; i++)
    {
        ix_EXPECT(q[i] == i * i);
    }

    auto r = ix_make_unique_array<size_t>(n);
    r = ix_move(q);
    ix_EXPECT(r.get() != nullptr);
    for (size_t i = 0; i < n; i++)
    {
        ix_EXPECT(r[i] == i * i);
    }

    const ix_UniquePointer<size_t[]> s(ix_move(r));
    ix_EXPECT(s.get() != nullptr);
    for (size_t i = 0; i < n; i++)
    {
        ix_EXPECT(s[i] == i * i);
    }

    auto t = ix_make_unique_array<size_t>(n);
    ix_EXPECT(t.get() != nullptr);
    t.swap(ix_ALLOC_ARRAY(size_t, n));
    ix_EXPECT(t.get() != nullptr);

    t = ix_move(t);
    ix_EXPECT(t.get() != nullptr);

    size_t *ptr = t.detach();
    ix_FREE(ptr);
}
