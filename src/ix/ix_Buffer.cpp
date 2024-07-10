#include "ix_Buffer.hpp"
#include "ix_DumbString.hpp"
#include "ix_TempFile.hpp"
#include "ix_doctest.hpp"
#include "ix_file.hpp"
#include "ix_memory.hpp"
#include "ix_min_max.hpp"
#include "ix_random_fill.hpp"
#include "ix_utility.hpp"

#include <fcntl.h>

static constexpr size_t BUFFER_SHAMT = ((sizeof(size_t) * 8) - 1);

ix_FORCE_INLINE static bool ix_Buffer_is_resizable(const ix_Buffer *buffer)
{
    return ((buffer->capacity() >> BUFFER_SHAMT) != 1);
}

[[maybe_unused]] ix_FORCE_INLINE static size_t ix_Buffer_true_capcity(const ix_Buffer *buffer)
{
    const bool is_resizable = ix_Buffer_is_resizable(buffer);
    const size_t fake_capacity = buffer->capacity();
    return is_resizable ? fake_capacity : (ix_SIZE_MAX - fake_capacity);
}

ix_Buffer::ix_Buffer(size_t initial_capacity)
    : m_capacity(initial_capacity)
{
    if (initial_capacity != 0)
    {
        m_data = ix_MALLOC(char *, initial_capacity);
    }
}

ix_Buffer::~ix_Buffer()
{
    if (ix_Buffer_is_resizable(this))
    {
        ix_FREE(m_data);
    }
}

ix_Buffer::ix_Buffer(ix_Buffer &&other) noexcept
    : m_data(other.m_data),
      m_size(other.m_size),
      m_capacity(other.m_capacity)
{
    other.m_data = nullptr;
    other.m_capacity = 0;
    other.m_size = 0;
}

ix_Buffer ix_Buffer::from_existing_region(char *p, size_t length)
{
    constexpr size_t MAX_SIZE = ((size_t{1} << BUFFER_SHAMT) - 1);
    ix_ASSERT_FATAL(length < MAX_SIZE);
    const size_t fake_capacity = ix_SIZE_MAX - length;
    ix_ASSERT_FATAL((fake_capacity >> BUFFER_SHAMT) == 1);
    ix_Buffer buffer;
    buffer.m_data = p;
    buffer.m_size = 0;
    buffer.m_capacity = fake_capacity;
    return buffer;
}

ix_Buffer &ix_Buffer::operator=(ix_Buffer &&other) noexcept
{
    if (this == &other)
    {
        return *this;
    }

    if (m_data != nullptr)
    {
        ix_FREE(m_data);
    }

    m_data = other.m_data;
    m_size = other.m_size;
    m_capacity = other.m_capacity;

    other.m_data = nullptr;
    other.m_capacity = 0;
    other.m_size = 0;

    return *this;
}

const char *ix_Buffer::data() const
{
    return m_data;
}

char *ix_Buffer::data()
{
    return m_data;
}

size_t ix_Buffer::size() const
{
    return m_size;
}

size_t ix_Buffer::capacity() const
{
    return m_capacity;
}

ix_TEST_CASE("ix_Buffer: move")
{
    ix_Buffer buffer0(64);
    buffer0.push_str("hello");
    ix_Buffer buffer1 = ix_move(buffer0);
    ix_EXPECT(buffer1.size() == 5);
    ix_Buffer buffer2(16);
    buffer2.push_str("foo");
    buffer2 = ix_move(buffer1);
    ix_EXPECT(buffer2.size() == 5);

    ix_DISABLE_CLANG_WARNING_BEGIN
    ix_DISABLE_CLANG_WARNING("-Wself-move")

    buffer2 = ix_move(buffer2);

    ix_DISABLE_CLANG_WARNING_END
}

ix_TEST_CASE("ix_Buffer::ix_Buffer")
{
    // nonzero capacity
    {
        const ix_Buffer buffer(1024);
        ix_EXPECT(buffer.data() != nullptr);
        ix_EXPECT(buffer.empty());
        ix_EXPECT(buffer.capacity() == 1024);
    }

    // zero capacity
    {
        const ix_Buffer buffer;
        ix_EXPECT(buffer.data() == nullptr);
        ix_EXPECT(buffer.empty());
        ix_EXPECT(buffer.capacity() == 0);
    }

    // // from raw pointer
    // {
    //     char *p = ix_MALLOC(char *, 32);
    //     ix_Buffer buf(p, 0, 32);
    //     buf.push_str("hello world!");
    //     ix_EXPECT(buf.size() == 12);
    //     ix_EXPECT(buf.capacity() == 32);
    // }
}

