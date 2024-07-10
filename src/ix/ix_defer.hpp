#pragma once

#include "ix.hpp"
#include "ix_utility.hpp"

template <typename F>
class ix_Deferred
{
    F m_deferred_function;
    bool m_active;

  public:
    ix_FORCE_INLINE explicit ix_Deferred(F &&f)
        : m_deferred_function(ix_move(f)),
          m_active(true)
    {
    }

    ix_FORCE_INLINE explicit ix_Deferred(bool active, F &&f)
        : m_deferred_function(ix_move(f)),
          m_active(active)
    {
    }

    ix_FORCE_INLINE ~ix_Deferred()
    {
        if (m_active)
        {
            m_deferred_function();
        }
    }

    ix_FORCE_INLINE void deactivate()
    {
        m_active = false;
    }

    ix_Deferred(const ix_Deferred &) = delete;
    ix_Deferred(ix_Deferred &&) = delete;
    ix_Deferred &operator=(const ix_Deferred &) = delete;
    ix_Deferred &operator=(ix_Deferred &&) = delete;
};

template <typename F>
ix_FORCE_INLINE ix_Deferred<F> ix_defer(F &&f)
{
    return ix_Deferred<F>(ix_move(f));
}

template <typename F>
ix_FORCE_INLINE ix_Deferred<F> ix_defer(bool active, F &&f)
{
    return ix_Deferred<F>(active, ix_move(f));
}
