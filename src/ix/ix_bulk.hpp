#pragma once

#include "ix.hpp"
#include "ix_memory.hpp"
#include "ix_new.hpp"
#include "ix_type_traits.hpp"
#include "ix_utility.hpp"

template <typename T>
ix_FORCE_INLINE constexpr void ix_bulk_default_construct(T *dst, size_t n)
{
    if constexpr (ix_is_scalar_v<T>)
    {
        ix_memset(dst, 0, n * sizeof(T));
    }
    else if constexpr (!ix_is_trivially_constructible_v<T>)
    {
        for (size_t i = 0; i < n; i++)
        {
            new (dst + i) T();
        }
    }
}

template <typename T>
ix_FORCE_INLINE constexpr void ix_bulk_copy_construct(T *dst, const T *src, size_t n)
{
    if constexpr (ix_is_trivially_copy_constructible_v<T>)
    {
        ix_memcpy(dst, src, sizeof(T) * n);
    }
    else
    {
        for (size_t i = 0; i < n; i++)
        {
            new (dst + i) T(src[i]);
        }
    }
}

template <typename T>
ix_FORCE_INLINE constexpr void ix_bulk_move_construct(T *dst, T *src, size_t n)
{
    if constexpr (ix_is_trivially_move_constructible_v<T>)
    {
        ix_memcpy(dst, src, sizeof(T) * n);
    }
    else
    {
        for (size_t i = 0; i < n; i++)
        {
            new (dst + i) T(ix_move(src[i]));
        }
    }
}

template <typename T>
ix_FORCE_INLINE constexpr void ix_bulk_copy_assign(T *dst, const T *src, size_t n)
{
    if constexpr (ix_is_trivially_move_assignable_v<T>)
    {
        ix_memcpy(dst, src, sizeof(T) * n);
    }
    else
    {
        for (size_t i = 0; i < n; i++)
        {
            dst[i] = src[i];
        }
    }
}

template <typename T>
ix_FORCE_INLINE constexpr void ix_bulk_move_assign(T *dst, T *src, size_t n)
{
    if constexpr (ix_is_trivially_move_assignable_v<T>)
    {
        ix_memcpy(dst, src, sizeof(T) * n);
    }
    else
    {
        for (size_t i = 0; i < n; i++)
        {
            dst[i] = ix_move(src[i]);
        }
    }
}

template <typename T>
ix_FORCE_INLINE constexpr void ix_bulk_destruct(T *p, size_t n)
{
    if constexpr (!ix_is_trivially_destructible_v<T>)
    {
        for (size_t i = 0; i < n; i++)
        {
            p[i].~T();
        }
    }
}
