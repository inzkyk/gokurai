#include "ix_filepath.hpp"
#include "ix_doctest.hpp"
#include "ix_file.hpp"
#include "ix_string.hpp"

bool ix_is_path_separator(char c)
{
#if ix_PLATFORM(WIN)
    return (c == '\\') || (c == '/');
#else
    return (c == '/');
#endif
}

char *ix_next_path_separator(const char *p)
{
    char c = *p;
    while (!ix_is_path_separator(c) && (c != L'\0'))
    {
        p += 1;
        c = *p;
    }
    return const_cast<char *>(p);
}

ix_TEST_CASE("ix_next_path_separator")
{
    const char *empty = "";
    ix_EXPECT_EQSTR(ix_next_path_separator(empty), "");

    const char *path = "foo/bar/foobar";
    path = ix_next_path_separator(path);
    ix_EXPECT_EQSTR(path, "/bar/foobar");
    path = ix_next_path_separator(path + 1);
    ix_EXPECT_EQSTR(path, "/foobar");
    path = ix_next_path_separator(path + 1);
    ix_EXPECT_EQSTR(path, "");

#if ix_PLATFORM(WIN)
    const char *mixed_path = "foo\\bar/foobar";
    mixed_path = ix_next_path_separator(mixed_path);
    ix_EXPECT_EQSTR(mixed_path, "\\bar/foobar");
    mixed_path = ix_next_path_separator(mixed_path + 1);
    ix_EXPECT_EQSTR(mixed_path, "/foobar");
    mixed_path = ix_next_path_separator(mixed_path + 1);
    ix_EXPECT_EQSTR(mixed_path, "");
#endif
}

size_t ix_dirname_length(const char *filename, size_t filename_length)
{
    const char *p = filename + filename_length;

    while (true)
    {
        if (ix_is_path_separator(*p))
        {
            return static_cast<size_t>(p - filename) + 1;
        }

        if (p == filename)
        {
            return 0;
        }

        p -= 1;
    }
}

size_t ix_basename_length(const char *filename, size_t filename_length)
{
    return filename_length - ix_dirname_length(filename);
}

size_t ix_dirname_length(const char *filename)
{
    return ix_dirname_length(filename, ix_strlen(filename));
}

size_t ix_basename_length(const char *filename)
{
    return ix_basename_length(filename, ix_strlen(filename));
}

ix_TEST_CASE("ix_dirname_length")
{
    ix_EXPECT(ix_dirname_length("") == 0);
    ix_EXPECT(ix_dirname_length("foo.txt") == 0);
    ix_EXPECT(ix_dirname_length("/") == 1);
    ix_EXPECT(ix_dirname_length("/foo.txt") == 1);
    ix_EXPECT(ix_dirname_length("./") == 2);
    ix_EXPECT(ix_dirname_length("./foo.txt") == 2);
    ix_EXPECT(ix_dirname_length("/foo/bar.txt") == 5);
    ix_EXPECT(ix_dirname_length("./foo/bar.txt") == 6);

    ix_EXPECT(ix_dirname_length("/foo") == 1);
    ix_EXPECT(ix_dirname_length("/foo/") == 5);
}

ix_TEST_CASE("ix_basename_length")
{
    ix_EXPECT(ix_basename_length("") == 0);
    ix_EXPECT(ix_basename_length("foo.txt") == 7);
    ix_EXPECT(ix_basename_length("/") == 0);
    ix_EXPECT(ix_basename_length("/foo.txt") == 7);
    ix_EXPECT(ix_basename_length("./") == 0);
    ix_EXPECT(ix_basename_length("./foo.txt") == 7);
    ix_EXPECT(ix_basename_length("/foo/bar.txt") == 7);
    ix_EXPECT(ix_basename_length("./foo/bar.txt") == 7);

    ix_EXPECT(ix_basename_length("/foo") == 3);
    ix_EXPECT(ix_basename_length("/foo/") == 0);

    const char *path = ix_temp_filename();
    ix_EXPECT(ix_basename_length(path) + ix_dirname_length(path) == ix_strlen(path));
}