void ix_Buffer::reserve(size_t new_capacity)
{
    if (new_capacity <= m_capacity)
    {
#if ix_OPT_LEVEL(DEBUG)
        ix_ASSERT_FATAL(new_capacity <= ix_Buffer_true_capcity(this));
#endif
        return;
    }

    m_data = ix_REALLOC(char *, m_data, new_capacity);
    m_capacity = new_capacity;
}

void ix_Buffer::reserve_aggressively(size_t minimum_capacity)
{
    if (minimum_capacity <= m_capacity)
    {
#if ix_OPT_LEVEL(DEBUG)
        ix_ASSERT_FATAL(minimum_capacity <= ix_Buffer_true_capcity(this));
#endif
        return;
    }

    const size_t new_capacity = ix_max(ix_grow_array_size(m_capacity), minimum_capacity);
    m_data = ix_REALLOC(char *, m_data, new_capacity);
    m_capacity = new_capacity;
}

ix_TEST_CASE("ix_Buffer::reserve")
{
    ix_Buffer buffer(32);
    buffer.reserve(5);
    ix_EXPECT(buffer.empty());
    ix_EXPECT(buffer.capacity() == 32);
    buffer.reserve(40);
    ix_EXPECT(buffer.empty());
    ix_EXPECT(buffer.capacity() == 40);
    buffer.reserve(128);
    ix_EXPECT(buffer.empty());
    ix_EXPECT(buffer.capacity() == 128);
    buffer.reserve(5);
    ix_EXPECT(buffer.empty());
    ix_EXPECT(buffer.capacity() == 128);
}

void ix_Buffer::ensure(size_t extra_capacity)
{
    reserve(m_size + extra_capacity);
}

void ix_Buffer::ensure_aggressively(size_t minimum_extra_capacity)
{
    reserve_aggressively(m_size + minimum_extra_capacity);
}

void *ix_Buffer::allocate(size_t size)
{
    ensure_aggressively(size);
    void *ans = static_cast<void *>(m_data + m_size);
    m_size += size;
    return ans;
}

ix_TEST_CASE("ix_Buffer::allocate")
{
    // zero
    {
        ix_Buffer buffer(5);
        const void *p = buffer.allocate(0);
        ix_EXPECT(p != nullptr);
        ix_EXPECT(buffer.empty());
        ix_EXPECT(buffer.capacity() == 5);
    }

    // zero with zero capacity
    {
        ix_Buffer buffer(0);
        const void *p = buffer.allocate(0);
        ix_EXPECT(p == nullptr);
        ix_EXPECT(buffer.empty());
        ix_EXPECT(buffer.capacity() == 0);
    }

    // without realloc
    {
        ix_Buffer buffer(5);
        char *str = static_cast<char *>(buffer.allocate(5));
        ix_EXPECT(buffer.size() == 5);
        str[0] = '0';
        str[1] = '1';
        str[2] = '2';
        str[3] = '3';
        str[4] = '\0';
        ix_EXPECT_EQSTR(str, "0123");
    }

    // with realloc
    {
        ix_Buffer buffer(1);
        ix_EXPECT(buffer.empty());
        ix_EXPECT(buffer.capacity() == 1);
        char *str = static_cast<char *>(buffer.allocate(5));
        ix_EXPECT(buffer.size() == 5);
        str[0] = '0';
        str[1] = '1';
        str[2] = '2';
        str[3] = '3';
        str[4] = '\0';
        ix_EXPECT_EQSTR(str, "0123");
    }

    // multiple
    {
        ix_Buffer buffer(5);
        buffer.allocate(5);
        ix_EXPECT(buffer.size() == 5);
        ix_EXPECT(buffer.capacity() == 5);
        buffer.allocate(5);
        ix_EXPECT(buffer.size() == 10);
        ix_EXPECT(buffer.capacity() == 10);
        buffer.allocate(5);
        ix_EXPECT(buffer.size() == 15);
        ix_EXPECT(buffer.capacity() == 20);
        buffer.allocate(5);
        ix_EXPECT(buffer.size() == 20);
        ix_EXPECT(buffer.capacity() == 20);
        buffer.allocate(5);
        ix_EXPECT(buffer.size() == 25);
        ix_EXPECT(buffer.capacity() == 40);
    }
}

