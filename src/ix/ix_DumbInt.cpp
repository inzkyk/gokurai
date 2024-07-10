#include "ix_DumbInt.hpp"
#include "ix_doctest.hpp"

ix_TEST_CASE("ix_DumbInt256")
{
    ix_EXPECT(ix_DumbInt256() == ix_DumbInt256(0));
    ix_EXPECT(ix_DumbInt256(1) == ix_DumbInt256(1));
    ix_EXPECT(ix_DumbInt256(1) == 1);
    ix_EXPECT(ix_DumbInt256(1) != ix_DumbInt256(10));
    ix_EXPECT(ix_DumbInt256(1) != 10);
}
