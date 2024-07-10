#include "ix_Writer.hpp"
#include "ix_TempFile.hpp"
#include "ix_doctest.hpp"
#include "ix_file.hpp"
#include "ix_memory.hpp"
#include "ix_printf.hpp"

#include <stdarg.h>
#include <stdio.h>

ix_Writer::ix_Writer(size_t buffer_initial_size, const ix_FileHandle *file)
    : m_buffer(buffer_initial_size),
      m_file(file)
{
}

ix_Writer::ix_Writer(ix_Buffer &&buffer, const ix_FileHandle *file)
    : m_buffer(ix_move(buffer)),
      m_file(file)
{
}

ix_Writer::~ix_Writer()
{
    flush();
}

ix_Writer::ix_Writer(ix_Writer &&other)
    : m_buffer(ix_move(other.m_buffer)),
      m_file(other.m_file)
{
    other.m_file = nullptr;
}

ix_Writer &ix_Writer::operator=(ix_Writer &&other)
{
    if (this == &other)
    {
        return *this;
    }

    flush();
    m_file = other.m_file;
    m_buffer = ix_move(other.m_buffer);

    other.m_file = nullptr;

    return *this;
}

size_t ix_Writer::buffer_size() const
{
    return m_buffer.size();
}

size_t ix_Writer::buffer_capacity() const
{
    return m_buffer.capacity();
}

void ix_Writer::reserve_buffer_capacity(size_t new_capacity)
{
    if (new_capacity < m_buffer.capacity())
    {
        return;
    }

    m_buffer.reserve(new_capacity);
}

ix_TEST_CASE("ix_Writer: reserve_buffer_capacity")
{
    ix_Writer w(64, nullptr);
    w.write_string("hello world");
    w.end_string();
    ix_EXPECT(w.buffer_capacity() == 64);
    ix_EXPECT_EQSTR(w.data(), "hello world");
    w.reserve_buffer_capacity(128);
    ix_EXPECT(w.buffer_capacity() == 128);
    ix_EXPECT_EQSTR(w.data(), "hello world");
    w.reserve_buffer_capacity(0);
    ix_EXPECT(w.buffer_capacity() == 128);
    ix_EXPECT_EQSTR(w.data(), "hello world");
}

void ix_Writer::flush()
{
    if (m_file != nullptr)
    {
        m_file->write(m_buffer.data(), m_buffer.size());
        m_buffer.clear();
    }
}

void ix_Writer::clear()
{
    flush();
    if (m_file == nullptr)
    {
        m_buffer.clear();
    }
}

const char *ix_Writer::data() const
{
    return m_buffer.data();
}

void ix_Writer::write(const void *data, size_t data_size)
{
    if (data_size == 0)
    {
        return;
    }

    if (m_file == nullptr)
    {
        m_buffer.push(data, data_size);
        return;
    }

    ix_ASSERT(m_file->is_valid());

    // If buf has enough sapce for data, we do not need to call write().
    if (m_buffer.size() + data_size <= m_buffer.capacity())
    {
        m_buffer.push(data, data_size);
        return;
    }

    // buf does not have enough space for data.
    // We need to write() buf and possibly part of data.
    m_file->write(m_buffer.data(), m_buffer.size());
    m_buffer.clear();

    if (data_size > m_buffer.capacity())
    {
        m_file->write(data, data_size);
    }
    else
    {
        m_buffer.push(data, data_size);
    }
}

ix_TEST_CASE("ix_Writer::write")
{
    // to buffer
    {
        ix_Writer writer(128);
        writer.write("hello ", 6);
        writer.write("world", 6);
        ix_EXPECT(writer.buffer_size() == 12);
        ix_EXPECT_EQSTR(writer.data(), "hello world");
    }

    // to buffer, many times
    {
        ix_Writer writer(1);
        for (size_t i = 0; i < 31; i++)
        {
            writer.write("a", 1);
        }
        writer.write("", 1);
        ix_EXPECT(writer.buffer_size() == 32);
        ix_EXPECT_EQSTR(writer.data(), "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa");
    }

    // to ix_FileHandle, with big buffer
    {
        const ix_FileHandle null = ix_FileHandle::null();
        ix_Writer writer(128, &null);
        writer.write("testing ", 8);
        writer.write("ix_Writer... ", 13);
        writer.write("(1/3)\n", 6);
        ix_EXPECT(writer.buffer_size() == 27);
    }

    // to ix_FileHandle, with small buffer
    {
        const ix_FileHandle null = ix_FileHandle::null();
        ix_Writer writer(1, &null);
        writer.write("testing ", 8);
        writer.write("ix_Writer... ", 13);
        writer.write("(2/3)\n", 6);
        ix_EXPECT(writer.buffer_size() == 0);
    }

    // to ix_FileHandle, with mid-sized buffer
    {
        const ix_FileHandle null = ix_FileHandle::null();
        ix_Writer writer(18, &null);
        writer.write("testing ", 8);
        writer.write("ix_Writer... ", 13);
        writer.write("(2/3)\n", 6);
        ix_EXPECT(writer.buffer_size() == 6);
    }
}

