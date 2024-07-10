#include "ix_printf.hpp"
#include "ix_assert.hpp"
#include "ix_doctest.hpp"

#include <stdio.h>

ix_PRINTF_FORMAT(3, 0) int ix_vsnprintf(char *buf, size_t buf_length, const char *format, va_list args)
{
    va_list args_copy;
    va_copy(args_copy, args); // Copy must occur here.

    const int formatted_length_signed = vsnprintf(buf, buf_length, format, args);
    ix_ASSERT(formatted_length_signed >= 0);

    if (buf == nullptr)
    {
        va_end(args_copy);
        return formatted_length_signed;
    }

    const size_t required_length = static_cast<size_t>(formatted_length_signed) + 1;
    ix_ASSERT_FATAL(required_length <= buf_length);

    const int wrote_length = vsnprintf(buf, required_length, format, args_copy);
    ix_ASSERT(wrote_length == formatted_length_signed);
    ix_ASSERT(buf[wrote_length] == '\0');

    va_end(args_copy);

    return wrote_length;
}

ix_PRINTF_FORMAT(3, 4) int ix_snprintf(char *buf, size_t buf_length, ix_FORMAT_ARG const char *format, ...)
{
    va_list args;
    va_start(args, format);
    const int v = ix_vsnprintf(buf, buf_length, format, args);
    va_end(args);
    return v;
}

ix_TEST_CASE("ix_snprintf")
{
    ix_DISABLE_GCC_WARNING_BEGIN
    ix_DISABLE_GCC_WARNING("-Wformat-zero-length")
    ix_EXPECT(ix_snprintf(nullptr, 0, "") == 0);
    ix_DISABLE_GCC_WARNING_END

    ix_EXPECT(ix_snprintf(nullptr, 0, "hello") == 5);
    ix_EXPECT(ix_snprintf(nullptr, 0, "%d", 0) == 1);
    ix_EXPECT(ix_snprintf(nullptr, 0, "%d", 123) == 3);
    ix_EXPECT(ix_snprintf(nullptr, 0, "%d", -123) == 4);
    ix_EXPECT(ix_snprintf(nullptr, 0, "%s", "hello") == 5);

    char buf[128];

    ix_DISABLE_GCC_WARNING_BEGIN
    ix_DISABLE_GCC_WARNING("-Wformat-zero-length")
    ix_snprintf(buf, 1, "");
    ix_EXPECT_EQSTR(buf, "");
    ix_DISABLE_GCC_WARNING_END

    ix_snprintf(buf, 6, "hello");
    ix_EXPECT_EQSTR(buf, "hello");
    ix_snprintf(buf, 2, "%d", 0);
    ix_EXPECT_EQSTR(buf, "0");
    ix_snprintf(buf, 4, "%d", 123);
    ix_EXPECT_EQSTR(buf, "123");
    ix_snprintf(buf, 5, "%d", -123);
    ix_EXPECT_EQSTR(buf, "-123");
    ix_snprintf(buf, 6, "%s", "hello");
    ix_EXPECT_EQSTR(buf, "hello");
}
