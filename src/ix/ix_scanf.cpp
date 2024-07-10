#include "ix_scanf.hpp"
#include "ix_doctest.hpp"
#include "ix_math.hpp"
#include "ix_random_fill.hpp"

#include <errno.h>
#include <float.h>
#include <limits.h>
#include <math.h>
#include <stdlib.h>

static bool ix_is_white(char c)
{
    return (c == ' ') || (c == '\n');
}

void ix_skip_to_next_word(const char **c)
{
    while ((**c != '\0') && !ix_is_white(**c))
    {
        *c += 1;
    }

    while ((**c != '\0') && ix_is_white(**c))
    {
        *c += 1;
    }
}

ix_TEST_CASE("ix_skip_to_next_word")
{
    const char *text = "hello world!";
    const char *p = text;
    ix_skip_to_next_word(&p);
    ix_EXPECT(p == text + 6);
    ix_EXPECT_EQSTR(p, "world!");

    ix_skip_to_next_word(&p);
    ix_EXPECT(p == text + 12);
    ix_EXPECT_EQSTR(p, "");

    ix_skip_to_next_word(&p);
    ix_EXPECT(p == text + 12);
    ix_EXPECT_EQSTR(p, "");
}

static char buf_ix_read_string[256];

const char *ix_read_string(const char **c)
{
    size_t size_written = 0;
    const size_t buf_len = ix_LENGTH_OF(buf_ix_read_string);
    while ((**c != '\0') && !ix_is_white(**c) && (size_written < buf_len - 1))
    {
        buf_ix_read_string[size_written] = **c;
        size_written += 1;
        *c += 1;
    }

    ix_ASSERT(size_written < buf_len);
    buf_ix_read_string[size_written] = '\0';

    if (size_written == buf_len - 1 && **c != '\0')
    {
        return nullptr;
    }

    while (ix_is_white(**c))
    {
        *c += 1;
    }

    return buf_ix_read_string;
}

ix_TEST_CASE("ix_read_string")
{
    // one word
    {
        const char *text = "hello";
        const char *p = ix_read_string(&text);
        ix_EXPECT(p != nullptr);
        ix_EXPECT_EQSTR(p, "hello");
    }

    // two words
    {
        const char *text = "hello world";
        const char *p = ix_read_string(&text);
        ix_EXPECT(p != nullptr);
        ix_EXPECT_EQSTR(p, "hello");
    }

    // length 255
    {
        char buf[255 + 1];
        ix_rand_fill_alphanumeric(buf, ix_LENGTH_OF(buf) - 1);
        const char *text = buf;
        const char *p = ix_read_string(&text);
        ix_EXPECT(p != nullptr);
    }

    // length 256
    {
        char buf[256 + 1];
        ix_rand_fill_alphanumeric(buf, ix_LENGTH_OF(buf) - 1);
        const char *text = buf;
        const char *p = ix_read_string(&text);
        ix_EXPECT(p == nullptr);
    }
}

float ix_read_float(const char **c)
{
    const float f = strtof(*c, nullptr);
    ix_skip_to_next_word(c);
    return f;
}

ix_TEST_CASE("ix_read_float")
{
    // one float
    {
        const char *text = "12.3";
        const float f = ix_read_float(&text);
        ix_EXPECT(f == doctest::ApproxF(12.3F));
        ix_EXPECT(*text == '\0');
    }

    // three floats
    {
        const char *text = "12.3 45.6 78.9";
        float f;
        f = ix_read_float(&text);
        ix_EXPECT(f == doctest::ApproxF(12.3F));
        f = ix_read_float(&text);
        ix_EXPECT(f == doctest::ApproxF(45.6F));
        f = ix_read_float(&text);
        ix_EXPECT(f == doctest::ApproxF(78.9F));
        ix_EXPECT(*text == '\0');
    }
}

int ix_read_int(const char **c)
{
    const int i = static_cast<int>(strtol(*c, nullptr, 10));
    ix_skip_to_next_word(c);
    return i;
}

ix_TEST_CASE("ix_read_int")
{
    // one int
    {
        const char *text = "123";
        const int i = ix_read_int(&text);
        ix_EXPECT(i == 123);
        ix_EXPECT(*text == '\0');
    }

    // three ints
    {
        const char *text = "123 -456 789";
        int i;
        i = ix_read_int(&text);
        ix_EXPECT(i == 123);
        i = ix_read_int(&text);
        ix_EXPECT(i == -456);
        i = ix_read_int(&text);
        ix_EXPECT(i == 789);
        ix_EXPECT(*text == '\0');
    }
}

