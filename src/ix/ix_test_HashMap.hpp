#pragma once

#include "ix.hpp"
#include "ix_DumbString.hpp"
#include "ix_Vector.hpp"
#include "ix_compare.hpp"
#include "ix_hash.hpp"
#include "ix_test_Map.hpp"

struct EqHash
{
    size_t operator()(uint64_t x) const
    {
        return static_cast<size_t>(x);
    }
};

struct DumbHash
{
    size_t operator()(uint64_t) const
    {
        return static_cast<size_t>(0xDeadBeefDeadBeef);
    }
};

template <typename T>
struct ix_Hash<ix_Vector<T>>
{
    size_t operator()(const ix_Vector<T> &x) const
    {
        return ix_hash(x.data(), x.size() * sizeof(T));
    }
};

template <template <typename...> typename HashMap, typename Value>
using StringHashMap = HashMap<const char *, Value, ix_StringHash, ix_StringEqual>;

template <template <typename...> typename HashMap>
void ix_test_HashMap_EqHash()
{
    HashMap<uint64_t, uint64_t, EqHash> m;

    ix_EXPECT(m.size() == 0);

    m.clear();
    ix_EXPECT(m.size() == 0);

    m.reserve(1);
    ix_EXPECT(m.size() == 0);
    ix_EXPECT(m.capacity() >= 1);

    m.clear();
    ix_EXPECT(m.size() == 0);
    ix_EXPECT(m.capacity() >= 1);

    for (uint64_t i = 0; i < 4; i++)
    {
        m.clear();
        m.emplace(1ULL, 11ULL);
        m.emplace(2ULL, 22ULL);
        m.emplace(3ULL, 33ULL);
        m.emplace(4ULL, 44ULL);
        ix_EXPECT(m[1] == 11);
        ix_EXPECT(m[2] == 22);
        ix_EXPECT(m[3] == 33);
        ix_EXPECT(m[4] == 44);

        ix_EXPECT(m.size() == 4);
    }

    m.clear();
    m.emplace(1ULL, 11ULL);
    m.emplace(9ULL, 99ULL);
    m.emplace(17ULL, 1717ULL);
    m.emplace(25ULL, 2525ULL);
    ix_EXPECT(m[1] == 11);
    ix_EXPECT(m[9] == 99);
    ix_EXPECT(m[17] == 1717);
    ix_EXPECT(m[25] == 2525);
    ix_EXPECT(m.size() == 4);

    m.clear();
    m.emplace(7ULL, 77ULL);
    m.emplace(15ULL, 1515ULL);
    m.emplace(22ULL, 2222ULL);
    m.emplace(29ULL, 2929ULL);
    ix_EXPECT(m[7] == 77);
    ix_EXPECT(m[15] == 1515);
    ix_EXPECT(m[22] == 2222);
    ix_EXPECT(m[29] == 2929);
    ix_EXPECT(m.size() == 4);

    m.clear();
    m.emplace(1ULL, 11ULL);
    m.emplace(8ULL, 88ULL);
    m.emplace(2ULL, 22ULL);
    m.emplace(10ULL, 1010ULL);
    ix_EXPECT(m[1] == 11);
    ix_EXPECT(m[8] == 88);
    ix_EXPECT(m[2] == 22);
    ix_EXPECT(m[10] == 1010);
    ix_EXPECT(m.size() == 4);

    m.clear();
    m.emplace(1ULL, 11ULL);
    m.emplace(3ULL, 33ULL);
    m.emplace(5ULL, 55ULL);
    m.emplace(7ULL, 77ULL);
    m.emplace(9ULL, 99ULL);
    m.emplace(11ULL, 1111ULL);
    ix_EXPECT(m[1] == 11);
    ix_EXPECT(m[3] == 33);
    ix_EXPECT(m[5] == 55);
    ix_EXPECT(m[7] == 77);
    ix_EXPECT(m[9] == 99);
    ix_EXPECT(m[11] == 1111);
    ix_EXPECT(m.size() == 6);

    constexpr uint64_t N = 512;
    m.clear();
    for (uint64_t i = 0; i < N; i++)
    {
        m.emplace(i, i * i);
    }
    for (uint64_t i = 0; i < N; i++)
    {
        ix_EXPECT(m[i] == i * i);
    }
    ix_EXPECT(m.size() == N);
}

template <template <typename...> typename HashMap>
void ix_test_HashMap_robin_hood()
{
    HashMap<uint64_t, uint64_t, EqHash> m;
    m.emplace(1ULL, 11ULL);
    m.emplace(17ULL, 1717ULL);
    m.emplace(3ULL, 33ULL);
    m.emplace(19ULL, 1919ULL);
    m.emplace(0ULL, 0ULL);
    m.emplace(16ULL, 88ULL);

    ix_EXPECT(m.size() == 6);
    ix_EXPECT(m[1] == 11);
    ix_EXPECT(m[17] == 1717);
    ix_EXPECT(m[3] == 33);
    ix_EXPECT(m[19] == 1919);
    ix_EXPECT(m[0] == 0);
    ix_EXPECT(m[16] == 88);
}

template <template <typename...> typename HashMap>
void ix_test_HashMap_many_collisions()
{
    HashMap<uint64_t, uint64_t, DumbHash> m;
    constexpr uint64_t N = 512;
    for (uint64_t i = 0; i < N; i++)
    {
        m.emplace(i, i * i);
    }
    for (uint64_t i = 0; i < N; i++)
    {
        ix_EXPECT(m[i] == i * i);
        ix_EXPECT(m.contains(i));
        ix_EXPECT(!m.contains(N + i));
    }
}

