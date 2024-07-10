#include "ix_hash.hpp"
#include "ix_doctest.hpp"

#include <stddef.h>

size_t ix_hash_between(const void *start, const void *end)
{
    const ptrdiff_t length = static_cast<const uint8_t *>(end) - static_cast<const uint8_t *>(start);
    return ix_hash(start, static_cast<size_t>(length));
}

size_t ix_hash(const void *p, size_t length)
{
    return ix_hash(static_cast<const char *>(p), length);
}

ix_TEST_CASE("ix_hash")
{
    // clang-format off
    ix_EXPECT(ix_hash("",      0) == ix_hash("",            0));
    ix_EXPECT(ix_hash("hello", 0) == ix_hash("hello world", 0));
    ix_EXPECT(ix_hash("hello", 1) == ix_hash("hello world", 1));
    ix_EXPECT(ix_hash("hello", 5) == ix_hash("hello world", 5));
    ix_EXPECT(ix_hash("hello", 6) != ix_hash("hello world", 6));
    ix_EXPECT(ix_hash("foo",   3) != ix_hash("bar",         3));
    ix_EXPECT(ix_hash("foo",   3) == ix_hash("foo\0bar",    3));
    // clang-format on
}

ix_TEST_CASE("ix_hash_str")
{
    ix_hash_str("hello");

    // clang-format off
    ix_EXPECT(ix_hash_str("")      == ix_hash_str(""));
    ix_EXPECT(ix_hash_str("hello") == ix_hash_str("hello"));
    ix_EXPECT(ix_hash_str("foo")   != ix_hash_str("bar"));
    ix_EXPECT(ix_hash_str("foo")   == ix_hash_str("foo\0bar"));
    // clang-format on
}

ix_TEST_CASE("ix_hash_between")
{
    const char *msg = "hello world";
    ix_EXPECT(ix_hash_between(msg, msg) == ix_hash_str(""));
    ix_EXPECT(ix_hash_between(msg, msg + 1) == ix_hash_str("h"));
    ix_EXPECT(ix_hash_between(msg, msg + 5) == ix_hash_str("hello"));
    ix_EXPECT(ix_hash_between(msg, msg + 11) == ix_hash_str("hello world"));
}

ix_TEST_CASE("ix_hash: constexpr or not")
{
    {
        constexpr const char *s = "";
        const char *s_runtime = s;
        const size_t x = ix_hash_str(s_runtime);
        constexpr size_t y = ix_hash_str(s);
        ix_EXPECT(x == y);
        ix_EXPECT(x == ix_hash(s, ix_strlen(s)));
    }

    {
        constexpr const char *s = "f";
        const char *s_runtime = s;
        const size_t x = ix_hash_str(s_runtime);
        constexpr size_t y = ix_hash_str(s);
        ix_EXPECT(x == y);
        ix_EXPECT(x == ix_hash(s, ix_strlen(s)));
    }

    {
        constexpr const char *s = "foooo";
        const char *s_runtime = s;
        const size_t x = ix_hash_str(s_runtime);
        constexpr size_t y = ix_hash_str(s);
        ix_EXPECT(x == y);
        ix_EXPECT(x == ix_hash(s, ix_strlen(s)));
    }

    {
        constexpr const char *s = "01234567890123456789012345678901234567890123456789";
        const char *s_runtime = s;
        const size_t x = ix_hash_str(s_runtime);
        constexpr size_t y = ix_hash_str(s);
        ix_EXPECT(x == y);
        ix_EXPECT(x == ix_hash(s, ix_strlen(s)));
    }
}

ix_TEST_CASE("ix_hash: hash functions ")
{
    ix_EXPECT(ix_Hash<int8_t>()(1) == ix_Hash<int8_t>()(1));
    ix_EXPECT(ix_Hash<int16_t>()(1) == ix_Hash<int16_t>()(1));
    ix_EXPECT(ix_Hash<int32_t>()(1) == ix_Hash<int32_t>()(1));
    ix_EXPECT(ix_Hash<int64_t>()(1) == ix_Hash<int64_t>()(1));
    ix_EXPECT(ix_Hash<uint8_t>()(1) == ix_Hash<uint8_t>()(1));
    ix_EXPECT(ix_Hash<uint16_t>()(1) == ix_Hash<uint16_t>()(1));
    ix_EXPECT(ix_Hash<uint32_t>()(1) == ix_Hash<uint32_t>()(1));
    ix_EXPECT(ix_Hash<uint64_t>()(1) == ix_Hash<uint64_t>()(1));
    ix_EXPECT(ix_Hash<float>()(1.0F) == ix_Hash<float>()(1.0F));
    ix_EXPECT(ix_Hash<double>()(1.0) == ix_Hash<double>()(1.0));
}
