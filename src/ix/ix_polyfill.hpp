#pragma once

#include "ix.hpp"

#if ix_COMPILER(CLANG) || ix_COMPILER(MSVC) || ix_COMPILER(GCC)

ix_FORCE_INLINE constexpr bool ix_is_constant_evaluated()
{
    return __builtin_is_constant_evaluated();
}

#else

#error "Please implement ix_is_constant_evaluated()."

#endif
