#include "ix_file.hpp"
#include "ix_DumbString.hpp"
#include "ix_HollowValue.hpp"
#include "ix_TempFile.hpp"
#include "ix_assert.hpp"
#include "ix_doctest.hpp"
#include "ix_filepath.hpp"
#include "ix_memory.hpp"
#include "ix_printf.hpp"
#include "ix_random_fill.hpp"
#include "ix_string.hpp"

#include <stdio.h>
#include <stdlib.h>

#if ix_PLATFORM(WIN)
#include "ix_Windows.hpp"

#include <errhandlingapi.h>
#include <fcntl.h>
#include <fileapi.h>
#include <io.h>
#include <stringapiset.h>
#else
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <sys/stat.h>
#endif

#if ix_PLATFORM(WASM)
#include <emscripten.h>
#endif

#if ix_PLATFORM(WASM)
#define CWD "/cwd/"
#else
#define CWD
#endif

[[maybe_unused]] static void mount_cwd()
{
#if ix_PLATFORM(WASM)
    // clang-format off
    EM_ASM(
        if (!FS.analyzePath('/cwd').exists) {
            FS.mkdir('/cwd');
        }
        FS.mount(NODEFS, {root : '.'}, '/cwd');,
        ""
    );
    // clang-format on
#endif
}

[[maybe_unused]] static void unmount_cwd()
{
#if ix_PLATFORM(WASM)
    // clang-format off
    EM_ASM(
        FS.unmount('/cwd');,
        ""
    );
    // clang-format on
#endif
}

#if ix_PLATFORM(WIN)
static size_t utf8_path_to_wchar(const char *path_utf8, int path_utf8_length, wchar_t path_wchar[ix_MAX_PATH])
{
    const int wchar_path_length = MultiByteToWideChar(CP_UTF8, 0, path_utf8, path_utf8_length, nullptr, 0);
#if !ix_MEASURE_COVERAGE
    if (wchar_path_length < 0)
    {
        ix_log_debug("MultiByteToWideChar() failed.\n");
        path_wchar[0] = '\0';
        return 0;
    }
#endif

    if (wchar_path_length > ix_MAX_PATH)
    {
        path_wchar[0] = '\0';
#if ix_MEASURE_COVERAGE
        return 0;
#else
        ix_log_debug("Path (%s) is too long.\n", path_utf8);
        path_wchar[0] = '\0';
        return 0;
#endif
    }

    const int ret = MultiByteToWideChar(CP_UTF8, 0, path_utf8, path_utf8_length, path_wchar, ix_MAX_PATH);
#if ix_MEASURE_COVERAGE
    ix_UNUSED(ret);
#else
    if (ret < 0)
    {
        ix_log_debug("MultiByteToWideChar() failed.\n");
        path_wchar[0] = '\0';
        return 0;
    }
#endif

    return static_cast<size_t>(wchar_path_length) - 1;
}

static size_t utf8_path_to_wchar(const char *path_utf8, wchar_t path_wchar[ix_MAX_PATH])
{
    const int path_utf8_length = static_cast<int>(ix_strlen(path_utf8) + 1);
    return utf8_path_to_wchar(path_utf8, path_utf8_length, path_wchar);
}

static void wchar_path_to_utf8(const wchar_t path_wchar[ix_MAX_PATH], char path_utf8[ix_MAX_PATH])
{
    const int ret =
        WideCharToMultiByte(CP_UTF8, 0, path_wchar, ix_MAX_PATH, path_utf8, ix_MAX_PATH, nullptr, nullptr);

#if ix_MEASURE_COVERAGE
    ix_UNUSED(ret);
#else
    if (ret == 0)
    {
        path_utf8[0] = '\0';
        ix_log_debug("WideCharToMultiByte() failed.\n");
    }
#endif
}

#endif

static ix_HollowValue<ix_FileHandle> g_filehandle_stdin;
static ix_HollowValue<ix_FileHandle> g_filehandle_stdout;
static ix_HollowValue<ix_FileHandle> g_filehandle_stderr;

#if ix_PLATFORM(WIN)
#if !defined(STDIN_FILENO)
#define STDIN_FILENO 0
#endif
#if !defined(STDOUT_FILENO)
#define STDOUT_FILENO 1
#endif
#if !defined(STDERR_FILENO)
#define STDERR_FILENO 2
#endif
#else
#include <unistd.h>
#endif

ix_Result ix_init_stdio()
{
#if ix_PLATFORM(WIN)
    const int stdin_oldmode = setmode(STDIN_FILENO, O_BINARY);

#if !ix_MEASURE_COVERAGE
    if (stdin_oldmode == -1)
    {
        ix_log_debug("setmode() failed.\n");
        return ix_ERROR;
    }
#endif

    const int stdout_oldmode = setmode(STDOUT_FILENO, O_BINARY);

#if !ix_MEASURE_COVERAGE
    if (stdout_oldmode == -1)
    {
        ix_log_debug("setmode() failed.\n");
        return ix_ERROR;
    }
#endif

    const int stderr_oldmode = setmode(STDERR_FILENO, O_BINARY);

#if !ix_MEASURE_COVERAGE
    if (stderr_oldmode == -1)
    {
        ix_log_debug("setmode() failed.\n");
        return ix_ERROR;
    }
#endif

#if ix_MEASURE_COVERAGE
    ix_UNUSED(stdin_oldmode);
    ix_UNUSED(stdout_oldmode);
    ix_UNUSED(stderr_oldmode);
#endif
#endif

    g_filehandle_stdin.construct();
    g_filehandle_stdout.construct();
    g_filehandle_stderr.construct();

#if ix_PLATFORM(WIN)
    g_filehandle_stdin.get().v.handle = GetStdHandle(STD_INPUT_HANDLE);
    g_filehandle_stdout.get().v.handle = GetStdHandle(STD_OUTPUT_HANDLE);
    g_filehandle_stderr.get().v.handle = GetStdHandle(STD_ERROR_HANDLE);
#else
    g_filehandle_stdin.get().v.fd = STDIN_FILENO;
    g_filehandle_stdout.get().v.fd = STDOUT_FILENO;
    g_filehandle_stderr.get().v.fd = STDERR_FILENO;
#endif

    return ix_OK;
}

ix_Result ix_remove_file(const char *path)
{
#if ix_PLATFORM(WIN)
    wchar_t path_wchar[ix_MAX_PATH];
    utf8_path_to_wchar(path, path_wchar);

    const BOOL ret = DeleteFile(path_wchar);
    return ix_Result(ret != 0);
#else
    const int ret = unlink(path);
    return ix_Result(ret == 0);
#endif
}

