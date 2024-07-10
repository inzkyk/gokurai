#pragma once

#include "ix.hpp"
#include "ix_Vector.hpp"
#include "ix_bit.hpp"
#include "ix_compare.hpp"
#include "ix_hash.hpp"
#include "ix_min_max.hpp"
#include "ix_wrap.hpp"

template <typename Key, typename Hash = ix_Hash<Key>, typename Equal = ix_Equal<Key>>
class ix_HashSet;

using ix_StringHashSet = ix_HashSet<const char *, ix_StringHash, ix_StringEqual>;

template <typename Key, typename Hash, typename Equal>
class ix_HashSet
{
    struct Bucket
    {
        uint32_t disp_and_higher_hash;
        uint32_t index;
    };

    ix_Vector<Key> m_keys;
    ix_Vector<Bucket> m_buckets;

    static constexpr auto hasher = Hash();
    static constexpr auto equal = Equal();
    static constexpr uint32_t DISP_LENGTH = 8;
    static constexpr uint32_t HIGHER_HASH_LENGTH = 32 - DISP_LENGTH;
    static constexpr uint32_t HIGHER_HASH_MASK = (1UL << HIGHER_HASH_LENGTH) - 1;
    static constexpr size_t INITIAL_LENGTH = 8;
    static constexpr size_t MAX_LOAD_FACTOR_NUMERATOR = 8;
    static constexpr size_t MAX_LOAD_FACTOR_DENOMINATOR = 10;

  public:
    ix_HashSet() = default;

    ix_HashSet(const Key *key_start, const Key *key_end)
    {
        size_t n = static_cast<size_t>(key_end - key_start);
        reserve(n);
        while (key_start < key_end)
        {
            emplace(*key_start);
            key_start += 1;
        }
    }

    // cppcheck-suppress noExplicitConstructor
    ix_HashSet(const std::initializer_list<Key> &keys)
    {
        reserve(keys.size());
        for (const Key &key : keys)
        {
            emplace(key);
        }
    }

    ix_FORCE_INLINE size_t size() const
    {
        return m_keys.size();
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
        return m_keys.empty();
    }

    void reserve(size_t size)
    {
        m_keys.reserve(size);
        const size_t n_minimum = size * MAX_LOAD_FACTOR_DENOMINATOR / MAX_LOAD_FACTOR_NUMERATOR + 1;
        const size_t n = ix_max(INITIAL_LENGTH, ix_ceil_2_power(n_minimum));
        rehash(n);
    }

    void clear()
    {
        m_keys.clear();
        m_buckets.clear();

        if (m_buckets.capacity() >= INITIAL_LENGTH)
        {
            m_buckets.resize(m_buckets.capacity());
            ix_memset(m_buckets.data(), 0, m_buckets.size() * sizeof(Bucket));
        }
    }

    ix_FORCE_INLINE Key *begin()
    {
        return m_keys.begin();
    }

    ix_FORCE_INLINE const Key *begin() const
    {
        return m_keys.begin();
    }

    ix_FORCE_INLINE Key *end()
    {
        return m_keys.end();
    }

    ix_FORCE_INLINE const Key *end() const
    {
        return m_keys.end();
    }

    ix_FORCE_INLINE void insert(const Key &key)
    {
        emplace(key);
    }

    ix_FORCE_INLINE void insert(Key &&key)
    {
        emplace(ix_move(key));
    }

    template <typename... Args>
    void emplace(Args &&...args)
    {
        if (ix_UNLIKELY(m_keys.size() >= capacity()))
        {
            rehash();
        }

        const size_t new_kv_pair_index = m_keys.size();
        m_keys.emplace_back(ix_forward<Args>(args)...);
        const Bucket *bucket = find_or_emplace_bucket(m_keys[new_kv_pair_index]);

        const size_t index = bucket->index;
        if (index != new_kv_pair_index)
        {
            m_keys.pop_back();
        }
    }

    ix_FORCE_INLINE bool contains(const Key &key) const
    {
        const Bucket *bucket = find_bucket(key);
        return (bucket != nullptr);
    }

    Key *find(const Key &key)
    {
        const Bucket *bucket = find_bucket(key);
        if (bucket == nullptr)
        {
            return end();
        }

        return &m_keys[bucket->index];
    }

    ix_FORCE_INLINE const Key *find(const Key &key) const
    {
        return const_cast<ix_HashSet *>(this)->find(key);
    }

