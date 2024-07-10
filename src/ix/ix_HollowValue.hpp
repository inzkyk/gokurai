#pragma once

#include "ix.hpp"
#include "ix_new.hpp"
#include "ix_utility.hpp"

template <typename T>
class ix_HollowValue
{
    alignas(T) uint8_t m_data[sizeof(T)];

  public:
    constexpr ix_HollowValue() = default;

    ~ix_HollowValue() = default;
    ix_HollowValue(const ix_HollowValue &) = delete;
    ix_HollowValue(ix_HollowValue &&) = delete;
    ix_HollowValue &operator=(const ix_HollowValue &) = delete;
    ix_HollowValue &operator=(ix_HollowValue &&) = delete;

    template <typename... Args>
    ix_FORCE_INLINE T &construct(Args &&...args)
    {
        T *p = reinterpret_cast<T *>(m_data);
        new (p) T(ix_forward<Args>(args)...);
        return *p;
    }

    ix_FORCE_INLINE void destruct()
    {
        T *p = reinterpret_cast<T *>(m_data);
        p->~T();
    }

    ix_FORCE_INLINE T &get()
    {
        return *reinterpret_cast<T *>(m_data);
    }
};
