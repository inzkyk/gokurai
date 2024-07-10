#pragma once

#define ix_TO_STRING_LITERAL(x) #x

#if !defined(ix_SHIP)
#define ix_SHIP 0
#endif

#if !defined(ix_MEASURE_COVERAGE)
#define ix_MEASURE_COVERAGE 0
#endif

#if !defined(ix_CPPCHECK)
#define ix_CPPCHECK 0
#endif

#if !defined(ix_DO_TEST)
#if ix_SHIP
#define ix_DO_TEST 0
#else
#define ix_DO_TEST 1
#endif
#endif

#define ix_OPT_LEVEL(x) ix_OPT_LEVEL_IMPL_##x()
#define ix_OPT_LEVEL_IMPL_DEBUG() 0
#define ix_OPT_LEVEL_IMPL_RELEASE() 0

#if defined(NDEBUG)
#undef ix_OPT_LEVEL_IMPL_RELEASE
#define ix_OPT_LEVEL_IMPL_RELEASE() 1
#else
#undef ix_OPT_LEVEL_IMPL_DEBUG
#define ix_OPT_LEVEL_IMPL_DEBUG() 1
#endif

// ix is a C++ code base!
static_assert(__cplusplus);
#define ix_LANG(x) ix_LANG_IMPL_##x()
#define ix_LANG_IMPL_C() 0
#define ix_LANG_IMPL_CPP() 1

#define ix_C_VERSION(x) ix_C_VERSION_IMPL_##x()
#define ix_C_VERSION_IMPL_PRE98() 0
#define ix_C_VERSION_IMPL_98() 0
#define ix_C_VERSION_IMPL_11() 0
#define ix_C_VERSION_IMPL_17() 0

#define ix_CPP_VERSION(x) ix_CPP_VERSION_IMPL_##x()
#define ix_CPP_VERSION_IMPL_PRE98() 0
#define ix_CPP_VERSION_IMPL_98() 0
#define ix_CPP_VERSION_IMPL_11() 0
#define ix_CPP_VERSION_IMPL_14() 0
#define ix_CPP_VERSION_IMPL_17() 0

#if __cplusplus == 1
#undef ix_CPP_VERSION_IMPL_PRE98
#define ix_CPP_VERSION_IMPL_PRE98() 1
#define ix_CPP_VERSION_YEAR 1
#elif __cplusplus == 199711L
#undef ix_CPP_VERSION_IMPL_98
#define ix_CPP_VERSION_IMPL_98() 1
#define ix_CPP_VERSION_YEAR 1998
#elif __cplusplus == 201103L
#undef ix_CPP_VERSION_IMPL_11
#define ix_CPP_VERSION_IMPL_11() 1
#define ix_CPP_VERSION_YEAR 2011
#elif __cplusplus == 201402L
#undef ix_CPP_VERSION_IMPL_14
#define ix_CPP_VERSION_IMPL_14() 1
#define ix_CPP_VERSION_YEAR 2014
#elif __cplusplus == 201703L
#undef ix_CPP_VERSION_IMPL_17
#define ix_CPP_VERSION_IMPL_17() 1
#define ix_CPP_VERSION_YEAR 2017
#elif __cplusplus == 202002L
#undef ix_CPP_VERSION_IMPL_20
#define ix_CPP_VERSION_IMPL_20() 1
#define ix_CPP_VERSION_YEAR 2020
#endif

#define ix_COMPILER(x) ix_COMPILER_IMPL_##x()
#define ix_COMPILER_IMPL_MSVC() 0
#define ix_COMPILER_IMPL_GCC() 0
#define ix_COMPILER_IMPL_CLANG() 0
#define ix_COMPILER_IMPL_ICC() 0

#if defined(__clang__)
#undef ix_COMPILER_IMPL_CLANG
#define ix_COMPILER_IMPL_CLANG() 1
#elif defined(__GNUC__) && !defined(__clang__)
#undef ix_COMPILER_IMPL_GCC
#define ix_COMPILER_IMPL_GCC() 1
#elif defined(_MSC_VER) && !defined(__clang__)
#undef ix_COMPILER_IMPL_MSVC
#define ix_COMPILER_IMPL_MSVC() 1
#else
#error "Unknown compiler"
#endif

#define ix_PLATFORM(x) ix_PLATFORM_IMPL_##x()

