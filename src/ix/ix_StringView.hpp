#pragma once

#include "ix.hpp"
#include "ix_hash.hpp"

class ix_StringView
{
    const char *m_data = nullptr;
    size_t m_length = 0;

  public:
    ix_FORCE_INLINE constexpr ix_StringView() = default;

    ix_FORCE_INLINE constexpr ix_StringView(const char *data, size_t length)
        : m_data(data),
          m_length(length)
    {
    }

    ix_FORCE_INLINE constexpr explicit ix_StringView(const char *text)
        : m_data(text),
          m_length(ix_strlen(text))
    {
    }

    ix_FORCE_INLINE constexpr ix_StringView(const char *data, const char *end)
        : m_data(data),
          m_length(static_cast<size_t>(end - m_data))
    {
    }

    ix_FORCE_INLINE constexpr const char *data() const
    {
        return m_data;
    }

    ix_FORCE_INLINE constexpr size_t length() const
    {
        return m_length;
    }

    ix_FORCE_INLINE constexpr size_t hash() const
    {
        return ix_hash(m_data, m_length);
    }

    ix_FORCE_INLINE constexpr bool operator==(const ix_StringView &other) const
    {
        if (m_length != other.length())
        {
            return false;
        }

        for (size_t i = 0; i < m_length; i++)
        {
            if (m_data[i] != other.m_data[i])
            {
                return false;
            }
        }

        return true;
    }

    ix_FORCE_INLINE constexpr bool operator!=(const ix_StringView &other) const
    {
        return !(*this == other);
    }

    constexpr bool operator==(const char *other) const
    {
        if (*other == '\0' && m_length != 0)
        {
            return false;
        }

        if (*other != '\0' && m_length == 0)
        {
            return false;
        }

        for (size_t i = 0; i < m_length; i++)
        {
            if (other[i] == '\0')
            {
                return false;
            }

            if (m_data[i] != other[i])
            {
                return false;
            }
        }

        return true;
    }

    ix_FORCE_INLINE constexpr bool operator!=(const char *other) const
    {
        return !(*this == other);
    }
};

ix_FORCE_INLINE constexpr bool operator==(const char *lhs, const ix_StringView &rhs)
{
    return (rhs == lhs);
}

ix_FORCE_INLINE constexpr bool operator!=(const char *lhs, const ix_StringView &rhs)
{
    return (rhs != lhs);
}

template <>
struct ix_Hash<ix_StringView>
{
    ix_FORCE_INLINE constexpr size_t operator()(const ix_StringView &x) const
    {
        return x.hash();
    }
};

namespace doctest
{
class String;
} // namespace doctest

doctest::String toString(const ix_StringView &sv);
