#include "ix_string.hpp"
#include "ix_doctest.hpp"

#include <stdlib.h>
#include <string.h>

ix_TEST_CASE("ix_strlen")
{
    ix_EXPECT(ix_strlen("") == 0);
    ix_EXPECT(ix_strlen("foo") == 3);
    ix_EXPECT(ix_strlen("foo bar") == 7);
    ix_EXPECT(ix_strlen("foo\0bar") == 3);

    ix_EXPECT(ix_strlen_runtime("") == 0);
    ix_EXPECT(ix_strlen_runtime("foo") == 3);
    ix_EXPECT(ix_strlen_runtime("foo bar") == 7);
    ix_EXPECT(ix_strlen_runtime("foo\0bar") == 3);
}

ix_TEST_CASE("ix_strcmp")
{
    ix_EXPECT(ix_strcmp("", "") == 0);
    ix_EXPECT(ix_strcmp("", "hello") < 0);
    ix_EXPECT(ix_strcmp("hello", "") > 0);
    ix_EXPECT(ix_strcmp("hello", "hello") == 0);

    ix_EXPECT(ix_strcmp_runtime("", "") == 0);
    ix_EXPECT(ix_strcmp_runtime("", "hello") < 0);
    ix_EXPECT(ix_strcmp_runtime("hello", "") > 0);
    ix_EXPECT(ix_strcmp_runtime("hello", "hello") == 0);
}

bool ix_starts_with(const char *a, const char *b)
{
    if (ix_UNLIKELY(*b == '\0'))
    {
        return true;
    }

    while (*a == *b)
    {
        a += 1;
        b += 1;
        if (*b == '\0')
        {
            return true;
        }
    }

    return false;
}

ix_TEST_CASE("ix_starts_with")
{
    ix_EXPECT(ix_starts_with("", ""));
    ix_EXPECT(ix_starts_with("foo", ""));
    ix_EXPECT(ix_starts_with("foo", "foo"));
    ix_EXPECT(ix_starts_with("foobar", "foo"));
    ix_EXPECT(!ix_starts_with("", "foo"));
    ix_EXPECT(!ix_starts_with("bar", "foo"));
    ix_EXPECT(!ix_starts_with("foa", "foo"));
}

char *ix_strstr(char *haystack, const char *needle)
{
    return strstr(haystack, needle);
}

const char *ix_strstr(const char *haystack, const char *needle)
{
    return strstr(haystack, needle);
}

ix_TEST_CASE("ix_strstr")
{
    ix_EXPECT_EQSTR(ix_strstr("", ""), "");
    ix_EXPECT_EQSTR(ix_strstr("foo", ""), "foo");
    ix_EXPECT_EQSTR(ix_strstr("foofoo", "foo"), "foofoo");
    ix_EXPECT_EQSTR(ix_strstr("foobar", "bar"), "bar");

    ix_EXPECT(ix_strstr("", "foo") == nullptr);
    ix_EXPECT(ix_strstr("foo", "bar") == nullptr);

    char buf[64];
    buf[0] = 'a';
    buf[1] = 'b';
    buf[2] = 'c';
    buf[3] = '\0';
    ix_EXPECT(ix_strstr(buf, "bc") == buf + 1);
}

const char *ix_strchr(const char *haystack, char c)
{
    return strchr(haystack, c);
}

ix_TEST_CASE("ix_strchr")
{
    ix_EXPECT(ix_strchr("", 'X') == nullptr);
    const char *msg = "FooBar";
    ix_EXPECT(ix_strchr(msg, 'F') == msg);
    ix_EXPECT(ix_strchr(msg, 'o') == msg + 1);
    ix_EXPECT(ix_strchr(msg, 'B') == msg + 3);
    ix_EXPECT(ix_strchr(msg, '\0') == msg + ix_strlen(msg));

    char buf[64];
    buf[0] = 'a';
    buf[1] = 'b';
    buf[2] = 'c';
    buf[3] = '\0';
    ix_EXPECT(ix_strchr(buf, 'b') == buf + 1);
}