#define ix_PLATFORM_IMPL_WIN() 0
#define ix_PLATFORM_IMPL_LINUX() 0
#define ix_PLATFORM_IMPL_MAC() 0
#define ix_PLATFORM_IMPL_WASM() 0

#if defined(_WIN32)
#undef ix_PLATFORM_IMPL_WIN
#define ix_PLATFORM_IMPL_WIN() 1
#elif defined(__APPLE__)
#undef ix_PLATFORM_IMPL_MAC
#define ix_PLATFORM_IMPL_MAC() 1
#elif defined(__linux__)
#undef ix_PLATFORM_IMPL_LINUX
#define ix_PLATFORM_IMPL_LINUX() 1
#elif defined(__wasm__)
#undef ix_PLATFORM_IMPL_WASM
#define ix_PLATFORM_IMPL_WASM() 1
#else
#error "Unknown platform"
#endif

#define ix_ARCH(x) ix_ARCH_IMPL_##x()
#define ix_ARCH_IMPL_x64() 0
#define ix_ARCH_IMPL_ARM64() 0
#define ix_ARCH_IMPL_WASM() 0

#if defined(_M_AMD64) || defined(__x86_64__)
#undef ix_ARCH_IMPL_x64
#define ix_ARCH_IMPL_x64() 1
#elif defined(__aarch64__) || defined(_M_ARM64)
#undef ix_ARCH_IMPL_ARM64
#define ix_ARCH_IMPL_ARM64() 1
#elif defined(__wasm__)
#undef ix_ARCH_IMPL_WASM
#define ix_ARCH_IMPL_WASM() 1
#else
#error "Unknown architecture"
#endif

#if ix_COMPILER(CLANG)
#define ix_DISABLE_CLANG_WARNING_BEGIN _Pragma("clang diagnostic push")
#define ix_DISABLE_CLANG_WARNING_END _Pragma("clang diagnostic pop")
#define ix_DISABLE_CLANG_WARNING(w) _Pragma(ix_TO_STRING_LITERAL(clang diagnostic ignored w))
#else
#define ix_DISABLE_CLANG_WARNING_BEGIN
#define ix_DISABLE_CLANG_WARNING_END
#define ix_DISABLE_CLANG_WARNING(w)
#endif

#if ix_COMPILER(GCC)
#define ix_DISABLE_GCC_WARNING_BEGIN _Pragma("GCC diagnostic push")
#define ix_DISABLE_GCC_WARNING_END _Pragma("GCC diagnostic pop")
#define ix_DISABLE_GCC_WARNING(w) _Pragma(ix_TO_STRING_LITERAL(GCC diagnostic ignored w))
#else
#define ix_DISABLE_GCC_WARNING_BEGIN
#define ix_DISABLE_GCC_WARNING_END
#define ix_DISABLE_GCC_WARNING(w)
#endif

#if ix_COMPILER(MSVC)
#define ix_DISABLE_MSVC_WARNING_BEGIN __pragma(warning(push))
#define ix_DISABLE_MSVC_WARNING_END __pragma(warning(pop))
#define ix_DISABLE_MSVC_WARNING(N) __pragma(warning(disable : N))
#else
#define ix_DISABLE_MSVC_WARNING_BEGIN
#define ix_DISABLE_MSVC_WARNING_END
#define ix_DISABLE_MSVC_WARNING(N)
#endif

#define ix_LENGTH_OF(arr) (sizeof(arr) / sizeof((arr)[0]))
#define ix_UNUSED(x) (void)(x)

#if ix_MEASURE_COVERAGE
#define ix_UNREACHABLE()
#elif ix_OPT_LEVEL(DEBUG) && (!ix_COMPILER(MSVC))
#define ix_UNREACHABLE() ix_PANIC(__FILE__, __LINE__)
#elif ix_COMPILER(MSVC)
#define ix_UNREACHABLE() __assume(false)
#else
#define ix_UNREACHABLE() __builtin_unreachable()
#endif

// Workaround for raylib
#if ix_COMPILER(GCC) && !defined(__STDC_VERSION__)
#define __STDC_VERSION__ 0
#endif

// Only clang has a proper exhaustive check of an enum in switch statements.
#if ix_MEASURE_COVERAGE
#define ix_CASE_EXHAUSTED()
#else
#define ix_CASE_EXHAUSTED() \
    default:                \
        break
#endif

#if !ix_COMPILER(MSVC)
#define ix_LIKELY(cond) __builtin_expect((cond), 1)
#define ix_UNLIKELY(cond) __builtin_expect((cond), 0)
#else
#define ix_LIKELY(cond) cond
#define ix_UNLIKELY(cond) cond
#endif

