#pragma once

#include "ix.hpp"
#include "ix_hash.hpp"
#include "ix_memory.hpp"

// WARN: ix_DumbInt is meant be used only in testing. Don't use it for other purposes.
template <size_t Size>
struct ix_DumbInt
{
    static_assert(Size > 0);
    static_assert((Size % 64) == 0);
    static constexpr size_t N = Size / 64;
    uint64_t v[N] = {};

    constexpr ix_DumbInt() = default;

    // cppcheck-suppress noExplicitConstructor
    constexpr ix_DumbInt(uint64_t x) // NOLINT
    {
        for (size_t i = 0; i < N; i++)
        {
            v[i] = x;
        }
    }

    // cppcheck-suppress noExplicitConstructor
    constexpr ix_DumbInt(int x) // NOLINT
    {
        for (size_t i = 0; i < N; i++)
        {
            v[i] = static_cast<uint64_t>(x);
        }
    }

    constexpr bool operator==(const ix_DumbInt &x) const
    {
        return (ix_memcmp(v, x.v, sizeof(v)) == 0);
    }

    constexpr bool operator!=(const ix_DumbInt &x) const
    {
        return (ix_memcmp(v, x.v, sizeof(v)) != 0);
    }
};

using ix_DumbInt256 = ix_DumbInt<256>;

template <size_t Size>
struct ix_Hash<ix_DumbInt<Size>>
{
    ix_FORCE_INLINE size_t operator()(const ix_DumbInt256 &x) const
    {
        return ix_hash(x.v, sizeof(x.v));
    }
};
