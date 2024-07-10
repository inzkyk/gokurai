#pragma once

#include "ix.hpp"

template <typename T>
int ix_popcount(T x);

extern template int ix_popcount(uint32_t);
extern template int ix_popcount(uint64_t);

#if ix_PLATFORM(WASM)
template <>
ix_FORCE_INLINE int ix_popcount(size_t x)
{
    return ix_popcount<uint32_t>(x);
}
#endif

template <typename T>
int ix_count_trailing_zeros(T x);

extern template int ix_count_trailing_zeros(uint32_t);
extern template int ix_count_trailing_zeros(uint64_t);

#if ix_PLATFORM(WASM)
template <>
ix_FORCE_INLINE int ix_count_trailing_zeros(size_t x)
{
    return ix_count_trailing_zeros<uint32_t>(x);
}
#endif

template <typename T>
int ix_count_leading_zeros(T x);

extern template int ix_count_leading_zeros(uint32_t);
extern template int ix_count_leading_zeros(uint64_t);

template <typename T>
T ix_ceil_2_power(T x);

extern template uint32_t ix_ceil_2_power(uint32_t);
extern template uint64_t ix_ceil_2_power(uint64_t);

#if ix_PLATFORM(WASM)
template <>
ix_FORCE_INLINE size_t ix_ceil_2_power(size_t x)
{
    return ix_ceil_2_power<uint32_t>(x);
}
#endif

uint32_t ix_bit_next_bit_set(uint32_t x);

template <typename T>
ix_FORCE_INLINE constexpr bool ix_is_power_of_2(T x)
{
    return (x != 0) && ((x & (x - 1)) == 0);
}
