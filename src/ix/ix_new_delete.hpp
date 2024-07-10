#pragma once

#include "ix.hpp"
#include "ix_memory.hpp"
#include "ix_utility.hpp"

template <typename T, typename... Args>
ix_FORCE_INLINE T *ix_new(Args &&...args)
{
    T *p = ix_ALLOC_STRUCT(T);
    return new (p) T(ix_forward<Args>(args)...);
}

template <typename T>
ix_FORCE_INLINE void ix_delete(T *p)
{
    p->~T();
    ix_FREE(p);
}
