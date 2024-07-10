#include "ix_HashMapSingleArray.hpp"
#include "ix_doctest.hpp"
#include "ix_test_HashMap.hpp"

namespace ix_HashMapSingleArrayDetail
{

uint32_t increment_disp(uint32_t disp_and_higher_hash)
{
    constexpr uint32_t disp_inc = 1ULL << ix_HashMapSingleArray<int, int>::HIGHER_HASH_LENGTH;
    const uint32_t maybe_next = disp_and_higher_hash + disp_inc;
    return ix_LIKELY(disp_and_higher_hash < maybe_next) ? maybe_next : disp_and_higher_hash;
}

void emplace_bucket(Bucket *bucket, Bucket bucket_to_emplace, Bucket *buckets_begin, Bucket *buckets_end)
{
    while (bucket->disp_and_higher_hash != 0)
    {
        const Bucket temp = *bucket;
        *bucket = bucket_to_emplace;
        bucket = ix_wrap_increment(bucket, buckets_begin, buckets_end);
        bucket_to_emplace.index = temp.index;
        bucket_to_emplace.disp_and_higher_hash = increment_disp(temp.disp_and_higher_hash);
    }

    *bucket = bucket_to_emplace;
}

} // namespace ix_HashMapSingleArrayDetail

ix_TEST_CASE("ix_HashMapSingleArray")
{
    ix_test_hash_map<ix_HashMapSingleArray>();
    ix_test_Map_iterator<ix_HashMapSingleArray, 1024>();
}