bool ix_is_file(const char *path)
{
#if ix_PLATFORM(WIN)
    wchar_t path_wchar[ix_MAX_PATH];
    utf8_path_to_wchar(path, path_wchar);
    const DWORD ret = GetFileAttributes(path_wchar);
    return (ret != INVALID_FILE_ATTRIBUTES) && ((ret & FILE_ATTRIBUTE_DIRECTORY) == 0);
#else
    struct stat sb;
    int ret = stat(path, &sb);
    return (ret == 0) && ((sb.st_mode & S_IFMT) == S_IFREG);
#endif
}

ix_TEST_CASE("ix_is_file")
{
    const char *filename = ix_temp_filename();
    ix_EXPECT(!ix_is_file(filename));
    const char *message = "hello world";
    ix_EXPECT(ix_write_string_to_file(filename, message).is_ok());
    ix_EXPECT(ix_is_file(filename));
    ix_EXPECT(ix_remove_file(filename).is_ok());
    ix_EXPECT(!ix_is_file(filename));

    const char *dirname = ix_temp_file_dirname();
    ix_EXPECT(!ix_is_file(dirname));
}

bool ix_is_directory(const char *path)
{
#if ix_PLATFORM(WIN)
    wchar_t path_wchar[ix_MAX_PATH];
    utf8_path_to_wchar(path, path_wchar);
    const DWORD ret = GetFileAttributes(path_wchar);
    return (ret != INVALID_FILE_ATTRIBUTES) && ((ret & FILE_ATTRIBUTE_DIRECTORY) != 0);
#else
    struct stat sb;
    int ret = stat(path, &sb);
    return (ret == 0) && ((sb.st_mode & S_IFMT) == S_IFDIR);
#endif
}

ix_TEST_CASE("ix_is_directory")
{
    mount_cwd();

    const char *filename = ix_temp_filename();
    ix_EXPECT(!ix_is_directory(filename));
    const char *message = "hello world";
    ix_EXPECT(ix_write_string_to_file(filename, message).is_ok());
    ix_EXPECT(!ix_is_directory(filename));
    ix_EXPECT(ix_remove_file(filename).is_ok());
    ix_EXPECT(!ix_is_directory(filename));

    const char *dirname = ix_temp_file_dirname();
    ix_EXPECT(ix_is_directory(dirname));

    ix_EXPECT(ix_is_directory("."));
    ix_EXPECT(ix_is_directory("./"));
    ix_EXPECT(ix_is_directory(".."));
    ix_EXPECT(ix_is_directory("../"));
    ix_EXPECT(ix_is_directory(CWD "build"));
    ix_EXPECT(ix_is_directory(CWD "./build"));
    ix_EXPECT(ix_is_directory(CWD "build/../build/../build"));
    ix_EXPECT(ix_is_directory(CWD "build/../build/../build/../build/"));

#if ix_PLATFORM(WIN)
    ix_EXPECT(ix_is_directory(".\\"));
    ix_EXPECT(ix_is_directory("..\\"));
    ix_EXPECT(ix_is_directory("..\\ix"));
    ix_EXPECT(ix_is_directory("..\\ix\\"));
    ix_EXPECT(ix_is_directory("..\\ix\\..\\ix\\..\\ix"));
    ix_EXPECT(ix_is_directory("..\\ix\\..\\ix\\..\\ix\\..\\ix"));
#endif

    unmount_cwd();
}

ix_Result ix_write_to_file(const char *filename, const char *data, size_t data_length)
{
    const ix_FileHandle file(filename, ix_WRITE_ONLY);
    ix_ASSERT(file.is_valid());
    const size_t bytes_written = file.write(data, data_length);
    return ix_Result(bytes_written == data_length);
}

ix_Result ix_write_string_to_file(const char *filename, const char *str)
{
    return ix_write_to_file(filename, str, ix_strlen(str));
}

ix_UniquePointer<char[]> ix_load_file(const char *filename, size_t *length)
{
    ix_ASSERT(filename != nullptr);

    ix_FileHandle file = ix_FileHandle(filename, ix_READ_ONLY);

    if (!file.is_valid())
    {
        return ix_UniquePointer<char[]>(nullptr);
    }

    const size_t file_size = file.size();
    if (file_size == ix_SIZE_MAX)
    {
        return ix_UniquePointer<char[]>(nullptr);
    }

    char *p = ix_MALLOC(char *, file_size + 1);
    const size_t bytes_read = file.read(p, file_size);
    if (bytes_read == ix_SIZE_MAX)
    {
        return ix_UniquePointer<char[]>(nullptr);
    }
    p[file_size] = '\0';

    file.close();

    if (length != nullptr)
    {
        *length = file_size;
    }

    return ix_UniquePointer<char[]>(ix_move(p));
}

ix_TEST_CASE("ix_load_file")
{
    // nonexistent file
    {
        ix_UniquePointer<char[]> p = ix_load_file("foo", nullptr);
        ix_EXPECT(p.get() == nullptr);
    }

    // small file
    {
        const char *filename = ix_temp_filename();
        const char *message = "hello world!";
        ix_EXPECT(ix_write_string_to_file(filename, message).is_ok());
        size_t length = 0;
        ix_UniquePointer<char[]> str = ix_load_file(filename, &length);
        ix_EXPECT(str.get() != nullptr);
        ix_EXPECT_EQSTR(str.get(), message);
        ix_EXPECT(length == 12);
        ix_EXPECT(ix_remove_file(filename).is_ok());
    }
}

ix_Result ix_remove_directory(const char *path)
{
#if ix_PLATFORM(WIN)
    wchar_t path_wchar[ix_MAX_PATH];
    utf8_path_to_wchar(path, path_wchar);

    const BOOL ret = RemoveDirectory(path_wchar);
    return ix_Result(ret != 0);
#else
    const int ret = rmdir(path);
    return ix_Result(ret == 0);
#endif
}

ix_Result ix_create_directory(const char *path)
{
#if ix_PLATFORM(WIN)
    wchar_t path_wchar[ix_MAX_PATH];
    utf8_path_to_wchar(path, path_wchar);

    const BOOL ret = CreateDirectory(path_wchar, nullptr);
    return ix_Result(ret != 0);
#else
    const int ret = mkdir(path, 0777);
    return ix_Result(ret == 0);
#endif
}

