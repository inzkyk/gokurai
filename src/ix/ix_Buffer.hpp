#pragma once

#include "ix.hpp"
#include "ix_UniquePointer.hpp"
#include "ix_file.hpp"

class ix_Buffer
{
    char *m_data = nullptr;
    size_t m_size = 0;
    size_t m_capacity = 0;

  public:
    ~ix_Buffer();

    ix_Buffer() = default;
    explicit ix_Buffer(size_t initial_capacity);
    // ix_Buffer(void *p, size_t size, size_t capacity);

    static ix_Buffer from_existing_region(char *p, size_t length);

    template <size_t N>
    static ix_Buffer from_existing_array(char (&arr)[N])
    {
        return ix_Buffer::from_existing_region(arr, N);
    }

    static ix_Buffer from_file(const char *filename);

    ix_Buffer(ix_Buffer &&other) noexcept;
    ix_Buffer(const ix_Buffer &other) = delete;
    ix_Buffer &operator=(ix_Buffer &&other) noexcept;
    ix_Buffer &operator=(const ix_Buffer &other) = delete;

    const char *data() const;
    size_t size() const;
    size_t capacity() const;
    bool empty() const;

    void clear();
    void reset();
    void pop_back(size_t length);

    // Exact
    void reserve(size_t new_capacity);
    void ensure(size_t extra_capacity);

    // Not exact
    void reserve_aggressively(size_t minimum_capacity);
    void ensure_aggressively(size_t minimum_extra_capacity);

    // Not exact (uses reserve_aggressively and ensure_aggressively)
    void push(const void *data, size_t data_size);
    void push_between(const void *start, const void *end);
    void push_str(const char *str);
    void push_char(char c);
    void push_char_repeat(char c, size_t n);

    // Not exact
    size_t load_file(const char *filename);
    // `first_read_size = 0` means `first_read_size = m_capacity - m_size`.
    size_t load_file_handle(const ix_FileHandle &file, size_t first_read_size = 0);

    char *data();
    char *begin();
    char *end();
    const char *begin() const;
    const char *end() const;

    // Danger zone. Use with caution!
    void set_size(size_t size);
    void add_size(size_t size);
    void *allocate(size_t size);
    ix_UniquePointer<char[]> detach();
};
