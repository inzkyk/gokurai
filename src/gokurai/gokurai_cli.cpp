#include "gokurai.hpp"

#include <ix.hpp>
#include <ix_Buffer.hpp>
#include <ix_CmdArgsEater.hpp>
#include <ix_SystemManager.hpp>
#include <ix_TempFile.hpp>
#include <ix_assert.hpp>
#include <ix_defer.hpp>
#include <ix_doctest.hpp>
#include <ix_file.hpp>
#include <ix_memory.hpp>
#include <ix_string.hpp>

#include <fcntl.h>

static constexpr const char *HELP_TEXT = R"(
gokurai version 1.0.0

USAGE:
  gokurai [OPTIONS] [FILE_NAME...]

OPTIONS:
  -h, --help: Show help.

)";

static constexpr const char *ERROR_TEXT_INVALID_INVOCATION = "Invalid invocation: no cmd args provided!\n";
static constexpr const char *ERROR_TEXT_STDIN_LOAD_FAILED = "Failed to read from stdin.\n";
static constexpr const char *ERROR_TEXT_FILE_NOT_FOUND = "File not found: %s\n";
static constexpr const char *ERROR_TEXT_FILE_LOAD_FAILED = "File load failed: %s\n";

static int gokurai_main(const ix_FileHandle &stdin_handle, const ix_FileHandle &stdout_handle,
                        const ix_FileHandle &stderr_handle, ix_CmdArgsEater args)
{
    if (args.size() == 0)
    {
        stderr_handle.write_string(ERROR_TEXT_INVALID_INVOCATION);
        return 1;
    }

    const bool show_help = args.eat_boolean({"-h", "--help"});
    if (show_help)
    {
        stdout_handle.write_string(HELP_TEXT);
        return 0;
    }

#if ix_DO_TEST
    const bool do_test = args.eat_boolean("--test");
    if (do_test)
    {
        ix_do_doctest(static_cast<int>(args.size()), &args[0]);
        return 0;
    }
#endif

    const bool quiet = args.eat_boolean({"-q", "--quiet"});

    const bool read_from_stdin = (args.size() == 1);
    ix_Buffer input_buffer(4096);
    if (read_from_stdin)
    {
        const size_t bytes_read = input_buffer.load_file_handle(stdin_handle);
        if (bytes_read == ix_SIZE_MAX)
        {
            stderr_handle.write_string(ERROR_TEXT_STDIN_LOAD_FAILED);
            return 1;
        }
        input_buffer.push_char('\0');
    }
    else
    {
        const size_t num_args = args.size();
        for (size_t i = 1; i < num_args; i++)
        {
            const char *filename = args[i];
            if (ix_strcmp(filename, "-") == 0)
            {
                const size_t bytes_read = input_buffer.load_file_handle(stdin_handle);
                if (bytes_read == ix_SIZE_MAX)
                {
                    stderr_handle.write_string(ERROR_TEXT_STDIN_LOAD_FAILED);
                    return 1;
                }
            }
            else
            {
                const ix_FileHandle file(filename, ix_READ_ONLY);
                if (!file.is_valid())
                {
                    stderr_handle.write_stringf(ERROR_TEXT_FILE_NOT_FOUND, filename);
                    return 1;
                }
                const size_t bytes_read = input_buffer.load_file_handle(file);
                if (bytes_read == ix_SIZE_MAX)
                {
                    stderr_handle.write_stringf(ERROR_TEXT_FILE_LOAD_FAILED, filename);
                    return 1;
                }
            }
        }

        input_buffer.push_char('\0');
    }

    ix_ASSERT(!input_buffer.empty());
    const size_t input_length = input_buffer.size() - 1;
    ix_UniquePointer<char[]> input = input_buffer.detach();

    GokuraiContext ctx = gokurai_context_create(quiet ? nullptr : &stdout_handle, &stderr_handle);
    gokurai_context_feed_input(ctx, input.get(), input_length);
    gokurai_context_end_input(ctx, nullptr);
    gokurai_context_destroy(ctx);

    return 0;
}

