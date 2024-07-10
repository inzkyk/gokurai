#pragma once

#include "ix.hpp"
#include "ix_HashSet.hpp"
#include "ix_limits.hpp"
#include "ix_random.hpp"

void ix_rand_fill_alphanumeric(char *buf, size_t length);

template <typename T>
void ix_rand_fill_unique(T *buf, size_t length);

template <typename T>
void ix_rand_range_fill_unique(T *buf, size_t length, T min, T max);

template <typename T>
void ix_rand_fill_unique(T *buf, size_t length)
{
    if constexpr (ix_numeric_limits<T>::is_integer)
    {
        ix_rand_range_fill_unique(buf, length, ix_numeric_limits<T>::min(), ix_numeric_limits<T>::max());
    }
    else
    {
        ix_HashSet<T> values;
        values.reserve(length);
        for (size_t i = 0; i < length; i++)
        {
            T candidate = ix_rand<T>();
            while (ix_UNLIKELY(values.contains(candidate)))
            {
                candidate = ix_rand<T>();
            }
            values.insert(candidate);
            buf[i] = candidate;
        }
    }
}

// clang-format off
extern template void ix_rand_range_fill_unique<int8_t>(int8_t *, size_t, int8_t, int8_t);
extern template void ix_rand_range_fill_unique<int16_t>(int16_t *, size_t, int16_t, int16_t);
extern template void ix_rand_range_fill_unique<int32_t>(int32_t *, size_t, int32_t, int32_t);
extern template void ix_rand_range_fill_unique<int64_t>(int64_t *, size_t, int64_t, int64_t);
extern template void ix_rand_range_fill_unique<uint8_t>(uint8_t *, size_t, uint8_t, uint8_t);
extern template void ix_rand_range_fill_unique<uint16_t>(uint16_t *, size_t, uint16_t, uint16_t);
extern template void ix_rand_range_fill_unique<uint32_t>(uint32_t *, size_t, uint32_t, uint32_t);
extern template void ix_rand_range_fill_unique<uint64_t>(uint64_t *, size_t, uint64_t, uint64_t);
extern template void ix_rand_range_fill_unique<float>(float *, size_t, float, float);
extern template void ix_rand_range_fill_unique<double>(double *, size_t, double, double);
// clang-format on