void ix_Writer::write_between(const void *begin, const void *end)
{
    const char *x = static_cast<const char *>(begin);
    const char *y = static_cast<const char *>(end);
    write(begin, static_cast<size_t>(y - x));
}

ix_TEST_CASE("ix_Writer::write_between")
{
    const char *msg = "hello world";
    ix_Writer writer(128);
    writer.write_between(msg, msg + 5);
    writer.end_string();
    ix_EXPECT_EQSTR(writer.data(), "hello");
}

void ix_Writer::write_char(char c)
{
    write(&c, 1);
}

ix_TEST_CASE("ix_Writer::write_char")
{
    ix_Writer writer(64);
    writer.write_char('h');
    writer.write_char('e');
    writer.write_char('l');
    writer.write_char('l');
    writer.write_char('o');
    writer.write_char(' ');
    writer.write_char('w');
    writer.write_char('o');
    writer.write_char('r');
    writer.write_char('l');
    writer.write_char('d');
    writer.write_char('!');
    writer.end_string();
    ix_EXPECT_EQSTR(writer.data(), "hello world!");
}

void ix_Writer::write_char_repeat(char c, size_t n)
{
    if (n == 0)
    {
        return;
    }

    if (m_file == nullptr)
    {
        m_buffer.push_char_repeat(c, n);
        return;
    }

    // If buf has enough sapce, we do not need to call write().
    if (m_buffer.size() + n <= m_buffer.capacity())
    {
        m_buffer.push_char_repeat(c, n);
        return;
    }

    // We need to write() buf and possibly part of data.
    m_file->write(m_buffer.data(), m_buffer.size());
    m_buffer.clear();

    if (n <= m_buffer.capacity())
    {
        m_buffer.push_char_repeat(c, n);
        return;
    }

    // WARN: very slow
    for (size_t i = 0; i < n; i++)
    {
        m_file->write(&c, 1);
    }
}

ix_TEST_CASE("ix_Writer::write_char_repeat")
{
    {
        ix_Writer writer(64);
        writer.write_char_repeat('h', 10);
        writer.end_string();
        ix_EXPECT_EQSTR(writer.data(), "hhhhhhhhhh");
    }

    {
        ix_Writer writer(64);
        writer.write_char_repeat('h', 0);
        writer.end_string();
        ix_EXPECT_EQSTR(writer.data(), "");
    }

    {
        ix_TempFileW temp;
        ix_Writer writer(8, &temp.file_handle());
        writer.write_char_repeat('h', 4);
        writer.write_char_repeat('h', 6);
        writer.write_char_repeat('h', 10);
        writer.end_string();
        writer.flush();
        ix_EXPECT_EQSTR(temp.data(), "hhhhhhhhhhhhhhhhhhhh");
    }
}

void ix_Writer::write_string(const char *str)
{
    write(str, ix_strlen(str));
}

ix_TEST_CASE("ix_Writer::write_string")
{
    ix_Writer writer(64);
    writer.write_string("hello ");
    writer.write_string("world!");
    writer.end_string();
    ix_EXPECT_EQSTR(writer.data(), "hello world!");
}

void ix_Writer::end_string()
{
    if (m_file == nullptr)
    {
        write_char('\0');
    }
}

ix_TEST_CASE("ix_Writer::end_string")
{
    ix_Writer writer(64);
    writer.end_string();
    ix_EXPECT_EQSTR(writer.data(), "");
}

ix_PRINTF_FORMAT(2, 0) void ix_Writer::write_stringfv(ix_FORMAT_ARG const char *format, va_list args)
{
    va_list args_copy;
    va_copy(args_copy, args);

    const int formatted_length_signed = ix_vsnprintf(nullptr, 0, format, args);
    ix_ASSERT(formatted_length_signed >= 0);

    const size_t required_length = static_cast<size_t>(formatted_length_signed) + 1;

    char buffer[ix_OPT_LEVEL(DEBUG) ? 32 : 1024];
    char *formatted = buffer;
    if (required_length > sizeof(buffer))
    {
        formatted = ix_MALLOC(char *, required_length);
    }

    const int wrote_length_signed = ix_vsnprintf(formatted, required_length, format, args_copy);
    ix_ASSERT(wrote_length_signed >= 0);

    const size_t wrote_length = static_cast<size_t>(wrote_length_signed);
    write(formatted, wrote_length);

    const bool data_allocated = (formatted != buffer);
    if (data_allocated)
    {
        ix_FREE(formatted);
    }

    va_end(args_copy);
}