    void erase(const Key &key)
    {
        Bucket *bucket_to_erase = find_bucket(key);
        if (bucket_to_erase == nullptr)
        {
            return;
        }

        const uint32_t erasing_key_index = bucket_to_erase->index;
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

        if (ix_LIKELY(erasing_key_index != m_keys.size() - 1))
        {
            const uint32_t last_key_index = static_cast<uint32_t>(m_keys.size() - 1);
            Key &&last_key = ix_move(m_keys[last_key_index]);
            Bucket *last_key_bucket = find_bucket(last_key);
            last_key_bucket->index = erasing_key_index;
            m_keys[erasing_key_index] = ix_move(last_key);
        }

        m_keys.pop_back();
    }

    bool operator==(const ix_HashSet &other) const
    {
        if (size() != other.size())
        {
            return false;
        }

        for (const Key &key : m_keys)
        {
            if (!other.contains(key))
            {
                return false;
            }
        }

        return true;
    }

    ix_FORCE_INLINE bool operator!=(const ix_HashSet &other) const
    {
        return !(*this == other);
    }

    ix_FORCE_INLINE const Key &back() const
    {
        return m_keys.back();
    }

  private:
    ix_FORCE_INLINE static uint32_t increment_disp(uint32_t disp_and_higher_hash)
    {
        constexpr uint32_t disp_inc = 1ULL << HIGHER_HASH_LENGTH;
        const uint32_t maybe_next = disp_and_higher_hash + disp_inc;
        return ix_LIKELY(disp_and_higher_hash < maybe_next) ? maybe_next : disp_and_higher_hash;
    }

    void rehash(size_t num_new_buckets = 0)
    {
        if (ix_LIKELY(num_new_buckets == 0))
        {
            num_new_buckets = ix_max(INITIAL_LENGTH, 2 * m_buckets.size());
        }

        m_buckets = ix_Vector<Bucket>(num_new_buckets);
        ix_memset(m_buckets.data(), 0, m_buckets.size() * sizeof(Bucket));

        const uint64_t buckets_mask = static_cast<uint32_t>(m_buckets.size() - 1);
        Bucket *buckets_begin = m_buckets.begin();
        Bucket *buckets_end = m_buckets.end();
        for (size_t i = 0; i < m_keys.size(); i++)
        {
            const size_t hash = hasher(m_keys[i]);
            const uint32_t higher_hash = static_cast<uint32_t>(hash >> (sizeof(size_t) * 8 - 32 + DISP_LENGTH));
            uint32_t disp_and_higher_hash = (1 << HIGHER_HASH_LENGTH) | higher_hash;
            Bucket *new_bucket_pos = &m_buckets[hash & buckets_mask];

            while (disp_and_higher_hash <= new_bucket_pos->disp_and_higher_hash)
            {
                disp_and_higher_hash = increment_disp(disp_and_higher_hash);
                new_bucket_pos = ix_wrap_increment(new_bucket_pos, buckets_begin, buckets_end);
            }

            Bucket new_bucket{disp_and_higher_hash, static_cast<uint32_t>(i)};
            emplace_bucket(new_bucket_pos, new_bucket, buckets_begin, buckets_end);
        }
    }

    void emplace_bucket(Bucket *bucket, Bucket bucket_to_emplace, Bucket *buckets_begin, Bucket *buckets_end)
    {
        while (ix_UNLIKELY(bucket->disp_and_higher_hash != 0))
        {
            Bucket temp = *bucket;
            *bucket = bucket_to_emplace;
            bucket = ix_wrap_increment(bucket, buckets_begin, buckets_end);
            bucket_to_emplace.index = temp.index;
            bucket_to_emplace.disp_and_higher_hash = increment_disp(temp.disp_and_higher_hash);
        }

        *bucket = bucket_to_emplace;
    }

    ix_FORCE_INLINE const Bucket *find_bucket(const Key &key) const
    {
        return const_cast<ix_HashSet *>(this)->find_bucket_kernel<false>(key);
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
            if (ix_UNLIKELY(m_keys.empty()))
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
                if (equal(key, m_keys[bucket->index]))
                {
                    return bucket;
                }
            }
            else if (disp_and_higher_hash > bucket->disp_and_higher_hash)
            {
                break;
            }
            disp_and_higher_hash = increment_disp(disp_and_higher_hash);
            bucket = ix_wrap_increment(bucket, buckets_begin, buckets_end);
        }

        if constexpr (emplace_bucket_if_not_found)
        {
            Bucket new_bucket{disp_and_higher_hash, static_cast<uint32_t>(m_keys.size() - 1)};
            emplace_bucket(bucket, new_bucket, buckets_begin, buckets_end);
            return bucket;
        }
        else
        {
            return nullptr;
        }
    }
};