const void *ix_memchr(const char *haystack, char c, size_t length)
{
    return memchr(haystack, c, length);
}

void *ix_memchr(char *haystack, char c, size_t length)
{
    return memchr(haystack, c, length);
}

ix_TEST_CASE("ix_memchr")
{
    ix_EXPECT(ix_memchr("", 'X', 0) == nullptr);
    const char *msg = "FooBar";
    ix_EXPECT(ix_memchr(msg, 'F', 0) == nullptr);
    ix_EXPECT(ix_memchr(msg, 'F', 1) == msg);
    ix_EXPECT(ix_memchr(msg, 'F', 3) == msg);
    ix_EXPECT(ix_memchr(msg, 'o', 6) == msg + 1);
    ix_EXPECT(ix_memchr(msg, 'B', 6) == msg + 3);
    ix_EXPECT(ix_memchr(msg, '\0', 7) == msg + ix_strlen(msg));

    char buf[64];
    buf[0] = 'a';
    buf[1] = 'b';
    buf[2] = 'c';
    buf[3] = '\0';
    ix_EXPECT(ix_memchr(buf, 'b', 2) == buf + 1);
}

char *ix_memnext(char *haystack, char c)
{
    while (*haystack != c)
    {
        haystack += 1;
    }
    return haystack;
}

const char *ix_memnext(const char *haystack, char c)
{
    while (*haystack != c)
    {
        haystack += 1;
    }
    return haystack;
}

ix_TEST_CASE("ix_memnext")
{
    const char *msg = "FooBar";
    ix_EXPECT(ix_memnext(msg, 'F') == msg);
    ix_EXPECT(ix_memnext(msg, 'o') == msg + 1);
    ix_EXPECT(ix_memnext(msg, 'B') == msg + 3);
    ix_EXPECT(ix_memnext(msg, '\0') == msg + ix_strlen(msg));

    char buf[64];
    buf[0] = 'a';
    buf[1] = 'b';
    buf[2] = 'c';
    buf[3] = '\0';
    ix_EXPECT(ix_memnext(buf, 'b') == buf + 1);
}

char *ix_memnext2(char *haystack, char c0, char c1)
{
    while ((*haystack != c0) && (*haystack != c1))
    {
        haystack += 1;
    }
    return haystack;
}

const char *ix_memnext2(const char *haystack, char c0, char c1)
{
    while ((*haystack != c0) && (*haystack != c1))
    {
        haystack += 1;
    }
    return haystack;
}

ix_TEST_CASE("ix_memnext2")
{
    const char *msg = "FooBar";
    ix_EXPECT(ix_memnext2(msg, 'F', 'B') == msg);
    ix_EXPECT(ix_memnext2(msg, 'B', 'F') == msg);
    ix_EXPECT(ix_memnext2(msg, 'B', 'a') == msg + 3);
    ix_EXPECT(ix_memnext2(msg, 'a', 'B') == msg + 3);
    ix_EXPECT(ix_memnext2(msg, '\0', 'a') == msg + 4);
    ix_EXPECT(ix_memnext2(msg, 'a', '\0') == msg + 4);
    ix_EXPECT(ix_memnext2(msg, '\0', '\0') == msg + ix_strlen(msg));

    char buf[64];
    buf[0] = 'a';
    buf[1] = 'b';
    buf[2] = 'c';
    buf[3] = '\0';
    ix_EXPECT(ix_memnext2(buf, 'b', 'c') == buf + 1);
    ix_EXPECT(ix_memnext2(buf, 'c', 'b') == buf + 1);
}

size_t ix_strlen_runtime(char const *s)
{
    return strlen(s);
}

int ix_strcmp_runtime(char const *a, const char *b)
{
    return strcmp(a, b);
}