ix_PRINTF_FORMAT(2, 3) void ix_Writer::write_stringf(ix_FORMAT_ARG const char *format, ...)
{
    va_list args;
    va_start(args, format);
    write_stringfv(format, args);
    va_end(args);
}

ix_TEST_CASE("ix_Writer::write_stringf")
{
    // empty
    {
        ix_Writer writer(64);

        ix_DISABLE_GCC_WARNING_BEGIN
        ix_DISABLE_GCC_WARNING("-Wformat-zero-length")
        writer.write_stringf("");
        ix_DISABLE_GCC_WARNING_END
        writer.end_string();
        ix_EXPECT_EQSTR(writer.data(), "");
    }

    // literal
    {
        ix_Writer writer(64);
        writer.write_stringf("hello world!\n");
        writer.end_string();
        ix_EXPECT_EQSTR(writer.data(), "hello world!\n");
    }

    // format string
    {
        ix_Writer writer(64);
        writer.write_stringf("%s %s!\n", "hello", "world");
        writer.end_string();
        ix_EXPECT_EQSTR(writer.data(), "hello world!\n");
    }

    // long format string
    {
        ix_Writer writer(32);
        const int x = 1 << 15;
        writer.write_stringf("%d%d%d%d%d%d%d%d", x, x, x, x, x, x, x, x);
        writer.end_string();
        ix_EXPECT_EQSTR(writer.data(), "3276832768327683276832768327683276832768");
    }
}

ix_UniquePointer<char[]> ix_Writer::detach()
{
    return m_buffer.detach();
}

const ix_FileHandle *ix_Writer::file_handle() const
{
    return m_file;
}

ix_TEST_CASE("ix_Writer: fd")
{
    ix_EXPECT(ix_Writer(64).file_handle() == nullptr);

    const ix_FileHandle null = ix_FileHandle::null();
    ix_EXPECT(ix_Writer(64, &null).file_handle() != nullptr);
}

ix_TEST_CASE("ix_Writer: clear")
{
    { // fd == -1
        ix_Writer w(64);
        w.write_string("hello world!");
        ix_EXPECT(w.buffer_size() == 12);
        w.clear();
        ix_EXPECT(w.buffer_size() == 0);
    }

    { // fd != -1
        ix_TempFileW temp;
        ix_Writer w(64, &temp.file_handle());
        const char *message = "hello world!";
        w.write_string(message);
        w.end_string();
        ix_EXPECT(w.buffer_size() == 12);
        w.clear();
        ix_EXPECT(w.buffer_size() == 0);
        ix_EXPECT_EQSTR(temp.data(), message);
    }
}

ix_TEST_CASE("ix_Writer: detach")
{
    ix_Writer w(64);
    const char *message = "hello world!";
    w.write_string(message);
    w.end_string();
    ix_UniquePointer<char[]> data = w.detach();
    ix_EXPECT_EQSTR(data.get(), message);
}

void ix_Writer::substitute(const ix_FileHandle *file)
{
    clear();
    m_file = file;
}

ix_TEST_CASE("ix_Writer: substitute")
{
    ix_FileHandle file[2] = {
        ix_FileHandle("foo.txt", ix_WRITE_ONLY),
        ix_FileHandle("bar.txt", ix_WRITE_ONLY),
    };

    ix_Writer w(128, &file[0]);
    w.write_string("This is foo.\n");
    w.substitute(&file[1]);
    w.write_string("This is bar.\n");
    w.substitute(nullptr);
    w.write_string("This is buf.\n");
    w.end_string();

    file[0].close();
    file[1].close();

    ix_EXPECT_EQSTR(ix_load_file("foo.txt").get(), "This is foo.\n");
    ix_EXPECT_EQSTR(ix_load_file("bar.txt").get(), "This is bar.\n");
    ix_EXPECT_EQSTR(w.data(), "This is buf.\n");

    w.substitute(nullptr);
    w.write_string("This is another buf.\n");
    w.end_string();

    ix_EXPECT_EQSTR(w.data(), "This is another buf.\n");

    ix_EXPECT(ix_remove_file("foo.txt").is_ok());
    ix_EXPECT(ix_remove_file("bar.txt").is_ok());
}

ix_TEST_CASE("ix_Writer: from_array")
{
    char buf[32];
    auto writer = ix_Writer::from_existing_array(buf);
    writer.write_stringf("hello %s!", "world");
    writer.end_string();
    ix_EXPECT_EQSTR(writer.data(), "hello world!");
}
