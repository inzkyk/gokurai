#include "ix_bit.hpp"
#include "ix_assert.hpp"
#include "ix_doctest.hpp"
#include "ix_type_traits.hpp"

#if ix_COMPILER(MSVC)
#include <intrin.h>
#endif

template <typename T>
int ix_popcount(T x)
{
    if constexpr (ix_is_same_v<T, uint32_t>)
    {
#if ix_COMPILER(MSVC)
        return static_cast<int>(__popcnt(x));
#else
        return __builtin_popcount(x);
#endif
    }
    else if constexpr (ix_is_same_v<T, uint64_t>)
    {
#if ix_COMPILER(MSVC)
        return static_cast<int>(__popcnt64(x));
#else
        return __builtin_popcountll(x);
#endif
    }
    ix_UNREACHABLE();
}

template int ix_popcount(uint32_t);
template int ix_popcount(uint64_t);

ix_TEST_CASE("ix_popcount<uint32_t>")
{
    ix_EXPECT(ix_popcount<uint32_t>(0b0) == 0);
    ix_EXPECT(ix_popcount<uint32_t>(0b1) == 1);
    ix_EXPECT(ix_popcount<uint32_t>(0b11) == 2);
    ix_EXPECT(ix_popcount<uint32_t>(0b11001100) == 4);
    ix_EXPECT(ix_popcount<uint32_t>(ix_UINT32_MAX) == 32);
}

ix_TEST_CASE("ix_popcount<uint64_t>")
{
    ix_EXPECT(ix_popcount<uint64_t>(0b0) == 0);
    ix_EXPECT(ix_popcount<uint64_t>(0b1) == 1);
    ix_EXPECT(ix_popcount<uint64_t>(0b11) == 2);
    ix_EXPECT(ix_popcount<uint64_t>(0b11001100) == 4);
    ix_EXPECT(ix_popcount<uint64_t>(ix_UINT32_MAX) == 32);
}

template <typename T>
int ix_count_trailing_zeros(T x)
{
    if constexpr (ix_is_same_v<T, uint32_t>)
    {
#if ix_COMPILER(MSVC)
        unsigned long ctz = 0;
        _BitScanForward(&ctz, x);
        return ctz;
#else
        return __builtin_ctz(x);
#endif
    }
    else if constexpr (ix_is_same_v<T, uint64_t>)
    {
#if ix_COMPILER(MSVC)
        unsigned long ctz = 0;
        _BitScanForward64(&ctz, x);
        return ctz;
#else
        return __builtin_ctzll(x);
#endif
    }

    ix_UNREACHABLE();
}

template int ix_count_trailing_zeros(uint32_t);
template int ix_count_trailing_zeros(uint64_t);

ix_TEST_CASE("ix_count_trailing_zeros<uint32_t>")
{
    ix_EXPECT(ix_count_trailing_zeros<uint32_t>(0b1) == 0);
    ix_EXPECT(ix_count_trailing_zeros<uint32_t>(0b10) == 1);
    ix_EXPECT(ix_count_trailing_zeros<uint32_t>(0b100) == 2);
    ix_EXPECT(ix_count_trailing_zeros<uint32_t>(0b100100) == 2);
    ix_EXPECT(ix_count_trailing_zeros<uint32_t>(0b10000000000) == 10);
    ix_EXPECT(ix_count_trailing_zeros<uint32_t>(0b10000000000010000000000) == 10);
    ix_EXPECT(ix_count_trailing_zeros<uint32_t>(0b11111111111110000000000) == 10);
}

ix_TEST_CASE("ix_count_trailing_zeros<uint64_t>")
{
    ix_EXPECT(ix_count_trailing_zeros<uint64_t>(0b1ULL) == 0);
    ix_EXPECT(ix_count_trailing_zeros<uint64_t>(0b10ULL) == 1);
    ix_EXPECT(ix_count_trailing_zeros<uint64_t>(0b100ULL) == 2);
    ix_EXPECT(ix_count_trailing_zeros<uint64_t>(0b100100ULL) == 2);
    ix_EXPECT(ix_count_trailing_zeros<uint64_t>(0b10000000000ULL) == 10);
    ix_EXPECT(ix_count_trailing_zeros<uint64_t>(0b10000000000010000000000ULL) == 10);
    ix_EXPECT(ix_count_trailing_zeros<uint64_t>(0b11111111111110000000000ULL) == 10);
}

