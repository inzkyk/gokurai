#pragma once

#include "ix.hpp"

#define ix_PI 3.14159265358979323846f
#define ix_2PI 6.28318530717958647693f

float ix_to_degree(float x);
float ix_to_radian(float x);
float ix_normalize_radian(float x);
float ix_normalize_degree(float x);
bool ix_is_approx_eq(float x, float correct);
uint64_t ix_log10(uint64_t n);

ix_DISABLE_GCC_WARNING_BEGIN
ix_DISABLE_GCC_WARNING("-Wfloat-equal")
ix_DISABLE_CLANG_WARNING_BEGIN
ix_DISABLE_CLANG_WARNING("-Wfloat-equal")

ix_FORCE_INLINE constexpr bool ix_is_equalf(float x, float y)
{
    return (x == y);
}

ix_FORCE_INLINE constexpr bool ix_is_equald(double x, double y)
{
    return (x == y);
}

ix_FORCE_INLINE constexpr bool ix_is_zerof(float x)
{
    return (x == 0.0F);
}

ix_FORCE_INLINE constexpr bool ix_is_zerod(double x)
{
    return (x == 0.0);
}

ix_DISABLE_GCC_WARNING_END
ix_DISABLE_CLANG_WARNING_END

// cf. https://zenn.dev/mod_poppo/articles/integer-division
template <typename T>
ix_FORCE_INLINE constexpr T ix_quot(T n, T d)
{
    return n / d;
}

template <typename T>
ix_FORCE_INLINE constexpr T ix_rem(T n, T d)
{
    return n % d;
}

template <typename T>
constexpr T ix_div(T n, T d)
{
    T quot = n / d;
    const bool diffrent_sign = ((n ^ d) < 0);
    const bool divisible = ((n % d) == 0);
    const bool needs_correction = diffrent_sign && !divisible;
    return needs_correction ? (quot - 1) : quot;
}

template <typename T>
constexpr T ix_mod(T n, T d)
{
    T rem = n % d;
    const bool diffrent_sign = ((n ^ d) < 0);
    const bool divisible = (rem == 0);
    const bool needs_correction = diffrent_sign && !divisible;
    return needs_correction ? (rem + d) : rem;
}
