#pragma once

#include "ix.hpp"
#include "ix_new_delete.hpp"

// Use `free()` to release the pointer, assuming that it is allocated by `malloc()`, not `new T(...)`.
template <typename T>
class ix_UniquePointer;

template <typename T, typename... Args>
ix_FORCE_INLINE ix_UniquePointer<T> ix_make_unique(Args &&...args)
{
    return ix_UniquePointer<T>(ix_new<T>(ix_forward<Args>(args)...));
}

template <typename T>
ix_FORCE_INLINE ix_UniquePointer<T[]> ix_make_unique_array(size_t n)
{
    return ix_UniquePointer<T[]>(ix_ALLOC_ARRAY(T, n));
}

template <typename T>
class ix_UniquePointer
{
    T *m_ptr;

  public:
    ix_FORCE_INLINE explicit ix_UniquePointer(T *&&ptr)
        : m_ptr(ptr)
    {
    }

    ix_FORCE_INLINE ~ix_UniquePointer()
    {
        if (m_ptr == nullptr)
        {
            return;
        }

        ix_delete(m_ptr);
    }

    ix_UniquePointer(const ix_UniquePointer &) = delete;
    ix_UniquePointer &operator=(const ix_UniquePointer &) = delete;

    ix_UniquePointer(ix_UniquePointer &&other) noexcept
        : m_ptr(other.m_ptr)
    {
        other.m_ptr = nullptr;
    }

    // cppcheck-suppress operatorEqVarError
    ix_UniquePointer &operator=(ix_UniquePointer &&other) noexcept
    {
        if (this == &other)
        {
            return *this;
        }

        if (m_ptr != nullptr)
        {
            m_ptr->~T();
            ix_FREE(m_ptr);
        }

        m_ptr = other.m_ptr;
        other.m_ptr = nullptr;
        return *this;
    }

    ix_FORCE_INLINE void swap(T *&&p)
    {
        if (m_ptr != nullptr)
        {
            m_ptr->~T();
            ix_FREE(m_ptr);
        }

        m_ptr = p;
    }

    ix_FORCE_INLINE T *detach()
    {
        T *p = m_ptr;
        m_ptr = nullptr;
        return p;
    }

    ix_FORCE_INLINE T *get()
    {
        return m_ptr;
    }

    ix_FORCE_INLINE const T *get() const
    {
        return m_ptr;
    }

    ix_FORCE_INLINE T &operator*()
    {
        return *m_ptr; // NOLINT(clang-analyzer-core.uninitialized.UndefReturn)
    }

    ix_FORCE_INLINE const T &operator*() const
    {
        return *m_ptr; // NOLINT(clang-analyzer-core.uninitialized.UndefReturn)
    }

    ix_FORCE_INLINE T *operator->()
    {
        return m_ptr;
    }

    ix_FORCE_INLINE const T *operator->() const
    {
        return m_ptr;
    }
};

template <typename T>
class ix_UniquePointer<T[]>
{
    static_assert(ix_is_trivially_destructible_v<T>);
    T *m_ptr;

  public:
    ix_FORCE_INLINE explicit ix_UniquePointer(T *&&ptr)
        : m_ptr(ptr)
    {
    }

    ix_FORCE_INLINE ~ix_UniquePointer()
    {
        ix_FREE(m_ptr);
    }

    ix_UniquePointer(const ix_UniquePointer &) = delete;
    ix_UniquePointer &operator=(const ix_UniquePointer &) = delete;

    ix_UniquePointer(ix_UniquePointer &&other) noexcept
        : m_ptr(other.m_ptr)
    {
        other.m_ptr = nullptr;
    }

    // cppcheck-suppress operatorEqVarError
    ix_UniquePointer &operator=(ix_UniquePointer &&other) noexcept
    {
        if (this == &other)
        {
            return *this;
        }

        ix_FREE(m_ptr);
        m_ptr = other.m_ptr;
        other.m_ptr = nullptr;
        return *this;
    }

    ix_FORCE_INLINE void swap(T *&&p)
    {
        ix_FREE(m_ptr);
        m_ptr = p;
    }

    ix_FORCE_INLINE T *detach()
    {
        T *p = m_ptr;
        m_ptr = nullptr;
        return p;
    }

    ix_FORCE_INLINE T *get()
    {
        return m_ptr;
    }

    ix_FORCE_INLINE const T *get() const
    {
        return m_ptr;
    }

    ix_FORCE_INLINE T &operator[](size_t i)
    {
        return m_ptr[i]; // NOLINT(clang-analyzer-core.uninitialized.UndefReturn)
    }

    ix_FORCE_INLINE const T &operator[](size_t i) const
    {
        return m_ptr[i]; // NOLINT(clang-analyzer-core.uninitialized.UndefReturn)
    }
};
