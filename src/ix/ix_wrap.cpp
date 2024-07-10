#include "ix_wrap.hpp"
#include "ix_doctest.hpp"

ix_TEST_CASE("ix_can_add")
{
    ix_EXPECT(ix_can_add(0, 0));
    ix_EXPECT(ix_can_add(0.0F, 0.0F));
    ix_EXPECT(ix_can_add(0.0, 0.0));

    ix_EXPECT(ix_can_add(ix_numeric_limits<int64_t>::max(), int64_t{0}));
    ix_EXPECT(!ix_can_add(ix_numeric_limits<int64_t>::max(), int64_t{1}));
    ix_EXPECT(!ix_can_add(ix_numeric_limits<int64_t>::max() - 90, int64_t{100}));
    ix_EXPECT(ix_can_add(ix_numeric_limits<int64_t>::max() - 90, int64_t{80}));

    ix_EXPECT(ix_can_add(int64_t{0}, ix_numeric_limits<int64_t>::max()));
    ix_EXPECT(!ix_can_add(int64_t{1}, ix_numeric_limits<int64_t>::max()));
    ix_EXPECT(!ix_can_add(int64_t{100}, ix_numeric_limits<int64_t>::max() - 90));
    ix_EXPECT(ix_can_add(int64_t{80}, ix_numeric_limits<int64_t>::max() - 90));

    ix_EXPECT(ix_can_add(ix_numeric_limits<int64_t>::min(), int64_t{0}));
    ix_EXPECT(!ix_can_add(ix_numeric_limits<int64_t>::min(), int64_t{-1}));
    ix_EXPECT(!ix_can_add(ix_numeric_limits<int64_t>::min() + 90, int64_t{-100}));
    ix_EXPECT(ix_can_add(ix_numeric_limits<int64_t>::min() + 90, int64_t{-80}));

    ix_EXPECT(ix_can_add(int64_t{0}, ix_numeric_limits<int64_t>::min()));
    ix_EXPECT(!ix_can_add(int64_t{-1}, ix_numeric_limits<int64_t>::min()));
    ix_EXPECT(!ix_can_add(int64_t{-100}, ix_numeric_limits<int64_t>::min() + 90));
    ix_EXPECT(ix_can_add(int64_t{-80}, ix_numeric_limits<int64_t>::min() + 90));

    ix_EXPECT(ix_can_add(ix_numeric_limits<int64_t>::min(), ix_numeric_limits<int64_t>::max()));
    ix_EXPECT(ix_can_add(ix_numeric_limits<int64_t>::max(), ix_numeric_limits<int64_t>::min()));
    ix_EXPECT(!ix_can_add(ix_numeric_limits<int64_t>::max(), ix_numeric_limits<int64_t>::max()));
    ix_EXPECT(!ix_can_add(ix_numeric_limits<int64_t>::min(), ix_numeric_limits<int64_t>::min()));
}

ix_TEST_CASE("ix_can_sub")
{
    ix_EXPECT(ix_can_sub(0, 0));
    ix_EXPECT(ix_can_sub(0.0F, 0.0F));
    ix_EXPECT(ix_can_sub(0.0, 0.0));

    ix_EXPECT(ix_can_sub(ix_numeric_limits<int64_t>::max(), int64_t{0}));
    ix_EXPECT(!ix_can_sub(ix_numeric_limits<int64_t>::max(), int64_t{-1}));
    ix_EXPECT(!ix_can_sub(ix_numeric_limits<int64_t>::max() - 90, int64_t{-100}));
    ix_EXPECT(ix_can_sub(ix_numeric_limits<int64_t>::max() - 90, int64_t{-80}));

    ix_EXPECT(ix_can_sub(int64_t{0}, ix_numeric_limits<int64_t>::max()));
    ix_EXPECT(ix_can_sub(int64_t{-1}, ix_numeric_limits<int64_t>::max()));
    ix_EXPECT(!ix_can_sub(int64_t{-2}, ix_numeric_limits<int64_t>::max()));
    ix_EXPECT(!ix_can_sub(int64_t{-100}, ix_numeric_limits<int64_t>::max() - 90));
    ix_EXPECT(ix_can_sub(int64_t{-80}, ix_numeric_limits<int64_t>::max() - 90));

    ix_EXPECT(ix_can_sub(ix_numeric_limits<int64_t>::min(), int64_t{0}));
    ix_EXPECT(!ix_can_sub(ix_numeric_limits<int64_t>::min(), int64_t{1}));
    ix_EXPECT(!ix_can_sub(ix_numeric_limits<int64_t>::min() + 90, int64_t{100}));
    ix_EXPECT(ix_can_sub(ix_numeric_limits<int64_t>::min() + 90, int64_t{80}));

    ix_EXPECT(!ix_can_sub(int64_t{0}, ix_numeric_limits<int64_t>::min()));
    ix_EXPECT(!ix_can_sub(int64_t{1}, ix_numeric_limits<int64_t>::min()));
    ix_EXPECT(!ix_can_sub(int64_t{100}, ix_numeric_limits<int64_t>::min() + 90));
    ix_EXPECT(ix_can_sub(int64_t{80}, ix_numeric_limits<int64_t>::min() + 90));

    ix_EXPECT(!ix_can_sub(ix_numeric_limits<int64_t>::min(), ix_numeric_limits<int64_t>::max()));
    ix_EXPECT(!ix_can_sub(ix_numeric_limits<int64_t>::max(), ix_numeric_limits<int64_t>::min()));
    ix_EXPECT(ix_can_sub(ix_numeric_limits<int64_t>::max(), ix_numeric_limits<int64_t>::max()));
    ix_EXPECT(ix_can_sub(ix_numeric_limits<int64_t>::min(), ix_numeric_limits<int64_t>::min()));
}

ix_TEST_CASE("ix_wrap_increment")
{
    ix_EXPECT(ix_wrap_increment(10, 5, 15) == 11);
    ix_EXPECT(ix_wrap_increment(5, 5, 15) == 6);
    ix_EXPECT(ix_wrap_increment(14, 5, 15) == 5);
}
