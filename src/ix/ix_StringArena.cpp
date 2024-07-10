#include "ix_StringArena.hpp"
#include "ix_doctest.hpp"
#include "ix_memory.hpp"
#include "ix_random_fill.hpp"
#include "ix_string.hpp"

ix_StringArena::ix_StringArena(size_t pool_size)
    : m_pool_size(pool_size),
      m_current_pool(nullptr)
{
    ix_ASSERT(pool_size != 0);
}

ix_StringArena::~ix_StringArena()
{
    for (const Pool &pool : m_pools)
    {
        ix_FREE(pool.start);
    }
}

void ix_StringArena::clear()
{
    if (m_pools.empty())
    {
        return;
    }

    const size_t pool_size = m_pools.size();
    for (size_t i = 1; i < pool_size; i++)
    {
        ix_FREE(m_pools[i].start);
    }

    if (!m_pools.empty())
    {
        m_pools.set_size(1);
        m_current_pool = &m_pools[0];
    }
}

ix_TEST_CASE("ix_StringArena::clear()")
{
    ix_StringArena arena(128);

    {
        const char *s1 = arena.push_str("foo");
        ix_EXPECT_EQSTR(s1, "foo");
        ix_EXPECT(arena.push_str("foo") != s1);

        const char *s2 = arena.push_str("bar");
        ix_EXPECT_EQSTR(s2, "bar");
        ix_EXPECT(arena.push_str("bar") != s2);

        const char *s3 = arena.push_str("xxx");
        ix_EXPECT_EQSTR(s3, "xxx");
    }

    arena.clear();

    {
        const char *s1 = arena.push_str("foo");
        ix_EXPECT_EQSTR(s1, "foo");
        ix_EXPECT(arena.push_str("foo") != s1);

        const char *s2 = arena.push_str("bar");
        ix_EXPECT_EQSTR(s2, "bar");
        ix_EXPECT(arena.push_str("bar") != s2);

        const char *s3 = arena.push_str("xxx");
        ix_EXPECT_EQSTR(s3, "xxx");
    }
}

size_t ix_StringArena::size() const
{
    size_t s = 0;
    for (const Pool &pool : m_pools)
    {
        s += static_cast<size_t>(pool.next - pool.start);
    }
    return s;
}

ix_TEST_CASE("ix_StringArena::size")
{
    ix_StringArena arena(6);

    arena.push_str("foo");
    ix_EXPECT(arena.size() == 4);

    arena.push_str("bar");
    ix_EXPECT(arena.size() == 8);

    arena.push_str("xxx");
    ix_EXPECT(arena.size() == 12);
}

const char *ix_StringArena::push_str(const char *str)
{
    return push(str, ix_strlen(str));
}

ix_TEST_CASE("ix_StringArena::push_str")
{
    // big pool
    {
        ix_StringArena arena(128);

        const char *s1 = arena.push_str("foo");
        ix_EXPECT_EQSTR(s1, "foo");
        ix_EXPECT(arena.push_str("foo") != s1);

        const char *s2 = arena.push_str("bar");
        ix_EXPECT_EQSTR(s2, "bar");
        ix_EXPECT(arena.push_str("bar") != s2);

        const char *s3 = arena.push_str("xxx");
        ix_EXPECT_EQSTR(s3, "xxx");
    }

    // small pool
    {
        ix_StringArena arena(4);

        const char *s1 = arena.push_str("foo");
        ix_EXPECT_EQSTR(s1, "foo");
        ix_EXPECT(arena.push_str("foo") != s1);

        const char *s2 = arena.push_str("bar");
        ix_EXPECT_EQSTR(s2, "bar");
        ix_EXPECT(arena.push_str("bar") != s2);

        const char *s3 = arena.push_str("xxx");
        ix_EXPECT_EQSTR(s3, "xxx");
    }

    // very small pool
    {
        ix_StringArena arena(1);

        const char *s1 = arena.push_str("foo");
        ix_EXPECT_EQSTR(s1, "foo");
        ix_EXPECT(arena.push_str("foo") != s1);

        const char *s2 = arena.push_str("bar");
        ix_EXPECT_EQSTR(s2, "bar");
        ix_EXPECT(arena.push_str("bar") != s2);

        const char *s3 = arena.push_str("xxx");
        ix_EXPECT_EQSTR(s3, "xxx");
    }
}

const char *ix_StringArena::push(const char *data, size_t length)
{
    const size_t required_space = length + 1;
    const bool new_pool_required = (m_current_pool == nullptr) || (m_current_pool->remain < required_space);

    if (new_pool_required)
    {
        const Pool *last_pool = nullptr;
        if (!m_pools.empty())
        {
            last_pool = &m_pools.back();
        }

        if (last_pool != m_current_pool)
        {
            m_current_pool += 1;
        }
        else
        {
            size_t new_pool_size = m_pool_size;
            if (new_pool_size < required_space)
            {
                new_pool_size = required_space;
            }

            Pool new_pool;
            new_pool.remain = new_pool_size;
            new_pool.start = ix_MALLOC(char *, new_pool_size);
            new_pool.next = new_pool.start;

            m_pools.push_back(new_pool);
            m_current_pool = &m_pools.back();
        }
    }

    ix_ASSERT(m_current_pool != nullptr);
    ix_memcpy(m_current_pool->next, data, length);
    m_current_pool->next[length] = '\0';

    const char *p = m_current_pool->next;
    m_current_pool->next += required_space;
    m_current_pool->remain -= required_space;

    return p;
}

