#pragma once

#include "ix.hpp"

#if ix_OPT_LEVEL(DEBUG)
#define ix_DEBUG_RANDOM_BRANCH(p) (ix_rand<double>() < (p))
#else
#define ix_DEBUG_RANDOM_BRANCH(p) true
#endif

// From https://prng.di.unimi.it/xoshiro256plusplus.c
// (by David Blackman and Sebastiano Vigna, public domain)
class ix_Xoshiro256PlusPlus
{
    uint64_t m_state[4];

  public:
    constexpr ix_Xoshiro256PlusPlus(uint64_t a, uint64_t b, uint64_t c, uint64_t d)
        : m_state{a, b, c, d}
    {
    }

    explicit ix_Xoshiro256PlusPlus(uint64_t x);
    void set_random_seed();
    void set_seed(uint64_t seed);
    uint64_t rand();
};

void ix_rand_set_seed(uint64_t seed);
void ix_rand_set_random_seed();

template <typename T>
T ix_rand();

template <typename T>
T ix_rand_non_negative();

template <typename T>
T ix_rand_range(T min, T max);

// clang-format off
extern template int8_t   ix_rand<int8_t>();
extern template int16_t  ix_rand<int16_t>();
extern template int32_t  ix_rand<int32_t>();
extern template int64_t  ix_rand<int64_t>();
extern template uint8_t  ix_rand<uint8_t>();
extern template uint16_t ix_rand<uint16_t>();
extern template uint32_t ix_rand<uint32_t>();
extern template uint64_t ix_rand<uint64_t>();
extern template float    ix_rand<float>();
extern template double   ix_rand<double>();

#if ix_PLATFORM(WASM)
template <>
ix_FORCE_INLINE size_t ix_rand()
{
    return ix_rand<uint32_t>();
}
#endif

extern template int8_t   ix_rand_non_negative<int8_t>();
extern template int16_t  ix_rand_non_negative<int16_t>();
extern template int32_t  ix_rand_non_negative<int32_t>();
extern template int64_t  ix_rand_non_negative<int64_t>();
extern template uint8_t  ix_rand_non_negative<uint8_t>();  // = ix_rand()
extern template uint16_t ix_rand_non_negative<uint16_t>(); // = ix_rand()
extern template uint32_t ix_rand_non_negative<uint32_t>(); // = ix_rand()
extern template uint64_t ix_rand_non_negative<uint64_t>(); // = ix_rand()

#if ix_PLATFORM(WASM)
template <>
ix_FORCE_INLINE size_t ix_rand_non_negative()
{
    return ix_rand<uint32_t>();
}
#endif

extern template int8_t   ix_rand_range<int8_t>(int8_t, int8_t);
extern template int16_t  ix_rand_range<int16_t>(int16_t, int16_t);
extern template int32_t  ix_rand_range<int32_t>(int32_t, int32_t);
extern template int64_t  ix_rand_range<int64_t>(int64_t, int64_t);
extern template uint8_t  ix_rand_range<uint8_t>(uint8_t, uint8_t);
extern template uint16_t ix_rand_range<uint16_t>(uint16_t, uint16_t);
extern template uint32_t ix_rand_range<uint32_t>(uint32_t, uint32_t);
extern template uint64_t ix_rand_range<uint64_t>(uint64_t, uint64_t);
extern template float    ix_rand_range<float>(float, float);
extern template double   ix_rand_range<double>(double, double);

#if ix_PLATFORM(WASM)
template <>
ix_FORCE_INLINE size_t ix_rand_range(size_t min, size_t max)
{
    return ix_rand_range<uint32_t>(min, max);
}
#endif
// clang-format on
