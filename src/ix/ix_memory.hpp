#pragma once

#include "ix.hpp"
#include "ix_polyfill.hpp"

#if ix_COMPILER(MSVC)
#define ix_ATTRIBUTE_MALLOC __declspec(restrict)
#else
#define ix_ATTRIBUTE_MALLOC __attribute__((malloc))
#endif

#define ix_MALLOC(T, size) static_cast<T>(ix_malloc(size))
#define ix_REALLOC(T, p, size) static_cast<T>(ix_realloc(p, size))
#define ix_FREE(p) ix_free(p)
#define ix_ALLOC_STRUCT(T) static_cast<T *>(ix_malloc(sizeof(T)))                                           // NOLINT
#define ix_ALLOC_ARRAY(ELEM_T, length) static_cast<ELEM_T *>(ix_malloc(sizeof(ELEM_T) * (length)))          // NOLINT
#define ix_REALLOC_ARRAY(ELEM_T, p, length) static_cast<ELEM_T *>(ix_realloc(p, sizeof(ELEM_T) * (length))) // NOLINT

void ix_free(void *p);
ix_ATTRIBUTE_MALLOC void *ix_malloc(size_t size);
ix_ATTRIBUTE_MALLOC void *ix_realloc(void *p, size_t size);
size_t ix_grow_array_size(size_t size);

void ix_memmove(void *dst, const void *src, size_t n);
void ix_memset(void *dst, char val, size_t n);
int ix_memcmp(const void *a, const void *b, size_t n);
void ix_memcpy(void *dst, const void *src, size_t n);

ix_FORCE_INLINE constexpr uint64_t ix_memread8(const char *src)
{
    uint64_t v = 0;
    if (ix_is_constant_evaluated())
    {
        v = static_cast<uint64_t>(static_cast<uint8_t>(*src));
        v |= static_cast<uint64_t>(static_cast<uint8_t>(*(src + 1))) << 8;
        v |= static_cast<uint64_t>(static_cast<uint8_t>(*(src + 2))) << 16;
        v |= static_cast<uint64_t>(static_cast<uint8_t>(*(src + 3))) << 24;
        v |= static_cast<uint64_t>(static_cast<uint8_t>(*(src + 4))) << 32;
        v |= static_cast<uint64_t>(static_cast<uint8_t>(*(src + 5))) << 40;
        v |= static_cast<uint64_t>(static_cast<uint8_t>(*(src + 6))) << 48;
        v |= static_cast<uint64_t>(static_cast<uint8_t>(*(src + 7))) << 56;
    }
    else
    {
        ix_memcpy(&v, src, 8);
    }

    return v;
}

ix_FORCE_INLINE constexpr uint32_t ix_memread4(const char *src)
{
    uint32_t v = 0;
    if (ix_is_constant_evaluated())
    {
        v = static_cast<uint32_t>(static_cast<uint8_t>(*src));
        v |= static_cast<uint32_t>(static_cast<uint8_t>(*(src + 1))) << 8;
        v |= static_cast<uint32_t>(static_cast<uint8_t>(*(src + 2))) << 16;
        v |= static_cast<uint32_t>(static_cast<uint8_t>(*(src + 3))) << 24;
    }
    else
    {
        ix_memcpy(&v, src, 4);
    }

    return v;
}
