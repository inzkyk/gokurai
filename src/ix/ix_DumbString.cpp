#include "ix_DumbString.hpp"
#include "ix_assert.hpp"
#include "ix_doctest.hpp"
#include "ix_initializer_list.hpp"
#include "ix_memory.hpp"
#include "ix_printf.hpp"
#include "ix_string.hpp"
#include "ix_utility.hpp"

static constexpr const char *dumb_message = "I am dumb.";

ix_DumbString::~ix_DumbString()
{
    ix_FREE(m_data);
    ix_ASSERT_FATAL(m_harness == m_harness_pointer);
}

ix_DumbString::ix_DumbString()
    : ix_DumbString(dumb_message)
{
}

ix_DumbString::ix_DumbString(const char *str)
    : m_length(ix_strlen(str)),
      m_data(ix_MALLOC(char *, m_length + 1))
{
    ix_memcpy(m_data, str, m_length + 1);
    ix_ASSERT(m_data[m_length] == '\0');
}

ix_DumbString::ix_DumbString(int n)
{
    char buf[64];
    const int length = ix_snprintf(buf, ix_LENGTH_OF(buf), "%d", n);
    ix_ASSERT(length >= 0);
    m_length = static_cast<size_t>(length);
    m_data = ix_MALLOC(char *, m_length + 1); // NOLINT(cppcoreguidelines-prefer-member-initializer) (false positive)
    ix_memcpy(m_data, buf, m_length + 1);
    ix_ASSERT(m_data[m_length] == '\0');
}

ix_DumbString::ix_DumbString(int64_t n)
    : ix_DumbString(static_cast<int>(n))
{
}

ix_DumbString::ix_DumbString(uint64_t n)
    : ix_DumbString(static_cast<int>(n))
{
}

ix_DumbString::ix_DumbString(const ix_DumbString &other)
    : ix_DumbString(other.m_data)
{
}

ix_DumbString::ix_DumbString(ix_DumbString &&other) noexcept
    : m_length(other.m_length),
      m_data(other.m_data)
{
    other.m_length = 0;
    other.m_data = nullptr;
}

ix_DumbString &ix_DumbString::operator=(const ix_DumbString &other) // cppcheck-suppress operatorEqVarError
{
    if (this == &other)
    {
        return *this;
    }

    ix_FREE(m_data);
    m_length = other.m_length;
    m_data = ix_MALLOC(char *, m_length + 1);
    ix_memcpy(m_data, other.m_data, m_length + 1);
    ix_ASSERT(m_data[m_length] == '\0');
    return *this;
}

ix_DumbString &ix_DumbString::operator=(ix_DumbString &&other) noexcept // cppcheck-suppress operatorEqVarError
{
    if (this == &other)
    {
        return *this;
    }

    ix_FREE(m_data);
    m_length = other.m_length;
    m_data = other.m_data;
    other.m_length = 0;
    other.m_data = nullptr;
    return *this;
}

bool operator==(const ix_DumbString &rhs, const ix_DumbString &lhs)
{
    return (ix_strcmp(rhs.data(), lhs.data()) == 0);
}

bool operator!=(const ix_DumbString &rhs, const ix_DumbString &lhs)
{
    return (ix_strcmp(rhs.data(), lhs.data()) != 0);
}

const char *ix_DumbString::data() const
{
    return m_data;
}

void ix_DumbString::repeat()
{
    const size_t old_length = m_length;
    char *old_data = m_data;
    m_length = 2 * m_length;
    m_data = ix_MALLOC(char *, m_length + 1);
    ix_memcpy(m_data, old_data, old_length);
    ix_memcpy(m_data + old_length, old_data, old_length + 1);
    ix_FREE(old_data);
}

ix_TEST_CASE("ix_DumbString")
{
    ix_EXPECT(ix_DumbString() == ix_DumbString());
    ix_EXPECT(ix_DumbString("foo") == ix_DumbString("foo"));
    ix_EXPECT(ix_DumbString("foo") == "foo");
    ix_EXPECT(ix_DumbString("foo") != ix_DumbString("bar"));
    ix_EXPECT(ix_DumbString("foo") != "bar");

    const ix_DumbString s0("copy");
    ix_DumbString s1(s0);
    s1.repeat();
    ix_EXPECT(s1 == "copycopy");
    ix_DumbString s2;
    s2 = s1;
    ix_EXPECT(s2 == "copycopy");

    ix_DumbString s3("move");
    ix_DumbString s4(ix_move(s3));
    ix_EXPECT(s4 == "move");
    ix_DumbString s5;
    s5 = ix_move(s4);
    ix_EXPECT(s5 == "move");

    ix_DISABLE_CLANG_WARNING_BEGIN
    ix_DISABLE_CLANG_WARNING("-Wself-move")
    ix_DISABLE_CLANG_WARNING("-Wself-assign-overloaded")

    s5 = ix_move(s5);
    s5 = s5;

    ix_DISABLE_CLANG_WARNING_END

    ix_DumbString s6("foo");
    s6.repeat();

    ix_EXPECT(s6 == "foofoo");
}

ix_TEST_CASE("ix_DumbString: ix_DumbString(n)")
{
    ix_EXPECT(ix_DumbString(100) == ix_DumbString("100"));
    ix_EXPECT(ix_DumbString(-100) == ix_DumbString("-100"));
    ix_EXPECT(ix_DumbString(0) == ix_DumbString("0"));
}