ix_TEST_CASE("gokurai: CUI")
{
    const ix_FileHandle null = ix_FileHandle::null();

    { // Invalid invocation.
        const ix_TempFileR in(nullptr);
        ix_TempFileW out;
        ix_TempFileW err;
        gokurai_main(in.file_handle(), out.file_handle(), err.file_handle(), {});
        ix_EXPECT_EQSTR(out.data(), "");
        ix_EXPECT_EQSTR(err.data(), ERROR_TEXT_INVALID_INVOCATION);
    }

    { // Show help.
        const ix_TempFileR in(nullptr);
        ix_TempFileW out;
        ix_TempFileW err;
        gokurai_main(in.file_handle(), out.file_handle(), err.file_handle(), {"gokurai", "-h"});
        ix_EXPECT_EQSTR(out.data(), HELP_TEXT);
        ix_EXPECT_EQSTR(err.data(), "");
    }

    { // Show help (long).
        ix_TempFileW out;
        ix_TempFileW err;
        const ix_TempFileR in(nullptr);
        gokurai_main(in.file_handle(), out.file_handle(), err.file_handle(), {"gokurai", "--help"});
        ix_EXPECT_EQSTR(out.data(), HELP_TEXT);
        ix_EXPECT_EQSTR(err.data(), "");
    }

    { // Read from stdin (with newline).
        ix_TempFileW out;
        ix_TempFileW err;
        const ix_TempFileR in("hello world\n");
        gokurai_main(in.file_handle(), out.file_handle(), err.file_handle(), {"gokurai"});
        ix_EXPECT_EQSTR(out.data(), "hello world\n");
        ix_EXPECT_EQSTR(err.data(), "");
    }

    { // Read from stdin (without the last newline).
        ix_TempFileW out;
        ix_TempFileW err;
        const ix_TempFileR in("hello world");
        gokurai_main(in.file_handle(), out.file_handle(), err.file_handle(), {"gokurai"});
        ix_EXPECT_EQSTR(out.data(), "hello world");
        ix_EXPECT_EQSTR(err.data(), "");
    }

    { // Read from stdin quietly (-q).
        ix_TempFileW out;
        ix_TempFileW err;
        const ix_TempFileR in("hello world\n");
        gokurai_main(in.file_handle(), out.file_handle(), err.file_handle(), {"gokurai", "-q"});
        ix_EXPECT_EQSTR(out.data(), "");
        ix_EXPECT_EQSTR(err.data(), "");
    }

    { // Read from stdin quietly (--quiet).
        ix_TempFileW out;
        ix_TempFileW err;
        const ix_TempFileR in("hello world\n");
        gokurai_main(in.file_handle(), out.file_handle(), err.file_handle(), {"gokurai", "--quiet"});
        ix_EXPECT_EQSTR(out.data(), "");
        ix_EXPECT_EQSTR(err.data(), "");
    }

    { // Read from a file.
        ix_TempFileW out;
        ix_TempFileW err;
        const ix_TempFileR in("hello world\n");
        gokurai_main(null, out.file_handle(), err.file_handle(), {"gokurai", in.filename()});
        ix_EXPECT_EQSTR(out.data(), "hello world\n");
        ix_EXPECT_EQSTR(err.data(), "");
    }

    { // Read from multiple files.
        ix_TempFileW out;
        ix_TempFileW err;
        const ix_TempFileR foo("foo\n");
        const ix_TempFileR bar("bar\n");
        const ix_TempFileR foobar("foobar\n");
        gokurai_main(null, out.file_handle(), err.file_handle(),
                     {"gokurai", foo.filename(), bar.filename(), foobar.filename()});
        ix_EXPECT_EQSTR(out.data(), "foo\n"
                                    "bar\n"
                                    "foobar\n");
        ix_EXPECT_EQSTR(err.data(), "");
    }

    { // Read from files and stdin.
        ix_TempFileW out;
        ix_TempFileW err;
        const ix_TempFileR foo("foo\n");
        const ix_TempFileR bar("bar\n");
        const ix_TempFileR foobar("foobar\n");
        gokurai_main(bar.file_handle(), out.file_handle(), err.file_handle(),
                     {"gokurai", foo.filename(), "-", foobar.filename()});
        ix_EXPECT_EQSTR(out.data(), "foo\n"
                                    "bar\n"
                                    "foobar\n");
        ix_EXPECT_EQSTR(err.data(), "");
    }

    { // Read from files and stdin.
        ix_TempFileW out;
        ix_TempFileW err;
        const ix_TempFileR foo("foo\n");
        const ix_TempFileR bar("bar\n");
        const ix_TempFileR foobar("foobar\n");
        gokurai_main(foobar.file_handle(), out.file_handle(), err.file_handle(),
                     {"gokurai", foo.filename(), bar.filename(), "-"});
        ix_EXPECT_EQSTR(out.data(), "foo\n"
                                    "bar\n"
                                    "foobar\n");
        ix_EXPECT_EQSTR(err.data(), "");
    }

    { // Read from files and stdin (quiet).
        const ix_TempFileR foo("foo\n");
        const ix_TempFileR bar("bar\n");
        const ix_TempFileR foobar("foobar\n");
        ix_TempFileW out;
        ix_TempFileW err;
        gokurai_main(foobar.file_handle(), out.file_handle(), err.file_handle(),
                     {"gokurai", foo.filename(), "-q", bar.filename(), "-"});
        ix_EXPECT_EQSTR(out.data(), "");
        ix_EXPECT_EQSTR(err.data(), "");
    }

    { // Read from stdin erroneously.
        ix_TempFileW out;
        ix_TempFileW err;
        gokurai_main(null, out.file_handle(), err.file_handle(), {"gokurai"});
        ix_EXPECT_EQSTR(out.data(), "");
        ix_EXPECT_EQSTR(err.data(), ERROR_TEXT_STDIN_LOAD_FAILED);
    }

    { // Read from stdin erroneously (with "-").
        ix_TempFileW out;
        ix_TempFileW err;
        gokurai_main(null, out.file_handle(), err.file_handle(), {"gokurai", "-"});
        ix_EXPECT_EQSTR(out.data(), "");
        ix_EXPECT_EQSTR(err.data(), ERROR_TEXT_STDIN_LOAD_FAILED);
    }

    { // Erroneous load from non-existent file.
        ix_TempFileW out;
        ix_TempFileW err;
        gokurai_main(null, out.file_handle(), err.file_handle(), {"gokurai", "foo.txt"});
        ix_EXPECT_EQSTR(out.data(), "");
        ix_EXPECT_EQSTR(err.data(), "File not found: foo.txt\n");
    }
}

int main(int argc, const char **argv)
{
    auto &sm = ix_SystemManager::init();
    auto _ = ix_defer(&ix_SystemManager::deinit);
    sm.init_stdio().assert_ok();
    sm.init_sokol_time().assert_ok();
    sm.init_logger().assert_ok();

    const int ret = gokurai_main(ix_FileHandle::of_stdin(), ix_FileHandle::of_stdout(), ix_FileHandle::of_stderr(),
                                 ix_CmdArgsEater(argc, argv));
    return ret;
}
