#pragma once

#include "ix.hpp"

#if (ix_COMPILER(GCC) && ix_OPT_LEVEL(RELEASE))

#include <new>

#elif ix_PLATFORM(WASM)

#include <__config>
inline _LIBCPP_HIDE_FROM_ABI void *operator new(size_t, void *__p) noexcept;

#elif ix_COMPILER(GCC)

ix_DISABLE_GCC_WARNING_BEGIN
ix_DISABLE_GCC_WARNING("-Wredundant-decls")
void *operator new(size_t size, void *p) noexcept;
ix_DISABLE_GCC_WARNING_END

#elif ix_COMPILER(MSVC)

void *operator new(size_t size, void *p) noexcept;

#elif ix_COMPILER(CLANG)

void *operator new(size_t size, void *p) noexcept;

#else

#error "Unknown compiler"

#endif