template <typename T>
int ix_count_leading_zeros(T x)
{
    if constexpr (ix_is_same_v<T, uint32_t>)
    {
#if ix_COMPILER(MSVC)
        unsigned long ctz = 0;
        _BitScanReverse(&ctz, x);
        return 31 - ctz;
#else
        return __builtin_clz(x);
#endif
    }
    else if constexpr (ix_is_same_v<T, uint64_t>)
    {
#if ix_COMPILER(MSVC)
        unsigned long ctz = 0;
        _BitScanReverse64(&ctz, x);
        return 63 - ctz;
#else
        return __builtin_clzll(x);
#endif
    }

    ix_UNREACHABLE();
}

template int ix_count_leading_zeros(uint32_t);
template int ix_count_leading_zeros(uint64_t);

ix_TEST_CASE("ix_count_leading_zeros<uint32_t>")
{
    ix_EXPECT(ix_count_leading_zeros<uint32_t>(1) == 31);
    ix_EXPECT(ix_count_leading_zeros<uint32_t>(2) == 30);
    ix_EXPECT(ix_count_leading_zeros<uint32_t>(3) == 30);
    ix_EXPECT(ix_count_leading_zeros<uint32_t>(0b00010000001110011110000000000000) == 3);
    ix_EXPECT(ix_count_leading_zeros<uint32_t>(0b00000000000000111111000111111110) == 14);
    ix_EXPECT(ix_count_leading_zeros<uint32_t>(0b00000000000000010000000000000000) == 15);
    ix_EXPECT(ix_count_leading_zeros<uint32_t>(ix_UINT32_MAX) == 0);
    ix_EXPECT(ix_count_leading_zeros<uint32_t>(ix_UINT16_MAX) == 16);
    ix_EXPECT(ix_count_leading_zeros<uint32_t>(ix_UINT8_MAX) == 24);
}

ix_TEST_CASE("ix_count_leading_zeros<uint64_t>")
{
    ix_EXPECT(ix_count_leading_zeros<uint64_t>(1) == 63);
    ix_EXPECT(ix_count_leading_zeros<uint64_t>(2) == 62);
    ix_EXPECT(ix_count_leading_zeros<uint64_t>(3) == 62);
    ix_EXPECT(ix_count_leading_zeros<uint64_t>(0b00010000001110011110000000000000) == 35);
    ix_EXPECT(ix_count_leading_zeros<uint64_t>(0b00000000000000111111000111111110) == 46);
    ix_EXPECT(ix_count_leading_zeros<uint64_t>(0b00000000000000010000000000000000) == 47);
    ix_EXPECT(ix_count_leading_zeros<uint64_t>(ix_UINT64_MAX) == 0);
    ix_EXPECT(ix_count_leading_zeros<uint64_t>(ix_UINT32_MAX) == 32);
    ix_EXPECT(ix_count_leading_zeros<uint64_t>(ix_UINT16_MAX) == 48);
    ix_EXPECT(ix_count_leading_zeros<uint64_t>(ix_UINT8_MAX) == 56);
}

// By Sean Eron Anderson (public domain)
// From http://graphics.stanford.edu/~seander/bithacks.html
uint32_t ix_bit_next_bit_set(uint32_t x)
{
    const uint32_t t = x | (x - 1); // t gets x's least significant 0 bits set to 1

    // Next set to 1 the most significant bit to change,
    // set to 0 the least significant ones, and add the necessary 1 bits.
    const uint32_t nt = ~t;
#if ix_COMPILER(MSVC)
    const uint32_t mnt = ~nt + 1;
#else
    const uint32_t mnt = -nt;
#endif
    return (t + 1) | (((nt & mnt) - 1) >> (ix_count_trailing_zeros(x) + 1));
}

