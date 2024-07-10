#pragma once

#include "ix.hpp"

#define ix_ASSERT_FATAL(...)              \
    do                                    \
    {                                     \
        if (ix_UNLIKELY(!(__VA_ARGS__)))  \
        {                                 \
            ix_PANIC(__FILE__, __LINE__); \
        }                                 \
    } while (0)

#if ix_MEASURE_COVERAGE
#define ix_ASSERT(...) ix_UNUSED((__VA_ARGS__))
#elif ix_OPT_LEVEL(DEBUG)
#define ix_ASSERT(...) ix_ASSERT_FATAL((__VA_ARGS__))
#else
#define ix_ASSERT(...)
#endif

#if ix_OPT_LEVEL(DEBUG)
#define ix_PANIC(file, line)                             \
    do                                                   \
    {                                                    \
        ix_log_debug("\n%s:%d: PANIC!!!\n", file, line); \
        ix_abort();                                      \
    } while (0)
#else
#define ix_PANIC(file, line) ix_abort()
#endif

#if ix_MEASURE_COVERAGE
void ix_abort();
#else
[[noreturn]] void ix_abort();
#endif
