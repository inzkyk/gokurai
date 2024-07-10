#pragma once

#include "ix.hpp"
#include "ix_polyfill.hpp"

bool ix_starts_with(const char *a, const char *b);

char *ix_strstr(char *haystack, const char *needle);
const char *ix_strstr(const char *haystack, const char *needle);

const char *ix_strchr(const char *haystack, char c);

void *ix_memchr(char *haystack, char c, size_t length);
const void *ix_memchr(const char *haystack, char c, size_t length);

char *ix_memnext(char *haystack, char c);
const char *ix_memnext(const char *haystack, char c);

char *ix_memnext2(char *haystack, char c0, char c1);
const char *ix_memnext2(const char *haystack, char c0, char c1);

size_t ix_strlen_runtime(char const *s);
int ix_strcmp_runtime(char const *a, const char *b);

ix_FORCE_INLINE constexpr size_t ix_strlen(const char *text)
{
#if !ix_MEASURE_COVERAGE
    if (!ix_is_constant_evaluated())
    {
        return ix_strlen_runtime(text);
    }
#endif

    size_t length = 0;
    while (*text != '\0')
    {
        text += 1;
        length += 1;
    }
    return length;
}

ix_FORCE_INLINE constexpr int ix_strcmp(const char *a, const char *b)
{
#if !ix_MEASURE_COVERAGE
    if (!ix_is_constant_evaluated())
    {
        return ix_strcmp_runtime(a, b);
    }
#endif

    while (*a != '\0')
    {
        if (*a != *b)
        {
            break;
        }

        a += 1;
        b += 1;
    }

    return (*a - *b);
}
