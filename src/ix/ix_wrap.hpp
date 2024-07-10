#pragma once

#include "ix.hpp"
#include "ix_limits.hpp"

template <typename T>
constexpr bool ix_can_add(T x, T y)
{
    if constexpr (!ix_numeric_limits<T>::is_integer || !ix_numeric_limits<T>::is_signed)
    {
        return true;
    }
    else
    {
        const bool no_overflow_max = (y <= 0) || (x <= ix_numeric_limits<T>::max() - y);
        const bool no_overflow_min = (y >= 0) || (x >= ix_numeric_limits<T>::min() - y);
        return (no_overflow_max && no_overflow_min);
    }
}

template <typename T>
constexpr bool ix_can_sub(T x, T y)
{
    if constexpr (!ix_numeric_limits<T>::is_integer || !ix_numeric_limits<T>::is_signed)
    {
        return true;
    }
    else
    {
        const bool no_overflow_max = (y >= 0) || (x <= ix_numeric_limits<T>::max() + y);
        const bool no_overflow_min = (y <= 0) || (x >= ix_numeric_limits<T>::min() + y);
        return (no_overflow_max && no_overflow_min);
    }
}

template <typename T>
ix_FORCE_INLINE constexpr T ix_wrap_increment(T x, T begin, T end)
{
    const T maybe_next = x + 1;
    // Any way to ensure that `cmov` is used?
    return ix_UNLIKELY(maybe_next == end) ? begin : maybe_next;
}
