#pragma once

#include "ix.hpp"
#include "ix_Vector.hpp"

class ix_StringArena
{
    //                                Pool
    //   |--------------------------|--------------------------------------|
    //   |                          |<------------------------------------>|
    // start                       next               remain
    //
    struct Pool
    {
        size_t remain;
        char *start;
        char *next;
    };

    size_t m_pool_size;
    ix_Vector<Pool> m_pools;
    Pool *m_current_pool;

  public:
    explicit ix_StringArena(size_t pool_size);
    ~ix_StringArena();
    ix_StringArena(ix_StringArena &&) = delete;
    ix_StringArena(const ix_StringArena &) = delete;
    ix_StringArena &operator=(ix_StringArena &&) = delete;
    ix_StringArena &operator=(const ix_StringArena &) = delete;

    size_t size() const;

    void clear();
    void reset_to(const char *p);
    const char *push(const char *data, size_t length);
    const char *push_str(const char *str);
    const char *push_between(const char *start, const char *end);
};
