#pragma once

#include "ix.hpp"
#include "ix_assert.hpp"
#include "ix_bulk.hpp"
#include "ix_initializer_list.hpp"
#include "ix_memory.hpp"
#include "ix_min_max.hpp"

template <typename T>
class ix_Vector
{
    T *m_data;
    size_t m_size;
    size_t m_capacity;

  public:
    ix_FORCE_INLINE constexpr ix_Vector()
        : m_data(nullptr),
          m_size(0),
          m_capacity(0)
    {
    }

    ix_FORCE_INLINE constexpr explicit ix_Vector(size_t initial_size)
        : m_data(ix_ALLOC_ARRAY(T, initial_size)),
          m_size(initial_size),
          m_capacity(initial_size)
    {
        ix_bulk_default_construct(m_data, initial_size);
    }

    // cppcheck-suppress noExplicitConstructor
    ix_FORCE_INLINE constexpr ix_Vector(const std::initializer_list<T> &xs)
        : m_size(xs.size()),
          m_capacity(xs.size())
    {
        m_data = ix_ALLOC_ARRAY(T, m_size);
        ix_bulk_copy_construct(m_data, xs.begin(), m_size);
    }

    ix_FORCE_INLINE ~ix_Vector()
    {
        ix_bulk_destruct(m_data, m_size);
        ix_FREE(m_data);
    }

    constexpr ix_Vector(const ix_Vector &other)
        : m_size(other.m_size),
          m_capacity(other.m_capacity)
    {
        m_data = ix_ALLOC_ARRAY(T, m_capacity);
        ix_bulk_copy_construct(m_data, other.m_data, m_size);
    }

    constexpr ix_Vector(ix_Vector &&other) noexcept
        : m_data(other.m_data),
          m_size(other.m_size),
          m_capacity(other.m_capacity)
    {
        other.m_data = nullptr;
        other.m_size = 0;
        other.m_capacity = 0;
    }

    constexpr ix_Vector<T> &operator=(const ix_Vector &other)
    {
        if (this == &other)
        {
            return *this;
        }

        ix_bulk_destruct(m_data, m_size);
        ix_FREE(m_data);

        m_size = other.m_size;
        m_capacity = other.m_capacity;

        m_data = ix_ALLOC_ARRAY(T, m_capacity);
        ix_bulk_copy_construct(m_data, other.m_data, m_size);

        return *this;
    }

    constexpr ix_Vector<T> &operator=(ix_Vector &&other) noexcept
    {
        if (this == &other)
        {
            return *this;
        }

        ix_bulk_destruct(m_data, m_size);
        ix_FREE(m_data);

        m_data = other.m_data;
        m_size = other.m_size;
        m_capacity = other.m_capacity;

        other.m_data = nullptr;
        other.m_size = 0;
        other.m_capacity = 0;

        return *this;
    }

    constexpr bool operator==(const ix_Vector &other) const
    {
        if (m_size != other.m_size)
        {
            return false;
        }

        for (size_t i = 0; i < m_size; i++)
        {
            if (operator[](i) != other[i])
            {
                return false;
            }
        }

        return true;
    }

    constexpr bool operator!=(const ix_Vector &other) const
    {
        return !(*this == other);
    }

    ix_FORCE_INLINE constexpr T *data()
    {
        return m_data;
    }

    ix_FORCE_INLINE constexpr const T *data() const
    {
        return m_data;
    }

    ix_FORCE_INLINE constexpr size_t size() const
    {
        return m_size;
    }

    ix_FORCE_INLINE constexpr size_t capacity() const
    {
        return m_capacity;
    }

    ix_FORCE_INLINE constexpr bool empty() const
    {
        return (m_size == 0);
    }

    ix_FORCE_INLINE constexpr const T *begin() const
    {
        return m_data;
    }

    ix_FORCE_INLINE constexpr T *begin()
    {
        return m_data;
    }

    ix_FORCE_INLINE constexpr const T *end() const
    {
        return m_data + m_size;
    }

    ix_FORCE_INLINE constexpr T *end()
    {
        return m_data + m_size;
    }

    ix_FORCE_INLINE constexpr T &operator[](size_t i)
    {
        ix_ASSERT(i < m_size);
        return m_data[i];
    }

    ix_FORCE_INLINE constexpr const T &operator[](size_t i) const
    {
        ix_ASSERT(i < m_size);
        return m_data[i];
    }

