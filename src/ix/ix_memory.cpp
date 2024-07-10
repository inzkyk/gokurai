#include "ix_memory.hpp"
#include "ix_assert.hpp"
#include "ix_doctest.hpp"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

constexpr size_t ix_MEMORY_LIMIT = (1024 * 1024 * 256);

ix_ATTRIBUTE_MALLOC void *ix_malloc(size_t size)
{
    ix_ASSERT_FATAL(size <= ix_MEMORY_LIMIT);
    void *ans = malloc(size);
    ix_ASSERT(ans != nullptr);
    return ans;
}

ix_TEST_CASE("ix_MALLOC")
{
    char *p = ix_MALLOC(char *, 100);
    ix_EXPECT(p != nullptr);
    ix_ASSERT(p != nullptr); // Suppress MSVC warning
    for (size_t i = 0; i < 100; i++)
    {
        p[i] = static_cast<char>(i);
    }
    ix_FREE(p);
}

ix_ATTRIBUTE_MALLOC void *ix_realloc(void *p, size_t size)
{
    ix_ASSERT_FATAL(size <= ix_MEMORY_LIMIT);
    void *ans = realloc(p, size);
    ix_ASSERT(ans != nullptr);
    return ans;
}

ix_TEST_CASE("ix_REALLOC")
{
    char *p = ix_MALLOC(char *, 100);
    ix_EXPECT(p != nullptr);
    ix_ASSERT(p != nullptr); // Suppress MSVC warning
    for (size_t i = 0; i < 100; i++)
    {
        p[i] = static_cast<char>(i);
    }
    p = ix_REALLOC(char *, p, 10);
    ix_EXPECT(p != nullptr);
    ix_ASSERT(p != nullptr); // Suppress MSVC warning
    for (size_t i = 0; i < 10; i++)
    {
        ix_EXPECT(p[i] == static_cast<char>(i));
    }
    p = ix_REALLOC(char *, p, 100);
    ix_EXPECT(p != nullptr);
    ix_ASSERT(p != nullptr); // Suppress MSVC warning
    for (size_t i = 0; i < 10; i++)
    {
        ix_EXPECT(p[i] == static_cast<char>(i));
    }
    ix_FREE(p);
}

void ix_free(void *p)
{
    free(p);
}

ix_TEST_CASE("ix_ALLOC_ARRAY")
{
    int *p = ix_ALLOC_ARRAY(int, 100);
    ix_EXPECT(p != nullptr);
    ix_ASSERT(p != nullptr); // Suppress MSVC warning
    for (size_t i = 0; i < 100; i++)
    {
        p[i] = static_cast<int>(i);
    }
    ix_FREE(p);
}

ix_TEST_CASE("ix_REALLOC_ARRAY")
{
    int *p = ix_ALLOC_ARRAY(int, 100);
    ix_EXPECT(p != nullptr);
    ix_ASSERT(p != nullptr); // Suppress MSVC warning
    for (size_t i = 0; i < 100; i++)
    {
        p[i] = static_cast<int>(i);
    }
    p = ix_REALLOC_ARRAY(int, p, 10);
    ix_EXPECT(p != nullptr);
    ix_ASSERT(p != nullptr); // Suppress MSVC warning
    for (size_t i = 0; i < 10; i++)
    {
        ix_EXPECT(p[i] == static_cast<int>(i));
    }
    ix_FREE(p);
}

size_t ix_grow_array_size(size_t size)
{
    return (size < 8) ? 8 : (2 * size);
}

ix_TEST_CASE("ix_grow_array_size")
{
    ix_EXPECT(ix_grow_array_size(1) == 8);
    ix_EXPECT(ix_grow_array_size(2) == 8);
    ix_EXPECT(ix_grow_array_size(3) == 8);
    ix_EXPECT(ix_grow_array_size(4) == 8);
    ix_EXPECT(ix_grow_array_size(5) == 8);
    ix_EXPECT(ix_grow_array_size(6) == 8);
    ix_EXPECT(ix_grow_array_size(7) == 8);
    ix_EXPECT(ix_grow_array_size(8) == 16);
    ix_EXPECT(ix_grow_array_size(16) == 32);
}

void ix_memmove(void *dst, const void *src, size_t n)
{
    memmove(dst, src, n);
}

void ix_memset(void *dst, char val, size_t n)
{
    memset(dst, val, n);
}

int ix_memcmp(const void *a, const void *b, size_t n)
{
    return memcmp(a, b, n);
}

void ix_memcpy(void *dst, const void *src, size_t n)
{
    memcpy(dst, src, n);
}