ix_TEST_CASE("ix_bit_next_bit_set")
{
    // 1
    {
        uint32_t x = 1;
        for (uint32_t i = 1; i < 32; i++)
        {
            x = ix_bit_next_bit_set(x);
            ix_EXPECT(x == (1UL << i));
        }
    }

    // 0b1111
    {
        // clang-format off
        uint32_t x = 0b001111;
        x = ix_bit_next_bit_set(x); ix_EXPECT(x == 0b010111);
        x = ix_bit_next_bit_set(x); ix_EXPECT(x == 0b011011);
        x = ix_bit_next_bit_set(x); ix_EXPECT(x == 0b011101);
        x = ix_bit_next_bit_set(x); ix_EXPECT(x == 0b011110);
        x = ix_bit_next_bit_set(x); ix_EXPECT(x == 0b100111);
        x = ix_bit_next_bit_set(x); ix_EXPECT(x == 0b101011);
        x = ix_bit_next_bit_set(x); ix_EXPECT(x == 0b101101);
        x = ix_bit_next_bit_set(x); ix_EXPECT(x == 0b101110);
        x = ix_bit_next_bit_set(x); ix_EXPECT(x == 0b110011);
        x = ix_bit_next_bit_set(x); ix_EXPECT(x == 0b110101);
        x = ix_bit_next_bit_set(x); ix_EXPECT(x == 0b110110);
        x = ix_bit_next_bit_set(x); ix_EXPECT(x == 0b111001);
        x = ix_bit_next_bit_set(x); ix_EXPECT(x == 0b111010);
        x = ix_bit_next_bit_set(x); ix_EXPECT(x == 0b111100);
        // clang-format on
    }
}

template <typename T>
T ix_ceil_2_power(T x)
{
    if constexpr (ix_is_same_v<T, uint32_t>)
    {
        if (ix_popcount(x) <= 1)
        {
            return x;
        }
        const int i = ix_count_leading_zeros(x);
        return 1 << (32 - i);
    }
    else if constexpr (ix_is_same_v<T, uint64_t>)
    {
        if (ix_popcount(x) <= 1)
        {
            return x;
        }
        const int i = ix_count_leading_zeros(x);
        return 1ULL << (64 - i);
    }

    ix_UNREACHABLE();
}

template uint32_t ix_ceil_2_power(uint32_t);
template uint64_t ix_ceil_2_power(uint64_t);

ix_TEST_CASE("ix_ceil_2_power<uint32_t>")
{
    ix_EXPECT(ix_ceil_2_power<uint32_t>(1) == 1);
    ix_EXPECT(ix_ceil_2_power<uint32_t>(2) == 2);
    ix_EXPECT(ix_ceil_2_power<uint32_t>(3) == 4);
    ix_EXPECT(ix_ceil_2_power<uint32_t>(24) == 32);
    ix_EXPECT(ix_ceil_2_power<uint32_t>(500) == 512);
    ix_EXPECT(ix_ceil_2_power<uint32_t>(1024) == 1024);
    ix_EXPECT(ix_ceil_2_power<uint32_t>(50302) == 65536);
    ix_EXPECT(ix_ceil_2_power<uint32_t>((1U << 30) + 1) == 1U << 31);
}

ix_TEST_CASE("ix_ceil_2_power<uint64_t>")
{
    ix_EXPECT(ix_ceil_2_power<uint64_t>(1) == 1);
    ix_EXPECT(ix_ceil_2_power<uint64_t>(2) == 2);
    ix_EXPECT(ix_ceil_2_power<uint64_t>(3) == 4);
    ix_EXPECT(ix_ceil_2_power<uint64_t>(6) == 8);
    ix_EXPECT(ix_ceil_2_power<uint64_t>(24) == 32);
    ix_EXPECT(ix_ceil_2_power<uint64_t>(500) == 512);
    ix_EXPECT(ix_ceil_2_power<uint64_t>(1024) == 1024);
    ix_EXPECT(ix_ceil_2_power<uint64_t>(50302) == 65536);
    ix_EXPECT(ix_ceil_2_power<uint64_t>((1ULL << 30) + 1) == 1ULL << 31);
    ix_EXPECT(ix_ceil_2_power<uint64_t>((1ULL << 50) + 1) == 1ULL << 51);
}

ix_TEST_CASE("ix_is_power_of_2")
{
    ix_EXPECT(ix_is_power_of_2(1));
    ix_EXPECT(ix_is_power_of_2(2));
    ix_EXPECT(ix_is_power_of_2(4));
    ix_EXPECT(ix_is_power_of_2(1024));
    ix_EXPECT(!ix_is_power_of_2(0));
    ix_EXPECT(!ix_is_power_of_2(3));
    ix_EXPECT(!ix_is_power_of_2(9832));
    ix_EXPECT(!ix_is_power_of_2(3893312413));
}
