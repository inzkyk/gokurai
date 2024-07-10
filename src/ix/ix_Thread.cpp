#include "ix_Thread.hpp"
#include "ix_assert.hpp"
#include "ix_doctest.hpp"
#include "ix_initializer_list.hpp"

#if ix_PLATFORM(WIN)
#include "ix_Windows.hpp"
#else
#include <pthread.h>
#include <unistd.h>
#endif

static_assert(sizeof(ix_Thread) == 32);

void ix_yield_this_thread()
{
#if ix_PLATFORM(WIN)
    SwitchToThread();
#else
    sched_yield();
#endif
}

// Loosely based on bx::Thread (by Branimir Karadzic, BSD 2-clause license).
struct ix_ThreadDetail
{
#if ix_PLATFORM(WIN)
    HANDLE m_handle;
    static DWORD system_entry(void *arg)
    {
        static_assert(sizeof(ix_ThreadDetail) <= sizeof(ix_Thread::m_detail));
        ix_Thread *thread = static_cast<ix_Thread *>(arg);
        thread->m_user_entry();
        return 0;
    }
#else
    pthread_t m_handle;
    static void *system_entry(void *arg)
    {
        static_assert(sizeof(ix_ThreadDetail) <= sizeof(ix_Thread::m_detail));
        ix_Thread *thread = static_cast<ix_Thread *>(arg);
        thread->m_user_entry();
        return nullptr;
    }
#endif
};

ix_Thread::~ix_Thread()
{
    join();
}

bool ix_Thread::is_joinable() const
{
    return !m_user_entry.is_dead();
}

bool ix_Thread::start(const ix_FunctionN<16, void()> &user_entry)
{
    ix_ASSERT(!is_joinable());

    m_user_entry = user_entry;
    ix_ThreadDetail *detail = reinterpret_cast<ix_ThreadDetail *>(m_detail);

#if ix_PLATFORM(WIN)
    detail->m_handle = CreateThread(nullptr,                       // lpThreadAttributes
                                    0,                             // dwStackSize
                                    ix_ThreadDetail::system_entry, // lpStartAddress
                                    this,                          // lpParameter
                                    0,                             // dwCreateFlags
                                    nullptr);                      // lpThreadId
    ix_ASSERT_FATAL(detail->m_handle != nullptr);
#else
    int result = pthread_create(&detail->m_handle, nullptr, &ix_ThreadDetail::system_entry, this);
    ix_ASSERT_FATAL(result == 0);
#endif

    return true;
}

void ix_Thread::join()
{
    if (!is_joinable())
    {
        return;
    }

    ix_ThreadDetail *detail = reinterpret_cast<ix_ThreadDetail *>(m_detail);

#if ix_PLATFORM(WIN)
    const DWORD val = WaitForSingleObject(detail->m_handle, INFINITE);
    ix_ASSERT_FATAL(val == WAIT_OBJECT_0);

    const BOOL ret = CloseHandle(detail->m_handle);
    ix_ASSERT_FATAL(ret != 0);

    detail->m_handle = INVALID_HANDLE_VALUE;
#else
    pthread_join(detail->m_handle, nullptr);
#endif

    m_user_entry.mark_dead();
}

ix_TEST_CASE("ix_Thread")
{
    constexpr int32_t N = 128;

    ix_Thread threads[N];
    static int32_t out[N] = {};
    for (int32_t i = 0; i < N; i++)
    {
        const bool ret = threads[i].start([=]() { out[i] = i * i; });
        ix_EXPECT(ret == true);
    }

    for (int32_t i = 0; i < N; i++)
    {
        threads[i].join();
    }

    for (int32_t i = 0; i < N; i++)
    {
        ix_EXPECT(out[i] == i * i);
    }
}

ix_TEST_CASE("ix_Thread: join() in destructor")
{
    ix_Thread t;
    t.start([]() {});
}