ix_TEST_CASE("ix_create_directory")
{
    mount_cwd();

    ix_EXPECT(ix_create_directory("").is_error());
    ix_EXPECT(ix_create_directory(".").is_error());
    ix_EXPECT(ix_create_directory("./").is_error());
    ix_EXPECT(ix_create_directory("..").is_error());
    ix_EXPECT(ix_create_directory("../").is_error());

#if ix_PLATFORM(WASM)
    ix_EXPECT(ix_create_directory("../cwd").is_error());
    ix_EXPECT(ix_create_directory("../cwd/").is_error());
#else
    ix_EXPECT(ix_create_directory("../ix").is_error());
    ix_EXPECT(ix_create_directory("../ix/").is_error());
#endif

    ix_EXPECT(ix_create_directory("foo").is_ok());
    ix_EXPECT(ix_create_directory("foo").is_error());
    ix_EXPECT(ix_is_directory("foo"));
    ix_EXPECT(ix_remove_directory("foo").is_ok());

    ix_EXPECT(ix_create_directory("foo/").is_ok());
    ix_EXPECT(ix_is_directory("foo"));
    ix_EXPECT(ix_remove_directory("foo").is_ok());

    ix_EXPECT(ix_create_directory("./foo/").is_ok());
    ix_EXPECT(ix_is_directory("foo"));
    ix_EXPECT(ix_remove_directory("foo").is_ok());

#if ix_PLATFORM(WASM)
    ix_EXPECT(ix_create_directory("./cwd/././../cwd/./../foo/").is_ok());
#else
    ix_EXPECT(ix_create_directory("./././../ix/././foo/").is_ok());
#endif
    ix_EXPECT(ix_is_directory("foo"));
    ix_EXPECT(ix_remove_directory("foo").is_ok());

    ix_EXPECT(ix_create_directory("foo").is_ok());
    ix_EXPECT(ix_create_directory("foo/bar").is_ok());
    ix_EXPECT(ix_create_directory("foo/bar/foobar").is_ok());
    ix_EXPECT(ix_is_directory("foo"));
    ix_EXPECT(ix_is_directory("foo/bar"));
    ix_EXPECT(ix_is_directory("foo/bar/foobar"));
    ix_EXPECT(ix_create_directory("foo").is_error());
    ix_EXPECT(ix_create_directory("foo/bar").is_error());
    ix_EXPECT(ix_create_directory("foo/bar/foobar").is_error());
    ix_EXPECT(ix_remove_directory("foo/bar/foobar").is_ok());
    ix_EXPECT(ix_remove_directory("foo/bar").is_ok());
    ix_EXPECT(ix_remove_directory("foo").is_ok());

    const char *path = ix_temp_filename();
    ix_EXPECT(ix_create_directory(path).is_ok());
    ix_EXPECT(ix_create_directory(path).is_error());
    ix_EXPECT(ix_is_directory(path));
    ix_EXPECT(ix_remove_directory(path).is_ok());

#if ix_PLATFORM(WIN)
    ix_EXPECT(ix_create_directory(".\\").is_error());
    ix_EXPECT(ix_create_directory("..\\").is_error());
    ix_EXPECT(ix_create_directory("..\\ix").is_error());

    ix_EXPECT(ix_create_directory("foo").is_ok());
    ix_EXPECT(ix_create_directory("foo\\bar").is_ok());
    ix_EXPECT(ix_create_directory("foo\\bar\\foobar").is_ok());
    ix_EXPECT(ix_is_directory("foo"));
    ix_EXPECT(ix_is_directory("foo/bar"));
    ix_EXPECT(ix_is_directory("foo/bar/foobar"));
    ix_EXPECT(ix_create_directory("foo").is_error());
    ix_EXPECT(ix_create_directory("foo\\bar").is_error());
    ix_EXPECT(ix_create_directory("foo\\bar\\foobar").is_error());
    ix_EXPECT(ix_remove_directory("foo/bar/foobar").is_ok());
    ix_EXPECT(ix_remove_directory("foo/bar").is_ok());
    ix_EXPECT(ix_remove_directory("foo").is_ok());
#endif

    unmount_cwd();
}

#if ix_PLATFORM(WIN)
ix_FORCE_INLINE static bool is_path_separator(wchar_t c)
{
    return (c == L'\\') || (c == '/');
}

static wchar_t *next_path_separator(wchar_t *p)
{
    wchar_t c = *p;
    while (!is_path_separator(c) && (c != L'\0'))
    {
        p += 1;
        c = *p;
    }
    return p;
}
#endif

ix_Result ix_ensure_directories(char *path)
{
#if ix_PLATFORM(WIN)
    return ix_ensure_directories(static_cast<const char *>(path));
#else
    if (*path == '\0')
    {
        return ix_OK;
    }

    char *p = ix_next_path_separator(path + 1);
    while (*p != '\0')
    {
        const char temp = *p;
        *p = '\0';
        errno = 0;
        const int ret = mkdir(path, 0777);
        const bool fail = (ret != 0);
        if (fail)
        {
            bool exists = (errno == EEXIST);
#if ix_PLATFORM(WASM)
            // At least on WASM, `mkdir(".")` does not set errno to EEXIST.
            exists |= ix_is_directory(path);
#endif
            if (!exists)
            {
                return ix_ERROR;
            }
        }
        *p = temp;
        p = ix_next_path_separator(p + 1);
    }

    const bool ends_with_separator = ix_is_path_separator(*(p - 1));
    if (!ends_with_separator)
    {
        errno = 0;
        const int ret = mkdir(path, 0777);
        const bool fail = (ret != 0);
        if (fail)
        {
            // At least on WASM, `mkdir(".")` does not set errno to EEXIST.
            bool exists = (errno == EEXIST);
#if ix_PLATFORM(WASM)
            // At least on WASM, `mkdir(".")` does not set errno to EEXIST.
            exists |= ix_is_directory(path);
#endif
            if (!exists)
            {
                return ix_ERROR;
            }
        }
    }

    return ix_OK;
#endif
}

ix_Result ix_ensure_directories(const char *path)
{
#if ix_PLATFORM(WIN)
    if (*path == '\0')
    {
        return ix_OK;
    }

    wchar_t path_wchar[ix_MAX_PATH];
    utf8_path_to_wchar(path, path_wchar);

    if (*path_wchar == L'\0')
    {
        return ix_ERROR;
    }

    wchar_t *path_wchar_end = next_path_separator(path_wchar + 1);
    while (*path_wchar_end != L'\0')
    {
        const wchar_t temp = *path_wchar_end;
        *path_wchar_end = L'\0';
        const BOOL ret = CreateDirectory(path_wchar, nullptr);
        *path_wchar_end = temp;
        const bool fail = (ret == 0);
        if (fail)
        {
            const DWORD err = GetLastError();
            if (err != ERROR_ALREADY_EXISTS)
            {
                return ix_ERROR;
            }
        }
        path_wchar_end = next_path_separator(path_wchar_end + 1);
    }

    const bool ends_with_separator = is_path_separator(*(path_wchar_end - 1));
    if (!ends_with_separator)
    {
        const BOOL ret = CreateDirectory(path_wchar, nullptr);
        const bool fail = (ret == 0);
        if (fail)
        {
            const DWORD err = GetLastError();
            if (err != ERROR_ALREADY_EXISTS)
            {
                return ix_ERROR;
            }
        }
    }

    return ix_OK;
#else
    char buf[ix_MAX_PATH + 1];
    strncpy(buf, path, sizeof(buf));
    const bool path_too_long = (buf[sizeof(buf) - 1] != '\0');
    if (path_too_long)
    {
        return ix_ERROR;
    }

    return ix_ensure_directories(buf);
#endif
}

