#include "ix_math.hpp"
#include "ix_doctest.hpp"
#include "ix_random.hpp"

#include <math.h>

float ix_to_degree(float x)
{
    return x * 180.0F / ix_PI;
}

ix_TEST_CASE("ix_to_degree")
{
    ix_EXPECT_EQF(ix_to_degree(0.0F), 0.0F);
    ix_EXPECT_EQF(ix_to_degree(ix_PI / 4.0F), 45.0F);
    ix_EXPECT_EQF(ix_to_degree(-ix_PI / 4.0F), -45.0F);
    ix_EXPECT_EQF(ix_to_degree(3.0F * ix_PI / 4.0F), 3.0F * 45.0F);
    ix_EXPECT_EQF(ix_to_degree(-3.0F * ix_PI / 4.0F), -3.0F * 45.0F);
    for (size_t i = 0; i < 25; i++)
    {
        const float x = ix_rand_range(-100.0F, 100.0F);
        ix_EXPECT_EQF(x, ix_to_radian(ix_to_degree(x)));
    }
}

float ix_to_radian(float x)
{
    return x / 180.0F * ix_PI;
}

ix_TEST_CASE("ix_to_radian")
{
    ix_EXPECT_EQF(ix_to_radian(0.0F), 0.0F);
    ix_EXPECT_EQF(ix_to_radian(45.0F), ix_PI / 4.0F);
    ix_EXPECT_EQF(ix_to_radian(-45.0F), -ix_PI / 4.0F);
    ix_EXPECT_EQF(ix_to_radian(3.0F * 45.0F), 3.0F * ix_PI / 4.0F);
    ix_EXPECT_EQF(ix_to_radian(-3.0F * 45.0F), -3.0F * ix_PI / 4.0F);
    for (size_t i = 0; i < 25; i++)
    {
        const float x = ix_rand_range(-100.0F, 100.0F);
        ix_EXPECT_EQF(x, ix_to_degree(ix_to_radian(x)));
    }
}

float ix_normalize_radian(float x)
{
    float rem = fmodf(x, ix_2PI);
    if (rem < -ix_PI)
    {
        rem += ix_2PI;
    }
    if (rem > ix_PI)
    {
        rem -= ix_2PI;
    }
    return rem;
}

ix_TEST_CASE("ix_normalize_radian")
{
    ix_EXPECT_EQF(ix_normalize_radian(0.0F), 0.0F);
    ix_EXPECT_EQF(ix_normalize_radian(ix_PI / 4.0F), ix_PI / 4.0F);
    ix_EXPECT_EQF(ix_normalize_radian(-ix_PI / 4.0F), -ix_PI / 4.0F);
    ix_EXPECT_EQF(ix_normalize_radian(7.0F * ix_PI / 4.0F), -ix_PI / 4.0F);
    ix_EXPECT_EQF(ix_normalize_radian(-7.0F * ix_PI / 4.0F), ix_PI / 4.0F);
    ix_EXPECT_EQF(ix_normalize_radian(ix_PI * 10 + ix_PI / 4.0F), ix_PI / 4.0F);
    ix_EXPECT_EQF(ix_normalize_radian(-ix_PI * 10 - ix_PI / 4.0F), -ix_PI / 4.0F);
}

float ix_normalize_degree(float x)
{
    float rem = fmodf(x, 360.0F);
    if (rem < -180.0F)
    {
        rem += 360.0F;
    }
    if (rem > 180.0F)
    {
        rem -= 360.0F;
    }
    return rem;
}

ix_TEST_CASE("ix_normalize_degree")
{
    ix_EXPECT_EQF(ix_normalize_degree(0.0F), 0.0F);
    ix_EXPECT_EQF(ix_normalize_degree(10.0F), 10.0F);
    ix_EXPECT_EQF(ix_normalize_degree(-10.0F), -10.0F);
    ix_EXPECT_EQF(ix_normalize_degree(200.0F), -160.0F);
    ix_EXPECT_EQF(ix_normalize_degree(-200.0F), 160.0F);
    ix_EXPECT_EQF(ix_normalize_degree(360.0F * 10 + 10.0F), 10.0F);
    ix_EXPECT_EQF(ix_normalize_degree(-360.0F * 10 - 10.0F), -10.0F);
}

ix_TEST_CASE("ix_is_zerof")
{
    ix_EXPECT(ix_is_zerof(0.0F));
    ix_EXPECT(!ix_is_zerof(1.0F));
    ix_EXPECT(!ix_is_zerof(-1.0F));
}

ix_TEST_CASE("ix_is_zerod")
{
    ix_EXPECT(ix_is_zerod(0.0));
    ix_EXPECT(!ix_is_zerod(1.0));
    ix_EXPECT(!ix_is_zerod(-1.0));
}

ix_TEST_CASE("ix_is_equald")
{
    ix_EXPECT(ix_is_equalf(0.0F, 0.0F));
    ix_EXPECT(ix_is_equalf(1.0F, 1.0F));
    ix_EXPECT(ix_is_equalf(-1.0F, -1.0F));
    ix_EXPECT(!ix_is_equalf(0.0F, 1.0F));
    ix_EXPECT(!ix_is_equalf(-1.0F, 0.0F));
}

