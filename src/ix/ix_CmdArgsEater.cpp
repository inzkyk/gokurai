#include "ix_CmdArgsEater.hpp"
#include "ix_doctest.hpp"
#include "ix_string.hpp"

ix_CmdArgsEater::ix_CmdArgsEater(int argc, const char *argv[])
{
    const size_t num_args = static_cast<size_t>(argc);
    m_args.initial_reserve(num_args);
    for (size_t i = 0; i < num_args; i++)
    {
        m_args.emplace_back_no_reserve(argv[i]);
    }
}

// cppcheck-suppress noExplicitConstructor; NOLINT
ix_CmdArgsEater::ix_CmdArgsEater(std::initializer_list<const char *> args)
    : ix_CmdArgsEater(static_cast<int>(args.size()), const_cast<const char **>(args.begin()))
{
}

size_t ix_CmdArgsEater::size() const
{
    return m_args.size();
}

int ix_CmdArgsEater::argc() const
{
    return static_cast<int>(m_args.size());
}

const char *const *ix_CmdArgsEater::argv() const
{
    return &m_args[0];
}

const char *const &ix_CmdArgsEater::operator[](size_t index) const
{
    return m_args[index];
}

bool ix_CmdArgsEater::eat_boolean(const char *name)
{
    const size_t num_args = m_args.size();
    for (size_t i = 1; i < num_args; i++)
    {
        const char **arg = &m_args[i];
        if (ix_strcmp(name, *arg) == 0)
        {
            m_args.erase(arg);
            return true;
        }
    }

    return false;
}

bool ix_CmdArgsEater::eat_boolean(ix_Span<const char *> names)
{
    bool exists = false;

    const size_t num_names = names.size();
    for (size_t i = 0; i < num_names; i++)
    {
        exists |= eat_boolean(names[i]);
    }

    return exists;
}

const char *ix_CmdArgsEater::eat_kv(const char *key)
{
    const size_t num_args = m_args.size();
    for (size_t i = 1; i < num_args - 1; i++)
    {
        const char **arg = &m_args[i];
        if (ix_strcmp(key, *arg) == 0)
        {
            const char *value = *(arg + 1);
            m_args.erase(arg, 2);
            return value;
        }
    }

    return nullptr;
}

const char *ix_CmdArgsEater::eat_kv(ix_Span<const char *> keys)
{
    const char *value = nullptr;

    const size_t num_keys = keys.size();
    for (size_t i = 0; i < num_keys; i++)
    {
        const char *this_value = eat_kv(keys[i]);
        if (this_value != nullptr)
        {
            value = this_value;
        }
    }

    return value;
}

bool ix_CmdArgsEater::eat_kv_n(const char *key, size_t n, const char **out)
{
    const size_t num_args = m_args.size();

    if (m_args.size() <= n)
    {
        return false;
    }

    for (size_t i = 1; i < num_args - n; i++)
    {
        const char **arg = &m_args[i];
        if (ix_strcmp(key, *arg) == 0)
        {
            for (size_t j = 0; j < n; j++)
            {
                out[j] = *(arg + j + 1);
            }
            m_args.erase(arg, n + 1);
            return true;
        }
    }

    return false;
}

using Args = ix_Span<const char *const>;

[[maybe_unused]] static bool rest_args_equal(const ix_CmdArgsEater &rhs, const Args &lhs)
{
    if (rhs.size() != lhs.size())
    {
        return false;
    }

    for (size_t i = 0; i < rhs.size(); i++)
    {
        if (ix_strcmp(rhs[i], lhs[i]) != 0)
        {
            return false;
        }
    }

    return true;
}

ix_TEST_CASE("ix_CmdArgConsumer: size")
{
    ix_CmdArgsEater args{};
    ix_EXPECT(args.size() == 0);

    args = {"ix", "--foo", "--bar"};
    ix_EXPECT(args.size() == 3);
}

ix_TEST_CASE("ix_CmdArgEater: argc & argv")
{
    ix_CmdArgsEater args{};
    ix_EXPECT(args.argc() == 0);

    args = {"ix", "--foo", "--bar"};
    ix_EXPECT(args.argc() == 3);
    ix_EXPECT_EQSTR(args.argv()[0], "ix");
    ix_EXPECT_EQSTR(args.argv()[1], "--foo");
    ix_EXPECT_EQSTR(args.argv()[2], "--bar");
}

ix_TEST_CASE("ix_CmdArgConsumer: operator[]")
{
    ix_CmdArgsEater args{"ix"};
    ix_EXPECT_EQSTR(args[0], "ix");

    args = {"ix", "--foo", "--bar"};
    ix_EXPECT_EQSTR(args[0], "ix");
    ix_EXPECT_EQSTR(args[1], "--foo");
    ix_EXPECT_EQSTR(args[2], "--bar");
}

ix_TEST_CASE("ix_CmdArgConsumer: eat_boolean")
{
    ix_CmdArgsEater args{"ix", "--foo", "--bar"};
    ix_EXPECT(rest_args_equal(args, {"ix", "--foo", "--bar"}));
    const bool foobar = args.eat_boolean("--foobar");
    ix_EXPECT(!foobar);
    ix_EXPECT(rest_args_equal(args, {"ix", "--foo", "--bar"}));
    const bool foo = args.eat_boolean("--foo");
    ix_EXPECT(foo);
    ix_EXPECT(rest_args_equal(args, {"ix", "--bar"}));
    const bool bar = args.eat_boolean("--bar");
    ix_EXPECT(bar);
    ix_EXPECT(rest_args_equal(args, {
                                        "ix",
                                    }));
}

