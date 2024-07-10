#pragma once

#include "ix.hpp"

template <typename T1, typename T2>
struct ix_Pair
{
    T1 first;
    T2 second;
};

template <typename Key, typename Value>
struct ix_KVPair
{
    Key key;
    Value value;
};
