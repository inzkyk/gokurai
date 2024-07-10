#pragma once

#include "ix.hpp"
#include "ix_DumbInt.hpp"
#include "ix_DumbString.hpp"
#include "ix_Vector.hpp"
#include "ix_random_fill.hpp"

template <template <typename...> typename HashMap, int N>
void ix_test_Map_basic()
{
    HashMap<int, int> m;
    for (int i = 0; i < N; i++)
    {
        m.emplace(i, i * i);
    }
    for (int i = 0; i < N; i++)
    {
        ix_EXPECT(m[i] == i * i);
    }
    ix_EXPECT(m.size() == N);
}

template <template <typename...> typename HashMap, int N>
void ix_test_Map_value_update()
{
    HashMap<int, int> m;
    for (int i = 0; i < N; i++)
    {
        m.emplace(i, i);
    }

    for (int i = 0; i < N; i++)
    {
        m.emplace(i, i * i);
    }

    for (int i = 0; i < N; i++)
    {
        ix_EXPECT(m[i] == i * i);
    }

    ix_EXPECT(m.size() == N);
}

template <template <typename...> typename HashMap, int N>
void ix_test_Map_normal_hash_with_large_key()
{
    HashMap<ix_DumbInt256, ix_DumbString> m;
    for (int i = 0; i < N / 2; i++)
    {
        m.emplace(i, ix_DumbString(i * i));
    }

    for (int i = N / 2; i < N; i++)
    {
        m.emplace(i, i * i);
    }

    for (int i = 0; i < N; i++)
    {
        ix_EXPECT(m[i] == ix_DumbString(i * i));
        ix_EXPECT(m.find(i) != nullptr);
    }
    ix_EXPECT(m.size() == N);
}

template <template <typename...> typename HashMap, int N>
void ix_test_Map_non_trivial_type()
{
    HashMap<ix_DumbString, ix_Vector<int>> m;

    m.emplace("0", ix_Vector<int>{0, 0});
    m.emplace("1", ix_Vector<int>{1, 1});
    m.emplace("2", ix_Vector<int>{2, 2});
    m.emplace("3", ix_Vector<int>{3, 3});
    m.emplace("4", ix_Vector<int>{4, 4});
    m.emplace("5", ix_Vector<int>{5, 5});
    ix_EXPECT(m["0"] == ix_Vector<int>{0, 0});
    ix_EXPECT(m["1"] == ix_Vector<int>{1, 1});
    ix_EXPECT(m["2"] == ix_Vector<int>{2, 2});
    ix_EXPECT(m["3"] == ix_Vector<int>{3, 3});
    ix_EXPECT(m["4"] == ix_Vector<int>{4, 4});
    ix_EXPECT(m["5"] == ix_Vector<int>{5, 5});

    ix_EXPECT(m.size() == 6);
}

template <template <typename...> typename HashMap, int N>
void ix_test_Map_emplace()
{
    HashMap<ix_DumbString, ix_Vector<int>> m;

    ix_DumbString s;
    ix_Vector<int> v;

    m.emplace("1", ix_Vector<int>{1, 1});
    ix_EXPECT(m["1"] == ix_Vector<int>{1, 1});
    ix_EXPECT(m.size() == 1);

    v = {2, 2};
    m.emplace("2", v);
    ix_EXPECT(m["2"] == v);
    ix_EXPECT(m.size() == 2);

    s = "3";
    m.emplace(s, ix_Vector<int>{3, 3});
    ix_EXPECT(m[s] == ix_Vector<int>{3, 3});
    ix_EXPECT(m.size() == 3);

    s = "4";
    v = {4, 4};
    m.emplace(s, v);
    ix_EXPECT(m[s] == v);
    ix_EXPECT(m.size() == 4);

    v = {4, 4, 4, 4};
    m.emplace(s, v);
    ix_EXPECT(m[s] == v);
    ix_EXPECT(m.size() == 4);
}

template <template <typename...> typename HashMap, int N>
void ix_test_Map_insert()
{
    HashMap<ix_DumbString, ix_Vector<int>> m;

    ix_DumbString s;
    ix_Vector<int> v;

    m.insert("1", {1, 1});
    ix_EXPECT(m["1"] == ix_Vector<int>{1, 1});
    ix_EXPECT(m.size() == 1);

    v = {2, 2};
    m.insert("2", v);
    ix_EXPECT(m["2"] == v);
    ix_EXPECT(m.size() == 2);

    s = "3";
    m.insert(s, {3, 3});
    ix_EXPECT(m[s] == ix_Vector<int>{3, 3});
    ix_EXPECT(m.size() == 3);

    s = "4";
    v = {4, 4};
    m.insert(s, v);
    ix_EXPECT(m[s] == v);
    ix_EXPECT(m.size() == 4);

    v = {4, 4, 4, 4};
    m.emplace(s, v);
    ix_EXPECT(m[s] == v);
    ix_EXPECT(m.size() == 4);
}

