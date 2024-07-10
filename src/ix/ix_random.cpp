#include "ix_random.hpp"
#include "ix_assert.hpp"
#include "ix_doctest.hpp"
#include "ix_environment.hpp"
#include "ix_hash.hpp"
#include "ix_limits.hpp"
#include "ix_type_traits.hpp"
#include "ix_wrap.hpp"

#include <time.h>

// From https://prng.di.unimi.it/xoshiro256starstar.c
// (by David Blackman and Sebastiano Vigna, public domain)
static uint64_t rotl(const uint64_t x, int k)
{
    return (x << k) | (x >> (64 - k));
}

ix_Xoshiro256PlusPlus::ix_Xoshiro256PlusPlus(uint64_t x)
    : m_state()
{
    set_seed(x);
}

void ix_Xoshiro256PlusPlus::set_random_seed()
{
    const uint64_t t =
        0xDeadBeefDeadBeefULL ^ static_cast<uint64_t>(time(nullptr)) ^ ix_process_id() ^ ix_thread_id();
    set_seed(t);
}

void ix_Xoshiro256PlusPlus::set_seed(uint64_t seed)
{
    // linear congruential generator
    constexpr uint64_t m = 1ULL << 31;
    constexpr uint64_t a = 1664525;
    constexpr uint64_t c = 1013904223;
    uint64_t x = seed;
    for (uint64_t i = 0; i < ix_LENGTH_OF(m_state); i++)
    {
        x = (a * x + c) % m;
        const uint64_t s1 = x;
        x = (a * x + c) % m;
        const uint64_t s2 = x;

        m_state[i] = (s1 << 32UL) | s2;
    }
}

uint64_t ix_Xoshiro256PlusPlus::rand()
{
    const uint64_t result = rotl(m_state[0] + m_state[3], 23) + m_state[0];

    const uint64_t t = m_state[1] << 17;

    m_state[2] ^= m_state[0];
    m_state[3] ^= m_state[1];
    m_state[1] ^= m_state[2];
    m_state[0] ^= m_state[3];

    m_state[2] ^= t;

    m_state[3] = rotl(m_state[3], 45);

    return result;
}

static thread_local ix_Xoshiro256PlusPlus g_generator(ix_hash64(91), ix_hash64(96), ix_hash64(60), ix_hash64(70));

void ix_rand_set_seed(uint64_t seed)
{
    g_generator.set_seed(seed);
}

void ix_rand_set_random_seed()
{
    g_generator.set_random_seed();
}

template <typename T>
T ix_rand()
{
    if constexpr (ix_is_same_v<T, float>)
    {
        // cf. https://stackoverflow.com/questions/52147419
        const uint32_t v = ix_rand<uint32_t>();
        constexpr uint32_t mask1 = 0x3F800000UL;
        constexpr uint32_t mask2 = 0x3FFFFFFFUL;
        const uint32_t masked = (v | mask1) & mask2;
        float one_to_two;
        ix_memcpy(&one_to_two, &masked, sizeof(float));
        return one_to_two - 1.0F;
    }
    else if constexpr (ix_is_same_v<T, double>)
    {
        // cf. https://stackoverflow.com/questions/52147419
        const uint64_t v = ix_rand<uint64_t>();
        constexpr uint64_t mask1 = 0x3FF0000000000000ULL;
        constexpr uint64_t mask2 = 0x3FFFFFFFFFFFFFFFULL;
        const uint64_t masked = (v | mask1) & mask2;
        double one_to_two;
        ix_memcpy(&one_to_two, &masked, sizeof(double));
        return one_to_two - 1.0;
    }
    else if constexpr (ix_is_same_v<T, uint64_t>)
    {
        return g_generator.rand();
    }
    else
    {
        T x;
        uint64_t v = g_generator.rand();
        ix_memcpy(&x, &v, sizeof(T));
        return x;
    }
}

ix_TEST_CASE("ix_rand")
{
    ix_rand_set_random_seed();
    for (uint64_t i = 0; i < 25; i++)
    {
        ix_rand<int8_t>();
        ix_rand<int16_t>();
        ix_rand<int32_t>();
        ix_rand<int64_t>();
        ix_rand<uint8_t>();
        ix_rand<uint16_t>();
        ix_rand<uint32_t>();
        ix_rand<uint64_t>();

        ix_EXPECT(0.0F <= ix_rand<float>());
        ix_EXPECT(ix_rand<float>() < 1.0F);

        ix_EXPECT(0.0 <= ix_rand<double>());
        ix_EXPECT(ix_rand<double>() < 1.0);
    }
}

