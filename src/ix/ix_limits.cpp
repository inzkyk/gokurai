#include "ix_limits.hpp"
#include "ix_doctest.hpp"

ix_TEST_CASE("ix_limits")
{
    ix_EXPECT(ix_numeric_limits<int8_t>::min() < ix_numeric_limits<int8_t>::max());
    ix_EXPECT(ix_numeric_limits<int16_t>::min() < ix_numeric_limits<int16_t>::max());
    ix_EXPECT(ix_numeric_limits<int32_t>::min() < ix_numeric_limits<int32_t>::max());
    ix_EXPECT(ix_numeric_limits<int64_t>::min() < ix_numeric_limits<int64_t>::max());
    ix_EXPECT(ix_numeric_limits<uint8_t>::min() < ix_numeric_limits<uint8_t>::max());
    ix_EXPECT(ix_numeric_limits<uint16_t>::min() < ix_numeric_limits<uint16_t>::max());
    ix_EXPECT(ix_numeric_limits<uint32_t>::min() < ix_numeric_limits<uint32_t>::max());
    ix_EXPECT(ix_numeric_limits<uint64_t>::min() < ix_numeric_limits<uint64_t>::max());
    ix_EXPECT(ix_numeric_limits<uint64_t>::min() < ix_numeric_limits<uint64_t>::max());
    ix_EXPECT(ix_numeric_limits<float>::min() < ix_numeric_limits<float>::max());
    ix_EXPECT(ix_numeric_limits<double>::min() < ix_numeric_limits<double>::max());
    ix_EXPECT(ix_numeric_limits<float>::epsilon() < 1.0F);
    ix_EXPECT(ix_numeric_limits<float>::epsilon() + 1 > 1);
    ix_EXPECT(ix_numeric_limits<double>::epsilon() < 1);
    ix_EXPECT(ix_numeric_limits<double>::epsilon() + 1 > 1);
}