ix_TEST_CASE("ix_ensure_directories")
{
    mount_cwd();

    ix_EXPECT(ix_ensure_directories("").is_ok());
    ix_EXPECT(ix_ensure_directories(".").is_ok());
    ix_EXPECT(ix_ensure_directories("./").is_ok());
    ix_EXPECT(ix_ensure_directories("..").is_ok());
    ix_EXPECT(ix_ensure_directories("../").is_ok());
#if ix_PLATFORM(WASM)
    ix_EXPECT(ix_ensure_directories("../cwd").is_ok());
    ix_EXPECT(ix_ensure_directories("../cwd/").is_ok());
#else
    ix_EXPECT(ix_ensure_directories("../ix").is_ok());
    ix_EXPECT(ix_ensure_directories("../ix/").is_ok());
#endif

    ix_EXPECT(ix_ensure_directories("foo/bar/foobar").is_ok());
    ix_EXPECT(ix_ensure_directories("foo").is_ok());
    ix_EXPECT(ix_ensure_directories("foo/bar").is_ok());
    ix_EXPECT(ix_ensure_directories("foo/bar/foobar").is_ok());
    ix_EXPECT(ix_is_directory("foo"));
    ix_EXPECT(ix_is_directory("foo/bar"));
    ix_EXPECT(ix_is_directory("foo/bar/foobar"));
    ix_EXPECT(ix_remove_directory("foo/bar/foobar").is_ok());
    ix_EXPECT(ix_remove_directory("foo/bar").is_ok());
    ix_EXPECT(ix_remove_directory("foo").is_ok());

    ix_EXPECT(ix_ensure_directories("foo/bar/foobar/").is_ok());
    ix_EXPECT(ix_is_directory("foo"));
    ix_EXPECT(ix_is_directory("foo/bar"));
    ix_EXPECT(ix_is_directory("foo/bar/foobar"));
    ix_EXPECT(ix_remove_directory("foo/bar/foobar").is_ok());
    ix_EXPECT(ix_remove_directory("foo/bar").is_ok());
    ix_EXPECT(ix_remove_directory("foo").is_ok());

    ix_EXPECT(ix_ensure_directories("./foo/bar/foobar/").is_ok());
    ix_EXPECT(ix_is_directory("foo"));
    ix_EXPECT(ix_is_directory("foo/bar"));
    ix_EXPECT(ix_is_directory("foo/bar/foobar"));
    ix_EXPECT(ix_remove_directory("foo/bar/foobar").is_ok());
    ix_EXPECT(ix_remove_directory("foo/bar").is_ok());
    ix_EXPECT(ix_remove_directory("foo").is_ok());

#if ix_PLATFORM(WASM)
    ix_EXPECT(ix_ensure_directories("./../cwd/././foo/./../foo/././bar/./../../foo/bar/././foobar/").is_ok());
    ix_EXPECT(ix_ensure_directories("./../cwd/././foo/./../foo/././bar/./../../foo/bar/././foobar/").is_ok());
#else
    ix_EXPECT(ix_ensure_directories("./../ix/././foo/./../foo/././bar/./../../foo/bar/././foobar/").is_ok());
    ix_EXPECT(ix_ensure_directories("./../ix/././foo/./../foo/././bar/./../../foo/bar/././foobar/").is_ok());
#endif

    ix_EXPECT(ix_is_directory(CWD "foo"));
    ix_EXPECT(ix_is_directory(CWD "foo/bar"));
    ix_EXPECT(ix_is_directory(CWD "foo/bar/foobar"));
    ix_EXPECT(ix_remove_directory(CWD "foo/bar/foobar").is_ok());
    ix_EXPECT(ix_remove_directory(CWD "foo/bar").is_ok());
    ix_EXPECT(ix_remove_directory(CWD "foo").is_ok());

    const char *path = ix_temp_filename();
    ix_EXPECT(ix_ensure_directories(path).is_ok());
    ix_EXPECT(ix_is_directory(path));
    ix_EXPECT(ix_remove_directory(path).is_ok());

#if ix_PLATFORM(WIN)
    ix_EXPECT(ix_ensure_directories(".\\").is_ok());
    ix_EXPECT(ix_ensure_directories("..\\").is_ok());
    ix_EXPECT(ix_ensure_directories("..\\ix").is_ok());
    ix_EXPECT(ix_ensure_directories("..\\ix\\").is_ok());

    ix_EXPECT(ix_ensure_directories(R"(.\../ix\././foo\./../foo/.\./bar/./..\..\foo/bar/.\./foobar\)").is_ok());
    ix_EXPECT(ix_ensure_directories(R"(.\../ix\././foo\./../foo/.\./bar/./..\..\foo/bar/.\./foobar\)").is_ok());
    ix_EXPECT(ix_is_directory("foo"));
    ix_EXPECT(ix_is_directory("foo/bar"));
    ix_EXPECT(ix_is_directory("foo/bar/foobar"));
    ix_EXPECT(ix_remove_directory("foo/bar/foobar").is_ok());
    ix_EXPECT(ix_remove_directory("foo/bar").is_ok());
    ix_EXPECT(ix_remove_directory("foo").is_ok());
#else
    ix_EXPECT(ix_ensure_directories("01234567890123456789012345678901234567890123456789"
                                    "01234567890123456789012345678901234567890123456789"
                                    "01234567890123456789012345678901234567890123456789"
                                    "01234567890123456789012345678901234567890123456789"
                                    "01234567890123456789012345678901234567890123456789"
                                    "01234567890123456789012345678901234567890123456789")
                  .is_error());
#endif

    unmount_cwd();
}