template <typename T>
T ix_rand_non_negative()
{
    T x = ix_rand<T>();
    if constexpr (ix_numeric_limits<T>::is_signed)
    {
        x = (x < 0) ? -x : x;
        if (ix_LIKELY(x >= 0))
        {
            return x;
        }
        ix_ASSERT(x == ix_numeric_limits<T>::min());
        while (x < 0)
        {
            x = ix_rand<T>();
        }
        return x;
    }
    else
    {
        return x;
    }
}

ix_TEST_CASE("ix_rand_non_negative")
{
    for (size_t i = 0; i < 25; i++)
    {
        ix_EXPECT(0 <= ix_rand_non_negative<int8_t>());
        ix_EXPECT(0 <= ix_rand_non_negative<int16_t>());
        ix_EXPECT(0 <= ix_rand_non_negative<int32_t>());
        ix_EXPECT(0 <= ix_rand_non_negative<int64_t>());
    }

    for (size_t i = 0; i < 5000; i++)
    {
        ix_EXPECT(0 <= ix_rand_non_negative<int8_t>());
    }
}

template <typename T>
T ix_rand_range(T min, T max)
{
    ix_ASSERT(min <= max);
    if constexpr (ix_numeric_limits<T>::is_integer && !ix_numeric_limits<T>::is_signed)
    {
        const T x = ix_rand_non_negative<T>();
        const bool whole_range = (min == ix_numeric_limits<T>::min()) && (max == ix_numeric_limits<T>::max());
        if (whole_range)
        {
            return x;
        }
        const T range = static_cast<T>(max - min) + 1;
        return (x % range) + min;
    }
    else if constexpr (ix_numeric_limits<T>::is_integer)
    {
        const bool range_representable = ix_can_sub<T>(max, min) && ix_can_add<T>(max - min, 1);
        if (ix_LIKELY(range_representable))
        {
            const T range = static_cast<T>(max - min) + 1;
            return static_cast<T>(ix_rand_non_negative<T>() % range) + min;
        }

        // `max - min + 1` is not representable in T.
        // This means that there is at least 1/2 chance to get the right value by `ix_rand<T>`.
        T x = ix_rand<T>();
        while (x < min || max < x)
        {
            x = ix_rand<T>();
        }
        return x;
    }
    else
    {
        const T range = max - min;
        const T x = ix_rand<T>() * range;
        return x + min;
    }
}

ix_TEST_CASE("ix_rand_range")
{
    for (uint64_t i = 0; i < 25; i++)
    {
        ix_EXPECT(-2 <= ix_rand_range<int8_t>(-2, 2));
        ix_EXPECT(-2 <= ix_rand_range<int16_t>(-2, 2));
        ix_EXPECT(-2 <= ix_rand_range<int32_t>(-2, 2));
        ix_EXPECT(-2 <= ix_rand_range<int64_t>(-2, 2));
        ix_EXPECT(ix_rand_range<int8_t>(-2, 2) <= 2);
        ix_EXPECT(ix_rand_range<int16_t>(-2, 2) <= 2);
        ix_EXPECT(ix_rand_range<int32_t>(-2, 2) <= 2);
        ix_EXPECT(ix_rand_range<int64_t>(-2, 2) <= 2);

        ix_EXPECT(2 <= ix_rand_range<uint8_t>(2, 4));
        ix_EXPECT(2 <= ix_rand_range<uint16_t>(2, 4));
        ix_EXPECT(2 <= ix_rand_range<uint32_t>(2, 4));
        ix_EXPECT(2 <= ix_rand_range<uint64_t>(2, 4));
        ix_EXPECT(ix_rand_range<uint8_t>(2, 4) <= 4);
        ix_EXPECT(ix_rand_range<uint16_t>(2, 4) <= 4);
        ix_EXPECT(ix_rand_range<uint32_t>(2, 4) <= 4);
        ix_EXPECT(ix_rand_range<uint64_t>(2, 4) <= 4);

        ix_EXPECT(-2.0F <= ix_rand_range<float>(-2.0F, 2.0F));
        ix_EXPECT(ix_rand_range<float>(-2.0F, 2.0F) < 2.0F);

        ix_EXPECT(-2.0 <= ix_rand_range<double>(-2.0, 2.0));
        ix_EXPECT(ix_rand_range<double>(-2.0, 2.0) < 2.0);
    }

    ix_EXPECT(ix_rand_range<int8_t>(2, 2) == 2);
    ix_EXPECT(ix_rand_range<int16_t>(2, 2) == 2);
    ix_EXPECT(ix_rand_range<int32_t>(2, 2) == 2);
    ix_EXPECT(ix_rand_range<int64_t>(2, 2) == 2);
    ix_EXPECT(ix_rand_range<uint8_t>(2, 2) == 2);
    ix_EXPECT(ix_rand_range<uint16_t>(2, 2) == 2);
    ix_EXPECT(ix_rand_range<uint32_t>(2, 2) == 2);
    ix_EXPECT(ix_rand_range<uint64_t>(2, 2) == 2);
    ix_EXPECT(ix_rand_range<float>(2.0F, 2.0F) == doctest::ApproxF(2.0F));
    ix_EXPECT(ix_rand_range<double>(2.0, 2.0) == doctest::Approx(2.0));

    ix_EXPECT(ix_rand_range<int8_t>(0, ix_numeric_limits<int8_t>::max()) >= 0);
    ix_EXPECT(ix_rand_range<int16_t>(0, ix_numeric_limits<int16_t>::max()) >= 0);
    ix_EXPECT(ix_rand_range<int32_t>(0, ix_numeric_limits<int32_t>::max()) >= 0);
    ix_EXPECT(ix_rand_range<int64_t>(0, ix_numeric_limits<int64_t>::max()) >= 0);

    ix_rand_range<int8_t>(ix_numeric_limits<int8_t>::min(), ix_numeric_limits<int8_t>::max());
    ix_rand_range<int16_t>(ix_numeric_limits<int16_t>::min(), ix_numeric_limits<int16_t>::max());
    ix_rand_range<int32_t>(ix_numeric_limits<int32_t>::min(), ix_numeric_limits<int32_t>::max());
    ix_rand_range<int64_t>(ix_numeric_limits<int64_t>::min(), ix_numeric_limits<int64_t>::max());
}

