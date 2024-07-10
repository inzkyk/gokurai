#pragma once

#include "ix.hpp"
#include "ix_Result.hpp"
#include "ix_UniquePointer.hpp"
#include "ix_printf.hpp"
#include "ix_string.hpp"

#define ix_MAX_PATH 255

class ix_FileHandle;

ix_Result ix_init_stdio();

bool ix_is_file(const char *path);
bool ix_is_directory(const char *path);

ix_Result ix_write_to_file(const char *filename, const char *data, size_t data_length);
ix_Result ix_write_string_to_file(const char *filename, const char *str);
ix_UniquePointer<char[]> ix_load_file(const char *filename, size_t *length = nullptr);
ix_Result ix_create_directory(const char *path);
ix_Result ix_ensure_directories(char *path);
ix_Result ix_ensure_directories(const char *path);
ix_FileHandle ix_create_directories_and_file(char *path);
ix_FileHandle ix_create_directories_and_file(const char *path);

ix_Result ix_remove_directory(const char *path);
ix_Result ix_remove_file(const char *path);

const char *ix_temp_file_dirname();
size_t ix_temp_file_dirname_length();

constexpr const size_t ix_TEMP_FILENAME_RANDOM_PART_LENGTH = 16;
const char *ix_temp_filename(const char *prefix = "ix_");
size_t ix_temp_filename_length(size_t prefix_length = ix_strlen("ix_"));

using HANDLE = void *;

enum ix_FileOpenOption : uint8_t
{
    ix_READ_ONLY,
    ix_WRITE_ONLY,
};

class ix_FileHandle
{
    friend ix_Result ix_init_stdio();

    union
    {
        int fd;
        HANDLE handle;
    } v;

  public:
    ix_FileHandle();
    ix_FileHandle(const char *path, ix_FileOpenOption option);

    ix_FileHandle(const ix_FileHandle &other) = delete;
    ix_FileHandle &operator=(const ix_FileHandle &) = delete;

    ix_FileHandle(ix_FileHandle &&other) noexcept;
    ix_FileHandle &operator=(ix_FileHandle &&other) noexcept;

    ~ix_FileHandle();

    void close();
    bool is_valid() const;
    size_t write(const void *data, size_t data_length) const;
    size_t write_string(const char *str) const;
    size_t write_char(char c) const;
    ix_PRINTF_FORMAT(2, 0) void write_stringfv(ix_FORMAT_ARG const char *format, va_list args) const;
    ix_PRINTF_FORMAT(2, 3) void write_stringf(ix_FORMAT_ARG const char *format, ...) const;
    size_t read(void *buffer, size_t buffer_length) const;
    ix_UniquePointer<char[]> read_all(size_t *size_out, size_t first_read_length = 1024) const;
    size_t size() const;

    static ix_FileHandle null(ix_FileOpenOption option = ix_WRITE_ONLY);

    static const ix_FileHandle &of_stdin();
    static const ix_FileHandle &of_stdout();
    static const ix_FileHandle &of_stderr();

  private:
    void invalidate();
};
