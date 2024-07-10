#pragma once

#include "ix.hpp"
#include "ix_memory.hpp"
#include "ix_string.hpp"

// Based on wyhash (https://github.com/wangyi-fudan/wyhash, The Unlicense)
size_t ix_hash(const void *p, size_t length);
size_t ix_hash_between(const void *start, const void *end);
constexpr size_t ix_hash(const char *p, size_t length);
ix_FORCE_INLINE constexpr size_t ix_hash_str(const char *s);
ix_FORCE_INLINE constexpr size_t ix_hash64(uint64_t x);

#if ix_PLATFORM(WASM)

static_assert(ix_COMPILER(CLANG));

ix_FORCE_INLINE constexpr uint32_t ix_wyr32(const char *p)
{
    uint32_t v = ix_memread4(p);

#if (ix_COMPILER(GCC) || ix_COMPILER(ICC) || ix_COMPILER(CLANG))
    if constexpr (!ix_is_constant_evaluated() && (ix_COMPILER(GCC) || ix_COMPILER(ICC) || ix_COMPILER(CLANG)))
    {
        return __builtin_bswap32(v);
    }
#endif

    return (((v >> 24) & 0xff) |       //
            ((v >> 8) & 0xff00) |      //
            ((v << 8) & 0xff0000) |    //
            ((v << 24) & 0xff000000)); //
}

ix_FORCE_INLINE constexpr uint32_t ix_wyr24(const char *p, uint32_t k)
{
    return ((static_cast<uint32_t>(p[0])) << 16) |     //
           ((static_cast<uint32_t>(p[k >> 1])) << 8) | //
           static_cast<uint32_t>(p[k - 1]);            //
}

ix_FORCE_INLINE constexpr void ix_wymix32(uint32_t *A, uint32_t *B)
{
    uint64_t c = *A ^ 0x53c5ca59U;
    c *= *B ^ 0x74743c1bU;
    *A = static_cast<uint32_t>(c);
    *B = static_cast<uint32_t>(c >> 32);
}

constexpr size_t ix_hash(const char *p, size_t length)
{
    uint32_t seed = 0xb380fdf4; // Random number
    uint64_t i = static_cast<uint64_t>(length);
    uint32_t see1 = static_cast<uint32_t>(length);
    // seed ^= static_cast<uint32_t>(length >> 32);
    ix_wymix32(&seed, &see1);
    for (; i > 8; i -= 8, p += 8)
    {
        seed ^= ix_wyr32(p);
        see1 ^= ix_wyr32(p + 4);
        ix_wymix32(&seed, &see1);
    }
    if (i >= 4)
    {
        seed ^= ix_wyr32(p);
        see1 ^= ix_wyr32(p + i - 4);
    }
    else if (i != 0U)
    {
        seed ^= ix_wyr24(p, static_cast<uint32_t>(i));
    }
    ix_wymix32(&seed, &see1);
    ix_wymix32(&seed, &see1);
    return seed ^ see1;
}

ix_FORCE_INLINE constexpr size_t ix_hash64(uint64_t x)
{
    uint32_t a = static_cast<uint32_t>(x);
    uint32_t b = static_cast<uint32_t>(x >> 32);
    ix_wymix32(&a, &b);
    return a ^ b;
}

#else

#if ix_COMPILER(MSVC) && ix_ARCH(x64)

extern "C" unsigned __int64 _umul128(unsigned __int64 Multiplier, unsigned __int64 Multiplicand,
                                     unsigned __int64 *HighProduct);
extern "C" unsigned long _byteswap_ulong(unsigned long val);
extern "C" unsigned __int64 _byteswap_uint64(unsigned __int64 val);

#endif