template <template <typename...> typename HashMap, int N>
void ix_test_Map_brace_initialization()
{
    {
        HashMap<int, int> m{
            {1, 11},
            {2, 22},
            {3, 33},
            {4, 44},
        };

        ix_EXPECT(m.size() == 4);
        ix_EXPECT(m[1] == 11);
        ix_EXPECT(m[2] == 22);
        ix_EXPECT(m[3] == 33);
        ix_EXPECT(m[4] == 44);
    }

    {
        HashMap<ix_DumbString, ix_Vector<int>> m{
            {"1", {1, 1}},
            {"2", {2, 2}},
            {"3", {3, 3}},
            {"4", {4, 4}},
        };

        ix_EXPECT(m.size() == 4);
        ix_EXPECT(m["1"] == ix_Vector<int>{1, 1});
        ix_EXPECT(m["2"] == ix_Vector<int>{2, 2});
        ix_EXPECT(m["3"] == ix_Vector<int>{3, 3});
        ix_EXPECT(m["4"] == ix_Vector<int>{4, 4});
    }
}

template <template <typename...> typename HashMap, int N>
void ix_test_Map_copy_and_move()
{
    // copy/move constructor
    {
        HashMap<ix_DumbString, ix_Vector<int>> m0{
            {"1", {1, 1}},
            {"2", {2, 2}},
            {"3", {3, 3}},
            {"4", {4, 4}},
        };

        HashMap<ix_DumbString, ix_Vector<int>> m1 = m0;

        ix_EXPECT(m0.size() == 4);
        ix_EXPECT(m0["1"] == ix_Vector<int>{1, 1});
        ix_EXPECT(m0["2"] == ix_Vector<int>{2, 2});
        ix_EXPECT(m0["3"] == ix_Vector<int>{3, 3});
        ix_EXPECT(m0["4"] == ix_Vector<int>{4, 4});
        ix_EXPECT(m1.size() == 4);
        ix_EXPECT(m1["1"] == ix_Vector<int>{1, 1});
        ix_EXPECT(m1["2"] == ix_Vector<int>{2, 2});
        ix_EXPECT(m1["3"] == ix_Vector<int>{3, 3});
        ix_EXPECT(m1["4"] == ix_Vector<int>{4, 4});

        HashMap<ix_DumbString, ix_Vector<int>> m2 = ix_move(m1);
        ix_EXPECT(m2.size() == 4);
        ix_EXPECT(m2["1"] == ix_Vector<int>{1, 1});
        ix_EXPECT(m2["2"] == ix_Vector<int>{2, 2});
        ix_EXPECT(m2["3"] == ix_Vector<int>{3, 3});
        ix_EXPECT(m2["4"] == ix_Vector<int>{4, 4});
    }

    // copy/move assignment
    {
        HashMap<ix_DumbString, ix_Vector<int>> m0{
            {"1", {1, 1}},
            {"2", {2, 2}},
            {"3", {3, 3}},
            {"4", {4, 4}},
        };

        HashMap<ix_DumbString, ix_Vector<int>> m1{
            {"foo", {3, 1, 4}},
        };

        m1 = m0;

        ix_EXPECT(m0.size() == 4);
        ix_EXPECT(m0["1"] == ix_Vector<int>{1, 1});
        ix_EXPECT(m0["2"] == ix_Vector<int>{2, 2});
        ix_EXPECT(m0["3"] == ix_Vector<int>{3, 3});
        ix_EXPECT(m0["4"] == ix_Vector<int>{4, 4});
        ix_EXPECT(m1.size() == 4);
        ix_EXPECT(m1["1"] == ix_Vector<int>{1, 1});
        ix_EXPECT(m1["2"] == ix_Vector<int>{2, 2});
        ix_EXPECT(m1["3"] == ix_Vector<int>{3, 3});
        ix_EXPECT(m1["4"] == ix_Vector<int>{4, 4});

        HashMap<ix_DumbString, ix_Vector<int>> m2{
            {"foo", {3, 1, 4}},
        };

        m2 = ix_move(m1);

        ix_EXPECT(m2.size() == 4);
        ix_EXPECT(m2["1"] == ix_Vector<int>{1, 1});
        ix_EXPECT(m2["2"] == ix_Vector<int>{2, 2});
        ix_EXPECT(m2["3"] == ix_Vector<int>{3, 3});
        ix_EXPECT(m2["4"] == ix_Vector<int>{4, 4});
    }
}

