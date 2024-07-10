#pragma once

#include "ix.hpp"
#include "ix_initializer_list.hpp"

template <typename T>
class ix_Span
{
    T *m_data = nullptr;
    size_t m_size = 0;

  public:
    ix_FORCE_INLINE ix_Span() = default;

    template <size_t N>
    ix_FORCE_INLINE explicit ix_Span(T (&data)[N])
        : m_data(data),
          m_size(N)
    {
    }

    ix_FORCE_INLINE ix_Span(T *data, size_t len)
        : m_data(data),
          m_size(len)
    {
    }

    ix_DISABLE_GCC_WARNING_BEGIN
    ix_DISABLE_GCC_WARNING("-Winit-list-lifetime")

    // cppcheck-suppress noExplicitConstructor
    ix_FORCE_INLINE ix_Span(std::initializer_list<T> &&list)
        : m_data(const_cast<T *>(list.begin())),
          m_size(list.size())
    {
    }

    ix_DISABLE_GCC_WARNING_END

    ix_FORCE_INLINE T *data() const
    {
        return m_data;
    }

    ix_FORCE_INLINE size_t size() const
    {
        return m_size;
    }

    ix_FORCE_INLINE const T *begin() const
    {
        return m_data;
    }

    ix_FORCE_INLINE const T *end() const
    {
        return m_data + m_size;
    }

    ix_FORCE_INLINE T *begin()
    {
        return m_data;
    }

    ix_FORCE_INLINE T *end()
    {
        return m_data + m_size;
    }

    ix_FORCE_INLINE T &operator[](size_t i)
    {
        return m_data[i];
    }

    ix_FORCE_INLINE const T &operator[](size_t i) const
    {
        return m_data[i];
    }

    bool contains(const T &value) const
    {
        for (const T &candidate : *this)
        {
            if (candidate == value)
            {
                return true;
            }
        }
        return false;
    }

    bool operator==(const ix_Span &other) const
    {
        if (m_size != other.m_size)
        {
            return false;
        }

        for (size_t i = 0; i < m_size; i++)
        {
            if (m_data[i] != other.m_data[i])
            {
                return false;
            }
        }

        return true;
    }

    ix_FORCE_INLINE bool operator!=(const ix_Span &other) const
    {
        return !(*this == other);
    }
};
