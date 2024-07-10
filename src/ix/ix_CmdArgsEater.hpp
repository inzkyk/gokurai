#pragma once

#include "ix.hpp"
#include "ix_Span.hpp"
#include "ix_Vector.hpp"
#include "ix_initializer_list.hpp"

// ix_CmdArgsEater is not efficient in terms of both space and time. However, this should not be a
// problem because N is almost always small. If you process thousands of arguments and hundreds of options,
// ix_CmdArgsEater might not suffice.
class ix_CmdArgsEater
{
    ix_Vector<const char *> m_args;

  public:
    ix_CmdArgsEater(int argc, const char *argv[]);
    ix_CmdArgsEater(std::initializer_list<const char *> args); // cppcheck-suppress noExplicitConstructor; NOLINT

    size_t size() const;
    int argc() const;
    const char *const *argv() const;
    const char *const &operator[](size_t index) const;
    bool eat_boolean(const char *name);
    bool eat_boolean(ix_Span<const char *> names);
    const char *eat_kv(const char *key);
    const char *eat_kv(ix_Span<const char *> keys);
    bool eat_kv_n(const char *key, size_t n, const char **out);
};