ix_TEST_CASE("ix_StringArena::push")
{
    ix_StringArena arena(128);

    const char *s1 = arena.push("foo", 4);
    ix_EXPECT_EQSTR(s1, "foo");
    ix_EXPECT(arena.push_str("foo") != s1);

    const char *s2 = arena.push("bar", 4);
    ix_EXPECT_EQSTR(s2, "bar");
    ix_EXPECT(arena.push_str("bar") != s2);

    const char *s3 = arena.push("xxx", 4);
    ix_EXPECT_EQSTR(s3, "xxx");
}

const char *ix_StringArena::push_between(const char *start, const char *end)
{
    ix_ASSERT(start <= end);
    const size_t length = static_cast<size_t>(end - start);
    return push(start, length);
}

ix_TEST_CASE("ix_StringArena::push_between")
{
    ix_StringArena arena(128);
    const char *s = "hello world!";
    const char *s1 = arena.push_between(s, s + 5);
    ix_EXPECT_EQSTR(s1, "hello");
    const char *s2 = arena.push_between(s + 6, s + 11);
    ix_EXPECT_EQSTR(s2, "world");
}

ix_TEST_CASE("ix_StringArena:many pools")
{
    ix_StringArena arena(10);
    char buf[10];
    for (size_t i = 0; i < 1000; i++)
    {
        const size_t length = ix_rand_range<size_t>(0, ix_LENGTH_OF(buf) - 1);
        ix_rand_fill_alphanumeric(buf, length);
        arena.push_str(buf);
    }
}

void ix_StringArena::reset_to(const char *p)
{
    Pool *pool;
    size_t pool_index;

    const size_t num_pools = m_pools.size();
    for (size_t i = num_pools - 1;; i--)
    {
        Pool *candidate = &m_pools[i];
        const bool p_is_inside_this_pool = (candidate->start <= p) && (p < candidate->next);
        if (p_is_inside_this_pool)
        {
            pool_index = i;
            pool = candidate;
            break;
        }

        const bool pool_not_found = (i == 0);
        if (pool_not_found)
        {
            return;
        }
    }

    if (pool->start != p)
    {
        ix_ASSERT(*(p - 1) == '\0');
    }

    for (size_t i = pool_index + 1; i < num_pools; i++)
    {
        m_pools[i].remain += static_cast<size_t>(m_pools[i].next - m_pools[i].start);
        m_pools[i].next = m_pools[i].start;
    }

    const char *end = pool->next + pool->remain;
    pool->next = const_cast<char *>(p);
    pool->remain = static_cast<size_t>(end - p);

    m_current_pool = pool;
}

ix_TEST_CASE("ix_StringArena::reset_to")
{
    // large pool
    {
        ix_StringArena arena(16);
        const char *p[4];

        p[0] = arena.push_str("foo");
        ix_EXPECT(arena.size() == 4);

        p[1] = arena.push_str("foo");
        p[2] = arena.push_str("foo");
        p[3] = arena.push_str("foo");
        ix_EXPECT(arena.size() == 16);

        arena.reset_to(p[3]);
        ix_EXPECT(arena.size() == 12);
        arena.reset_to(p[2]);
        ix_EXPECT(arena.size() == 8);
        arena.reset_to(p[1]);
        ix_EXPECT(arena.size() == 4);
        arena.reset_to(p[0]);
        ix_EXPECT(arena.size() == 0);

        arena.push_str("foo");
        arena.push_str("foo");
        arena.push_str("foo");
        ix_EXPECT(arena.size() == 12);

        arena.reset_to("fooo");
        ix_EXPECT(arena.size() == 12);
    }

    // small pool
    {
        ix_StringArena arena(9);
        const char *p[4];

        p[0] = arena.push_str("foo");
        ix_EXPECT(arena.size() == 4);

        p[1] = arena.push_str("foo");
        p[2] = arena.push_str("foo");
        p[3] = arena.push_str("foo");
        ix_EXPECT(arena.size() == 16);

        arena.reset_to(p[3]);
        ix_EXPECT(arena.size() == 12);
        arena.reset_to(p[2]);
        ix_EXPECT(arena.size() == 8);
        arena.reset_to(p[1]);
        ix_EXPECT(arena.size() == 4);
        arena.reset_to(p[0]);
        ix_EXPECT(arena.size() == 0);

        arena.push_str("foo");
        arena.push_str("foo");
        arena.push_str("foo");
        ix_EXPECT(arena.size() == 12);
    }

    // many
    {
        ix_StringArena arena(8);
        const char *p = nullptr;

        for (size_t i = 0; i < 256; i++)
        {
            const char *q = arena.push_str("");
            if (i == 128)
            {
                p = q;
            }
        }

        ix_EXPECT(arena.size() == 256);
        arena.reset_to(p);
        ix_EXPECT(arena.size() == 128);

        for (size_t i = 0; i < 256; i++)
        {
            arena.push_str("");
        }

        ix_EXPECT(arena.size() == 384);
    }
}
