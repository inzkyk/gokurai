#include "ix_environment.hpp"
#include "ix_Thread.hpp"
#include "ix_doctest.hpp"

#if ix_PLATFORM(WIN)
#include "ix_Windows.hpp"
#include <process.h>
#elif ix_PLATFORM(LINUX) || ix_PLATFORM(MAC) || ix_PLATFORM(WASM)
#include <pthread.h>
#include <unistd.h>
#endif

#if ix_PLATFORM(LINUX) || ix_PLATFORM(MAC)
#include <valgrind/valgrind.h>
#endif

size_t ix_hardware_concurrency()
{
#if ix_PLATFORM(WIN)
    SYSTEM_INFO info;
    GetNativeSystemInfo(&info);
    return info.dwNumberOfProcessors;
#elif ix_PLATFORM(LINUX) || ix_PLATFORM(MAC)
    return static_cast<size_t>(sysconf(_SC_NPROCESSORS_ONLN));
#elif ix_PLATFORM(WASM)
    return 0;
#else
#error
#endif
}

ix_TEST_CASE("ix_hardware_concurrency")
{
    const size_t c = ix_hardware_concurrency();
    if (c != 0)
    {
        ix_EXPECT(c < 4096); // Maybe someday...?
    }
}

size_t ix_process_id()
{
    return static_cast<size_t>(getpid());
}

ix_TEST_CASE("ix_process_id")
{
    ix_EXPECT(ix_process_id() > 0);
}

size_t ix_thread_id()
{
#if ix_PLATFORM(WIN)
    return GetCurrentThreadId();
#else
    return static_cast<size_t>(gettid());
#endif
}

ix_TEST_CASE("ix_thread_id")
{
    const size_t parent_thread_id = ix_thread_id();
    ix_EXPECT(parent_thread_id > 0);
    for (size_t i = 0; i < 16; i++)
    {
        ix_Thread thread;
        thread.start([=]() {
            const size_t child_thread_id = ix_thread_id();
            ix_EXPECT(ix_thread_id() > 0);
            ix_EXPECT(parent_thread_id != child_thread_id);
        });
    }
}

bool ix_is_valgrind_active()
{
#if ix_PLATFORM(LINUX) || ix_PLATFORM(MAC)
    return RUNNING_ON_VALGRIND;
#else
    return false;
#endif
}

ix_TEST_CASE("ix_is_valgrind_active")
{
    ix_UNUSED(ix_is_valgrind_active()); // No way to test!
}
