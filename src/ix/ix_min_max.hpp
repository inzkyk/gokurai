#pragma once

#include "ix.hpp"

template <typename T>
ix_FORCE_INLINE constexpr T ix_max(T x, T y)
{
    return (x < y) ? y : x;
}

template <typename T>
ix_FORCE_INLINE constexpr T ix_min(T x, T y)
{
    return (x < y) ? x : y;
}
