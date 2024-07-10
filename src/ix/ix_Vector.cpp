#include "ix_Vector.hpp"
#include "ix_doctest.hpp"
#include "ix_test_Vector.hpp"

ix_TEST_CASE("ix_Vector")
{
    ix_test_Vector<ix_Vector>();
}

ix_TEST_CASE("ix_Vector: default constructor")
{
    ix_Vector<uint64_t> v;
    ix_EXPECT(v.data() == nullptr);
    ix_EXPECT(v.empty());
    ix_EXPECT(v.capacity() == 0);
}

ix_TEST_CASE("ix_Vector: initializer list")
{
    const ix_Vector<int> v{};
    ix_EXPECT(v.empty());
    ix_EXPECT(v.capacity() == 0);

    const ix_Vector<int> w{0, 10, 20, 30};
    ix_EXPECT(w.size() == 4);
    ix_EXPECT(w.capacity() == 4);
    ix_EXPECT(w[0] == 0);
    ix_EXPECT(w[1] == 10);
    ix_EXPECT(w[2] == 20);
    ix_EXPECT(w[3] == 30);
}
