#pragma once

#include "ix.hpp"
#include "ix_compare.hpp"
#include "ix_hash.hpp"

// WARN: ix_DumbString is meant be used only in testing. Don't use it for other purposes.
class ix_DumbString
{
    size_t m_length;
    char *m_data;

    // Because of this harness, you can't memcpy() ix_DumbString.
    uint64_t m_harness[1];
    uint64_t *m_harness_pointer = m_harness;

  public:
    ~ix_DumbString();
    ix_DumbString();
    ix_DumbString(const char *str); // cppcheck-suppress noExplicitConstructor; NOLINT
    explicit ix_DumbString(int n);
    explicit ix_DumbString(int64_t n);
    explicit ix_DumbString(uint64_t n);
    ix_DumbString(const ix_DumbString &other);
    ix_DumbString(ix_DumbString &&other) noexcept;
    ix_DumbString &operator=(const ix_DumbString &other);
    ix_DumbString &operator=(ix_DumbString &&other) noexcept;
    const char *data() const;
    void repeat();
};

bool operator==(const ix_DumbString &rhs, const ix_DumbString &lhs);
bool operator!=(const ix_DumbString &rhs, const ix_DumbString &lhs);

template <>
struct ix_Compare<ix_DumbString>
{
    int8_t operator()(const ix_DumbString &x, const ix_DumbString &y) const
    {
        return static_cast<int8_t>(ix_strcmp(x.data(), y.data()));
    }
};

template <>
struct ix_Hash<ix_DumbString>
{
    ix_FORCE_INLINE size_t operator()(const ix_DumbString &x) const
    {
        return ix_hash_str(x.data());
    }
};