unsigned int ix_read_uint(const char **c)
{
    const unsigned int i = static_cast<unsigned int>(strtoul(*c, nullptr, 10));
    ix_skip_to_next_word(c);
    return i;
}

ix_TEST_CASE("ix_read_uint: three ints")
{
    const char *text = "123 456 789";
    unsigned int i;
    i = ix_read_uint(&text);
    ix_EXPECT(i == 123);
    i = ix_read_uint(&text);
    ix_EXPECT(i == 456);
    i = ix_read_uint(&text);
    ix_EXPECT(i == 789);
    ix_EXPECT(*text == '\0');
}

void ix_next_line(const char **c)
{
    if (**c == '\0')
    {
        return;
    }

    while (**c != '\n')
    {
        *c += 1;
    }

    *c += 1;
}

ix_TEST_CASE("ix_next_line")
{
    const char *text = "hello\n"
                       "\n"
                       "world\n"
                       "\n";
    ix_next_line(&text);
    ix_EXPECT_EQSTR(text, "\nworld\n\n");
    ix_next_line(&text);
    ix_EXPECT_EQSTR(text, "world\n\n");
    ix_next_line(&text);
    ix_EXPECT_EQSTR(text, "\n");
    ix_next_line(&text);
    ix_EXPECT_EQSTR(text, "");
    ix_next_line(&text);
    ix_EXPECT_EQSTR(text, "");
}

void ix_skip_empty_lines(const char **c)
{
    while (**c == '\n')
    {
        ix_next_line(c);
    }
}

ix_TEST_CASE("ix_skip_empty_lines")
{
    const char *text = "hello\n"
                       "\n"
                       "world\n"
                       "\n";
    ix_skip_empty_lines(&text);
    ix_EXPECT_EQSTR(text, "hello\n\nworld\n\n");
    ix_next_line(&text);
    ix_skip_empty_lines(&text);
    ix_EXPECT_EQSTR(text, "world\n\n");
    ix_next_line(&text);
    ix_skip_empty_lines(&text);
    ix_EXPECT_EQSTR(text, "");
}

void ix_next_non_empty_line(const char **c)
{
    ix_next_line(c);
    while (**c == '\n')
    {
        ix_next_line(c);
    }
}

ix_TEST_CASE("ix_next_non_empty_line")
{
    const char *text = "hello\n"
                       "\n"
                       "world\n"
                       "\n";
    ix_next_non_empty_line(&text);
    ix_EXPECT_EQSTR(text, "world\n\n");
    ix_next_non_empty_line(&text);
    ix_EXPECT_EQSTR(text, "");
    ix_next_non_empty_line(&text);
    ix_EXPECT_EQSTR(text, "");
}

template <typename T>
ix_Result ix_string_convert(const char *s, T *x)
{
    const bool starts_with_space = (*s == ' ');
    if (starts_with_space)
    {
        return ix_ERROR;
    }

    if constexpr (ix_is_same_v<T, float>)
    {
        char *end;
        errno = 0;
        const float v = strtof(s, &end);
        if ((s == end) || (*end != '\0'))
        {
            return ix_ERROR;
        }

        if (errno == ERANGE)
        {
            ix_ASSERT(ix_is_equalf(v, HUGE_VALF) || ix_is_equalf(v, -HUGE_VALF) || //
                      ix_is_equalf(v, FLT_MIN) || ix_is_equalf(v, -FLT_MIN) || ix_is_equalf(v, 0.0F));
            return ix_ERROR;
        }

        *x = v;
        return ix_OK;
    }
    else if constexpr (ix_is_same_v<T, double>)
    {
        char *end;
        errno = 0;
        const double v = strtod(s, &end);
        if ((s == end) || (*end != '\0'))
        {
            return ix_ERROR;
        }

        if (errno == ERANGE)
        {
            ix_ASSERT(ix_is_equald(v, HUGE_VAL) || ix_is_equald(v, -HUGE_VAL) || //
                      ix_is_equald(v, DBL_MIN) || ix_is_equald(v, -DBL_MIN) || ix_is_equald(v, 0.0));
            return ix_ERROR;
        }

        *x = v;
        return ix_OK;
    }
    else if constexpr (ix_is_same_v<T, long>) // cppcheck-suppress multiCondition
    {
        char *end;
        errno = 0;
        const long v = strtol(s, &end, 10);
        if ((s == end) || (*end != '\0'))
        {
            return ix_ERROR;
        }

        if (errno == ERANGE)
        {
            ix_ASSERT((v == LONG_MIN) || (v == LONG_MAX));
            return ix_ERROR;
        }

        *x = v;
        return ix_OK;
    }
    else if constexpr (ix_is_same_v<T, long long>) // cppcheck-suppress multiCondition
    {
        char *end;
        errno = 0;
        const long long v = strtoll(s, &end, 10);
        if ((s == end) || (*end != '\0'))
        {
            return ix_ERROR;
        }

        if (errno == ERANGE)
        {
            ix_ASSERT((v == LLONG_MIN) || (v == LLONG_MAX));
            return ix_ERROR;
        }

        *x = v;
        return ix_OK;
    }
    else if constexpr (ix_is_same_v<T, unsigned long>) // cppcheck-suppress multiCondition
    {
        const bool has_minus_sign = (*s == '-');
        if (has_minus_sign)
        {
            return ix_ERROR;
        }

        char *end;
        errno = 0;
        const unsigned long v = strtoul(s, &end, 10);
        if ((s == end) || (*end != '\0'))
        {
            return ix_ERROR;
        }

        if (errno == ERANGE)
        {
            ix_ASSERT(v == ULONG_MAX);
            return ix_ERROR;
        }

        *x = v;
        return ix_OK;
    }
    else if constexpr (ix_is_same_v<T, unsigned long long>) // cppcheck-suppress multiCondition
    {
        const bool has_minus_sign = (*s == '-');
        if (has_minus_sign)
        {
            return ix_ERROR;
        }

        char *end;
        errno = 0;
        const unsigned long long v = strtoull(s, &end, 10);
        if ((s == end) || (*end != '\0'))
        {
            return ix_ERROR;
        }

        if (errno == ERANGE)
        {
            ix_ASSERT(v == ULLONG_MAX);
            return ix_ERROR;
        }

        *x = v;
        return ix_OK;
    }
    ix_UNREACHABLE();
}

