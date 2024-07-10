#pragma once

#include "ix.hpp"
#include "ix_HashSet.hpp"
#include "ix_limits.hpp"
#include "ix_random.hpp"

template <typename T>
void ix_shuffle(T *data, size_t length);

template <typename T>
bool ix_is_unique(T *data, size_t length);

// cf. https://en.wikipedia.org/wiki/Fisher%E2%80%93Yates_shuffle#The_modern_algorithm
template <typename T>
void ix_shuffle(T *data, size_t length)
{
    for (size_t i = length - 1; i != 0; i--)
    {
        const size_t j = ix_rand<size_t>() % i;
        ix_swap(data[i], data[j]);
    }
}

template <typename T>
bool ix_is_unique(T *data, size_t length)
{
    ix_HashSet<T> values;
    values.reserve(length);
    for (size_t i = 0; i < length; i++)
    {
        if (values.contains(data[i]))
        {
            return false;
        }
        values.insert(data[i]);
    }
    return true;
}