ix_TEST_CASE("ix_is_equald")
{
    ix_EXPECT(ix_is_equald(0.0, 0.0));
    ix_EXPECT(ix_is_equald(1.0, 1.0));
    ix_EXPECT(ix_is_equald(-1.0, -1.0));
    ix_EXPECT(!ix_is_equald(0.0, 1.0));
    ix_EXPECT(!ix_is_equald(-1.0, 0.0));
}

bool ix_is_approx_eq(float x, float correct)
{
    if (ix_is_zerof(correct))
    {
        return x < 0.001F;
    }

    const float diff = correct - x;
    const float relative_diff = diff / correct;
    return fabsf(relative_diff) < 0.001F;
}

ix_TEST_CASE("ix_is_approx_eq")
{
    ix_EXPECT(ix_is_approx_eq(0, 0));
    ix_EXPECT(!ix_is_approx_eq(0.5, 0));
    ix_EXPECT(ix_is_approx_eq(100.09F, 100.0F));
    ix_EXPECT(!ix_is_approx_eq(100.11F, 100.0F));
}

uint64_t ix_log10(uint64_t n)
{
    uint64_t ans = 0;
    while (n > 9)
    {
        n /= 10;
        ans += 1;
    }
    return ans;
}

ix_TEST_CASE("ix_log10")
{
    ix_EXPECT(ix_log10(0) == 0);
    ix_EXPECT(ix_log10(1) == 0);
    ix_EXPECT(ix_log10(10) == 1);
    ix_EXPECT(ix_log10(30) == 1);
    ix_EXPECT(ix_log10(99) == 1);
    ix_EXPECT(ix_log10(100) == 2);
    ix_EXPECT(ix_log10(300) == 2);
    ix_EXPECT(ix_log10(999) == 2);
    ix_EXPECT(ix_log10(1000) == 3);
}

ix_TEST_CASE("ix_quot")
{
    ix_EXPECT(ix_quot(17, 3) == 5);
    ix_EXPECT(ix_quot(-17, 3) == -5);
    ix_EXPECT(ix_quot(17, -3) == -5);
    ix_EXPECT(ix_quot(-17, -3) == 5);

    ix_EXPECT(ix_quot(21, 3) == 7);
    ix_EXPECT(ix_quot(21, -3) == -7);
    ix_EXPECT(ix_quot(-21, 3) == -7);
    ix_EXPECT(ix_quot(-21, -3) == 7);

    ix_EXPECT(ix_quot(17, 1) == 17);
    ix_EXPECT(ix_quot(17, -1) == -17);
    ix_EXPECT(ix_quot(-17, 1) == -17);
    ix_EXPECT(ix_quot(-17, -1) == 17);
}

ix_TEST_CASE("ix_rem")
{
    ix_EXPECT(ix_rem(17, 3) == 2);
    ix_EXPECT(ix_rem(-17, 3) == -2);
    ix_EXPECT(ix_rem(17, -3) == 2);
    ix_EXPECT(ix_rem(-17, -3) == -2);

    ix_EXPECT(ix_rem(21, 3) == 0);
    ix_EXPECT(ix_rem(21, -3) == 0);
    ix_EXPECT(ix_rem(-21, 3) == 0);
    ix_EXPECT(ix_rem(-21, -3) == 0);

    ix_EXPECT(ix_rem(17, 1) == 0);
    ix_EXPECT(ix_rem(17, -1) == 0);
    ix_EXPECT(ix_rem(-17, 1) == 0);
    ix_EXPECT(ix_rem(-17, -1) == 0);
}

ix_TEST_CASE("ix_div")
{
    ix_EXPECT(ix_div(17, 3) == 5);
    ix_EXPECT(ix_div(-17, 3) == -6);
    ix_EXPECT(ix_div(17, -3) == -6);
    ix_EXPECT(ix_div(-17, -3) == 5);

    ix_EXPECT(ix_div(21, 3) == 7);
    ix_EXPECT(ix_div(21, -3) == -7);
    ix_EXPECT(ix_div(-21, 3) == -7);
    ix_EXPECT(ix_div(-21, -3) == 7);

    ix_EXPECT(ix_div(17, 1) == 17);
    ix_EXPECT(ix_div(17, -1) == -17);
    ix_EXPECT(ix_div(-17, 1) == -17);
    ix_EXPECT(ix_div(-17, -1) == 17);
}

ix_TEST_CASE("ix_mod")
{
    ix_EXPECT(ix_mod(17, 3) == 2);
    ix_EXPECT(ix_mod(-17, 3) == 1);
    ix_EXPECT(ix_mod(17, -3) == -1);
    ix_EXPECT(ix_mod(-17, -3) == -2);

    ix_EXPECT(ix_mod(21, 3) == 0);
    ix_EXPECT(ix_mod(21, -3) == 0);
    ix_EXPECT(ix_mod(-21, 3) == 0);
    ix_EXPECT(ix_mod(-21, -3) == 0);

    ix_EXPECT(ix_mod(17, 1) == 0);
    ix_EXPECT(ix_mod(17, -1) == 0);
    ix_EXPECT(ix_mod(-17, 1) == 0);
    ix_EXPECT(ix_mod(-17, -1) == 0);
}