void ix_Buffer::clear()
{
    m_size = 0;
}

void ix_Buffer::reset()
{
    ix_FREE(m_data);
    m_data = nullptr;
    m_capacity = 0;
    m_size = 0;
}

ix_TEST_CASE("ix_Buffer::clear")
{
    ix_Buffer buffer(32);
    buffer.push_str("hello");
    ix_EXPECT(buffer.size() == 5);
    ix_EXPECT(buffer.capacity() == 32);
    buffer.clear();
    ix_EXPECT(buffer.empty());
    ix_EXPECT(buffer.capacity() == 32);
    buffer.clear();
    ix_EXPECT(buffer.empty());
    ix_EXPECT(buffer.capacity() == 32);
}

ix_TEST_CASE("ix_Buffer::reset")
{
    ix_Buffer buffer(32);
    buffer.push_str("hello");
    ix_EXPECT(buffer.size() == 5);
    ix_EXPECT(buffer.capacity() == 32);
    buffer.reset();
    ix_EXPECT(buffer.data() == nullptr);
    ix_EXPECT(buffer.empty());
    ix_EXPECT(buffer.capacity() == 0);
    buffer.reset();
    ix_EXPECT(buffer.data() == nullptr);
    ix_EXPECT(buffer.empty());
    ix_EXPECT(buffer.capacity() == 0);
}

void ix_Buffer::push(const void *new_data, size_t new_data_size)
{
    if (new_data_size == 0)
    {
        return;
    }

    void *ans = allocate(new_data_size);
    ix_memcpy(ans, new_data, new_data_size);
}

ix_TEST_CASE("ix_Buffer::push")
{
    // without realloc
    {
        ix_Buffer buffer(4);
        buffer.push("foo", 1);
        buffer.push("oo", 2);
        buffer.push("", 1);
        ix_EXPECT_EQSTR(buffer.data(), "foo");
    }

    // with realloc
    {
        ix_Buffer buffer(4);
        buffer.push("hello world", 12);
        ix_EXPECT_EQSTR(buffer.data(), "hello world");
    }
}

void ix_Buffer::push_between(const void *start, const void *end)
{
    ix_ASSERT(start <= end);
    const ptrdiff_t len_i = static_cast<const uint8_t *>(end) - static_cast<const uint8_t *>(start);
    const size_t len = static_cast<size_t>(len_i);
    push(start, len);
}

ix_TEST_CASE("ix_Buffer::push_between")
{
    ix_Buffer buffer(64);
    const char *msg = "hello";
    buffer.push_between(msg, msg);
    ix_EXPECT(buffer.empty());
    buffer.push_between(msg, msg + 4);
    ix_EXPECT(buffer.size() == 4);
    buffer.push_char('\0');
    ix_EXPECT(buffer.data() == ix_DumbString("hell"));
}

void ix_Buffer::push_char(char c)
{
    ensure_aggressively(1);
    m_data[m_size] = c;
    m_size += 1;
}

ix_TEST_CASE("ix_Buffer::push_char")
{
    ix_Buffer buffer(64);
    buffer.push_char('h');
    buffer.push_char('e');
    buffer.push_char('l');
    buffer.push_char('l');
    buffer.push_char('o');
    buffer.push_char(' ');
    buffer.push_char('w');
    buffer.push_char('o');
    buffer.push_char('r');
    buffer.push_char('l');
    buffer.push_char('d');
    buffer.push_char('!');
    buffer.push_char('\0');
    ix_EXPECT_EQSTR(buffer.data(), "hello world!");
    ix_EXPECT(buffer.size() == 13);
}

void ix_Buffer::push_char_repeat(char c, size_t n)
{
    ensure_aggressively(n);
    ix_memset(m_data + m_size, c, n);
    m_size += n;
}

ix_TEST_CASE("ix_Buffer::push_char_repeat")
{
    ix_Buffer buffer(64);
    buffer.push_char_repeat('h', 10);
    buffer.push_char('\0');
    ix_EXPECT_EQSTR(buffer.data(), "hhhhhhhhhh");
    ix_EXPECT(buffer.size() == 11);
}

void ix_Buffer::push_str(const char *str)
{
    push(str, ix_strlen(str));
}