ix_FileHandle ix_create_directories_and_file(char *path)
{
    const size_t dirname_length = ix_dirname_length(path);
    const bool path_too_long = (dirname_length >= ix_MAX_PATH);
    if (path_too_long)
    {
        return ix_FileHandle();
    }

    const char tmp = path[dirname_length];
    path[dirname_length] = '\0';
    const ix_Result result = ix_ensure_directories(path);
    path[dirname_length] = tmp;
    if (result.is_error())
    {
        return ix_FileHandle();
    }

    return ix_FileHandle(path, ix_WRITE_ONLY);
}

ix_FileHandle ix_create_directories_and_file(const char *path)
{
    const size_t dirname_length = ix_dirname_length(path);
    const bool path_too_long = (dirname_length >= ix_MAX_PATH);
    if (path_too_long)
    {
        return ix_FileHandle();
    }

    char buf[ix_MAX_PATH + 1];
    ix_memcpy(buf, path, dirname_length);
    buf[dirname_length] = '\0';
    const ix_Result result = ix_ensure_directories(buf);
    if (result.is_error())
    {
        return ix_FileHandle();
    }

    return ix_FileHandle(path, ix_WRITE_ONLY);
}

ix_TEST_CASE("ix_create_directories_and_file")
{
    ix_EXPECT(!ix_create_directories_and_file("").is_valid());

    auto f = [](const char *path) {
        ix_EXPECT(!ix_is_file(path));

        ix_FileHandle foo = ix_create_directories_and_file(path);
        ix_EXPECT(foo.is_valid());
        foo.close();

        ix_EXPECT(ix_is_file(path));
    };

    f("foo.txt");
    ix_EXPECT(ix_remove_file("foo.txt").is_ok());

    f("foo/foo/foo.txt");
    ix_EXPECT(ix_remove_file("foo/foo/foo.txt").is_ok());
    ix_EXPECT(ix_remove_directory("foo/foo/").is_ok());
    ix_EXPECT(ix_remove_directory("foo/").is_ok());

    const char *temp_path;
    temp_path = ix_temp_filename();
    f(temp_path);
    ix_EXPECT(ix_remove_file(temp_path).is_ok());

    auto g = [](const char *path) {
        ix_EXPECT(!ix_is_file(path));

        char buf[128];
        const size_t path_length = ix_strlen(path);
        ix_memcpy(buf, path, path_length);
        buf[path_length] = '\0';

        ix_FileHandle foo = ix_create_directories_and_file(buf);
        ix_EXPECT(foo.is_valid());
        foo.close();

        ix_EXPECT(ix_is_file(path));
    };

    g("foo.txt");
    ix_EXPECT(ix_remove_file("foo.txt").is_ok());

    g("foo/foo/foo.txt");
    ix_EXPECT(ix_remove_file("foo/foo/foo.txt").is_ok());
    ix_EXPECT(ix_remove_directory("foo/foo/").is_ok());
    ix_EXPECT(ix_remove_directory("foo/").is_ok());

    temp_path = ix_temp_filename();
    g(temp_path);
    ix_EXPECT(ix_remove_file(temp_path).is_ok());
}

const char *ix_temp_file_dirname()
{
#if ix_PLATFORM(WIN)
    static char path_utf8[ix_MAX_PATH] = {};

    const bool already_calcuated = path_utf8[0] != '\0';
    if (ix_LIKELY(already_calcuated))
    {
        return path_utf8;
    }

    wchar_t path_wchar[ix_MAX_PATH];

    const DWORD ret = GetTempPath(ix_LENGTH_OF(path_wchar), path_wchar);
#if ix_MEASURE_COVERAGE
    ix_UNUSED(ret);
#else
    if (ret == 0)
    {
        ix_log_debug("GetTempPath() failed.\n");
        return nullptr;
    }
#endif

    wchar_path_to_utf8(path_wchar, path_utf8);

    return path_utf8;
#else
    return "/tmp/";
#endif
}

ix_TEST_CASE("ix_temp_file_dirname")
{
    ix_EXPECT_EQSTR(ix_temp_file_dirname(), ix_temp_file_dirname());
    ix_EXPECT_EQSTR(ix_temp_file_dirname(), ix_temp_file_dirname());
    ix_EXPECT_EQSTR(ix_temp_file_dirname(), ix_temp_file_dirname());
    ix_EXPECT_EQSTR(ix_temp_file_dirname(), ix_temp_file_dirname());
    ix_EXPECT_EQSTR(ix_temp_file_dirname(), ix_temp_file_dirname());
}

size_t ix_temp_file_dirname_length()
{
#if ix_PLATFORM(WIN)
    static const size_t length = ix_strlen(ix_temp_file_dirname());
    return length;
#else
    return ix_strlen("/tmp/");
#endif
}

ix_TEST_CASE("ix_temp_file_dirname_length")
{
    ix_EXPECT(ix_temp_file_dirname_length() > 0);
    ix_EXPECT(ix_temp_file_dirname_length() == ix_temp_file_dirname_length());
    ix_EXPECT(ix_temp_file_dirname()[ix_temp_file_dirname_length()] == '\0');
    ix_EXPECT(ix_temp_file_dirname()[ix_temp_file_dirname_length()] == '\0');
    ix_EXPECT(ix_temp_file_dirname()[ix_temp_file_dirname_length()] == '\0');
}

const char *ix_temp_filename(const char *prefix)
{
    static char buf[ix_MAX_PATH] = {};
    static size_t temp_dirname_length = 0;

    const bool first_call = (buf[0] == '\0');
    if (ix_UNLIKELY(first_call))
    {
        const char *temp_file_dirname = ix_temp_file_dirname();
        temp_dirname_length = ix_strlen(temp_file_dirname);
        ix_memcpy(buf, temp_file_dirname, temp_dirname_length);
    }

    const size_t prefix_length = ix_strlen(prefix);

    const size_t temp_filename_length = temp_dirname_length + prefix_length + ix_TEMP_FILENAME_RANDOM_PART_LENGTH;
#if ix_MEASURE_COVERAGE
    ix_UNUSED(temp_filename_length);
#else
    if (temp_filename_length >= ix_MAX_PATH)
    {
        ix_log_debug("ix_temp_filename: too long prefix.\n");
        return nullptr;
    }
#endif

    ix_memcpy(buf + temp_dirname_length, prefix, prefix_length);
    ix_rand_fill_alphanumeric(buf + temp_dirname_length + prefix_length, ix_TEMP_FILENAME_RANDOM_PART_LENGTH);

    return buf;
}

