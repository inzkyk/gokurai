#pragma once

#include "ix.hpp"
#include "ix_type_traits.hpp"

template <class T>
ix_FORCE_INLINE constexpr T &&ix_forward(ix_remove_reference_t<T> &arg) noexcept
{
    return static_cast<T &&>(arg);
}

template <class T>
ix_FORCE_INLINE constexpr T &&ix_forward(ix_remove_reference_t<T> &&arg) noexcept
{
    static_assert(!ix_is_lvalue_reference_v<T>, "bad forward call");
    return static_cast<T &&>(arg);
}

template <class T>
ix_FORCE_INLINE constexpr ix_remove_reference_t<T> &&ix_move(T &&arg) noexcept
{
    return static_cast<ix_remove_reference_t<T> &&>(arg);
}

template <class T>
ix_FORCE_INLINE constexpr void ix_swap(T &x, T &y)
{
    T temp = ix_move(x);
    x = ix_move(y);
    y = ix_move(temp);
}