// Can't use pointers because MSVC will crush when you take the pointer of a value inside a constexpr function.
ix_FORCE_INLINE constexpr void ix_wymum(uint64_t &a, uint64_t &b)
{
#if __SIZEOF_INT128__
    if constexpr (__SIZEOF_INT128__)
    {
        __uint128_t r = a;
        r *= b;
        a = static_cast<uint64_t>(r);
        b = static_cast<uint64_t>(r >> 64);
        return;
    }
#endif

#if ix_COMPILER(MSVC) && ix_ARCH(x64)
    if constexpr (!ix_is_constant_evaluated() && ix_COMPILER(MSVC) && ix_ARCH(x64))
    {
        // It's ok to take the pointer of 'b' because this path is not constexpr.
        a = _umul128(a, b, &b);
        return;
    }
#endif

    const uint64_t ha = a >> 32;
    const uint64_t hb = b >> 32;
    const uint64_t la = static_cast<uint32_t>(a);
    const uint64_t lb = static_cast<uint32_t>(b);
    const uint64_t rh = ha * hb;
    const uint64_t rm0 = ha * lb;
    const uint64_t rm1 = hb * la;
    const uint64_t rl = la * lb;
    const uint64_t t = rl + (rm0 << 32);
    uint64_t hi = 0;
    uint64_t lo = 0;
    uint64_t c = static_cast<uint64_t>(t < rl);
    lo = t + (rm1 << 32);
    c += static_cast<uint64_t>(lo < t);
    hi = rh + (rm0 >> 32) + (rm1 >> 32) + c;
    a = lo;
    b = hi;
}

ix_FORCE_INLINE constexpr uint64_t ix_wymix(uint64_t a, uint64_t b)
{
    ix_wymum(a, b);
    return a ^ b;
}

ix_FORCE_INLINE constexpr uint64_t ix_wyr8(const char *p)
{
    const uint64_t v = ix_memread8(p);

#if (ix_COMPILER(GCC) || ix_COMPILER(ICC) || ix_COMPILER(CLANG))
    if constexpr (!ix_is_constant_evaluated() && (ix_COMPILER(GCC) || ix_COMPILER(ICC) || ix_COMPILER(CLANG)))
    {
        return __builtin_bswap64(v);
    }
#endif

#if ix_COMPILER(MSVC)
    if constexpr (!ix_is_constant_evaluated() && ix_COMPILER(MSVC))
    {
        return _byteswap_uint64(v);
    }
#endif

    return (((v >> 56) & 0xff) |               //
            ((v >> 40) & 0xff00) |             //
            ((v >> 24) & 0xff0000) |           //
            ((v >> 8) & 0xff000000) |          //
            ((v << 8) & 0xff00000000) |        //
            ((v << 24) & 0xff0000000000) |     //
            ((v << 40) & 0xff000000000000) |   //
            ((v << 56) & 0xff00000000000000)); //
}

ix_FORCE_INLINE constexpr uint64_t ix_wyr4(const char *p)
{
    const uint32_t v = ix_memread4(p);

#if (ix_COMPILER(GCC) || ix_COMPILER(ICC) || ix_COMPILER(CLANG))
    if constexpr (!ix_is_constant_evaluated() && (ix_COMPILER(GCC) || ix_COMPILER(ICC) || ix_COMPILER(CLANG)))
    {
        return __builtin_bswap32(v);
    }
#endif

#if ix_COMPILER(MSVC)
    if constexpr (!ix_is_constant_evaluated() && ix_COMPILER(MSVC))
    {
        return _byteswap_ulong(v);
    }
#endif

    return (((v >> 24) & 0xff) |       //
            ((v >> 8) & 0xff00) |      //
            ((v << 8) & 0xff0000) |    //
            ((v << 24) & 0xff000000)); //
}

ix_FORCE_INLINE constexpr uint64_t ix_wyr3(const char *p, uint64_t k)
{
    return (static_cast<uint64_t>(p[0]) << 16) |     //
           (static_cast<uint64_t>(p[k >> 1]) << 8) | //
           (static_cast<uint64_t>(p[k - 1]));
}

