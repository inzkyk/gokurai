#pragma once

#include "ix.hpp"

static_assert(ix_PLATFORM(WIN));

#if !defined(NOMINMAX) //
#define NOMINMAX       // Macros min(a,b) and max(a,b)
#endif                 //

ix_DISABLE_CLANG_WARNING_BEGIN
ix_DISABLE_CLANG_WARNING("-Wreserved-macro-identifier")

#if ix_ARCH(x64)
#define _AMD64_ 1 // NOLINT
#elif ix_ARCH(ARM64)
#define _ARM64_ 1 // NOLINT
#else
#error "Unknown architecture"
#endif

ix_DISABLE_CLANG_WARNING_END

#include <windef.h>

#include <WinBase.h>