ix_TEST_CASE("ix_temp_filename")
{
    for (size_t i = 0; i < 10; i++)
    {
        const ix_DumbString a(ix_temp_filename());
        const ix_DumbString b(ix_temp_filename());
        ix_EXPECT(a != b);
    }

    for (size_t i = 0; i < 10; i++)
    {
        const ix_DumbString a(ix_temp_filename("foo_"));
        const ix_DumbString b(ix_temp_filename("foo_"));
        ix_EXPECT(a != b);
    }

    for (size_t i = 0; i < 10; i++)
    {
        const ix_DumbString a(ix_temp_filename("一時ファイル_"));
        const ix_DumbString b(ix_temp_filename("一時ファイル_"));
        ix_EXPECT(a != b);
    }
}

size_t ix_temp_filename_length(size_t prefix_length)
{
    return ix_temp_file_dirname_length() + prefix_length + ix_TEMP_FILENAME_RANDOM_PART_LENGTH;
}

ix_TEST_CASE("ix_temp_filename_length")
{
    ix_EXPECT(ix_temp_filename_length() > 0);
    ix_EXPECT(ix_temp_filename_length() == ix_temp_filename_length());
    ix_EXPECT(ix_temp_filename()[ix_temp_filename_length()] == '\0');
    ix_EXPECT(ix_temp_filename()[ix_temp_filename_length()] == '\0');

    const char *prefix = "temporary_";
    const size_t prefix_length = ix_strlen(prefix);
    ix_EXPECT(ix_temp_filename_length(prefix_length) > 0);
    ix_EXPECT(ix_temp_filename_length(prefix_length) == ix_temp_filename_length(prefix_length));
    ix_EXPECT(ix_temp_filename(prefix)[ix_temp_filename_length(prefix_length)] == '\0');
    ix_EXPECT(ix_temp_filename(prefix)[ix_temp_filename_length(prefix_length)] == '\0');
}

ix_FileHandle ix_FileHandle::null(ix_FileOpenOption option)
{
#if ix_PLATFORM(WIN)
    return ix_FileHandle("NUL", option);
#else
    return ix_FileHandle("/dev/null", option);
#endif
}

ix_TEST_CASE("ix_FileHandle: null")
{
    const ix_FileHandle null_read = ix_FileHandle::null(ix_READ_ONLY);
    size_t bytes_read = 0;
    null_read.read_all(&bytes_read);
    ix_EXPECT(bytes_read == 0);

    const ix_FileHandle null_write = ix_FileHandle::null(ix_WRITE_ONLY);
    const char *message = "hello null!";
    const size_t bytes_written = null_write.write_string(message);
    ix_EXPECT(bytes_written == ix_strlen(message));
}

ix_FileHandle::ix_FileHandle()
{
    invalidate();
}

