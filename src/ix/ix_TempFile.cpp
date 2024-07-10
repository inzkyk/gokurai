#include "ix_TempFile.hpp"
#include "ix_Buffer.hpp"
#include "ix_Logger.hpp"
#include "ix_doctest.hpp"
#include "ix_file.hpp"
#include "ix_memory.hpp"
#include "ix_string.hpp"

#include <fcntl.h>

ix_TempFile::~ix_TempFile()
{
    const ix_Result result = ix_remove_file(m_filename);
    if (result.is_error())
    {
        ix_log_error("ix_TempFile: Failed to remove \"%s\"", m_filename);
    }
    ix_FREE(m_filename);
}

ix_TempFile::ix_TempFile(const char *str, size_t str_length)
{
    const size_t filename_length = ix_temp_filename_length();
    m_filename = ix_MALLOC(char *, filename_length + 1);
    ix_memcpy(m_filename, ix_temp_filename(), filename_length);
    m_filename[filename_length] = '\0';

    const ix_FileHandle file(m_filename, ix_WRITE_ONLY);
    ix_ASSERT(file.is_valid());
    file.write(str, str_length);
}

ix_TempFile::ix_TempFile()
    : ix_TempFile(nullptr, 0)
{
}

ix_TempFile::ix_TempFile(const char *str)
    : ix_TempFile(str, ix_strlen(str))
{
}

const char *ix_TempFile::filename() const
{
    return m_filename;
}

ix_TempFileR::~ix_TempFileR()
{
    if (m_filename == nullptr)
    {
        ix_ASSERT(!m_file.is_valid());
        return;
    }

    m_file.close();
    const ix_Result result = ix_remove_file(m_filename);
    if (result.is_error())
    {
        ix_log_error("ix_TempFileR: Failed to remove \"%s\"", m_filename);
    }
    ix_FREE(m_filename);
}

ix_TempFileR::ix_TempFileR(const char *str)
    : ix_TempFileR(str, (str == nullptr) ? 0 : ix_strlen(str))
{
}

ix_TempFileR::ix_TempFileR(const char *str, size_t str_length)
    : m_filename(nullptr)
{
    if (str == nullptr)
    {
        return;
    }

    const size_t filename_length = ix_temp_filename_length();
    m_filename = ix_MALLOC(char *, filename_length + 1);
    ix_memcpy(m_filename, ix_temp_filename(), filename_length);
    m_filename[filename_length] = '\0';

    m_file = ix_FileHandle(m_filename, ix_WRITE_ONLY);
    m_file.write(str, str_length);
    m_file.close();

    m_file = ix_FileHandle(m_filename, ix_READ_ONLY);
    ix_ASSERT(m_file.is_valid());
}

const ix_FileHandle &ix_TempFileR::file_handle() const
{
    return m_file;
}

const char *ix_TempFileR::filename() const
{
    return m_filename;
}

ix_TempFileW::~ix_TempFileW()
{
    if (m_file.is_valid())
    {
        m_file.close();
    }

    const ix_Result result = ix_remove_file(m_filename);
    if (result.is_error())
    {
        ix_log_error("ix_TempFileW: Failed to remove \"%s\"", m_filename);
    }
    ix_FREE(m_filename);
}

ix_TempFileW::ix_TempFileW()
    : m_filename(nullptr),
      m_data(nullptr)
{
    const size_t filename_length = ix_temp_filename_length();
    m_filename = ix_MALLOC(char *, filename_length + 1);
    ix_memcpy(m_filename, ix_temp_filename(), filename_length);
    m_filename[filename_length] = '\0';

    m_file = ix_FileHandle(m_filename, ix_WRITE_ONLY);
    ix_ASSERT(m_file.is_valid());
}

const ix_FileHandle &ix_TempFileW::file_handle() const
{
    return m_file;
}

const char *ix_TempFileW::filename() const
{
    return m_filename;
}