ix_TEST_CASE("ix_CmdArgConsumer: eat_boolean (multiple)")
{
    ix_CmdArgsEater args{"ix", "--foo", "--bar"};
    const bool foo_bar = args.eat_boolean({"--foo", "--bar"});
    ix_EXPECT(foo_bar);
    ix_EXPECT(rest_args_equal(args, {
                                        "ix",
                                    }));

    args = {"ix", "--foo", "--bar"};
    const bool foo = args.eat_boolean({"--foo", "--foofoo", "--foofoofoo"});
    ix_EXPECT(foo);
    ix_EXPECT(rest_args_equal(args, {"ix", "--bar"}));

    args = {"ix", "--foo", "--bar"};
    const bool f_ff_fff = args.eat_boolean({"--f", "--ff", "--fff"});
    ix_EXPECT(!f_ff_fff);
    ix_EXPECT(rest_args_equal(args, {"ix", "--foo", "--bar"}));
}

ix_TEST_CASE("ix_CmdArgConsumer: eat_boolean (when the name is same as program's name)")
{
    ix_CmdArgsEater args{"ix", "--foo", "--bar"};
    const bool ix = args.eat_boolean("ix");
    ix_EXPECT(!ix);
}

ix_TEST_CASE("ix_CmdArgConsumer: eat_kv")
{
    ix_CmdArgsEater args{"ix", "--foo", "foo", "--bar", "bar"};

    const char *foo = args.eat_kv("--foo");
    ix_EXPECT_EQSTR(foo, "foo");
    ix_EXPECT(rest_args_equal(args, {"ix", "--bar", "bar"}));

    const char *bar = args.eat_kv("--bar");
    ix_EXPECT_EQSTR(bar, "bar");
    ix_EXPECT(rest_args_equal(args, {"ix"}));

    args = {"ix", "--foo", "foo", "--bar", "bar"};
    const char *foobar = args.eat_kv("--foobar");
    ix_EXPECT(foobar == nullptr);
    ix_EXPECT(rest_args_equal(args, {"ix", "--foo", "foo", "--bar", "bar"}));

    args = {"ix", "--foo", "--bar"};
    const char *null = args.eat_kv("--bar");
    ix_EXPECT(null == nullptr);
}

ix_TEST_CASE("ix_CmdArgConsumer: eat_kv (multiple)")
{
    ix_CmdArgsEater args{"ix", "--foo", "foo", "--bar", "bar"};
    const char *bar = args.eat_kv({"--foo", "--bar"});
    ix_EXPECT_EQSTR(bar, "bar");

    args = {"ix", "--foo", "foo", "--bar", "bar"};
    const char *foo_foofoo = args.eat_kv({"--foo", "--foofoo"});
    ix_EXPECT_EQSTR(foo_foofoo, "foo");
    ix_EXPECT(rest_args_equal(args, {"ix", "--bar", "bar"}));

    args = {"ix", "--foo", "foo", "--bar", "bar"};
    const char *bar_barbar = args.eat_kv({"--bar", "--barbar"});
    ix_EXPECT_EQSTR(bar_barbar, "bar");
    ix_EXPECT(rest_args_equal(args, {"ix", "--foo", "foo"}));

    args = {"ix", "--foo", "foo", "--bar", "bar"};
    const char *foo = args.eat_kv({"--bar", "--foo"});
    ix_EXPECT_EQSTR(foo, "foo");

    args = {"ix", "--foo", "foo", "--bar", "bar"};
    const char *null = args.eat_kv({"--foobar", "--barfoo"});
    ix_EXPECT(null == nullptr);
}

ix_TEST_CASE("ix_CmdArgConsumer: eat_kv_n")
{
    ix_CmdArgsEater args{"ix", "--foo", "--resolution", "100", "200", "--bar"};

    const char *out[16] = {};
    bool found = args.eat_kv_n("--resolution", 2, out);
    ix_EXPECT(found);
    ix_EXPECT_EQSTR(out[0], "100");
    ix_EXPECT_EQSTR(out[1], "200");
    ix_EXPECT(rest_args_equal(args, {"ix", "--foo", "--bar"}));

    args = {"ix", "--foo", "--resolution", "100", "200", "--bar"};
    found = args.eat_kv_n("--foobar", 2, out);
    ix_EXPECT(!found);

    args = {"ix", "--foo", "--resolution", "100", "200", "--bar"};
    found = args.eat_kv_n("--resolution", 3, out);
    ix_EXPECT(found);
    ix_EXPECT_EQSTR(out[0], "100");
    ix_EXPECT_EQSTR(out[1], "200");
    ix_EXPECT_EQSTR(out[2], "--bar");
    ix_EXPECT(rest_args_equal(args, {"ix", "--foo"}));

    args = {"ix", "--foo", "--resolution", "100", "200", "--bar"};
    found = args.eat_kv_n("--resolution", 6, out);
    ix_EXPECT(!found);
}
