#pragma once

#include "ix.hpp"

template <class T>
struct ix_numeric_limits
{
    static constexpr T min();
    static constexpr T max();
    static constexpr bool is_signed = false;
    static constexpr bool is_integer = false;
};

template <>
struct ix_numeric_limits<int8_t>
{
    static constexpr int8_t min()
    {
        return (-127 - 1);
    }

    static constexpr int8_t max()
    {
        return 127;
    }

    static constexpr bool is_signed = true;
    static constexpr bool is_integer = true;
};

template <>
struct ix_numeric_limits<int16_t>
{
    static constexpr int16_t min()
    {
        return (-32767 - 1);
    }

    static constexpr int16_t max()
    {
        return 32767;
    }

    static constexpr bool is_signed = true;
    static constexpr bool is_integer = true;
};

template <>
struct ix_numeric_limits<int32_t>
{
    static constexpr int32_t min()
    {
        return (-2147483647 - 1);
    }

    static constexpr int32_t max()
    {
        return 2147483647;
    }

    static constexpr bool is_signed = true;
    static constexpr bool is_integer = true;
};

template <>
struct ix_numeric_limits<int64_t>
{
    static constexpr int64_t min()
    {
        return (-9223372036854775807LL - 1);
    }

    static constexpr int64_t max()
    {
        return 9223372036854775807LL;
    }

    static constexpr bool is_signed = true;
    static constexpr bool is_integer = true;
};

template <>
struct ix_numeric_limits<uint8_t>
{
    static constexpr uint8_t min()
    {
        return 0;
    }

    static constexpr uint8_t max()
    {
        return 0xFF;
    }

    static constexpr bool is_signed = false;
    static constexpr bool is_integer = true;
};

template <>
struct ix_numeric_limits<uint16_t>
{
    static constexpr uint16_t min()
    {
        return 0;
    }

    static constexpr uint16_t max()
    {
        return 0xFFFF;
    }

    static constexpr bool is_signed = false;
    static constexpr bool is_integer = true;
};

template <>
struct ix_numeric_limits<uint32_t>
{
    static constexpr uint32_t min()
    {
        return 0;
    }

    static constexpr uint32_t max()
    {
        return 0xFFFFFFFF;
    }

    static constexpr bool is_signed = false;
    static constexpr bool is_integer = true;
};

#if ix_PLATFORM(WASM)
template <>
struct ix_numeric_limits<size_t>
{
    static constexpr size_t min()
    {
        return 0;
    }

    static constexpr size_t max()
    {
        return 0xFFFFFFFF;
    }

    static constexpr bool is_signed = false;
    static constexpr bool is_integer = true;
};
#endif

template <>
struct ix_numeric_limits<uint64_t>
{
    static constexpr uint64_t min()
    {
        return 0;
    }

    static constexpr uint64_t max()
    {
        return 0xFFFFFFFFFFFFFFFF;
    }

    static constexpr bool is_signed = false;
    static constexpr bool is_integer = true;
};

template <>
struct ix_numeric_limits<float>
{
    static constexpr float min()
    {
        return 1.17549435e-38F;
    }

    static constexpr float max()
    {
        return 3.40282347e+38F;
    }

    static constexpr float epsilon()
    {
        return 1.19209290e-7F;
    }

    static constexpr bool is_signed = true;
    static constexpr bool is_integer = false;
};

template <>
struct ix_numeric_limits<double>
{
    static constexpr double min()
    {
        return 2.2250738585072014e-308;
    }

    static constexpr double max()
    {
        return 1.7976931348623157e+308;
    }

    static constexpr double epsilon()
    {
        return 2.2204460492503131e-16;
    }

    static constexpr bool is_signed = true;
    static constexpr bool is_integer = false;
};
