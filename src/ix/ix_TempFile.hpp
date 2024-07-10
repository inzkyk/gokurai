#pragma once

#include "ix.hpp"
#include "ix_UniquePointer.hpp"
#include "ix_file.hpp"

class ix_TempFile
{
    char *m_filename;

  public:
    ix_TempFile(const ix_TempFile &) = delete;
    ix_TempFile(ix_TempFile &&other) = delete;
    ix_TempFile &operator=(const ix_TempFile &) = delete;
    ix_TempFile &operator=(ix_TempFile &&) = delete;

    ~ix_TempFile();
    ix_TempFile();
    explicit ix_TempFile(const char *str);
    ix_TempFile(const char *str, size_t str_length);

    const char *filename() const;
};

class ix_TempFileR
{
    char *m_filename;
    ix_FileHandle m_file;

  public:
    ix_TempFileR(const ix_TempFileR &) = delete;
    ix_TempFileR(ix_TempFileR &&other) = delete;
    ix_TempFileR &operator=(const ix_TempFileR &) = delete;
    ix_TempFileR &operator=(ix_TempFileR &&) = delete;

    ~ix_TempFileR();
    explicit ix_TempFileR(const char *str);
    ix_TempFileR(const char *str, size_t str_length);

    const ix_FileHandle &file_handle() const;
    const char *filename() const;
};

class ix_TempFileW
{
    char *m_filename;
    ix_FileHandle m_file;
    ix_UniquePointer<char[]> m_data;

  public:
    ix_TempFileW(const ix_TempFileW &) = delete;
    ix_TempFileW(ix_TempFileW &&) = delete;
    ix_TempFileW &operator=(const ix_TempFileW &) = delete;
    ix_TempFileW &operator=(ix_TempFileW &&) = delete;

    ~ix_TempFileW();
    ix_TempFileW();

    const ix_FileHandle &file_handle() const;
    const char *filename() const;

    void close();
    char *data();
    void reset();
};
