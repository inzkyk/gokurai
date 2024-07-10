#include "ix_random_fill.hpp"
#include "ix_combinatorics.hpp"
#include "ix_doctest.hpp"

void ix_rand_fill_alphanumeric(char *buf, size_t length)
{
    constexpr const char alphanumric[] = "abndefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
    for (size_t i = 0; i < length; i++)
    {
        const size_t idx = ix_rand<size_t>() % (ix_LENGTH_OF(alphanumric) - 1);
        buf[i] = static_cast<char>(alphanumric[idx]);
    }
    buf[length] = '\0';
}

ix_TEST_CASE("ix_rand_fill_alphanumeric")
{
    char buf[10 + 1];
    ix_rand_fill_alphanumeric(buf, 10);
    ix_EXPECT(ix_strlen(buf) == 10);

    ix_rand_fill_alphanumeric(buf, 5);
    ix_EXPECT(ix_strlen(buf) == 5);

    ix_rand_fill_alphanumeric(buf, 0);
    ix_EXPECT(ix_strlen(buf) == 0);
}

template <typename T>
void ix_rand_range_fill_unique(T *buf, size_t length, T min, T max)
{
    ix_ASSERT_FATAL(min < max);
    if constexpr (ix_numeric_limits<T>::is_integer)
    {
        const size_t range = static_cast<size_t>(max) - static_cast<size_t>(min);
        ix_ASSERT_FATAL(length - 1 <= range);

        const bool use_direct_pick = (range <= 2 * length);
        if (use_direct_pick)
        {
            ix_Vector<T> values;
            ix_ASSERT(range < ix_numeric_limits<size_t>::max()); // Check against overflow.
            values.reserve(range + 1);
            values.set_size(range + 1);
            T value = min;
            for (size_t i = 0; i <= range; i++)
            {
                values[i] = value;
                value += 1;
            }
            ix_ASSERT(value == static_cast<T>(max + 1));

            for (size_t i = 0; i < length; i++)
            {
                const size_t index = ix_rand<size_t>() % (range + 1 - i);
                buf[i] = values[index];
                values.swap_erase(index);
            }
            return;
        }
    }

    ix_HashSet<T> values;
    values.reserve(length);
    for (size_t i = 0; i < length; i++)
    {
        T candidate = ix_rand_range(min, max);
        while (ix_UNLIKELY(values.contains(candidate)))
        {
            candidate = ix_rand_range(min, max);
        }
        values.insert(candidate);
        buf[i] = candidate;
    }
}

ix_TEST_CASE("ix_rand_range_fill_unique")
{
    for (size_t _ = 0; _ < 10; _++)
    {
        constexpr size_t N = 100;
        constexpr uint64_t min = 200;
        constexpr uint64_t max = 20000;
        uint64_t buf[N];
        ix_rand_range_fill_unique<uint64_t>(buf, N, min, max);
        ix_EXPECT(ix_is_unique(buf, N));
        for (size_t i = 0; i < N; i++)
        {
            ix_EXPECT(min <= buf[i]);
            ix_EXPECT(buf[i] <= max);
        }
    }

    for (size_t _ = 0; _ < 10; _++)
    {
        constexpr size_t N = 127;
        constexpr uint8_t min = 10;
        constexpr uint8_t max = 255;
        uint8_t buf[N];
        ix_rand_range_fill_unique<uint8_t>(buf, N, min, max);
        ix_EXPECT(ix_is_unique(buf, N));
        for (size_t i = 0; i < N; i++)
        {
            ix_EXPECT(min <= buf[i]);
            ix_EXPECT(buf[i] <= max);
        }
    }

    for (size_t _ = 0; _ < 10; _++)
    {
        constexpr size_t N = 10;
        constexpr uint8_t min = 11;
        constexpr uint8_t max = 20;
        uint8_t buf[N];
        ix_rand_range_fill_unique<uint8_t>(buf, N, min, max);
        ix_EXPECT(ix_is_unique(buf, N));
        for (size_t i = 0; i < N; i++)
        {
            ix_EXPECT(min <= buf[i]);
            ix_EXPECT(buf[i] <= max);
        }
    }

    {
        constexpr size_t N = 100;
        constexpr uint64_t min = 11;
        constexpr uint64_t max = 110;
        uint64_t buf[N];
        ix_rand_range_fill_unique<uint64_t>(buf, N, min, max);
        ix_EXPECT(ix_is_unique(buf, N));
        for (size_t i = 0; i < N; i++)
        {
            ix_EXPECT(min <= buf[i]);
            ix_EXPECT(buf[i] <= max);
        }
    }

    for (size_t _ = 0; _ < 10; _++)
    {
        constexpr size_t N = 100;
        constexpr float min = 10.0F;
        constexpr float max = 100.0F;
        float buf[N];
        ix_rand_range_fill_unique<float>(buf, N, min, max);
        ix_EXPECT(ix_is_unique(buf, N));
        for (size_t i = 0; i < N; i++)
        {
            ix_EXPECT(min <= buf[i]);
            ix_EXPECT(buf[i] <= max);
        }
    }
}

ix_TEST_CASE("ix_rand_fill_unique")
{
    {
        constexpr size_t N = 100;
        uint64_t buf[N];
        ix_rand_fill_unique(buf, N);
        ix_EXPECT(ix_is_unique(buf, N));
    }

    {
        constexpr size_t N = 256;
        uint8_t buf[N];
        ix_rand_fill_unique(buf, N);
        ix_is_unique(buf, N);
        ix_EXPECT(ix_is_unique(buf, N));
    }
}

// clang-format off
template void ix_rand_range_fill_unique<int8_t>(int8_t *, size_t, int8_t, int8_t);
template void ix_rand_range_fill_unique<int16_t>(int16_t *, size_t, int16_t, int16_t);
template void ix_rand_range_fill_unique<int32_t>(int32_t *, size_t, int32_t, int32_t);
template void ix_rand_range_fill_unique<int64_t>(int64_t *, size_t, int64_t, int64_t);
template void ix_rand_range_fill_unique<uint8_t>(uint8_t *, size_t, uint8_t, uint8_t);
template void ix_rand_range_fill_unique<uint16_t>(uint16_t *, size_t, uint16_t, uint16_t);
template void ix_rand_range_fill_unique<uint32_t>(uint32_t *, size_t, uint32_t, uint32_t);
template void ix_rand_range_fill_unique<uint64_t>(uint64_t *, size_t, uint64_t, uint64_t);
template void ix_rand_range_fill_unique<float>(float *, size_t, float, float);
template void ix_rand_range_fill_unique<double>(double *, size_t, double, double);
// clang-format on
