#include "ix_defer.hpp"
#include "ix_doctest.hpp"

ix_TEST_CASE("ix_defer")
{
    bool ok = false;
    {
        auto _ = ix_defer([&] { ok = true; });
    }
    ix_EXPECT(ok);
}

ix_TEST_CASE("ix_defer: moved lambda")
{
    bool ok = false;
    {
        auto f = [&] { ok = true; };
        auto _ = ix_defer(ix_move(f));
    }
    ix_EXPECT(ok);
}

ix_TEST_CASE("ix_defer: deactivate")
{
    bool ok = false;
    auto f = [&] { ok = true; };
    auto g = f;
    {
        auto d = ix_defer(ix_move(f));
        d.deactivate();
    }
    ix_EXPECT(!ok);
    g();
    ix_EXPECT(ok);
}

ix_TEST_CASE("ix_defer: deactivate (constructor)")
{
    bool ok = false;
    auto f = [&] { ok = true; };
    auto g = f;
    {
        auto _ = ix_defer(false, ix_move(f));
    }
    ix_EXPECT(!ok);
    g();
    ix_EXPECT(ok);
}
