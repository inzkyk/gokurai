#pragma once

#include "ix.hpp"

// Loosely based on bx::Thread (by Branimir Karadzic, BSD 2-clause license).
class ix_Mutex
{
    alignas(void *) uint8_t m_detail[64];

  public:
    ix_Mutex();
    ix_Mutex(const ix_Mutex &) = delete;
    ix_Mutex(ix_Mutex &&) = delete;
    ix_Mutex &operator=(const ix_Mutex &) = delete;
    ix_Mutex &operator=(ix_Mutex &&) = delete;
    ~ix_Mutex();

    void lock();
    void unlock();

    template <typename T>
    T *native_handle()
    {
        return reinterpret_cast<T *>(m_detail);
    }
};