    ix_FORCE_INLINE constexpr T &back()
    {
        ix_ASSERT(0 < m_size);
        return m_data[m_size - 1];
    }

    ix_FORCE_INLINE constexpr const T &back() const
    {
        ix_ASSERT(0 < m_size);
        return m_data[m_size - 1];
    }

    ix_FORCE_INLINE constexpr void pop_back()
    {
        ix_ASSERT(0 < m_size);
        m_data[m_size - 1].~T();
        m_size -= 1;
    }

    ix_FORCE_INLINE constexpr void clear()
    {
        ix_bulk_destruct(m_data, m_size);
        m_size = 0;
    }

    ix_FORCE_INLINE constexpr void reserve(size_t n)
    {
        if (n <= m_capacity)
        {
            return;
        }

        if constexpr (ix_is_trivially_move_constructible_v<T> && ix_is_trivially_destructible_v<T>)
        {
            m_data = ix_REALLOC_ARRAY(T, m_data, n);
            m_capacity = n;
        }
        else
        {
            T *new_data = ix_ALLOC_ARRAY(T, n);

            ix_bulk_move_construct(new_data, m_data, m_size);
            ix_bulk_destruct(m_data, m_size);

            ix_FREE(m_data);
            m_data = new_data;
            m_capacity = n;
        }
    }

    ix_FORCE_INLINE constexpr void initial_reserve(size_t capacity)
    {
        ix_ASSERT(m_capacity == 0);
        m_data = ix_ALLOC_ARRAY(T, capacity);
        m_capacity = capacity;
    }

    ix_FORCE_INLINE constexpr void resize(size_t n)
    {
        if (n < m_size)
        {
            ix_bulk_destruct(m_data + n, m_size - n);
            m_size = n;
            return;
        }

        if (m_capacity < n)
        {
            reserve(n);
        }

        ix_bulk_default_construct(m_data + m_size, n - m_size);
        m_size = n;
    }

    ix_FORCE_INLINE constexpr void resize(size_t n, const T &x)
    {
        if (n < m_size)
        {
            ix_bulk_destruct(m_data + n, m_size - n);
            m_size = n;
            return;
        }

        if (m_capacity < n)
        {
            reserve(n);
        }

        T *p = m_data + m_size;
        const T *p_end = m_data + n;
        while (p < p_end)
        {
            new (p) T(x);
            p += 1;
        }

        m_size = n;
    }

    ix_FORCE_INLINE constexpr void push_back(const T &x)
    {
        emplace_back(x);
    }

    ix_FORCE_INLINE constexpr void push_back(T &&x)
    {
        emplace_back(ix_move(x));
    }

    ix_FORCE_INLINE constexpr T *push_back_void()
    {
        if (ix_UNLIKELY(m_size == m_capacity))
        {
            size_t new_capacity = ix_grow_array_size(m_size);
            reserve(new_capacity);
        }

        T *p = m_data + m_size;
        m_size += 1;
        return p;
    }

    ix_FORCE_INLINE constexpr void set_size(size_t size)
    {
        m_size = size;
    }

    void swap_erase(size_t index)
    {
        const size_t last_index = m_size - 1;

        if (index != last_index)
        {
            (*this)[index] = ix_move((*this)[last_index]);
        }

        pop_back();
    }

    template <typename... Args>
    ix_FORCE_INLINE constexpr void emplace_back(Args &&...args)
    {
        if (ix_UNLIKELY(m_size == m_capacity))
        {
            size_t new_capacity = ix_grow_array_size(m_size);
            reserve(new_capacity);
        }

        T *p = m_data + m_size;
        new (p) T(ix_forward<Args>(args)...);
        m_size += 1;
    }

    template <typename... Args>
    ix_FORCE_INLINE constexpr void emplace_back_no_reserve(Args &&...args)
    {
        ix_ASSERT(m_size < m_capacity);
        T *p = m_data + m_size;
        new (p) T(ix_forward<Args>(args)...);
        m_size += 1;
    }