template ix_Result ix_string_convert<float>(const char *s, float *x);
template ix_Result ix_string_convert<double>(const char *s, double *x);
template ix_Result ix_string_convert<long>(const char *s, long *x);
template ix_Result ix_string_convert<long long>(const char *s, long long *x);
template ix_Result ix_string_convert<unsigned long>(const char *s, unsigned long *x);
template ix_Result ix_string_convert<unsigned long long>(const char *s, unsigned long long *x);

ix_TEST_CASE("ix_string_convert<float>")
{
    float x = 41.0F;
    ix_EXPECT(ix_string_convert<float>("123.456", &x).is_ok());
    ix_EXPECT(x == doctest::ApproxF(123.456F));
    ix_EXPECT(ix_string_convert<float>("-123.456", &x).is_ok());
    ix_EXPECT(x == doctest::ApproxF(-123.456F));

    ix_EXPECT(ix_string_convert<float>("", &x).is_error());
    ix_EXPECT(ix_string_convert<float>(" 100", &x).is_error());
    ix_EXPECT(ix_string_convert<float>("10000000000000000000000000000000000000000000000", &x).is_error());
    ix_EXPECT(ix_string_convert<float>("-10000000000000000000000000000000000000000000000", &x).is_error());
    ix_EXPECT(ix_string_convert<float>("0.00000000000000000000000000000000000000000000001", &x).is_error());
    ix_EXPECT(ix_string_convert<float>("-0.00000000000000000000000000000000000000000000001", &x).is_error());
    ix_EXPECT(ix_string_convert<float>("-100x", &x).is_error());
    ix_EXPECT(ix_string_convert<float>("hello world", &x).is_error());

    ix_EXPECT(ix_string_convert<float>("3.40282347e+38", &x).is_ok());
    ix_EXPECT(ix_is_equalf(x, FLT_MAX));
    ix_EXPECT(ix_string_convert<float>("3.40282347e+39", &x).is_error());

    ix_EXPECT(ix_string_convert<float>("-3.40282347e+38", &x).is_ok());
    ix_EXPECT(ix_is_equalf(x, -FLT_MAX));
    ix_EXPECT(ix_string_convert<float>("-3.40282347e+39", &x).is_error());

    ix_EXPECT(ix_string_convert<float>("1.17549435e-38", &x).is_ok());
    ix_EXPECT(ix_is_equalf(x, FLT_MIN));
    ix_EXPECT(ix_string_convert<float>("1.17549435e-100", &x).is_error());

    ix_EXPECT(ix_string_convert<float>("-1.17549435e-38", &x).is_ok());
    ix_EXPECT(ix_is_equalf(x, -FLT_MIN));
    ix_EXPECT(ix_string_convert<float>("-1.17549435e-100", &x).is_error());
}

