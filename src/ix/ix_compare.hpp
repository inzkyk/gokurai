#pragma once

#include "ix.hpp"
#include "ix_math.hpp"
#include "ix_string.hpp"

ix_DISABLE_GCC_WARNING_BEGIN
ix_DISABLE_GCC_WARNING("-Wfloat-equal")
ix_DISABLE_CLANG_WARNING_BEGIN
ix_DISABLE_CLANG_WARNING("-Wfloat-equal")

template <typename T>
struct ix_Equal
{
    constexpr bool operator()(const T &x, const T &y) const
    {
        return (x == y);
    }
};

ix_DISABLE_GCC_WARNING_END
ix_DISABLE_CLANG_WARNING_END

template <typename T>
struct ix_Less
{
    constexpr bool operator()(const T &x, const T &y) const
    {
        return (x < y);
    }
};

template <typename T>
struct ix_Greater
{
    constexpr bool operator()(const T &x, const T &y) const
    {
        return (x > y);
    }
};

struct ix_StringEqual
{
    constexpr bool operator()(const char *x, const char *y) const
    {
        return (ix_strcmp(x, y) == 0);
    }
};

template <typename T>
struct ix_Compare
{
    constexpr int8_t operator()(const T x, const T y) const
    {
        if (x < y)
        {
            return -1;
        }
        if (x > y)
        {
            return 1;
        }
        return 0;
    }
};

template <>
struct ix_Compare<const char *>
{
    constexpr int8_t operator()(const char *x, const char *y) const
    {
        return static_cast<int8_t>(ix_strcmp(x, y));
    }
};