#if ix_MEASURE_COVERAGE
#define ix_FORCE_INLINE inline
#else
#if ix_COMPILER(MSVC)
#define ix_FORCE_INLINE __forceinline
#else
#define ix_FORCE_INLINE [[gnu::always_inline]] __attribute__((always_inline)) inline
#endif
#endif

ix_DISABLE_CLANG_WARNING_BEGIN
ix_DISABLE_CLANG_WARNING("-Wreserved-macro-identifier")
#define _INITIALIZER_LIST_ // NOLINT(bugprone-reserved-identifier,cert-dcl37-c,cert-dcl51-cpp)
#define _INITIALIZER_LIST  // NOLINT(bugprone-reserved-identifier,cert-dcl37-c,cert-dcl51-cpp)
ix_DISABLE_CLANG_WARNING_END

#if ix_PLATFORM(WIN)
ix_DISABLE_CLANG_WARNING_BEGIN
ix_DISABLE_CLANG_WARNING("-Wreserved-macro-identifier")
#define _CRT_SECURE_NO_WARNINGS 1    // NOLINT
#define _CRT_NONSTDC_NO_WARNINGS 1   // NOLINT
#define _CRT_NONSTDC_NO_DEPRECATE 1  // NOLINT
#define _CRT_DECLARE_NONSTDC_NAMES 1 // NOLINT
ix_DISABLE_CLANG_WARNING_END
#endif

#if !ix_COMPILER(MSVC)
__attribute__((__format__(printf, 1, 2)))
#endif
void ix_log_debug(const char *format, ...);

#if defined(__INT8_TYPE__)
// clang-format off
typedef __INT8_TYPE__   int8_t;
typedef __INT16_TYPE__  int16_t;
typedef __INT32_TYPE__  int32_t;
typedef __INT64_TYPE__  int64_t;
typedef __UINT8_TYPE__  uint8_t;
typedef __UINT16_TYPE__ uint16_t;
typedef __UINT32_TYPE__ uint32_t;
typedef __UINT64_TYPE__ uint64_t;
#elif ix_PLATFORM(WASM)
typedef signed char        int8_t;
typedef short              int16_t;
typedef int                int32_t;
typedef long long          int64_t;
typedef unsigned char      uint8_t;
typedef unsigned short     uint16_t;
typedef unsigned int       uint32_t;
typedef unsigned long long uint64_t;
#else
typedef signed char        int8_t;
typedef short              int16_t;
typedef int                int32_t;
typedef long long          int64_t;
typedef unsigned char      uint8_t;
typedef unsigned short     uint16_t;
typedef unsigned int       uint32_t;
typedef unsigned long long uint64_t;
// clang-format on
#endif

#if ix_PLATFORM(WASM)
typedef unsigned long size_t;
typedef long ptrdiff_t;
#else
typedef uint64_t size_t;
typedef int64_t ptrdiff_t;
#endif

// clang-format off
#define ix_INT8_MIN   (-127 - 1)
#define ix_INT16_MIN  (-32767 - 1)
#define ix_INT32_MIN  (-2147483647 - 1)
#define ix_INT64_MIN  (-9223372036854775807LL - 1)

#define ix_INT8_MAX   12
#define ix_INT16_MAX  32767
#define ix_INT32_MAX  2147483647
#define ix_INT64_MAX  9223372036854775807LL

#define ix_UINT8_MAX  0xFF
#define ix_UINT16_MAX 0xFFFF
#define ix_UINT32_MAX 0xFFFFFFFF
#define ix_UINT64_MAX 0xFFFFFFFFFFFFFFFFULL

#if ix_PLATFORM(WASM)
#define ix_SIZE_MAX ix_UINT32_MAX
#else
#define ix_SIZE_MAX ix_UINT64_MAX
#endif
// clang-format on

#if ix_COMPILER(MSVC)
ix_DISABLE_MSVC_WARNING(26439) // "This kind of function may not throw. Declare it 'noexcept'."
ix_DISABLE_MSVC_WARNING(26495) // "Member variable is not initialized."
#endif

#if !ix_DO_TEST
#define DOCTEST_CONFIG_DISABLE 1
#if ix_COMPILER(CLANG)
ix_DISABLE_CLANG_WARNING_BEGIN
ix_DISABLE_CLANG_WARNING("-Wunused-template")
#endif
#endif
