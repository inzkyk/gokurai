#pragma once

#include "ix.hpp"
#include "ix_assert.hpp"

template <typename T, size_t N>
struct ix_Array
{
    T m_data[N];

    ix_FORCE_INLINE constexpr size_t size() const
    {
        return N;
    }

    ix_FORCE_INLINE constexpr T *data()
    {
        return m_data;
    }

    ix_FORCE_INLINE constexpr const T *data() const
    {
        return m_data;
    }

    ix_FORCE_INLINE constexpr T &operator[](size_t i)
    {
        ix_ASSERT(i < N);
        return m_data[i];
    }

    ix_FORCE_INLINE constexpr const T &operator[](size_t i) const
    {
        ix_ASSERT(i < N);
        return m_data[i];
    }

    ix_FORCE_INLINE constexpr T *begin()
    {
        return m_data;
    }

    ix_FORCE_INLINE constexpr const T *begin() const
    {
        return m_data;
    }

    ix_FORCE_INLINE constexpr T *end()
    {
        return m_data + N;
    }

    ix_FORCE_INLINE constexpr const T *end() const
    {
        return m_data + N;
    }
};

template <typename T>
struct ix_Array<T, 0>
{
    ix_FORCE_INLINE constexpr size_t size() const
    {
        return 0;
    }

    ix_FORCE_INLINE constexpr T *data()
    {
        return nullptr;
    }

    ix_FORCE_INLINE constexpr const T *data() const
    {
        return nullptr;
    }

    ix_FORCE_INLINE constexpr T *begin()
    {
        return nullptr;
    }

    ix_FORCE_INLINE constexpr const T *begin() const
    {
        return nullptr;
    }

    ix_FORCE_INLINE constexpr T *end()
    {
        return nullptr;
    }

    ix_FORCE_INLINE constexpr const T *end() const
    {
        return nullptr;
    }
};
