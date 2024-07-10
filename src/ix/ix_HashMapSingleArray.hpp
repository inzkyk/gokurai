#pragma once

#include "ix.hpp"
#include "ix_Pair.hpp"
#include "ix_Vector.hpp"
#include "ix_bit.hpp"
#include "ix_compare.hpp"
#include "ix_hash.hpp"
#include "ix_wrap.hpp"

template <typename Key, typename Value, typename Hash = ix_Hash<Key>, typename Equal = ix_Equal<Key>>
class ix_HashMapSingleArray;

template <typename Value>
using ix_StringHashMapSingleArray = ix_HashMapSingleArray<const char *, Value, ix_StringHash, ix_StringEqual>;

namespace ix_HashMapSingleArrayDetail
{

struct Bucket
{
    uint32_t disp_and_higher_hash;
    uint32_t index;
};

uint32_t increment_disp(uint32_t disp_and_higher_hash);
void emplace_bucket(Bucket *bucket, Bucket bucket_to_emplace, Bucket *buckets_begin, Bucket *buckets_end);

} // namespace ix_HashMapSingleArrayDetail

template <typename Key, typename Value, typename Hash, typename Equal>
class ix_HashMapSingleArray
{
    using Bucket = ix_HashMapSingleArrayDetail::Bucket;
    using KVPair = ix_KVPair<Key, Value>;

    ix_Vector<KVPair> m_kv_pairs;
    ix_Vector<Bucket> m_buckets;

    static constexpr auto hasher = Hash();
    static constexpr auto equal = Equal();
    static constexpr uint32_t DISP_LENGTH = 8;
    static constexpr uint32_t HIGHER_HASH_LENGTH = 32 - DISP_LENGTH;
    static constexpr uint32_t HIGHER_HASH_MASK = (1UL << HIGHER_HASH_LENGTH) - 1;
    static constexpr size_t INITIAL_LENGTH = 8;
    static constexpr size_t MAX_LOAD_FACTOR_NUMERATOR = 8;
    static constexpr size_t MAX_LOAD_FACTOR_DENOMINATOR = 10;

    friend uint32_t ix_HashMapSingleArrayDetail::increment_disp(uint32_t disp_and_higher_hash);

  public:
    ix_HashMapSingleArray() = default;

    // cppcheck-suppress noExplicitConstructor
    ix_HashMapSingleArray(const std::initializer_list<KVPair> &kv_pairs)
    {
        reserve(kv_pairs.size());
        for (const KVPair &kv_pair : kv_pairs)
        {
            emplace(kv_pair.key, kv_pair.value);
        }
    }

    ix_FORCE_INLINE size_t size() const
    {
        return m_kv_pairs.size();
    }

    ix_FORCE_INLINE size_t bucket_count() const
    {
        return m_buckets.size();
    }

    ix_FORCE_INLINE size_t capacity() const
    {
        return m_buckets.size() * MAX_LOAD_FACTOR_NUMERATOR / MAX_LOAD_FACTOR_DENOMINATOR;
    }

    ix_FORCE_INLINE bool empty() const
    {
        return m_kv_pairs.empty();
    }

    void reserve(size_t size)
    {
        m_kv_pairs.reserve(size);
        const size_t n_minimum = size * MAX_LOAD_FACTOR_DENOMINATOR / MAX_LOAD_FACTOR_NUMERATOR + 1;
        const size_t n = ix_max(INITIAL_LENGTH, ix_ceil_2_power(n_minimum));
        rehash(n);
    }

    void clear()
    {
        if (m_kv_pairs.empty())
        {
            return;
        }

        m_kv_pairs.clear();
        ix_memset(m_buckets.data(), 0, m_buckets.size() * sizeof(Bucket));
    }

    ix_FORCE_INLINE KVPair *begin()
    {
        return m_kv_pairs.begin();
    }

    ix_FORCE_INLINE const KVPair *begin() const
    {
        return m_kv_pairs.begin();
    }

    ix_FORCE_INLINE KVPair *end()
    {
        return m_kv_pairs.end();
    }

    ix_FORCE_INLINE const KVPair *end() const
    {
        return m_kv_pairs.end();
    }