ix_TEST_CASE("ix_Buffer::push_str")
{
    ix_Buffer buffer(64);
    buffer.push_str("foo");
    ix_EXPECT(buffer.size() == 3);
    buffer.push_str("");
    ix_EXPECT(buffer.size() == 3);
    buffer.push_str("foo");
    ix_EXPECT(buffer.size() == 6);
    buffer.push_str("foo");
    ix_EXPECT(buffer.size() == 9);
}

void ix_Buffer::pop_back(size_t length)
{
    ix_ASSERT(length <= m_size);
    m_size -= length;
}

ix_TEST_CASE("ix_Buffer::pop_back")
{
    ix_Buffer buffer(64);
    buffer.push_str("hello");
    buffer.push_str("hello");
    ix_EXPECT(buffer.size() == 10);
    buffer.pop_back(5);
    ix_EXPECT(buffer.size() == 5);
}

void ix_Buffer::add_size(size_t size)
{
    m_size += size;
    ix_ASSERT(m_size <= m_capacity);
}

ix_TEST_CASE("ix_Buffer::add_size")
{
    ix_Buffer buffer(8);
    buffer.add_size(0);
    buffer.add_size(4);
    buffer.add_size(4);
    ix_EXPECT(buffer.size() == 8);
    ix_EXPECT(buffer.capacity() == 8);
}

ix_UniquePointer<char[]> ix_Buffer::detach()
{
    char *ans = m_data;
    m_data = nullptr;
    m_size = 0;
    m_capacity = 0;
    return ix_UniquePointer<char[]>(ix_move(ans));
}

ix_TEST_CASE("ix_Buffer::detach")
{
    ix_Buffer buffer(16);
    buffer.push_str("hello world");
    ix_UniquePointer<char[]> s = buffer.detach();
    ix_EXPECT(s.get() != nullptr);
    ix_EXPECT(ix_Buffer().detach().get() == nullptr);
}

void ix_Buffer::set_size(size_t size)
{
    ix_ASSERT(size < m_capacity);
    m_size = size;
}

ix_TEST_CASE("ix_Buffer::set_size")
{
    ix_Buffer buffer(32);
    buffer.set_size(0);
    buffer.push_str("hello world");
    buffer.set_size(8);
    ix_EXPECT(buffer.size() == 8);
    buffer.set_size(0);
    ix_EXPECT(buffer.empty());
}

const char *ix_Buffer::begin() const
{
    return m_data;
}

char *ix_Buffer::begin()
{
    return m_data;
}

const char *ix_Buffer::end() const
{
    return m_data + m_size;
}

char *ix_Buffer::end()
{
    return m_data + m_size;
}

ix_TEST_CASE("ix_Buffer: begin/end")
{
    // non-const
    {
        ix_Buffer buffer(32);
        ix_EXPECT(buffer.begin() == buffer.end());
        buffer.push_str("xxxxx");
        ix_EXPECT(buffer.end() == buffer.begin() + buffer.size());

        size_t num = 0;
        for (const char c : buffer)
        {
            num += 1;
            ix_EXPECT(c == 'x');
        }
        ix_EXPECT(num == buffer.size());
    }

    // const
    {
        char p[32];
        p[0] = 'x';
        p[1] = 'x';
        p[2] = 'x';
        const auto buffer = ix_Buffer::from_existing_array(p);
        size_t num = 0;
        for (const char c : buffer)
        {
            num += 1;
            ix_EXPECT(c == 'x');
        }
        ix_EXPECT(num == buffer.size());
    }
}

bool ix_Buffer::empty() const
{
    return (m_size == 0);
}

ix_TEST_CASE("ix_Buffer: empty")
{
    ix_Buffer buffer(0);
    ix_EXPECT(buffer.empty());
    buffer.push_str("foo");
    ix_EXPECT(!buffer.empty());
    buffer.clear();
    ix_EXPECT(buffer.empty());
}

size_t ix_Buffer::load_file(const char *filename)
{
    ix_ASSERT(filename != nullptr);

    ix_FileHandle file(filename, ix_READ_ONLY);
    if (!file.is_valid())
    {
        return ix_SIZE_MAX;
    }

    const size_t file_size = file.size();
    char *p = static_cast<char *>(allocate(file_size + 1));
    file.read(p, file_size);
    if (file_size == ix_SIZE_MAX)
    {
        return ix_SIZE_MAX;
    }
    p[file_size] = '\0';

    file.close();

    return file_size;
}