    constexpr void insert(T *pos, T &&x)
    {
        if (m_size + 1 > m_capacity)
        {
            const auto pos_index = pos - m_data;
            size_t new_capacity = ix_grow_array_size(m_capacity);
            reserve(new_capacity);
            pos = m_data + pos_index;
        }

        T *new_element = m_data + m_size;

        if (new_element == pos)
        {
            new (new_element) T(ix_move(x));
            m_size += 1;
            return;
        }

        T *last_element = new_element - 1;
        new (new_element) T(ix_move(*last_element));

        if constexpr (ix_is_trivially_move_assignable_v<T>)
        {
            size_t n = static_cast<size_t>(last_element - pos);
            ix_memmove(pos + 1, pos, sizeof(T) * n);
        }
        else
        {

            T *p = last_element;
            while (pos < p)
            {
                *p = ix_move(*(p - 1));
                p -= 1;
            }
        }

        *pos = ix_move(x);

        m_size += 1;
    }

    constexpr void insert(T *pos, const T *start, const T *end)
    {
        size_t n = static_cast<size_t>(end - start);
        size_t required_capacity = m_size + n;
        if (required_capacity >= m_capacity)
        {
            const auto pos_index = pos - m_data;
            size_t new_capacity = ix_max(ix_grow_array_size(m_capacity), required_capacity);
            reserve(new_capacity);
            pos = m_data + pos_index;
        }

        T *new_element_start = m_data + m_size;
        if (new_element_start == pos)
        {
            ix_bulk_copy_construct(new_element_start, start, n);
            m_size += n;
            return;
        }

        const size_t m = static_cast<size_t>(new_element_start - pos);
        if (m >= n)
        {
            //             pos                  new_element_start
            //              |                          |
            //              |<----------- m ---------->|
            // |------------|--------------------------|
            //              ^                          |
            //              |                          |
            //       |=============|                   |
            //       |<---- n ---->|                   |
            //                                         |
            //                                         |<---- n ---->|
            // |------------|=============|--------------------------|
            //              |<---- n ---->|<----------- m ---------->|

            ix_bulk_copy_construct(new_element_start, new_element_start - n, n);

            if constexpr (ix_is_trivially_copy_assignable_v<T>)
            {
                ix_memmove(pos + n, pos, sizeof(T) * (m - n));
            }
            else
            {
                T *p = new_element_start - 1;
                T *p_last = pos + n;
                while (p_last <= p)
                {
                    *p = *(p - n);
                    p -= 1;
                }
            }

            ix_bulk_copy_assign(pos, start, n);
        }
        else
        {

            //             pos           new_element_start
            //              |                   |
            //              |<------- m ------->|
            // |------------|-------------------|
            //              ^                   |
            //              |                   |
            //   |==========================|   |
            //   |<----------- n ---------->|   |
            //                                  |
            //                                  |<----------- n ---------->|
            // |------------|==========================|-------------------|
            //              |<----------- n ---------->|<------- m ------->|

            const size_t num_from_external = n - m;
            ix_bulk_copy_construct(new_element_start, start + m, num_from_external);
            ix_bulk_copy_construct(new_element_start + num_from_external, pos, m);
            ix_bulk_copy_assign(pos, start, m);
        }

        m_size += n;
    }

    ix_FORCE_INLINE constexpr void insert(T *pos, const T &x)
    {
        insert(pos, ix_move(T(x)));
    }

    constexpr void erase(T *pos)
    {
        ix_ASSERT(m_data <= pos);
        if constexpr (ix_is_trivially_move_assignable_v<T>)
        {
            const size_t num_elements_to_move = m_size - static_cast<size_t>(pos - m_data) - 1;
            ix_memmove(pos, pos + 1, sizeof(T) * num_elements_to_move);
            T *last = m_data + m_size - 1;
            last->~T();
        }
        else
        {
            T *p = pos;
            const T *p_end = m_data + m_size;
            while (p < p_end - 1)
            {
                *p = ix_move(*(p + 1));
                p += 1;
            }
            p->~T();
        }

        m_size -= 1;
    }

    constexpr void erase(T *pos, size_t n)
    {
        ix_ASSERT(m_data <= pos);
        ix_ASSERT(n <= m_size);

        if (n == 0)
        {
            return;
        }

        if constexpr (ix_is_trivially_move_assignable_v<T>)
        {
            const size_t num_elements_to_move = m_size - static_cast<size_t>(pos - m_data) - n;
            ix_memmove(pos, pos + n, sizeof(T) * num_elements_to_move);
            ix_bulk_destruct(m_data + m_size - 1 - n, n);
        }
        else
        {
            T *p = pos;
            const T *p_end = m_data + m_size;
            while (p < p_end - n)
            {
                *p = ix_move(*(p + n));
                p += 1;
            }
            p->~T();
        }

        m_size -= n;
    }
};