template <template <typename...> typename HashMap, int N>
void ix_test_Map_contains()
{
    HashMap<int, int> m;
    for (int i = 0; i < N; i++)
    {
        m.emplace(i, i * i);
    }
    for (int i = 0; i < N; i++)
    {
        ix_EXPECT(m.contains(i));
        ix_EXPECT(!m.contains(i + N));
    }
}

template <template <typename...> typename HashMap, int N>
void ix_test_Map_erase()
{
    {
        HashMap<int, int> m;

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
    }

    // many erase
    {
        HashMap<int, int> m;

        for (int i = 0; i < N; i++)
        {
            m.emplace(i, i);
        }

        int buf[N];
        ix_rand_range_fill_unique<int>(buf, N, 0, N - 1);
        for (int i = 0; i < N; i++)
        {
            m.erase(buf[i]);
        }

        ix_EXPECT(m.size() == 0);
    }
}

template <template <typename...> typename HashMap, int N>
void ix_test_Map_empty()
{
    HashMap<int, int> m;
    ix_EXPECT(m.empty());
    m.emplace(0, 10);
    ix_EXPECT(!m.empty());
    m.clear();
    ix_EXPECT(m.empty());
}

template <template <typename...> typename HashMap, int N>
void ix_test_Map_insert_by_move()
{
    { // Basic.
        HashMap<ix_Vector<int>, ix_Vector<int>> m;
        const ix_Vector<int> v0({0, 1, 2});
        const ix_Vector<int> v1({0, 1, 2});
        m.insert(v0, v1);
        ix_EXPECT(m.size() == 1);
        ix_EXPECT(v0.size() == 3);
        ix_EXPECT(v1.size() == 3);
    }

    { // Move key.
        HashMap<ix_Vector<int>, ix_Vector<int>> m;
        ix_Vector<int> v0({0, 1, 2});
        const ix_Vector<int> v1({0, 1, 2});
        m.insert(ix_move(v0), v1);
        ix_EXPECT(m.size() == 1);
        ix_EXPECT(v1.size() == 3);
    }

    { // Move value.
        HashMap<ix_Vector<int>, ix_Vector<int>> m;
        const ix_Vector<int> v0({0, 1, 2});
        ix_Vector<int> v1({0, 1, 2});
        m.insert(v0, ix_move(v1));
        ix_EXPECT(m.size() == 1);
        ix_EXPECT(v0.size() == 3);
    }

    { // Move both.
        HashMap<ix_Vector<int>, ix_Vector<int>> m;
        ix_Vector<int> v0({0, 1, 2});
        ix_Vector<int> v1({0, 1, 2});
        m.insert(ix_move(v0), ix_move(v1));
        ix_EXPECT(m.size() == 1);
    }
}

template <template <typename...> typename HashMap, int N>
void ix_test_Map_emplace_by_move()
{
    { // Basic.
        HashMap<ix_Vector<int>, ix_Vector<int>> m;
        const ix_Vector<int> v0({0, 1, 2});
        const ix_Vector<int> v1({0, 1, 2});
        m.emplace(v0, v1);
        ix_EXPECT(m.size() == 1);
        ix_EXPECT(v0.size() == 3);
        ix_EXPECT(v1.size() == 3);
    }

    { // Move key.
        HashMap<ix_Vector<int>, ix_Vector<int>> m;
        ix_Vector<int> v0({0, 1, 2});
        const ix_Vector<int> v1({0, 1, 2});
        m.emplace(ix_move(v0), v1);
        ix_EXPECT(m.size() == 1);
        ix_EXPECT(v1.size() == 3);
    }

    { // Move value.
        HashMap<ix_Vector<int>, ix_Vector<int>> m;
        const ix_Vector<int> v0({0, 1, 2});
        ix_Vector<int> v1({0, 1, 2});
        m.emplace(v0, ix_move(v1));
        ix_EXPECT(m.size() == 1);
        ix_EXPECT(v0.size() == 3);
    }

    { // Move both.
        HashMap<ix_Vector<int>, ix_Vector<int>> m;
        ix_Vector<int> v0({0, 1, 2});
        ix_Vector<int> v1({0, 1, 2});
        m.emplace(ix_move(v0), ix_move(v1));
        ix_EXPECT(m.size() == 1);
    }

    // reserve() when threre's elements.
    {
        HashMap<int, int> m;
        m.emplace(1, 11);
        m.emplace(2, 22);
        m.emplace(3, 33);
        m.emplace(4, 44);
        m.reserve(100);
        ix_EXPECT(m.size() == 4);
        ix_EXPECT(m[1] == 11);
        ix_EXPECT(m[2] == 22);
        ix_EXPECT(m[3] == 33);
        ix_EXPECT(m[4] == 44);
        ix_EXPECT(m.capacity() >= 100);
    }
}