ix_TEST_CASE("ix_rand_set_seed")
{
    ix_rand_set_seed(0);
    ix_EXPECT(ix_rand<uint64_t>() != 0);
    ix_EXPECT(ix_rand<uint64_t>() != 0);
    ix_EXPECT(ix_rand<uint64_t>() != 0);
    ix_EXPECT(ix_rand<uint64_t>() != 0);
    ix_EXPECT(ix_rand<uint64_t>() != 0);
    ix_rand_set_random_seed();
}

ix_TEST_CASE("ix_Xoshiro256PlusPlus: constructor")
{
    ix_Xoshiro256PlusPlus rng(0);
    rng = ix_Xoshiro256PlusPlus(0, 1, 2, 3);
    ix_EXPECT(rng.rand() != 0);
}

// clang-format off
template int8_t   ix_rand<int8_t>();
template int16_t  ix_rand<int16_t>();
template int32_t  ix_rand<int32_t>();
template int64_t  ix_rand<int64_t>();
template uint8_t  ix_rand<uint8_t>();
template uint16_t ix_rand<uint16_t>();
template uint32_t ix_rand<uint32_t>();
template uint64_t ix_rand<uint64_t>();
template float    ix_rand<float>();
template double   ix_rand<double>();

template int8_t   ix_rand_non_negative<int8_t>();
template int16_t  ix_rand_non_negative<int16_t>();
template int32_t  ix_rand_non_negative<int32_t>();
template int64_t  ix_rand_non_negative<int64_t>();
template uint8_t  ix_rand_non_negative<uint8_t>();
template uint16_t ix_rand_non_negative<uint16_t>();
template uint32_t ix_rand_non_negative<uint32_t>();
template uint64_t ix_rand_non_negative<uint64_t>();

template int8_t   ix_rand_range<int8_t>(int8_t, int8_t);
template int16_t  ix_rand_range<int16_t>(int16_t, int16_t);
template int32_t  ix_rand_range<int32_t>(int32_t, int32_t);
template int64_t  ix_rand_range<int64_t>(int64_t, int64_t);
template uint8_t  ix_rand_range<uint8_t>(uint8_t, uint8_t);
template uint16_t ix_rand_range<uint16_t>(uint16_t, uint16_t);
template uint32_t ix_rand_range<uint32_t>(uint32_t, uint32_t);
template uint64_t ix_rand_range<uint64_t>(uint64_t, uint64_t);
template float    ix_rand_range<float>(float, float);
template double   ix_rand_range<double>(double, double);
// clang-format on