template <template <typename...> typename HashMap>
void ix_test_HashMap_tricky_erase()
{
    HashMap<int, int, EqHash> m;

    m.erase(0);
    ix_EXPECT(m.size() == 0);

    m.clear();
    m.emplace(1, 11);
    m.erase(1);
    ix_EXPECT(!m.contains(1));
    ix_EXPECT(m.size() == 0);

    m.clear();
    m.emplace(1, 11);
    m.emplace(2, 22);
    m.emplace(3, 33);
    m.erase(1);
    ix_EXPECT(m.size() == 2);
    ix_EXPECT(!m.contains(1));
    ix_EXPECT(m[2] == 22);
    ix_EXPECT(m[3] == 33);

    m.clear();
    m.emplace(1, 11);
    m.emplace(2, 22);
    m.emplace(3, 33);
    m.erase(2);
    ix_EXPECT(m.size() == 2);
    ix_EXPECT(!m.contains(2));
    ix_EXPECT(m[1] == 11);
    ix_EXPECT(m[3] == 33);

    m.clear();
    m.emplace(1, 11);
    m.emplace(2, 22);
    m.emplace(3, 33);
    m.erase(3);
    ix_EXPECT(m.size() == 2);
    ix_EXPECT(!m.contains(3));
    ix_EXPECT(m[1] == 11);
    ix_EXPECT(m[2] == 22);

    m.clear();
    m.emplace(1, 11);
    m.emplace(9, 99);
    m.emplace(17, 1717);
    m.emplace(25, 2525);
    m.erase(1);
    ix_EXPECT(m.size() == 3);
    ix_EXPECT(!m.contains(1));
    // ix_EXPECT(m[1] == 11);
    ix_EXPECT(m[9] == 99);
    ix_EXPECT(m[17] == 1717);
    ix_EXPECT(m[25] == 2525);

    m.clear();
    m.emplace(1, 11);
    m.emplace(9, 99);
    m.emplace(17, 1717);
    m.emplace(25, 2525);
    m.erase(9);
    ix_EXPECT(m.size() == 3);
    ix_EXPECT(!m.contains(9));
    ix_EXPECT(m[1] == 11);
    // ix_EXPECT(m[9] == 99);
    ix_EXPECT(m[17] == 1717);
    ix_EXPECT(m[25] == 2525);

    m.clear();
    m.emplace(1, 11);
    m.emplace(9, 99);
    m.emplace(17, 1717);
    m.emplace(25, 2525);
    m.erase(17);
    ix_EXPECT(m.size() == 3);
    ix_EXPECT(!m.contains(17));
    ix_EXPECT(m[1] == 11);
    ix_EXPECT(m[9] == 99);
    // ix_EXPECT(m[17] == 1717);
    ix_EXPECT(m[25] == 2525);

    m.clear();
    m.emplace(1, 11);
    m.emplace(9, 99);
    m.emplace(17, 1717);
    m.emplace(25, 2525);
    m.erase(25);
    ix_EXPECT(m.size() == 3);
    ix_EXPECT(!m.contains(25));
    ix_EXPECT(m[1] == 11);
    ix_EXPECT(m[9] == 99);
    ix_EXPECT(m[17] == 1717);
    // ix_EXPECT(m[25] == 2525);

    m.clear();
    m.emplace(7, 77);
    m.emplace(15, 1515);
    m.emplace(23, 2323);
    m.emplace(31, 3131);
    m.erase(7);
    ix_EXPECT(m.size() == 3);
    ix_EXPECT(!m.contains(7));
    // ix_EXPECT(m[7] == 77);
    ix_EXPECT(m[15] == 1515);
    ix_EXPECT(m[23] == 2323);
    ix_EXPECT(m[31] == 3131);

    m.clear();
    m.emplace(7, 77);
    m.emplace(15, 1515);
    m.emplace(23, 2323);
    m.emplace(31, 3131);
    m.erase(7);
    m.erase(23);
    m.erase(15);
    m.erase(31);
    ix_EXPECT(m.size() == 0);
}

template <template <typename...> typename HashMap>
void ix_test_HashMap_StringHashMap()
{
    StringHashMap<HashMap, int> m;
    m.emplace("hello", 10);
    m.emplace("world", 20);
    m.emplace("foo", 30);
    m.emplace("bar", 40);
    ix_EXPECT(m.size() == 4);
    ix_EXPECT(m[ix_DumbString("hello").data()] == 10);
    ix_EXPECT(m[ix_DumbString("world").data()] == 20);
    ix_EXPECT(m[ix_DumbString("foo").data()] == 30);
    ix_EXPECT(m[ix_DumbString("bar").data()] == 40);
    ix_EXPECT(m.find(ix_DumbString("foobar").data()) == nullptr);
}

template <template <typename...> typename HashMap>
void ix_test_HashMap_capacity()
{

    HashMap<int, int> m;
    ix_EXPECT(m.capacity() == 0);
    m.insert(1, 11);
    ix_EXPECT(m.capacity() == 6);
    m.insert(2, 22);
    m.insert(3, 33);
    m.insert(4, 44);
    m.insert(5, 55);
    m.insert(6, 66);
    ix_EXPECT(m.size() == 6);
    ix_EXPECT(m.bucket_count() == 8);
    m.insert(7, 77);
    ix_EXPECT(m.size() == 7);
    ix_EXPECT(m.bucket_count() == 16);
}

template <template <typename...> typename HashMap>
void ix_test_hash_map()
{
    ix_test_Map<HashMap, 1024>();
    ix_test_HashMap_EqHash<HashMap>();
    ix_test_HashMap_robin_hood<HashMap>();
    ix_test_HashMap_many_collisions<HashMap>();
    ix_test_HashMap_StringHashMap<HashMap>();
    ix_test_HashMap_capacity<HashMap>();
}
