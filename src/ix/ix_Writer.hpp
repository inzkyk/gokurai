#pragma once

#include "ix.hpp"
#include "ix_Buffer.hpp"
#include "ix_UniquePointer.hpp"
#include "ix_printf.hpp"

#include <stdarg.h>

class ix_Writer
{
    ix_Buffer m_buffer;
    const ix_FileHandle *m_file;

  public:
    explicit ix_Writer(size_t buffer_initial_size, const ix_FileHandle *file = nullptr);

    ~ix_Writer();
    ix_Writer(ix_Writer &&);
    ix_Writer(const ix_Writer &) = delete;
    ix_Writer &operator=(ix_Writer &&);
    ix_Writer &operator=(const ix_Writer &) = delete;

    ix_FORCE_INLINE static ix_Writer from_existing_region(char *p, size_t length, const ix_FileHandle *file = nullptr)
    {
        return ix_Writer(ix_Buffer::from_existing_region(p, length), file);
    }

    template <size_t N>
    ix_FORCE_INLINE static ix_Writer from_existing_array(char (&arr)[N], const ix_FileHandle *file = nullptr)
    {
        return ix_Writer::from_existing_region(arr, N, file);
    }

    const ix_FileHandle *file_handle() const;
    const char *data() const;
    size_t buffer_size() const;
    size_t buffer_capacity() const;
    void reserve_buffer_capacity(size_t new_capacity);

    void write(const void *data, size_t data_size);
    void write_between(const void *begin, const void *end);
    void write_char(char c);
    void write_char_repeat(char c, size_t n);
    void write_string(const char *str);
    ix_PRINTF_FORMAT(2, 0) void write_stringfv(ix_FORMAT_ARG const char *format, va_list args);
    ix_PRINTF_FORMAT(2, 3) void write_stringf(ix_FORMAT_ARG const char *format, ...);
    void end_string();
    void flush();
    void clear();

    ix_UniquePointer<char[]> detach();
    void substitute(const ix_FileHandle *file);

  private:
    ix_Writer(ix_Buffer &&buffer, const ix_FileHandle *file);
};