ix_FileHandle::ix_FileHandle(const char *path, ix_FileOpenOption option)
{
    invalidate();

#if ix_PLATFORM(WIN)
    wchar_t path_wchar[ix_MAX_PATH];
    utf8_path_to_wchar(path, path_wchar);
#endif

    switch (option)
    {
    case ix_READ_ONLY: {
#if ix_PLATFORM(WIN)
        v.handle = CreateFile(path_wchar, GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING,
                              FILE_ATTRIBUTE_NORMAL, nullptr);
#else
        v.fd = open(path, O_RDONLY);
#endif
        break;
    }
    case ix_WRITE_ONLY: {
#if ix_PLATFORM(WIN)
        v.handle = CreateFile(path_wchar, GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
#else
        v.fd = open(path, O_WRONLY | O_CREAT | O_TRUNC, 0666);
#endif
        break;
    }
        ix_CASE_EXHAUSTED();
    }
}

ix_FileHandle::ix_FileHandle(ix_FileHandle &&other) noexcept
    : v(other.v)
{
    other.invalidate();
}

ix_FileHandle &ix_FileHandle::operator=(ix_FileHandle &&other) noexcept
{
    if (is_valid())
    {
        close();
    }

    v = other.v;
    other.invalidate();

    return *this;
}

ix_FileHandle::~ix_FileHandle()
{
    if (is_valid())
    {
        close();
    }
}

void ix_FileHandle::invalidate()
{
#if ix_PLATFORM(WIN)
    v.handle = INVALID_HANDLE_VALUE;
#else
    v.fd = -1;
#endif
}

void ix_FileHandle::close()
{
#if ix_PLATFORM(WIN)
    const BOOL ret = CloseHandle(v.handle);
    ix_ASSERT_FATAL(ret != 0);
#else
    int ret = ::close(v.fd);
    ix_ASSERT_FATAL(ret == 0);
#endif
    invalidate();
}

ix_TEST_CASE("ix_FileHandle: close")
{
    mount_cwd();
    ix_FileHandle file = ix_FileHandle(CWD "ix_file.cpp", ix_READ_ONLY);
    ix_EXPECT(file.is_valid());
    file.close();
    ix_EXPECT(!file.is_valid());
    unmount_cwd();
}

bool ix_FileHandle::is_valid() const
{
#if ix_PLATFORM(WIN)
    return (v.handle != INVALID_HANDLE_VALUE);
#else
    return (v.fd != -1);
#endif
}

ix_TEST_CASE("ix_FileHandle: is_valid")
{
    mount_cwd();
    ix_FileHandle handle;
    ix_EXPECT(!handle.is_valid());
    handle = ix_FileHandle(CWD "ix_file.cpp", ix_READ_ONLY);
    ix_EXPECT(handle.is_valid());
    handle = ix_FileHandle("foo", ix_READ_ONLY);
    ix_EXPECT(!handle.is_valid());
    unmount_cwd();
}

size_t ix_FileHandle::write(const void *data, size_t data_length) const
{
#if ix_PLATFORM(WIN)
    size_t total_bytes_written = 0;
    while (data_length > 0)
    {
        DWORD bytes_written;
        // For some reason, this makes the write much faster (see benchmark_io).
        const DWORD write_amount = ix_min(static_cast<DWORD>(data_length), 1024UL * 1024UL);
        const BOOL ret = WriteFile(v.handle, data, write_amount, &bytes_written, nullptr);
        const bool fail = (ret == 0);
        if (fail)
        {
            return SIZE_MAX;
        }
        data = static_cast<const uint8_t *>(data) + bytes_written;
        data_length -= bytes_written;
        total_bytes_written += bytes_written;
    }
    return total_bytes_written;
#else
    size_t total_bytes_written = 0;
    while (data_length > 0)
    {
        const ssize_t bytes_written = ::write(v.fd, data, static_cast<unsigned int>(data_length));
        const bool fail = (bytes_written == -1);
        if (fail)
        {
            return ix_SIZE_MAX;
        }
        data = static_cast<const uint8_t *>(data) + bytes_written;
        const size_t bytes_written_unsigned = static_cast<size_t>(bytes_written);
        data_length -= bytes_written_unsigned;
        total_bytes_written += bytes_written_unsigned;
    }
    return total_bytes_written;
#endif
}

size_t ix_FileHandle::write_string(const char *str) const
{
    return write(str, ix_strlen(str));
}

size_t ix_FileHandle::write_char(char c) const
{
    return write(&c, 1);
}

ix_TEST_CASE("ix_FileHandle: write_char")
{
    ix_TempFileW w;
    w.file_handle().write_char('a');
    w.file_handle().write_char('b');
    w.file_handle().write_char('c');
    ix_EXPECT_EQSTR(w.data(), "abc");
}

ix_PRINTF_FORMAT(2, 0) void ix_FileHandle::write_stringfv(ix_FORMAT_ARG const char *format, va_list args) const
{
    va_list args_copy;
    va_copy(args_copy, args); // Copy must occur here.

    const int required_length_s = ix_vsnprintf(nullptr, 0, format, args) + 1;
    ix_ASSERT(required_length_s > 0);
    const size_t required_length = static_cast<size_t>(required_length_s);

    char buffer[ix_OPT_LEVEL(DEBUG) ? 32 : 1024];
    char *formatted = buffer;
    if (required_length > ix_LENGTH_OF(buffer))
    {
        formatted = ix_MALLOC(char *, required_length);
    }

    const int formatted_length_signed = ix_vsnprintf(formatted, required_length, format, args_copy);
    ix_ASSERT(formatted_length_signed >= 0);

    const size_t formatted_length = static_cast<size_t>(formatted_length_signed);
    write(formatted, formatted_length);

    const bool data_allocated = (formatted != buffer);
    if (data_allocated)
    {
        ix_FREE(formatted);
    }

    va_end(args_copy);
}

ix_PRINTF_FORMAT(2, 3) void ix_FileHandle::write_stringf(ix_FORMAT_ARG const char *format, ...) const
{
    va_list args;
    va_start(args, format);
    write_stringfv(format, args);
    va_end(args);
}

ix_TEST_CASE("ix_FileHandle: write_stringf")
{
    // empty
    {
        const char *filename = ix_temp_filename();
        ix_FileHandle file(filename, ix_WRITE_ONLY);

        ix_DISABLE_GCC_WARNING_BEGIN
        ix_DISABLE_GCC_WARNING("-Wformat-zero-length")
        file.write_stringf("");
        ix_DISABLE_GCC_WARNING_END
        file.close();
        ix_UniquePointer<char[]> output = ix_load_file(filename);
        ix_EXPECT_EQSTR(output.get(), "");
        ix_EXPECT(ix_remove_file(filename).is_ok());
    }

    // literal
    {
        const char *filename = ix_temp_filename();
        ix_FileHandle file(filename, ix_WRITE_ONLY);
        file.write_stringf("hello world!");
        file.close();
        ix_UniquePointer<char[]> output = ix_load_file(filename);
        ix_EXPECT_EQSTR(output.get(), "hello world!");
        ix_EXPECT(ix_remove_file(filename).is_ok());
    }

    // format string
    {
        const char *filename = ix_temp_filename();
        ix_FileHandle file(filename, ix_WRITE_ONLY);
        file.write_stringf("%s %s!", "hello", "world");
        file.close();
        ix_UniquePointer<char[]> output = ix_load_file(filename);
        ix_EXPECT_EQSTR(output.get(), "hello world!");
        ix_EXPECT(ix_remove_file(filename).is_ok());
    }

    // long format string
    {
        const char *filename = ix_temp_filename();
        ix_FileHandle file(filename, ix_WRITE_ONLY);
        const int x = 1 << 15;
        file.write_stringf("%d%d%d%d%d%d%d%d", x, x, x, x, x, x, x, x);
        file.close();
        ix_UniquePointer<char[]> output = ix_load_file(filename);
        ix_EXPECT_EQSTR(output.get(), "3276832768327683276832768327683276832768");
        ix_EXPECT(ix_remove_file(filename).is_ok());
    }
}

size_t ix_FileHandle::read(void *buffer, size_t buffer_length) const
{
#if ix_PLATFORM(WIN)
    size_t total_bytes_read = 0;
    while (buffer_length > 0)
    {
        DWORD bytes_read;
        const BOOL ret = ReadFile(v.handle, buffer, static_cast<DWORD>(buffer_length), &bytes_read, nullptr);
        const bool fail = (ret == 0);
        if (fail)
        {
            const DWORD err = GetLastError();
            return (err == ERROR_BROKEN_PIPE) ? total_bytes_read : ix_SIZE_MAX;
        }
        ix_ASSERT_FATAL(ret != 0);
        const bool eof = (bytes_read == 0);
        if (eof)
        {
            break;
        }
        buffer = static_cast<char *>(buffer) + bytes_read;
        const size_t bytes_read_unsigned = static_cast<size_t>(bytes_read);
        buffer_length -= bytes_read_unsigned;
        total_bytes_read += bytes_read_unsigned;
    }

    return total_bytes_read;
#else
    if (v.fd == -1)
    {
        return ix_SIZE_MAX;
    }

    size_t total_bytes_read = 0;
    while (buffer_length > 0)
    {
        const ssize_t bytes_read = ::read(v.fd, buffer, buffer_length);
        const bool fail = (bytes_read == -1);
        if (fail)
        {
            return ix_SIZE_MAX;
        }
        const bool eof = (bytes_read == 0);
        if (eof)
        {
            break;
        }
        buffer = static_cast<char *>(buffer) + bytes_read;
        const size_t bytes_read_unsigned = static_cast<size_t>(bytes_read);
        buffer_length -= bytes_read_unsigned;
        total_bytes_read += bytes_read_unsigned;
    }

    return total_bytes_read;
#endif
}

ix_TEST_CASE("ix_FileHandle: read/write")
{
    const char *path = ix_temp_filename();
    const char *message = "hello world!\n";

    ix_FileHandle file = ix_FileHandle(path, ix_WRITE_ONLY);
    ix_EXPECT(file.is_valid());
    const size_t bytes_written = file.write_string(message);
    ix_EXPECT(bytes_written == ix_strlen(message));
    file.close();

    ix_EXPECT(ix_is_file(path));
    file = ix_FileHandle(path, ix_READ_ONLY);
    ix_EXPECT(file.is_valid());

    char buf[128];
    const size_t bytes_read = file.read(buf, sizeof(buf));
    file.close();
    ix_EXPECT(bytes_read == ix_strlen(message));
    buf[bytes_read] = '\0';
    ix_EXPECT_EQSTR(buf, message);
    ix_EXPECT(ix_remove_file(path).is_ok());
}

ix_TEST_CASE("ix_FileHandle: read/write with a mutibyte filename")
{
    const char *filename = ix_temp_filename("一時ファイル_");
    const char *message = "こんにちは、世界！";
    ix_EXPECT(ix_write_string_to_file(filename, message).is_ok());
    ix_FileHandle file(filename, ix_READ_ONLY);
    ix_EXPECT(file.is_valid());
    ix_UniquePointer<char[]> str = file.read_all(nullptr);
    ix_EXPECT_EQSTR(str.get(), message);
    file.close();
    ix_EXPECT(ix_remove_file(filename).is_ok());
}

ix_UniquePointer<char[]> ix_FileHandle::read_all(size_t *size_out, size_t first_read_length) const
{
    const size_t file_size = size();
    if ((file_size != ix_SIZE_MAX) && ix_DEBUG_RANDOM_BRANCH(0.5))
    {
        char *data = ix_MALLOC(char *, file_size + 1);
        const size_t bytes_written = read(data, file_size);
        if (bytes_written != file_size)
        {
            ix_FREE(data);
            return ix_UniquePointer<char[]>(nullptr);
        }
        data[file_size] = '\0';
        if (size_out != nullptr)
        {
            *size_out = file_size;
        }
        return ix_UniquePointer<char[]>(ix_move(data));
    }

    char *data = nullptr;
    size_t data_size = 0;
    size_t read_size = first_read_length;
    do // do-while is used to make cppcheck happy
    {
        data = ix_REALLOC(char *, data, data_size + read_size + 1); // 1 for the last '\0'

        const size_t bytes_read = read(data + data_size, read_size);
        if (bytes_read == ix_SIZE_MAX)
        {
            ix_FREE(data);
            return ix_UniquePointer<char[]>(nullptr);
        }

        data_size += bytes_read;

        const bool consumed = (bytes_read == 0);
        if (consumed)
        {
            break;
        }

        if (bytes_read == read_size)
        {
            read_size *= 2;
        }

    } while (true);

    data[data_size] = '\0';

    if (size_out != nullptr)
    {
        *size_out = data_size;
    }

    return ix_UniquePointer<char[]>(ix_move(data));
}

ix_TEST_CASE("ix_read_all")
{
    // without size
    {
        const char *filename = ix_temp_filename();
        const char *message = "hello world!";
        ix_EXPECT(ix_write_string_to_file(filename, message).is_ok());
        ix_FileHandle file(filename, ix_READ_ONLY);
        ix_UniquePointer<char[]> str = file.read_all(nullptr);
        file.close();
        ix_EXPECT(str.get() != nullptr);
        ix_EXPECT_EQSTR(str.get(), "hello world!");
        ix_EXPECT(ix_remove_file(filename).is_ok());
    }

    // with size
    {
        const char *filename = ix_temp_filename();
        const char *message = "hello world!";
        ix_EXPECT(ix_write_string_to_file(filename, message).is_ok());
        ix_FileHandle file(filename, ix_READ_ONLY);
        size_t size = 0;
        const ix_UniquePointer<char[]> str = file.read_all(&size);
        file.close();
        ix_EXPECT(str.get() != nullptr);
        ix_EXPECT_EQSTR(str.get(), "hello world!");
        ix_EXPECT(str[size] == '\0');
        ix_EXPECT(size == 12);
        ix_EXPECT(ix_remove_file(filename).is_ok());
    }

    // mid-sized file
    {
        char message[2048 + 1];
        ix_rand_fill_alphanumeric(message, ix_LENGTH_OF(message) - 1);

        const char *filename = ix_temp_filename();
        ix_EXPECT(ix_write_string_to_file(filename, message).is_ok());

        ix_FileHandle file(filename, ix_READ_ONLY);
        const ix_UniquePointer<char[]> p = file.read_all(nullptr);
        file.close();

        ix_EXPECT(ix_remove_file(filename).is_ok());
    }
}

size_t ix_FileHandle::size() const
{
#if ix_PLATFORM(WIN)
    LARGE_INTEGER file_size;
    const BOOL ret = GetFileSizeEx(v.handle, &file_size);
    if (ret == 0)
    {
        return ix_SIZE_MAX;
    }
    return static_cast<size_t>(file_size.QuadPart);
#else
    const off_t curr = lseek(v.fd, 0, SEEK_CUR);
    if (curr == off_t{-1})
    {
        return ix_SIZE_MAX;
    }

    const off_t file_size = lseek(v.fd, 0, SEEK_END);
    if (file_size == off_t{-1})
    {
        return ix_SIZE_MAX;
    }

    const off_t ret = lseek(v.fd, curr, SEEK_SET);
    if (ret == off_t{-1})
    {
        return ix_SIZE_MAX;
    }

    return static_cast<size_t>(file_size);
#endif
}

ix_TEST_CASE("ix_FileHandle: size")
{
    // zero
    {
        const char *filename = ix_temp_filename();
        const char *message = "";
        ix_EXPECT(ix_write_string_to_file(filename, message).is_ok());
        ix_FileHandle file = ix_FileHandle(filename, ix_READ_ONLY);
        ix_EXPECT(file.is_valid());
        ix_EXPECT(file.size() == 0);
        file.close();
        ix_EXPECT(ix_remove_file(filename).is_ok());
    }

    // 11 bytes
    {
        const char *filename = ix_temp_filename();
        const char *message = "hello world";
        ix_EXPECT(ix_write_string_to_file(filename, message).is_ok());
        ix_FileHandle file = ix_FileHandle(filename, ix_READ_ONLY);
        ix_EXPECT(file.size() == 11);
        file.close();
        ix_EXPECT(ix_remove_file(filename).is_ok());
    }
}

const ix_FileHandle &ix_FileHandle::of_stdin()
{
    ix_ASSERT(g_filehandle_stdin.get().is_valid());
    return g_filehandle_stdin.get();
}

const ix_FileHandle &ix_FileHandle::of_stdout()
{
    ix_ASSERT(g_filehandle_stdin.get().is_valid());
    return g_filehandle_stdout.get();
}

const ix_FileHandle &ix_FileHandle::of_stderr()
{
    ix_ASSERT(g_filehandle_stdin.get().is_valid());
    return g_filehandle_stderr.get();
}

ix_TEST_CASE("ix_FileHandle: stdio")
{
    ix_EXPECT(ix_FileHandle::of_stdin().is_valid());
    ix_EXPECT(ix_FileHandle::of_stdout().is_valid());
    ix_EXPECT(ix_FileHandle::of_stderr().is_valid());
}
