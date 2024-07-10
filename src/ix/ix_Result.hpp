#pragma once

#include "ix.hpp"
#include "ix_assert.hpp"

// TODO: Implement as class with `const char *msg`, where `msg == nullptr` means no error.
class [[nodiscard]] ix_Result
{
    bool ok;

  public:
    ix_FORCE_INLINE explicit constexpr ix_Result(bool cond)
        : ok(cond)
    {
    }

    ix_FORCE_INLINE constexpr bool is_ok() const
    {
        return ok;
    }

    ix_FORCE_INLINE constexpr bool is_error() const
    {
        return !ok;
    }

    ix_FORCE_INLINE constexpr void assert_ok() const
    {
        ix_ASSERT_FATAL(ok);
    }
};

constexpr ix_Result ix_OK = ix_Result(true);
constexpr ix_Result ix_ERROR = ix_Result(false);