    ix_FORCE_INLINE void insert(const Key &key, const Value &value)
    {
        emplace(key, value);
    }

    ix_FORCE_INLINE void insert(const Key &key, Value &&value)
    {
        emplace(key, ix_move(value));
    }

    ix_FORCE_INLINE void insert(Key &&key, const Value &value)
    {
        emplace(ix_move(key), value);
    }

    ix_FORCE_INLINE void insert(Key &&key, Value &&value)
    {
        emplace(ix_move(key), ix_move(value));
    }

    template <typename... Args>
    ix_FORCE_INLINE void emplace(const Key &key, Args &&...args)
    {
        emplace(ix_move(Key(key)), ix_forward<Args>(args)...);
    }

    template <typename... Args>
    void emplace(Key &&key, Args &&...args)
    {
        if (ix_UNLIKELY(m_kv_pairs.size() >= capacity()))
        {
            rehash();
        }

        const Bucket *bucket = find_or_emplace_bucket(key);
        const size_t index = bucket->index;
        const bool already_exists = (index != m_kv_pairs.size());
        if (already_exists)
        {
            m_kv_pairs[index].value = Value{ix_forward<Args>(args)...};
        }
        else
        {
            m_kv_pairs.emplace_back(KVPair{ix_move(key), ix_move(Value{ix_forward<Args>(args)...})});
        }
    }

    const Value &operator[](const Key &key) const
    {
        const Bucket *bucket = find_bucket(key);
        ix_ASSERT(bucket != nullptr);
        return m_kv_pairs[bucket->index].value;
    }

    ix_FORCE_INLINE bool contains(const Key &key) const
    {
        const Bucket *bucket = find_bucket(key);
        return (bucket != nullptr);
    }

    Value *find(const Key &key)
    {
        const Bucket *bucket = find_bucket(key);
        return (bucket == nullptr) ? nullptr : &m_kv_pairs[bucket->index].value;
    }

    const Value *find(const Key &key) const
    {
        const Bucket *bucket = find_bucket(key);
        return (bucket == nullptr) ? nullptr : &m_kv_pairs[bucket->index].value;
    }

    Value &get_or(const Key &key, Value &value)
    {
        const Bucket *bucket = find_bucket(key);
        return (bucket == nullptr) ? value : m_kv_pairs[bucket->index].value;
    }

    const Value &get_or(const Key &key, const Value &value) const
    {
        const Bucket *bucket = find_bucket(key);
        return (bucket == nullptr) ? value : m_kv_pairs[bucket->index].value;
    }

    void erase(const Key &key)
    {
        Bucket *bucket_to_erase = find_bucket(key);
        if (bucket_to_erase == nullptr)
        {
            return;
        }

        const uint32_t erasing_kv_pair_index = bucket_to_erase->index;
        Bucket *buckets_begin = m_buckets.begin();
        Bucket *buckets_end = m_buckets.end();
        Bucket *bucket = ix_wrap_increment(bucket_to_erase, buckets_begin, buckets_end);
        Bucket *prev_bucket = bucket_to_erase;
        while ((HIGHER_HASH_MASK + (1 << HIGHER_HASH_LENGTH)) < bucket->disp_and_higher_hash)
        {
            prev_bucket->index = bucket->index;
            prev_bucket->disp_and_higher_hash = bucket->disp_and_higher_hash - (1 << HIGHER_HASH_LENGTH);
            prev_bucket = bucket;
            bucket = ix_wrap_increment(bucket, buckets_begin, buckets_end);
        }
        prev_bucket->disp_and_higher_hash = 0;

        if (ix_LIKELY(erasing_kv_pair_index != m_kv_pairs.size() - 1))
        {
            const uint32_t last_kv_pair_index = static_cast<uint32_t>(m_kv_pairs.size() - 1);
            KVPair &&last_kv_pair = ix_move(m_kv_pairs[last_kv_pair_index]);
            Bucket *last_kv_pair_bucket = find_bucket(last_kv_pair.key);
            last_kv_pair_bucket->index = erasing_kv_pair_index;
            m_kv_pairs[erasing_kv_pair_index].key = ix_move(last_kv_pair.key);
            m_kv_pairs[erasing_kv_pair_index].value = ix_move(last_kv_pair.value);
        }

        m_kv_pairs.pop_back();
    }