constexpr size_t ix_hash(const char *p, uint64_t length)
{
    constexpr uint64_t secret[4] = {0xa0761d6478bd642fULL, 0xe7037ed1a0b428dbULL, //
                                    0x8ebc6af09c88c6e3ULL, 0x589965cc75374cc3ULL};
    uint64_t seed = 0xfc5473a997896bdaULL; // Random number

    seed ^= ix_wymix(seed ^ secret[0], secret[1]);
    uint64_t a = 0;
    uint64_t b = 0;
    if (ix_LIKELY(length <= 16))
    {
        if (ix_LIKELY(length >= 4))
        {
            a = (ix_wyr4(p) << 32) | ix_wyr4(p + ((length >> 3) << 2));
            b = (ix_wyr4(p + length - 4) << 32) | ix_wyr4(p + length - 4 - ((length >> 3) << 2));
        }
        else if (ix_LIKELY(length > 0))
        {
            a = ix_wyr3(p, length);
            b = 0;
        }
        else
        {
            a = b = 0;
        }
    }
    else
    {
        size_t i = length;
        if (ix_UNLIKELY(i >= 48))
        {
            uint64_t see1 = seed;
            uint64_t see2 = seed;
            do
            {
                seed = ix_wymix(ix_wyr8(p) ^ secret[1], ix_wyr8(p + 8) ^ seed);
                see1 = ix_wymix(ix_wyr8(p + 16) ^ secret[2], ix_wyr8(p + 24) ^ see1);
                see2 = ix_wymix(ix_wyr8(p + 32) ^ secret[3], ix_wyr8(p + 40) ^ see2);
                p += 48;
                i -= 48;
            } while (ix_LIKELY(i >= 48));
            seed ^= see1 ^ see2;
        }
        while (ix_UNLIKELY(i > 16))
        {
            seed = ix_wymix(ix_wyr8(p) ^ secret[1], ix_wyr8(p + 8) ^ seed);
            i -= 16;
            p += 16;
        }
        a = ix_wyr8(p + i - 16);
        b = ix_wyr8(p + i - 8);
    }
    a ^= secret[1];
    b ^= seed;
    ix_wymum(a, b);
    return ix_wymix(a ^ secret[0] ^ length, b ^ secret[1]);
}

ix_FORCE_INLINE constexpr size_t ix_hash64(uint64_t x)
{
    constexpr uint64_t s = 0x1a83813678375e8dULL; // Random number.
    return ix_wymix(x, s);
}

#endif

ix_FORCE_INLINE constexpr size_t ix_hash_str(const char *s)
{
    return ix_hash(s, ix_strlen(s));
}

template <typename T>
struct ix_Hash;

#define ix_DEFINE_INTEGER_HASH(T)                              \
    template <>                                                \
    struct ix_Hash<T>                                          \
    {                                                          \
        ix_FORCE_INLINE constexpr size_t operator()(T x) const \
        {                                                      \
            return ix_hash64(static_cast<uint64_t>(x));        \
        }                                                      \
    }

ix_DEFINE_INTEGER_HASH(int8_t);
ix_DEFINE_INTEGER_HASH(int16_t);
ix_DEFINE_INTEGER_HASH(int32_t);
ix_DEFINE_INTEGER_HASH(int64_t);
ix_DEFINE_INTEGER_HASH(uint8_t);
ix_DEFINE_INTEGER_HASH(uint16_t);
ix_DEFINE_INTEGER_HASH(uint32_t);
ix_DEFINE_INTEGER_HASH(uint64_t);

#if ix_PLATFORM(WASM)
ix_DEFINE_INTEGER_HASH(unsigned long);
#endif

template <>
struct ix_Hash<float>
{
    ix_FORCE_INLINE constexpr size_t operator()(float x) const
    {
        union Temp
        {
            float f;
            uint32_t i;
        } temp{};
        temp.f = x;
        return ix_hash64(static_cast<uint64_t>(temp.i));
    }
};

template <>
struct ix_Hash<double>
{
    ix_FORCE_INLINE constexpr size_t operator()(double x) const
    {
        union Temp
        {
            double d;
            size_t i;
        } temp{};
        temp.d = x;
        return ix_hash64(temp.i);
    }
};

template <typename T>
struct ix_Hash<T *>
{
    ix_FORCE_INLINE constexpr size_t operator()(T *x) const
    {
        return ix_hash64(reinterpret_cast<size_t>(x));
    }
};

struct ix_StringHash
{
    ix_FORCE_INLINE constexpr size_t operator()(const char *x) const
    {
        return ix_hash_str(x);
    }
};
