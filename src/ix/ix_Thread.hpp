#pragma once

#include "ix.hpp"
#include "ix_Function.hpp"

void ix_yield_this_thread();

// Loosely based on bx::Thread (by Branimir Karadzic, BSD 2-clause license).
class ix_Thread
{
    friend struct ix_ThreadDetail;

    ix_FunctionN<16, void()> m_user_entry;
    uint8_t m_detail[8];

  public:
    ix_Thread() = default;
    ~ix_Thread();
    ix_Thread(const ix_Thread &) = delete;
    ix_Thread(ix_Thread &&other) = delete;
    ix_Thread &operator=(const ix_Thread &) = delete;
    ix_Thread &operator=(ix_Thread &&) = delete;

    bool is_joinable() const;

    bool start(const ix_FunctionN<16, void()> &user_entry);
    void join();
};