ix_TEST_CASE("ix_string_convert<double>")
{
    double x = 41.0;
    ix_EXPECT(ix_string_convert<double>("123.456", &x).is_ok());
    ix_EXPECT(x == doctest::Approx(123.456));
    ix_EXPECT(ix_string_convert<double>("-123.456", &x).is_ok());
    ix_EXPECT(x == doctest::Approx(-123.456));

    ix_EXPECT(ix_string_convert<double>("", &x).is_error());
    ix_EXPECT(ix_string_convert<double>(" 100", &x).is_error());
    ix_EXPECT(ix_string_convert<double>("1.0e10000", &x).is_error());
    ix_EXPECT(ix_string_convert<double>("-1.0e10000", &x).is_error());
    ix_EXPECT(ix_string_convert<double>("1.0e-10000", &x).is_error());
    ix_EXPECT(ix_string_convert<double>("-1.0e-10000", &x).is_error());
    ix_EXPECT(ix_string_convert<double>("-100x", &x).is_error());
    ix_EXPECT(ix_string_convert<double>("hello world", &x).is_error());

    ix_EXPECT(ix_string_convert<double>("1.7976931348623157e+308", &x).is_ok());
    ix_EXPECT(ix_is_equald(x, DBL_MAX));
    ix_EXPECT(ix_string_convert<double>("1.7976931348623157e+309", &x).is_error());

    ix_EXPECT(ix_string_convert<double>("-1.7976931348623157e+308", &x).is_ok());
    ix_EXPECT(ix_is_equald(x, -DBL_MAX));
    ix_EXPECT(ix_string_convert<double>("1.7976931348623157e+310", &x).is_error());

    ix_EXPECT(ix_string_convert<double>("2.2250738585072014e-308", &x).is_ok());
    ix_EXPECT(ix_is_equald(x, DBL_MIN));
    ix_EXPECT(ix_string_convert<double>("2.2250738585072014e-400", &x).is_error());

    ix_EXPECT(ix_string_convert<double>("-2.2250738585072014e-308", &x).is_ok());
    ix_EXPECT(ix_is_equald(x, -DBL_MIN));
    ix_EXPECT(ix_string_convert<double>("-2.2250738585072014e-400", &x).is_error());
}

ix_TEST_CASE("ix_string_convert<long>")
{
    long x = 41;
    ix_EXPECT(ix_string_convert<long>("100", &x).is_ok());
    ix_EXPECT(x == 100);
    ix_EXPECT(ix_string_convert<long>("-100", &x).is_ok());
    ix_EXPECT(x == -100);

    ix_EXPECT(ix_string_convert<long>("", &x).is_error());
    ix_EXPECT(ix_string_convert<long>(" 100", &x).is_error());
#if !ix_COMPILER(MSVC)
    // cf. https://developercommunity.visualstudio.com/t/Cannot-detect-strtol-range-errors-with-A/10412869
    ix_EXPECT(ix_string_convert<long>("10000000000000000000000000000000000000000000000", &x).is_error());
    ix_EXPECT(ix_string_convert<long>("-10000000000000000000000000000000000000000000000", &x).is_error());
#endif
    ix_EXPECT(ix_string_convert<long>("-100x", &x).is_error());
    ix_EXPECT(ix_string_convert<long>("hello world", &x).is_error());

    if constexpr (sizeof(long) == 4)
    {
        ix_EXPECT(ix_string_convert<long>("-2147483648", &x).is_ok());
        ix_EXPECT(x == LONG_MIN);

        ix_EXPECT(ix_string_convert<long>("2147483647", &x).is_ok());
        ix_EXPECT(x == LONG_MAX);

#if !ix_COMPILER(MSVC)
        // cf. https://developercommunity.visualstudio.com/t/Cannot-detect-strtol-range-errors-with-A/10412869
        ix_EXPECT(ix_string_convert<long>("-2147483649", &x).is_error());
        ix_EXPECT(ix_string_convert<long>("2147483648", &x).is_error());
#endif
    }
    else
    {
        ix_EXPECT(ix_string_convert<long>("-9223372036854775808", &x).is_ok());
        ix_EXPECT(x == LONG_MIN);

        ix_EXPECT(ix_string_convert<long>("9223372036854775807", &x).is_ok());
        ix_EXPECT(x == LONG_MAX);

        ix_EXPECT(ix_string_convert<long>("-9223372036854775809", &x).is_error());
        ix_EXPECT(ix_string_convert<long>("9223372036854775808", &x).is_error());
    }
}