  private:
    void rehash(size_t num_new_buckets = 0)
    {
        if (ix_LIKELY(num_new_buckets == 0))
        {
            num_new_buckets = ix_max(INITIAL_LENGTH, 2 * m_buckets.size());
        }

        m_buckets.resize(num_new_buckets);
        ix_memset(m_buckets.data(), 0, num_new_buckets * sizeof(Bucket));

        const uint64_t buckets_mask = static_cast<uint32_t>(num_new_buckets - 1);
        Bucket *buckets_begin = m_buckets.begin();
        Bucket *buckets_end = m_buckets.end();
        for (size_t i = 0; i < m_kv_pairs.size(); i++)
        {
            const size_t hash = hasher(m_kv_pairs[i].key);
            const uint32_t higher_hash = static_cast<uint32_t>(hash >> (sizeof(size_t) * 8 - 32 + DISP_LENGTH));
            uint32_t disp_and_higher_hash = (1 << HIGHER_HASH_LENGTH) | higher_hash;
            Bucket *new_bucket_pos = &m_buckets[hash & buckets_mask];

            while (disp_and_higher_hash <= new_bucket_pos->disp_and_higher_hash)
            {
                disp_and_higher_hash = ix_HashMapSingleArrayDetail::increment_disp(disp_and_higher_hash);
                new_bucket_pos = ix_wrap_increment(new_bucket_pos, buckets_begin, buckets_end);
            }

            Bucket new_bucket{disp_and_higher_hash, static_cast<uint32_t>(i)};
            emplace_bucket(new_bucket_pos, new_bucket, buckets_begin, buckets_end);
        }
    }

    ix_FORCE_INLINE const Bucket *find_bucket(const Key &key) const
    {
        return const_cast<ix_HashMapSingleArray *>(this)->find_bucket_kernel<false>(key);
    }

    ix_FORCE_INLINE Bucket *find_bucket(const Key &key)
    {
        return find_bucket_kernel<false>(key);
    }

    ix_FORCE_INLINE Bucket *find_or_emplace_bucket(const Key &key)
    {
        return find_bucket_kernel<true>(key);
    }

    template <bool emplace_bucket_if_not_found>
    Bucket *find_bucket_kernel(const Key &key)
    {
        if constexpr (!emplace_bucket_if_not_found)
        {
            if (ix_UNLIKELY(m_kv_pairs.empty()))
            {
                return nullptr;
            }
        }

        const size_t hash = hasher(key);
        const uint32_t higher_hash = static_cast<uint32_t>(hash >> (sizeof(size_t) * 8 - 32 + DISP_LENGTH));
        uint32_t disp_and_higher_hash = (1 << HIGHER_HASH_LENGTH) | higher_hash;
        const uint64_t buckets_mask = static_cast<uint32_t>(m_buckets.size() - 1);
        Bucket *bucket = &m_buckets[hash & buckets_mask];
        Bucket *buckets_begin = m_buckets.begin();
        Bucket *buckets_end = m_buckets.end();

        while (true)
        {
            if (disp_and_higher_hash == bucket->disp_and_higher_hash)
            {
                if (equal(key, m_kv_pairs[bucket->index].key))
                {
                    return bucket;
                }
            }
            else if (disp_and_higher_hash > bucket->disp_and_higher_hash)
            {
                break;
            }
            disp_and_higher_hash = ix_HashMapSingleArrayDetail::increment_disp(disp_and_higher_hash);
            bucket = ix_wrap_increment(bucket, buckets_begin, buckets_end);
        }

        if constexpr (emplace_bucket_if_not_found)
        {
            Bucket new_bucket{disp_and_higher_hash, static_cast<uint32_t>(m_kv_pairs.size())};
            emplace_bucket(bucket, new_bucket, buckets_begin, buckets_end);
            return bucket;
        }
        else
        {
            return nullptr;
        }
    }
};
