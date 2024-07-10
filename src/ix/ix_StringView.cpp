#include "ix_StringView.hpp"
#include "ix_doctest.hpp"
#include "ix_hash.hpp"

#if !ix_MEASURE_COVERAGE
[[maybe_unused]] doctest::String toString(const ix_StringView &sv)
{
    return doctest::String(sv.data(), static_cast<doctest::String::size_type>(sv.length()));
}
#endif

ix_TEST_CASE("ix_StringView::ix_StringView")
{
    // from a string literal
    {
        const ix_StringView empty("");
        ix_EXPECT_EQSTR(empty.data(), "");
        ix_EXPECT(empty.length() == 0);

        const ix_StringView hello("hello");
        ix_EXPECT_EQSTR(hello.data(), "hello");
        ix_EXPECT(hello.length() == 5);
    }

    // from a string buffer
    {
        const char *s = "hello world";
        const ix_StringView hello(s, s + 5);
        ix_EXPECT(hello.length() == 5);
        ix_EXPECT(hello == ix_StringView("hello"));
        const ix_StringView world(s + 6, s + 11);
        ix_EXPECT(world.length() == 5);
        ix_EXPECT(world == ix_StringView("world"));
    }

    // from a string buffer, with length specified
    {
        const char *s = "hello world";
        const ix_StringView hello(s, 5);
        ix_EXPECT(hello.length() == 5);
        ix_EXPECT(hello == ix_StringView("hello"));
        const ix_StringView world(s + 6, 5);
        ix_EXPECT(world.length() == 5);
        ix_EXPECT(world == ix_StringView("world"));
    }
}

ix_TEST_CASE("ix_StringView::operators")
{
    const ix_StringView empty("");
    const ix_StringView hello("hello");
    const ix_StringView world("world");

    // ix_StringView and ix_StringView
    {

        ix_EXPECT(empty == empty);
        ix_EXPECT(hello == hello);
        ix_EXPECT(empty != hello);
        ix_EXPECT(hello != empty);
        ix_EXPECT(hello != world);
    }

    // ix_StringView and const char *
    {
        ix_EXPECT(empty == "");
        ix_EXPECT(empty != "hello");
        ix_EXPECT("" == empty);
        ix_EXPECT("hello" != empty);

        ix_EXPECT(hello != "");
        ix_EXPECT(hello == "hello");
        ix_EXPECT("" != hello);
        ix_EXPECT("hello" == hello);
        ix_EXPECT("hell" != hello);
        ix_EXPECT("hellloo" != hello);
        ix_EXPECT(hello != "hell");
        ix_EXPECT(hello != "hellloo");
    }
}

ix_TEST_CASE("ix_StringView: hash")
{
    constexpr auto hash = ix_Hash<ix_StringView>();
    const ix_StringView empty("");
    const ix_StringView hello("hello");
    ix_EXPECT(hash(empty) == hash(empty));
    ix_EXPECT(hash(empty) == ix_hash_str(""));
    ix_EXPECT(hash(hello) == hash(hello));
    ix_EXPECT(hash(hello) == ix_hash_str("hello"));
}