ix_TEST_CASE("ix_string_convert<long long>")
{
    long long x = 41;
    ix_EXPECT(ix_string_convert<long long>("100", &x).is_ok());
    ix_EXPECT(x == 100);
    ix_EXPECT(ix_string_convert<long long>("-100", &x).is_ok());
    ix_EXPECT(x == -100);

    ix_EXPECT(ix_string_convert<long long>("", &x).is_error());
    ix_EXPECT(ix_string_convert<long long>(" 100", &x).is_error());
    ix_EXPECT(ix_string_convert<long long>("10000000000000000000000000000000000000000000000", &x).is_error());
    ix_EXPECT(ix_string_convert<long long>("-10000000000000000000000000000000000000000000000", &x).is_error());
    ix_EXPECT(ix_string_convert<long long>("-100x", &x).is_error());
    ix_EXPECT(ix_string_convert<long long>("hello world", &x).is_error());

    if constexpr (sizeof(long long) == 8)
    {
        ix_EXPECT(ix_string_convert<long long>("-9223372036854775808", &x).is_ok());
        ix_EXPECT(x == LLONG_MIN);

        ix_EXPECT(ix_string_convert<long long>("9223372036854775807", &x).is_ok());
        ix_EXPECT(x == LLONG_MAX);

        ix_EXPECT(ix_string_convert<long long>("-9223372036854775809", &x).is_error());
        ix_EXPECT(ix_string_convert<long long>("9223372036854775808", &x).is_error());
    }
    else
    {
        ix_EXPECT(false);
    }
}

ix_TEST_CASE("ix_string_convert<unsigned long>")
{
    unsigned long x = 41;
    ix_EXPECT(ix_string_convert<unsigned long>("100", &x).is_ok());
    ix_EXPECT(x == 100);

    ix_EXPECT(ix_string_convert<unsigned long>("", &x).is_error());
    ix_EXPECT(ix_string_convert<unsigned long>(" 100", &x).is_error());
    ix_EXPECT(ix_string_convert<unsigned long>("-100", &x).is_error());
    ix_EXPECT(ix_string_convert<unsigned long>("10000000000000000000000000000000000000000000000", &x).is_error());
    ix_EXPECT(ix_string_convert<unsigned long>("-10000000000000000000000000000000000000000000000", &x).is_error());
    ix_EXPECT(ix_string_convert<unsigned long>("100x", &x).is_error());
    ix_EXPECT(ix_string_convert<unsigned long>("-100x", &x).is_error());
    ix_EXPECT(ix_string_convert<unsigned long>("hello world", &x).is_error());

    if constexpr (sizeof(unsigned long) == 4)
    {
        ix_EXPECT(ix_string_convert<unsigned long>("4294967295", &x).is_ok());
        ix_EXPECT(x == ULONG_MAX);

        ix_EXPECT(ix_string_convert<unsigned long>("4294967296", &x).is_error());
    }
    else
    {
        ix_EXPECT(ix_string_convert<unsigned long>("18446744073709551615", &x).is_ok());
        ix_EXPECT(x == ULONG_MAX);

        ix_EXPECT(ix_string_convert<unsigned long>("18446744073709551616", &x).is_error());
    }
}

ix_TEST_CASE("ix_string_convert<unsigned long long>")
{
    unsigned long long x = 41;
    ix_EXPECT(ix_string_convert<unsigned long long>("100", &x).is_ok());
    ix_EXPECT(x == 100);

    ix_EXPECT(ix_string_convert<unsigned long long>("", &x).is_error());
    ix_EXPECT(ix_string_convert<unsigned long long>(" 100", &x).is_error());
    ix_EXPECT(ix_string_convert<unsigned long long>("-100", &x).is_error());
    ix_EXPECT(ix_string_convert<unsigned long long>("10000000000000000000000000000000000000000000", &x).is_error());
    ix_EXPECT(ix_string_convert<unsigned long long>("-10000000000000000000000000000000000000000000", &x).is_error());
    ix_EXPECT(ix_string_convert<unsigned long long>("100x", &x).is_error());
    ix_EXPECT(ix_string_convert<unsigned long long>("-100x", &x).is_error());
    ix_EXPECT(ix_string_convert<unsigned long long>("hello world", &x).is_error());

    if constexpr (sizeof(unsigned long long) == 8)
    {
        ix_EXPECT(ix_string_convert<unsigned long long>("18446744073709551615", &x).is_ok());
        ix_EXPECT(x == ULLONG_MAX);

        ix_EXPECT(ix_string_convert<unsigned long long>("18446744073709551616", &x).is_error());
    }
    else
    {
        ix_EXPECT(false);
    }
}
