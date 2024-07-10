#include "ix_Mutex.hpp"
#include "ix_Thread.hpp"
#include "ix_doctest.hpp"

#if ix_PLATFORM(WIN)
#include "ix_Windows.hpp"
#include <synchapi.h>
#else
#include <pthread.h>
#endif

// Loosely based on bx::Thread (by Branimir Karadzic, BSD 2-clause license).
#if ix_PLATFORM(WIN)
using pthread_mutex_t = CRITICAL_SECTION;
using pthread_mutexattr_t = unsigned;

static inline int pthread_mutex_lock(pthread_mutex_t *mutex)
{
    EnterCriticalSection(mutex);
    return 0;
}

static inline int pthread_mutex_unlock(pthread_mutex_t *mutex)
{
    LeaveCriticalSection(mutex);
    return 0;
}

static inline int pthread_mutex_init(pthread_mutex_t *mutex, pthread_mutexattr_t * /*attr*/)
{
    InitializeCriticalSection(mutex);
    return 0;
}

static inline int pthread_mutex_destroy(pthread_mutex_t *mutex)
{
    DeleteCriticalSection(mutex);
    return 0;
}
#endif

ix_Mutex::ix_Mutex()
{
    static_assert(sizeof(pthread_mutex_t) <= sizeof(m_detail));
    pthread_mutex_t *handle = reinterpret_cast<pthread_mutex_t *>(m_detail);
    pthread_mutex_init(handle, nullptr);
}

ix_Mutex::~ix_Mutex()
{
    pthread_mutex_t *handle = reinterpret_cast<pthread_mutex_t *>(m_detail);
    pthread_mutex_destroy(handle);
}

void ix_Mutex::lock()
{
    pthread_mutex_t *handle = reinterpret_cast<pthread_mutex_t *>(m_detail);
    pthread_mutex_lock(handle);
}

void ix_Mutex::unlock()
{
    pthread_mutex_t *handle = reinterpret_cast<pthread_mutex_t *>(m_detail);
    pthread_mutex_unlock(handle);
}

ix_TEST_CASE("ix_Mutex")
{
    constexpr static const size_t N = 16;
    constexpr static const size_t M = 1024;
    ix_Thread threads[N];

    static struct SharedData
    {
        ix_Mutex *mutex;
        size_t *array;
    } shared_data;

    ix_Mutex mutex;
    size_t array[M] = {};

    shared_data.mutex = &mutex;
    shared_data.array = array;

    for (size_t i = 0; i < N; i++)
    {
        threads[i].start([&]() {
            shared_data.mutex->lock();
            for (size_t j = 0; j < M; j++)
            {
                const size_t current_value = shared_data.array[j];
                ix_yield_this_thread();
                shared_data.array[j] = current_value + 1;
            }
            shared_data.mutex->unlock();
        });
    }

    for (size_t i = 0; i < N; i++)
    {
        threads[i].join();
    }

    for (size_t i = 0; i < M; i++)
    {
        ix_EXPECT(array[i] == N);
    }
}

ix_TEST_CASE("ix_Mutex")
{
    ix_Mutex mutex;
#if ix_PLATFORM(WIN)
    CRITICAL_SECTION *handle = mutex.native_handle<CRITICAL_SECTION>();
    EnterCriticalSection(handle);
    LeaveCriticalSection(handle);
#else
    pthread_mutex_t *handle = mutex.native_handle<pthread_mutex_t>();
    pthread_mutex_lock(handle);
    pthread_mutex_unlock(handle);
#endif
}