template <template <typename...> typename HashMap, int N>
void ix_test_Map_get_or()
{
    // returns value
    {
        HashMap<int, int> m;
        m.emplace(1, 11);
        m.emplace(2, 22);
        m.emplace(3, 33);
        m.emplace(4, 44);
        ix_EXPECT(m.get_or(1, 100) == 11);
        ix_EXPECT(m.get_or(2, 100) == 22);
        ix_EXPECT(m.get_or(3, 100) == 33);
        ix_EXPECT(m.get_or(4, 100) == 44);
        ix_EXPECT(m.get_or(0, 100) == 100);
        ix_EXPECT(m.get_or(5, 100) == 100);
        ix_EXPECT(m.get_or(100, 100) == 100);
    }

    // returns const reference
    {
        HashMap<uint64_t, ix_Vector<uint64_t>> m;
        m.emplace(1, ix_Vector<uint64_t>{1});
        m.emplace(2, ix_Vector<uint64_t>{2, 2});
        m.emplace(3, ix_Vector<uint64_t>{3, 3, 3});
        m.emplace(4, ix_Vector<uint64_t>{4, 4, 4, 4});
        const ix_Vector<uint64_t> default_value{0, 1, 2, 3};
        ix_EXPECT(m.get_or(1, default_value) == ix_Vector<uint64_t>{1});
        ix_EXPECT(m.get_or(2, default_value) == ix_Vector<uint64_t>{2, 2});
        ix_EXPECT(m.get_or(3, default_value) == ix_Vector<uint64_t>{3, 3, 3});
        ix_EXPECT(m.get_or(4, default_value) == ix_Vector<uint64_t>{4, 4, 4, 4});
        ix_EXPECT(m.get_or(0, default_value) == default_value);
        ix_EXPECT(m.get_or(5, default_value) == default_value);
    }
}

template <template <typename...> typename HashMap, int N>
void ix_test_Map_iterator()
{
    // normal iterator
    {
        HashMap<uint64_t, ix_Vector<uint64_t>> m;
        m.emplace(1, ix_Vector<uint64_t>{1});
        m.emplace(2, ix_Vector<uint64_t>{2, 2});
        m.emplace(3, ix_Vector<uint64_t>{3, 3, 3});
        m.emplace(4, ix_Vector<uint64_t>{4, 4, 4, 4});

        {
            auto it = m.find(10);
            ix_EXPECT(it == nullptr);
            it = m.find(1);
            ix_EXPECT(it != nullptr);
            ix_ASSERT(it != nullptr); // Suppress clang-tidy warning
            ix_EXPECT(*it == ix_Vector<uint64_t>{1});
            ix_Vector<uint64_t> new_value{11};
            *it = new_value;
            ix_EXPECT(m[1] == new_value);
        }

        {
            const auto *it = m.find(1);
            ix_EXPECT(it != nullptr);
            ix_ASSERT(it != nullptr); // Suppress clang-tidy warning
            ix_EXPECT(*it == ix_Vector<uint64_t>{11});
        }
    }

    // const iterator
    {
        const HashMap<int, int> m{{1, 1}, {2, 22}, {3, 333}, {4, 4444}};

        auto it = m.find(10);
        ix_EXPECT(it == nullptr);

        it = m.find(3);
        ix_EXPECT(it != nullptr);
        ix_ASSERT(it != nullptr); // Suppress MSVC warning
        ix_EXPECT(*it == 333);
    }
}

template <template <typename...> typename Map, int N>
void ix_test_Map()
{
    ix_test_Map_basic<Map, N>();
    ix_test_Map_value_update<Map, N>();
    ix_test_Map_normal_hash_with_large_key<Map, N>();
    ix_test_Map_non_trivial_type<Map, N>();
    ix_test_Map_emplace<Map, N>();
    ix_test_Map_insert<Map, N>();
    ix_test_Map_brace_initialization<Map, N>();
    ix_test_Map_copy_and_move<Map, N>();
    ix_test_Map_contains<Map, N>();
    ix_test_Map_erase<Map, N>();
    ix_test_Map_empty<Map, N>();
    ix_test_Map_insert_by_move<Map, N>();
    ix_test_Map_emplace_by_move<Map, N>();
    ix_test_Map_get_or<Map, N>();

    // ix_hashMapDuobleArray does not provide begin()/end().
    // ix_test_Map_iterator<Map, N>();
}
