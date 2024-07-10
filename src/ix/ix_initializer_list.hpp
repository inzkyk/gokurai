#pragma once

#include "ix.hpp"

#if ix_PLATFORM(WASM)

#include <initializer_list>

#elif ix_COMPILER(GCC)

namespace std
{

template <typename T>
class initializer_list // NOLINT (cert-dcl58-cpp)
{
    const T *m_begin;
    size_t m_length;

  public:
    ix_FORCE_INLINE constexpr initializer_list()
        : m_begin(nullptr),
          m_length(0)
    {
    }

    ix_FORCE_INLINE constexpr initializer_list(const T *begin, size_t length)
        : m_begin(begin),
          m_length(length)
    {
    }

    ix_FORCE_INLINE constexpr const T *begin() const
    {
        return m_begin;
    }

    ix_FORCE_INLINE constexpr const T *end() const
    {
        return m_begin + m_length;
    }

    ix_FORCE_INLINE constexpr size_t size() const
    {
        return m_length;
    }
};

template <typename T>
ix_FORCE_INLINE constexpr const T *begin(initializer_list<T> list) // NOLINT (cert-dcl58-cpp)
{
    return list.begin();
}

template <typename T>
ix_FORCE_INLINE constexpr const T *end(initializer_list<T> list) // NOLINT (cert-dcl58-cpp)
{
    return list.end();
}

} // namespace std

#elif ix_COMPILER(MSVC) || ix_COMPILER(CLANG)

namespace std
{

template <typename T>
class initializer_list // NOLINT (cert-dcl58-cpp)
{
    const T *m_begin;
    const T *m_end;

  public:
    ix_FORCE_INLINE constexpr initializer_list()
        : m_begin(nullptr),
          m_end(nullptr)
    {
    }

#if ix_COMPILER(MSVC)
    ix_FORCE_INLINE constexpr initializer_list(const T *begin, const T *end)
        : m_begin(begin),
          m_end(end)
    {
    }
#else
    ix_FORCE_INLINE constexpr initializer_list(const T *begin, size_t length)
        : m_begin(begin),
          m_end(begin + length)
    {
    }
#endif

    ix_FORCE_INLINE constexpr const T *begin() const
    {
        return m_begin;
    }

    ix_FORCE_INLINE constexpr const T *end() const
    {
        return m_end;
    }

    ix_FORCE_INLINE constexpr size_t size() const
    {
        return static_cast<size_t>(m_end - m_begin);
    }
};

template <typename T>
ix_FORCE_INLINE constexpr const T *begin(initializer_list<T> list) // NOLINT (cert-dcl58-cpp)
{
    return list.begin();
}

template <typename T>
ix_FORCE_INLINE constexpr const T *end(initializer_list<T> list) // NOLINT (cert-dcl58-cpp)
{
    return list.end();
}

} // namespace std

#else
#error "Unknown compiler"
#endif