void ix_TempFileW::close()
{
    if (m_file.is_valid())
    {
        m_file.close();
    }
}

char *ix_TempFileW::data()
{
    if (m_data.get() == nullptr)
    {
        close();
        ix_Buffer buffer;
        buffer.load_file(m_filename);
        m_data = buffer.detach();
    }
    return m_data.get();
}

void ix_TempFileW::reset()
{
    // Destruct.
    close();
    const ix_Result result = ix_remove_file(m_filename);
    if (result.is_error())
    {
        ix_log_error("ix_TempFileW: Failed to remove \"%s\"", m_filename);
    }
    ix_FREE(m_filename);

    // Construct.
    m_filename = nullptr;
    m_data.swap(nullptr);

    const size_t filename_length = ix_temp_filename_length();
    m_filename = ix_MALLOC(char *, filename_length + 1);
    ix_memcpy(m_filename, ix_temp_filename(), filename_length);
    m_filename[filename_length] = '\0';

    m_file = ix_FileHandle(m_filename, ix_WRITE_ONLY);
    ix_ASSERT(m_file.is_valid());
}

ix_TEST_CASE("ix_TempFile")
{
    {
        const ix_TempFile temp;
        ix_EXPECT(temp.filename() != nullptr);
        ix_EXPECT_EQSTR(ix_Buffer::from_file(temp.filename()).data(), "");
    }

    {
        const char *msg = "hello world";
        const ix_TempFile temp(msg);
        ix_EXPECT(temp.filename() != nullptr);
        ix_EXPECT_EQSTR(ix_Buffer::from_file(temp.filename()).data(), msg);
    }

    {
        const char *msg = "hello world";
        const ix_TempFile temp(msg, 5);
        ix_EXPECT(temp.filename() != nullptr);
        ix_EXPECT_EQSTR(ix_Buffer::from_file(temp.filename()).data(), "hello");
    }
}

ix_TEST_CASE("ix_TempFileR")
{
    const char *s = "hello world!";
    ix_UniquePointer<char[]> content(nullptr);

    // Constructor with a string.
    content = ix_TempFileR(s).file_handle().read_all(nullptr);
    ix_EXPECT_EQSTR(content.get(), s);

    // Constructor with a pointer and a length.
    content = ix_TempFileR(s, 5).file_handle().read_all(nullptr);
    ix_EXPECT_EQSTR(content.get(), "hello");
}

ix_TEST_CASE("ix_TempFileW")
{
    { // Do nothing.
        const ix_TempFileW w;
        ix_EXPECT(w.file_handle().is_valid());
        ix_EXPECT(w.filename() != nullptr);
    }

    { // Write and don't read.
        const ix_TempFileW w;
        const char *s = "hello world!";
        w.file_handle().write_string(s);
    }

    { // Write and data().
        ix_TempFileW w;
        const char *s = "hello world!";
        w.file_handle().write_string(s);
        ix_EXPECT_EQSTR(w.data(), s);
    }

    { // Write, close(), and load with filename().
        ix_TempFileW w;
        const char *s = "hello world!";
        w.file_handle().write_string(s);
        w.close();
        ix_UniquePointer<char[]> content = ix_load_file(w.filename());
        ix_EXPECT_EQSTR(content.get(), s);
    }

    { // Call data() multiple times.
        ix_TempFileW w;
        const char *s = "hello world!";
        w.file_handle().write_string(s);
        ix_EXPECT_EQSTR(w.data(), s);
        ix_EXPECT_EQSTR(w.data(), s);
        ix_EXPECT_EQSTR(w.data(), s);
    }

    { // reset()
        ix_TempFileW w;
        const char *s = "hello world!";
        w.file_handle().write_string(s);
        ix_EXPECT_EQSTR(w.data(), s);

        w.reset();
        s = "bye world!";
        w.file_handle().write_string(s);
        ix_EXPECT_EQSTR(w.data(), s);
    }
}
