#include "ix_HollowValue.hpp"
#include "ix_Vector.hpp"
#include "ix_doctest.hpp"

ix_TEST_CASE("ix_HollowValue")
{
    static ix_HollowValue<ix_Vector<int>> g_vector;

    g_vector.construct(size_t{10});
    const ix_Vector<int> &v_const = g_vector.get();
    ix_EXPECT(v_const.size() == 10);
    ix_Vector<int> &v = g_vector.get();
    v[0] = 10;
    ix_EXPECT(v_const[0] == 10);
    g_vector.destruct();
}