ix_TEST_CASE("ix_Buffer: load_file")
{
    ix_Buffer buffer(64);
    ix_EXPECT(buffer.load_file("non-existent.txt") == ix_SIZE_MAX);
    ix_EXPECT(buffer.empty());

    const char *filename = ix_temp_filename();
    const char *message = "hello world!";
    ix_EXPECT(ix_write_string_to_file(filename, message).is_ok());

    ix_EXPECT(buffer.load_file(filename) == 12);
    ix_EXPECT(buffer.size() == 13);
    ix_EXPECT(buffer.data() == ix_DumbString("hello world!"));

    buffer.clear();
    ix_EXPECT(buffer.load_file(filename) == 12);
    buffer.pop_back(1);
    ix_EXPECT(buffer.load_file(filename) == 12);
    buffer.pop_back(1);
    ix_EXPECT(buffer.load_file(filename) == 12);
    ix_EXPECT(buffer.size() == 37);
    ix_EXPECT(buffer.data() == ix_DumbString("hello world!hello world!hello world!"));

    ix_EXPECT(ix_remove_file(filename).is_ok());
}

size_t ix_Buffer::load_file_handle(const ix_FileHandle &file, size_t first_read_size)
{
    const size_t file_size = file.size();
    if ((file_size != ix_SIZE_MAX) && (file_size != 0))
    {
        first_read_size = file_size;
    }
    else if (first_read_size == 0)
    {
        first_read_size = ix_max(m_capacity - m_size, size_t{1024});
    }

    const size_t initial_size = m_size;
    size_t read_size = first_read_size;
    while (true)
    {
        ensure(read_size);
        const size_t bytes_read = file.read(m_data + m_size, read_size);
        if (bytes_read == ix_SIZE_MAX)
        {
            return ix_SIZE_MAX;
        }

        const bool consumed = (bytes_read == 0);
        if (consumed)
        {
            break;
        }

        m_size += bytes_read;

        if (bytes_read == read_size)
        {
            read_size *= 2;
        }
    }

    return (m_size - initial_size);
}

ix_TEST_CASE("ix_Buffer: load_fd")
{
    ix_Buffer buffer(64);
    const ix_FileHandle null = ix_FileHandle::null(ix_READ_ONLY);
    ix_EXPECT(buffer.load_file_handle(null) == 0);

    static constexpr size_t MESSAGE_LENGTH = 2048;
    char message[MESSAGE_LENGTH + 1];
    ix_rand_fill_alphanumeric(message, MESSAGE_LENGTH);

    const char *filename = ix_temp_filename();
    ix_EXPECT(ix_write_string_to_file(filename, message).is_ok());

    {
        ix_FileHandle file(filename, ix_READ_ONLY);
        ix_EXPECT(file.is_valid());
        const size_t size = buffer.load_file_handle(file);
        ix_EXPECT(size == MESSAGE_LENGTH);
        ix_EXPECT(buffer.size() == MESSAGE_LENGTH);
        file.close();

        buffer.push_char('\0');
        ix_EXPECT_EQSTR(buffer.data(), message);
    }

    {
        ix_FileHandle file(filename, ix_READ_ONLY);
        ix_EXPECT(file.is_valid());
        const size_t size = buffer.load_file_handle(file, 128);
        ix_EXPECT(size == MESSAGE_LENGTH);
        file.close();

        buffer.push_char('\0');
        ix_EXPECT_EQSTR(buffer.data(), message);
    }

    ix_EXPECT(ix_remove_file(filename).is_ok());
}

ix_Buffer ix_Buffer::from_file(const char *filename)
{
    ix_Buffer buffer;
    buffer.load_file(filename);
    return buffer;
}

ix_TEST_CASE("ix_Buffer: from_file")
{
    const char *msg = "hello world";
    const ix_TempFile temp(msg);
    ix_EXPECT(temp.filename() != nullptr);
    ix_EXPECT_EQSTR(ix_Buffer::from_file(temp.filename()).data(), msg);
}

ix_TEST_CASE("ix_Buffer: from_array")
{
    char buf[32];
    auto buffer = ix_Buffer::from_existing_array(buf);

    buffer.push_str("hello world!");
    buffer.push_char('\0');
    ix_EXPECT_EQSTR(buffer.data(), "hello world!");
}
