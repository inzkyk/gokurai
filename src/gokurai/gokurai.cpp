#include "gokurai.hpp"

#include <ix.hpp>
#include <ix_Buffer.hpp>
#include <ix_HashMapSingleArray.hpp>
#include <ix_StringArena.hpp>
#include <ix_StringView.hpp>
#include <ix_TempFile.hpp>
#include <ix_Writer.hpp>
#include <ix_assert.hpp>
#include <ix_doctest.hpp>
#include <ix_file.hpp>
#include <ix_memory.hpp>
#include <ix_printf.hpp>
#include <ix_string.hpp>

#include <inttypes.h>
#include <lauxlib.h>
#include <lua.h>
#include <lualib.h>

// IDEA: Interop between gokurai and lua.

// clang-format off
constexpr ix_StringView GLOBAL_MACRO_HEADER       ("#+MACRO ");
constexpr ix_StringView GLOBAL_BLOCK_MACRO_HEADER ("#+MACRO_BEGIN ");
constexpr ix_StringView GLOBAL_BLOCK_MACRO_FOOTER ("#+MACRO_END\n");
constexpr ix_StringView LOCAL_MACRO_HEADER        ("#+LOCAL_MACRO ");
constexpr ix_StringView LOCAL_BLOCK_MACRO_HEADER  ("#+LOCAL_MACRO_BEGIN ");
constexpr ix_StringView LOCAL_BLOCK_MACRO_FOOTER  ("#+LOCAL_MACRO_END\n");
constexpr ix_StringView LUA_BLOCK_HEADER          ("#+LUA_BEGIN\n");
constexpr ix_StringView LUA_BLOCK_FOOTER          ("#+LUA_END\n");
constexpr ix_StringView COMMENT_BLOCK_HEADER      ("#+COMMENT_BEGIN\n");
constexpr ix_StringView COMMENT_BLOCK_FOOTER      ("#+COMMENT_END\n");
constexpr ix_StringView LUA_RETURN                ("return ");
// clang-format on

static constexpr size_t MAX_NUM_ARGS = 9;

class GokuraiResultImpl
{
    ix_UniquePointer<char[]> m_output = ix_UniquePointer<char[]>(nullptr);
    size_t m_output_length = 0;

  public:
    GokuraiResultImpl() = default;

    ix_FORCE_INLINE const ix_UniquePointer<char[]> &data() const
    {
        return m_output;
    }

    ix_FORCE_INLINE size_t size() const
    {
        return m_output_length;
    }

    ix_FORCE_INLINE void clear()
    {
        m_output.swap(nullptr);
        m_output_length = 0;
    }

    ix_FORCE_INLINE ix_UniquePointer<char[]> detach()
    {
        return ix_move(m_output);
    }

  private:
    ix_FORCE_INLINE GokuraiResultImpl(ix_UniquePointer<char[]> output, size_t output_length)
        : m_output(ix_move(output)),
          m_output_length(output_length)
    {
    }

    friend class GokuraiContextImpl;
};

static void unquote_macro_calls(ix_Buffer &buffer)
{
    char *p = buffer.data();
    size_t size = buffer.size();
    const bool no_quote = (ix_memchr(p, '\'', size) == nullptr);
    if (ix_LIKELY(no_quote))
    {
        return;
    }

    // Code below is not optimal, but should be fine since this path is very cold.
    buffer.push_char('\0'); // To use ix_strstr.
                            // IDEA: Implement ix_strstr_length().
    p = buffer.data();
    const char *end = p + buffer.size();
    char *quote = ix_strstr(p, "'[[[");
    while (quote != nullptr)
    {
        const size_t mvmt = static_cast<size_t>(end - quote) - 1;
        ix_memmove(quote, quote + 1, mvmt);
        quote = ix_strstr(quote, "'[[[");
        size -= 1;
    }

    p = buffer.data();
    quote = ix_strstr(p, "']]]");
    while (quote != nullptr)
    {
        const size_t mvmt = static_cast<size_t>(end - quote) - 1;
        ix_memmove(quote, quote + 1, mvmt);
        quote = ix_strstr(quote, "']]]");
        size -= 1;
    }

    p = buffer.data();
    quote = ix_strstr(p, "'^[[[");
    while (quote != nullptr)
    {
        const size_t mvmt = static_cast<size_t>(end - quote) - 1;
        ix_memmove(quote, quote + 1, mvmt);
        quote = ix_strstr(quote, "'^[[[");
        size -= 1;
    }

    buffer.set_size(size);
}

static void unquote_directive(ix_Buffer &buffer)
{
    char *p = buffer.data();
    const bool no_quote = (*p != '\'');
    if (ix_LIKELY(no_quote))
    {
        return;
    }

    while (*p == '\'')
    {
        p += 1;
    }

    const bool yet_no_quote = (*p != '#');
    if (yet_no_quote)
    {
        return;
    }

    // IDEA: Use Aho-Corasick.
    if (ix_starts_with(p, GLOBAL_MACRO_HEADER.data()) ||       //
        ix_starts_with(p, GLOBAL_BLOCK_MACRO_HEADER.data()) || //
        ix_starts_with(p, GLOBAL_BLOCK_MACRO_FOOTER.data()) || //
        ix_starts_with(p, LOCAL_MACRO_HEADER.data()) ||        //
        ix_starts_with(p, LOCAL_BLOCK_MACRO_HEADER.data()) ||  //
        ix_starts_with(p, LOCAL_BLOCK_MACRO_FOOTER.data()) ||  //
        ix_starts_with(p, LUA_BLOCK_HEADER.data()) ||          //
        ix_starts_with(p, LUA_BLOCK_FOOTER.data()) ||          //
        ix_starts_with(p, COMMENT_BLOCK_HEADER.data()) ||      //
        ix_starts_with(p, COMMENT_BLOCK_FOOTER.data()))        //
    {
        const char *end = buffer.data() + buffer.size();
        const size_t mvmt = static_cast<size_t>(end - p);
        ix_memmove(p - 1, p, mvmt);
        buffer.pop_back(1);
    }
}

static const char *find_first_unmatched_call_end(const char *start, const char *end)
{
    int balance = 0;
    const char *p = start;
    while (p < end)
    {
        const char c = *p;

        const bool normal_char = (c != ']') && (c != '^') && (c != '\'');
        if (ix_LIKELY(normal_char))
        {
            p += 1;
            continue;
        }

        if (c == ']')
        {
            p += 1;
            uint8_t count = 2;
            while ((p < end) && (*p == ']') && (balance != -1))
            {
                count -= 1;
                p += 1;
                if (count == 0)
                {
                    count = 3;
                    balance -= 1;
                }
            }
            const bool unmatched_call_end_found = (balance == -1);
            if (unmatched_call_end_found)
            {
                return p;
            }
            continue;
        }

        if (c == '^')
        {
            p += 1;
            uint8_t count = 3;
            while ((p < end) && (*p == '[') && (0 < count))
            {
                count -= 1;
                p += 1;
            }
            if (count == 0)
            {
                balance += 1;
            }
            continue;
        }

        if (c == '\'')
        {
            p += ix_strlen("'[[[");
            continue;
        }

        ix_UNREACHABLE();
    }

    return nullptr;
}

static void push_macro_argument(ix_Buffer &buffer, const char *start, const char *end)
{
    ix_ASSERT(start <= end);
    const size_t max_size = static_cast<size_t>(end - start);
    buffer.ensure(max_size);

    char *dst = buffer.end();
    const char *dst_original = dst;
    const char *src = start;

    while (src < end)
    {
        const char c = *src;
        if (ix_LIKELY((c != '\\') && (c != '^')))
        {
            *dst = *src;
            src += 1;
            dst += 1;
            continue;
        }

        if (c == '\\')
        {
            size_t num_backslashes = 1;
            const char *s = src + 1;
            while ((s < end) && (*s == '\\'))
            {
                num_backslashes += 1;
                s += 1;
            }

            const bool escaping = (*s == ',');

            if (!escaping)
            {
                ix_memcpy(dst, src, num_backslashes);
                src += num_backslashes;
                dst += num_backslashes;
                continue;
            }

            const size_t num_backslashes_represented = num_backslashes / 2;
            ix_memset(dst, '\\', num_backslashes_represented);
            src += num_backslashes;
            dst += num_backslashes_represented;

            const bool escaping_comma = ((num_backslashes % 2) == 1);
            if (escaping_comma)
            {
                *dst = ',';
                src += 1;
                dst += 1;
            }
            else
            {
                ix_ASSERT(s == end);
            }

            continue;
        }

        ix_ASSERT(c == '^');
        const char *until = src + 1;

        uint8_t count = 0;
        while ((until < end) && (*until == '[') && (count < ix_strlen("[[[")))
        {
            count += 1;
            until += 1;
        }

        const bool lazy_call_starts = (count == ix_strlen("[[["));
        if (lazy_call_starts)
        {
            const char *lazy_call_end = find_first_unmatched_call_end(until, end);
            const bool terminated_properly = (lazy_call_end != nullptr);
            if (terminated_properly)
            {
                until = lazy_call_end;
            }
        }

        while (src < until)
        {
            *dst = *src;
            src += 1;
            dst += 1;
        }
    }

    const size_t size_pushed = static_cast<size_t>(dst - dst_original);
    buffer.add_size(size_pushed);
}

static void parse_args(const char *args[MAX_NUM_ARGS + 1], const char *start, const char *end)
{
    args[0] = start;
    uint8_t current_index = 1;

    const char *p = start;
    while (p < end)
    {
        const char c = *p;

        const bool normal_char = (c != '\\') && (c != ',') && (c != '^');
        if (ix_LIKELY(normal_char))
        {
            p += 1;
            continue;
        }

        if (ix_LIKELY(c == ','))
        {
            if (ix_UNLIKELY(current_index == MAX_NUM_ARGS))
            {
                break;
            }
            args[current_index] = p + ix_strlen(",");
            current_index += 1;
            p += 1;
            continue;
        }

        if (c == '\\')
        {
            size_t num_backslashes = 1;
            p += 1;
            while (*p == '\\')
            {
                num_backslashes += 1;
                p += 1;
            }
            const bool comma_escaped = (*p == ',') && ((num_backslashes % 2) == 1);
            if (comma_escaped)
            {
                p += 1;
            }

            continue;
        }

        ix_ASSERT(c == '^');
        const char *pp = p + 1;
        uint8_t count = 0;
        while ((pp < end) && (*pp == '[') && (count < ix_strlen("[[[")))
        {
            count += 1;
            pp += 1;
        }
        const bool lazy_call_starts = (count == ix_strlen("[[["));
        if (!lazy_call_starts)
        {
            p = pp;
            continue;
        }

        const char *lazy_call_end = find_first_unmatched_call_end(pp, end);

        // All lazy calls within a non-lazy macro calls are balanced.
        ix_ASSERT(lazy_call_end != nullptr);

        p = lazy_call_end;
    }

    const char *sentinel = end + ix_strlen(",");
    args[current_index] = sentinel;
    args[MAX_NUM_ARGS] = sentinel;
}

ix_FORCE_INLINE static bool is_digit(char c)
{
    return static_cast<unsigned char>(c - '0') < 10;
}

static void expand_call_with_args(const char *args[MAX_NUM_ARGS + 1], const char *str, size_t str_length,
                                  ix_Buffer &buffer)
{
    const char *next_write_start = str;
    const char *p = str;
    const char *end = str + str_length;
    while (p != end)
    {
        char c = *p;

        const bool normal_char = (c != '$');
        if (ix_LIKELY(normal_char))
        {
            p += 1;
            continue;
        }

        p += 1;
        c = *p;
        if (!is_digit(c))
        {
            continue;
        }

        buffer.push_between(next_write_start, p - 1);
        next_write_start = p + 1;
        const uint8_t arg_index = static_cast<uint8_t>(c - '0');
        if (arg_index == 0)
        {
            const char *from = args[0];
            const char *until = args[MAX_NUM_ARGS] - ix_strlen(",");
            buffer.push_between(from, until);
        }
        else if (args[arg_index] != nullptr)
        {
            const char *from = args[arg_index - 1];
            const char *until = args[arg_index] - ix_strlen(",");
            push_macro_argument(buffer, from, until);
        }
        p += 1;
    }

    const char *macro_body_end = str + str_length;
    buffer.push_between(next_write_start, macro_body_end);
}

static int eval_lua_program(lua_State *L, const char *program, size_t program_len, const ix_FileHandle *err_out)
{
    const bool print_warnings = (err_out != nullptr);
    int result;

    result = luaL_loadbuffer(L, program, program_len, program);
    if (result != LUA_OK)
    {
        if (print_warnings)
        {
            err_out->write_stringf("Lua load failed: %s\n", lua_tostring(L, -1));
        }
        return result;
    }

    const int num_args = 0;
    const int num_results = 1;
    const int msgh = 0;
    result = lua_pcall(L, num_args, num_results, msgh);
    if (result != LUA_OK)
    {
        if (print_warnings)
        {
            err_out->write_stringf("Lua evaluation failed: %s\n", lua_tostring(L, -1));
        }
    }
    return result;
}

struct Macro
{
    size_t body_length;
    size_t first_line_length;
    const char *body;

    ix_FORCE_INLINE bool is_oneline() const
    {
        return (first_line_length == 0);
    }
};

struct MacroCall
{
    char *start;
    const char *end;
    size_t head_length;
    size_t tail_length;
    uint8_t offset; // 1 if this macro is lazy, 0 otherwise.

    ix_FORCE_INLINE bool is_lazy() const
    {
        return (offset == 1);
    }

    ix_FORCE_INLINE size_t length() const
    {
        return static_cast<size_t>(end - start);
    }
};

class GokuraiContextImpl
{
    const char *m_input;
    size_t m_input_length;
    size_t m_input_remaining;
    const ix_FileHandle *m_err_handle;

    bool m_lua_enabled;
    bool m_current_line_has_lazy_call;
    bool m_redo_macro_expansion;
    bool m_clear_local_macro_on_next_read;
    uint64_t m_current_input_line_number;
    uint64_t m_current_output_line_number;
    ix_Writer m_output_writer;
    ix_Buffer m_line_buffer;
    ix_Buffer m_block_buffer;
    ix_Buffer m_temp_buffer;
    ix_Buffer m_secondary_input_buffer;
    size_t m_secondary_input_offset;
    ix_StringArena m_global_string_arena;
    ix_StringArena m_local_string_arena;
    ix_HashMapSingleArray<ix_StringView, Macro> m_global_macros;
    ix_HashMapSingleArray<ix_StringView, Macro> m_local_macros;
    lua_State *m_lua_state;

  public:
    GokuraiContextImpl(const GokuraiContextImpl &) = delete;
    GokuraiContextImpl(GokuraiContextImpl &&) = delete;
    GokuraiContextImpl &operator=(const GokuraiContextImpl &) = delete;
    GokuraiContextImpl &operator=(GokuraiContextImpl &&) = delete;

    ~GokuraiContextImpl()
    {
        m_output_writer.flush();

        if (m_lua_state != nullptr)
        {
            lua_close(m_lua_state);
        }
    }

    GokuraiContextImpl(const ix_FileHandle *out_handle, const ix_FileHandle *err_handle)
        : m_input(nullptr),
          m_input_length(0),
          m_input_remaining(0),
          m_err_handle(err_handle),
          m_lua_enabled(true),
          m_clear_local_macro_on_next_read(false),
          m_current_input_line_number(0),
          m_current_output_line_number(1),
          m_output_writer(8192, out_handle),
          m_line_buffer(ix_OPT_LEVEL(DEBUG) ? 1 : 1024),
          m_block_buffer(ix_OPT_LEVEL(DEBUG) ? 1 : 1024),
          m_temp_buffer(ix_OPT_LEVEL(DEBUG) ? 1 : 1024),
          m_secondary_input_buffer(ix_OPT_LEVEL(DEBUG) ? 1 : 1024),
          m_secondary_input_offset(0),
          m_global_string_arena(ix_OPT_LEVEL(DEBUG) ? 1 : 4096),
          m_local_string_arena(ix_OPT_LEVEL(DEBUG) ? 1 : 1024),
          m_lua_state(nullptr)
    {
    }

    void clear()
    {
        m_output_writer.flush();

        m_input = nullptr;
        m_input_length = 0;
        m_input_remaining = 0;

        m_lua_enabled = true;
        m_current_line_has_lazy_call = false;
        m_redo_macro_expansion = false;
        m_clear_local_macro_on_next_read = false;

        m_current_input_line_number = 0;
        m_current_output_line_number = 1;
        m_output_writer.clear();
        m_line_buffer.clear();
        m_block_buffer.clear();
        m_temp_buffer.clear();
        m_secondary_input_buffer.clear();
        m_secondary_input_offset = 0;
        m_global_string_arena.clear();
        m_local_string_arena.clear();
        m_global_macros.clear();
        m_local_macros.clear();
        if (m_lua_state != nullptr)
        {
            lua_close(m_lua_state);
            m_lua_state = nullptr;
        }
    }

    void feed_input(const char *input, size_t input_length)
    {
        m_input = input;
        m_input_length = input_length;
        m_input_remaining = input_length;

        if (m_output_writer.buffer_capacity() < input_length)
        {
            m_output_writer.reserve_buffer_capacity(input_length);
        }

        if (ix_UNLIKELY(input_length == 0))
        {
            return;
        }

        const bool input_ends_with_newline = (input[input_length - 1] == '\n');

        // The main loop.
        while (true)
        {
            m_line_buffer.clear();
            load_next_line(true);

            if (m_line_buffer.empty())
            {
                break;
            }

            m_clear_local_macro_on_next_read = true;
            bool directive_found;
            do
            {
                m_redo_macro_expansion = false;
                expand_non_lazy_macros();

                directive_found = find_and_process_directive();
                if (directive_found)
                {
                    break;
                }

                if (m_current_line_has_lazy_call)
                {
                    expand_lazy_macros();
                }

                directive_found = find_and_process_directive();
                if (directive_found)
                {
                    break;
                }

            } while (m_redo_macro_expansion);

            if (directive_found)
            {
                continue;
            }

            unquote_macro_calls(m_line_buffer);
            unquote_directive(m_line_buffer);

            const bool trim_newline = !input_ends_with_newline && //
                                      (m_input_remaining == 0) && //
                                      (m_secondary_input_buffer.size() == m_secondary_input_offset);
            if (ix_UNLIKELY(trim_newline))
            {
                m_output_writer.write(m_line_buffer.data(), m_line_buffer.size() - ix_strlen("\n"));
            }
            else
            {
                m_output_writer.write(m_line_buffer.data(), m_line_buffer.size());
            }
            m_current_output_line_number += 1;
        }
    }

    void end_input(GokuraiResultImpl *result)
    {
        if (result != nullptr)
        {
            result->clear();
        }

        const bool backed_by_file_handle = (m_output_writer.file_handle() != nullptr);
        if (backed_by_file_handle)
        {
            m_output_writer.flush();
            return;
        }

        m_output_writer.write_char('\0');
        const size_t size = m_output_writer.buffer_size() - 1;
        if (result != nullptr)
        {
            *result = GokuraiResultImpl(m_output_writer.detach(), size);
        }
    }

  private:
    bool find_and_process_directive()
    {
        const char *line_start = m_line_buffer.data();

        if (ix_LIKELY(*line_start != '#'))
        {
            return false;
        }

        const size_t line_length = m_line_buffer.size();
        if ((line_length >= GLOBAL_MACRO_HEADER.length()) && //
            ix_starts_with(line_start, GLOBAL_MACRO_HEADER.data()))
        {
            read_global_macro_definition();
            return true;
        }

        if ((line_length >= LOCAL_MACRO_HEADER.length()) && //
            ix_starts_with(line_start, LOCAL_MACRO_HEADER.data()))
        {
            m_clear_local_macro_on_next_read = false;
            read_local_macro_definition();
            return true;
        }

        if ((line_length >= GLOBAL_BLOCK_MACRO_HEADER.length()) && //
            ix_starts_with(line_start, GLOBAL_BLOCK_MACRO_HEADER.data()))
        {
            read_global_block_macro_definition();
            return true;
        }

        if ((line_length >= LOCAL_BLOCK_MACRO_HEADER.length()) && //
            ix_starts_with(line_start, LOCAL_BLOCK_MACRO_HEADER.data()))
        {
            m_clear_local_macro_on_next_read = false;
            read_local_block_macro_definition();
            return true;
        }

        if ((line_length >= LUA_BLOCK_HEADER.length()) && //
            ix_starts_with(line_start, LUA_BLOCK_HEADER.data()))
        {
            read_and_eval_lua_block();
            return true;
        }

        if ((line_length >= COMMENT_BLOCK_HEADER.length()) && //
            ix_starts_with(line_start, COMMENT_BLOCK_HEADER.data()))
        {
            skip_comment_block(true);
            return true;
        }

        return false;
    }

#if !ix_MEASURE_COVERAGE
    void debug_print_line()
    {
        const char *line_start = m_line_buffer.data();
        const char *line_end = m_line_buffer.data() + m_line_buffer.size();
        for (const char *p = line_start; p < line_end; p++)
        {
            m_err_handle->write_char(*p);
        }
    }

    void debug_print_macro_call(const MacroCall &call)
    {
        debug_print_line();
        const char *line_start = m_line_buffer.data();
        for (const char *p = line_start; p < call.start; p++)
        {
            m_err_handle->write_char(' ');
        }
        m_err_handle->write_char('|');
        for (const char *p = call.start + 1; p < call.end; p++)
        {
            m_err_handle->write_char(' ');
        }
        m_err_handle->write_string("|\n");
    }
#endif

    void load_next_line(bool clear_local_macro)
    {
        const bool secondary_is_empty = (m_secondary_input_buffer.size() == m_secondary_input_offset);
        if (ix_LIKELY(secondary_is_empty))
        {
            if (ix_UNLIKELY(m_clear_local_macro_on_next_read && clear_local_macro))
            {
                m_local_macros.clear();
                m_local_string_arena.clear();
            }

            const bool input_exhausted = (m_input_remaining == 0);
            if (input_exhausted)
            {
                return;
            }

            const char *line_start = m_input + (m_input_length - m_input_remaining);
            const char *line_end = static_cast<const char *>(ix_memchr(line_start, '\n', m_input_remaining));

            const bool this_line_is_last_and_has_no_newline = (line_end == nullptr);
            if (this_line_is_last_and_has_no_newline)
            {
                line_end = m_input + m_input_length;
            }

            m_line_buffer.push_between(line_start, line_end);
            m_line_buffer.push_char('\n');
            m_input_remaining = static_cast<size_t>((m_input + m_input_length) - line_end) - 1;
            if (this_line_is_last_and_has_no_newline)
            {
                ix_ASSERT(m_input_remaining == ix_SIZE_MAX);
                m_input_remaining = 0;
            }
            m_current_input_line_number += 1;
        }
        else
        {
            const char *line_start = m_secondary_input_buffer.data() + m_secondary_input_offset;
            const char *line_end = ix_memnext(line_start, '\n') + 1;
            m_line_buffer.push_between(line_start, line_end);
            m_secondary_input_offset += static_cast<size_t>(line_end - line_start);
        }
    }

    ix_FORCE_INLINE void expand_non_lazy_macros()
    {
        m_current_line_has_lazy_call = false;
        expand_macros(false);
    }

    ix_FORCE_INLINE void expand_lazy_macros()
    {
        expand_macros(true);
    }

    void expand_macros(bool expand_lazy_calls)
    {
        size_t macro_free_suffix_length = ix_strlen("\n");

        while (true)
        {
            const MacroCall call = find_last_call(&macro_free_suffix_length, expand_lazy_calls);

            const bool no_more_call = (call.start == nullptr);
            if (no_more_call)
            {
                break;
            }

            //   line_start                 call.start                  call.end                   line_end
            //       |                          |                           |                          |
            //       |<--- call.head_length --->|<----- call.length() ----->|<--- call.tail_length --->|
            //       |--------------------------|[[[.....................]]]|--------------------------|
            ix_ASSERT(call.end != nullptr);
            ix_ASSERT((expand_lazy_calls && call.is_lazy()) || (!expand_lazy_calls && !call.is_lazy()));

            const char *macro_name_start = call.start + ix_strlen("[[[");
            const char *macro_name_end = ix_memnext2(macro_name_start, '(', ']');

            const bool constant_macro = (*macro_name_end == ']');
            if (!constant_macro)
            {
                const char last_char = *(call.end - ix_strlen(")]]]"));
                const bool macro_args_ended_properly = (last_char == ')');
                if (!macro_args_ended_properly)
                {
                    clear_call(call);
                    continue;
                }
            }

            const ix_StringView macro_name_view = ix_StringView(macro_name_start, macro_name_end);
            const char first_char = *macro_name_start;
            const bool normal_macro = (first_char != '_');
            if (ix_LIKELY(normal_macro))
            {
                goto MACRO_LOOKUP;
            }

            if (m_lua_enabled)
            {
                constexpr ix_StringView LUA("__LUA__");
                const bool lua_call = (macro_name_view == LUA);
                if (ix_UNLIKELY(lua_call))
                {
                    const char *fragment_start = call.start + ix_strlen("[[[__LUA__(");
                    const size_t fragment_length = call.length() - ix_strlen("[[[__LUA__()]]]");
                    m_temp_buffer.clear();
                    m_temp_buffer.reserve(LUA_RETURN.length() + fragment_length + 1); // 1 = len("\0")
                    m_temp_buffer.push(LUA_RETURN.data(), LUA_RETURN.length());
                    m_temp_buffer.push(fragment_start, fragment_length);
                    m_temp_buffer.push_char('\0');

                    const char *output;
                    size_t output_length;
                    eval_lua_fragment(m_temp_buffer.data(), m_temp_buffer.size(), &output, &output_length);

                    if (output_length == 0)
                    {
                        clear_call(call);
                    }
                    else
                    {
                        const char *first_line_end_minus_one = ix_strchr(output, '\n');
                        const bool multiline_output = (first_line_end_minus_one != nullptr);
                        if (!multiline_output)
                        {
                            replace_call(call, output, output_length);
                        }
                        else
                        {
                            const char *first_line_end = first_line_end_minus_one + 1;
                            const size_t first_line_length = static_cast<size_t>(first_line_end - output);
                            replace_call_multiline(call, output, output_length, first_line_length);
                            macro_free_suffix_length = ix_strlen("\n");
                        }
                    }

                    lua_settop(m_lua_state, 0);
                    continue;
                }
            }

            if (*call.end == '\n')
            {
                constexpr ix_StringView NO_NEWLINE("__NO_NEWLINE__");
                const bool no_newline = (macro_name_view == NO_NEWLINE);
                if (ix_UNLIKELY(no_newline))
                {
                    m_line_buffer.pop_back(call.offset + ix_strlen("[[[__NO_NEWLINE__]]]\n"));
                    const size_t old_size = m_line_buffer.size();
                    // This load does not clear the local macro.
                    load_next_line(false);
                    const bool nothing_loaded = (old_size == m_line_buffer.size());
                    if (nothing_loaded)
                    {
                        m_line_buffer.push_char('\n');
                    }
                    if (expand_lazy_calls)
                    {
                        // IDEA: Avoid recursion.
                        expand_non_lazy_macros();
                    }
                    macro_free_suffix_length = ix_strlen("\n");
                    continue;
                }
            }

            {
                constexpr ix_StringView INPUT_LINE_NUMBER("__INPUT_LINE_NUMBER__");
                const bool input_line_number = (macro_name_view == INPUT_LINE_NUMBER);
                if (ix_UNLIKELY(input_line_number))
                {
                    char buf[32];
                    const int length =
                        ix_snprintf(buf, ix_LENGTH_OF(buf), "%" PRIu64 "", m_current_input_line_number);
                    replace_call(call, buf, static_cast<size_t>(length));
                    continue;
                }
            }

            {
                constexpr ix_StringView OUTPUT_LINE_NUMBER("__OUTPUT_LINE_NUMBER__");
                const bool output_line_number = (macro_name_view == OUTPUT_LINE_NUMBER);
                if (ix_UNLIKELY(output_line_number))
                {
                    char buf[32];
                    const int length =
                        ix_snprintf(buf, ix_LENGTH_OF(buf), "%" PRIu64 "", m_current_output_line_number);
                    replace_call(call, buf, static_cast<size_t>(length));
                    continue;
                }
            }

            {
                constexpr ix_StringView ENABLE_LUA("__ENABLE_LUA__");
                const bool enable_lua = (macro_name_view == ENABLE_LUA);
                if (ix_UNLIKELY(enable_lua))
                {
                    m_lua_enabled = true;
                    clear_call(call);
                    continue;
                }
            }

            {
                constexpr ix_StringView DISABLE_LUA("__DISABLE_LUA__");
                const bool disable_lua = (macro_name_view == DISABLE_LUA);
                if (ix_UNLIKELY(disable_lua))
                {
                    m_lua_enabled = false;
                    clear_call(call);
                    continue;
                }
            }

        MACRO_LOOKUP:
            const Macro *macro = m_local_macros.find(macro_name_view);
            bool macro_found = (macro != nullptr);
            if (ix_LIKELY(!macro_found))
            {
                macro = m_global_macros.find(macro_name_view);
                macro_found = (macro != nullptr);
            }

            if (ix_UNLIKELY(!macro_found))
            {
                clear_call(call);
                continue;
            }

            if (constant_macro)
            {
                if (macro->is_oneline())
                {
                    replace_call(call, macro->body, macro->body_length);
                    continue;
                }

                // constant multiline macro
                replace_call_multiline(call, macro->body, macro->body_length, macro->first_line_length);
                macro_free_suffix_length = ix_strlen("\n");
                continue;
            }

            // macro with arguments
            const char *args[MAX_NUM_ARGS + 1] = {};
            const char *args_start = macro_name_end + ix_strlen("(");
            const char *args_end = call.end - ix_strlen(")]]]");
            parse_args(args, args_start, args_end);

            m_temp_buffer.clear();
            expand_call_with_args(args, macro->body, macro->body_length, m_temp_buffer);

            if (macro->is_oneline())
            {
                replace_call(call, m_temp_buffer.data(), m_temp_buffer.size());
                continue;
            }

            // The most complicated case (multiline macro with arguments).
            const char *expanded_macro = m_temp_buffer.data();
            const char *first_line_end = ix_memnext(expanded_macro, '\n');
            const size_t first_line_length = static_cast<size_t>(first_line_end - expanded_macro) + 1;
            replace_call_multiline(call, m_temp_buffer.data(), m_temp_buffer.size(), first_line_length);
            macro_free_suffix_length = ix_strlen("\n");
        }
    }

    MacroCall find_last_call(size_t *macro_free_suffix_length, bool find_lazy_call)
    {
        // We need to calculate 'line_start' and 'line_end' here, because the main loop modifies `m_line_buffer`.
        char *line_start = m_line_buffer.data();
        char *line_end = m_line_buffer.data() + m_line_buffer.size(); // cppcheck-suppress constVariablePointer
        ix_ASSERT(line_end[-1] == '\n');

        char *search_start = line_start;
        char *search_end = line_end - *macro_free_suffix_length;
        ix_ASSERT(search_start <= search_end);

        MacroCall call;
        ix_memset(&call, 0, sizeof(call));

        if (static_cast<size_t>(search_end - search_start) < ix_strlen("[[[]]]"))
        {
            return call;
        }

        char *p = search_end;
        const char *call_end_search_end = search_start + ix_strlen("[[[]]]") - 1;
        const char *call_start_search_end = search_start + ix_strlen("[[[") - 1;

    SEARCH_ENTRY:
        // First phase: Search for an unquoted "]]]".
        while (call_end_search_end <= p)
        {
            if (ix_LIKELY(*p != ']'))
            {
                p -= 1;
                continue;
            }

            p -= 1;
            size_t num_closing_square_bracket = 1;
            while ((search_start <= p) && (*p == ']'))
            {
                num_closing_square_bracket += 1;
                p -= 1;
            }

            const bool not_call_end = (num_closing_square_bracket < ix_strlen("]]]"));
            if (not_call_end)
            {
                continue;
            }

            const bool quoted = (search_start <= p) && (*p == '\'');
            if (ix_UNLIKELY(quoted))
            {
                if (num_closing_square_bracket < 2 * ix_strlen("]]]"))
                {
                    continue;
                }
                num_closing_square_bracket -= ix_strlen("]]]");
                p += ix_strlen("]]]");
            }

            *macro_free_suffix_length = static_cast<size_t>(line_end - p) - num_closing_square_bracket - 1;
            call.end = p + ix_strlen("]]]") + 1;
            break;
        }

        if (call.end == nullptr)
        {
            return call;
        }

        // Second phase: Search for an unquoted "[[[".
        // However, if we find "]]]" during this search, that "]]]" replaces the one we found in the first phase.
        // This complication makes the search slower, which is why this function is split into two phases.
        while (call_start_search_end <= p)
        {
            const char c = *p;
            if ((c != '[') && (c != ']'))
            {
                p -= 1;
                continue;
            }

            if (c == '[')
            {
                p -= 1;
                size_t num_opening_square_bracket = 1;
                while ((search_start <= p) && (*p == '['))
                {
                    num_opening_square_bracket += 1;
                    p -= 1;
                }
                if (num_opening_square_bracket < ix_strlen("[[["))
                {
                    continue;
                }

                const bool quoted = (search_start <= p) && (*p == '\'');
                if (ix_UNLIKELY(quoted))
                {
                    if (num_opening_square_bracket < 2 * ix_strlen("[[["))
                    {
                        continue;
                    }
                }

                call.start = p + num_opening_square_bracket - ix_strlen("[[[") + 1;
                const bool lazy = (line_start < call.start) && (call.start[-1] == '^');
                call.offset = lazy ? 1 : 0;

                if (ix_UNLIKELY(!find_lazy_call && lazy))
                {
                    m_current_line_has_lazy_call = true;

                    call.start = nullptr;
                    call.end = find_first_unmatched_call_end(call.end, line_end);
                    if (call.end == nullptr)
                    {
                        goto SEARCH_ENTRY;
                    }

                    // I think this adjustment is not needed, but I'm not really sure.
                    // *macro_free_suffix_length = static_cast<size_t>(line_end - call.end);
                    // {
                    //     char *e = call.end;
                    //     while ((call.end < line_end) && (*e == ']'))
                    //     {
                    //         *macro_free_suffix_length -= 1;
                    //         e += 1;
                    //     }
                    // }

                    p -= 1;
                    continue;
                }

                // VERY unlikely
                if (ix_UNLIKELY(find_lazy_call && !lazy))
                {
                    m_redo_macro_expansion = true;
                    call.start = nullptr;
                    return call;
                }

                // Macro call found.
                call.head_length = static_cast<size_t>(call.start - line_start);
                call.tail_length = static_cast<size_t>(line_end - call.end);
                return call;
            }

            ix_ASSERT(c == ']');
            p -= 1;
            size_t num_closing_square_bracket = 1;
            while ((search_start <= p) && (*p == ']'))
            {
                num_closing_square_bracket += 1;
                p -= 1;
            }
            if (num_closing_square_bracket < ix_strlen("]]]"))
            {
                continue;
            }
            const bool quoted = (search_start <= p) && (*p == '\'');
            if (ix_UNLIKELY(quoted))
            {
                if (num_closing_square_bracket < 2 * ix_strlen("]]]"))
                {
                    continue;
                }
                p += ix_strlen("]]]");
            }
            call.end = p + ix_strlen("]]]") + 1;
        }

        return call;
    }

    void clear_call(const MacroCall &call)
    {
        const size_t offset = call.offset;
        ix_memmove(call.start - offset, call.end, call.tail_length);
        m_line_buffer.pop_back(call.length() + offset);
    }

    void replace_call(const MacroCall &call, const char *str, size_t str_length)
    {
        const size_t offset = call.offset;
        const size_t call_length = call.length();
        if (str_length < call_length)
        {
            ix_memcpy(call.start - offset, str, str_length);
            ix_memmove(call.start - offset + str_length, call.end, call.tail_length);
            m_line_buffer.pop_back(call_length + offset - str_length);
        }
        else
        {
            const size_t required_additional_space = str_length - offset - call_length;
            m_line_buffer.ensure(required_additional_space); // May reallocate.
            char *line_start = m_line_buffer.data();
            char *call_start = line_start + call.head_length;
            ix_memmove(call_start - offset + str_length, call_start + call_length, call.tail_length);
            ix_memcpy(call_start - offset, str, str_length);
            m_line_buffer.add_size(required_additional_space);
        }
    }

    void replace_call_multiline(const MacroCall &call, const char *str, size_t str_length, size_t first_line_length)
    {
        ix_ASSERT(str[first_line_length - 1] == '\n');
        replace_call(call, str, first_line_length);
        const size_t new_line_length = call.head_length - call.offset + first_line_length;
        ix_ASSERT(*(m_line_buffer.data() + new_line_length - 1) == '\n');
        m_line_buffer.push(m_secondary_input_buffer.data() + m_secondary_input_offset,
                           m_secondary_input_buffer.size() - m_secondary_input_offset);
        m_secondary_input_buffer.clear();
        m_secondary_input_buffer.push(str + first_line_length, str_length - first_line_length);
        m_secondary_input_buffer.push(m_line_buffer.data() + new_line_length, m_line_buffer.size() - new_line_length);
        m_line_buffer.set_size(new_line_length);
        m_secondary_input_offset = 0;
    }

    ix_FORCE_INLINE void read_global_macro_definition()
    {
        read_macro_definition(GLOBAL_MACRO_HEADER.length(), m_global_string_arena, m_global_macros);
    }

    ix_FORCE_INLINE void read_local_macro_definition()
    {
        read_macro_definition(LOCAL_MACRO_HEADER.length(), m_local_string_arena, m_local_macros);
    }

    void read_macro_definition(size_t header_length, ix_StringArena &arena,
                               ix_HashMapSingleArray<ix_StringView, Macro> &macros)
    {
        const char *line_start = m_line_buffer.data();
        const char *line_end = m_line_buffer.data() + m_line_buffer.size();
        const char *name_start = line_start + header_length;
        const char *name_end = ix_memnext2(name_start, ' ', '\n');
        const size_t name_length = static_cast<size_t>(name_end - name_start);

        const bool name_not_present = (*name_end == '\n');
        if (name_not_present)
        {
            return;
        }

        const char *name = arena.push(name_start, name_length);
        const char *body_start = name_end + ix_strlen(" ");
        const char *body_end = line_end - 1;
        const size_t body_length = static_cast<size_t>(body_end - body_start);
        const char *body = arena.push(body_start, body_length);
        macros.emplace(ix_StringView(name, name_length), Macro{body_length, 0, body});
    }

    ix_FORCE_INLINE void read_global_block_macro_definition()
    {
        read_block_macro_definition(GLOBAL_BLOCK_MACRO_HEADER, GLOBAL_BLOCK_MACRO_FOOTER, //
                                    m_global_string_arena, m_global_macros);
    }

    ix_FORCE_INLINE void read_local_block_macro_definition()
    {
        read_block_macro_definition(LOCAL_BLOCK_MACRO_HEADER, LOCAL_BLOCK_MACRO_FOOTER, //
                                    m_local_string_arena, m_local_macros);
    }

    void read_block_macro_definition(const ix_StringView &header, const ix_StringView &footer, //
                                     ix_StringArena &arena, ix_HashMapSingleArray<ix_StringView, Macro> &macros)
    {
        const char *name_start = m_line_buffer.data() + header.length();
        const char *name_end = ix_memnext(name_start, '\n');
        const size_t name_length = static_cast<size_t>(name_end - name_start);
        const char *name = arena.push(name_start, name_length);

        size_t first_line_length = 0;
        size_t depth = 1;
        m_block_buffer.clear();
        while (true)
        {
            m_line_buffer.clear();
            load_next_line(true);

            if (m_line_buffer.empty())
            {
                return;
            }

            expand_non_lazy_macros();

            const char *line_start = m_line_buffer.data();
            if (*line_start == '#')
            {
                const ix_StringView line_view(line_start, m_line_buffer.size());

                const bool block_end = (line_view == footer);
                if (block_end)
                {
                    depth -= 1;
                    const bool definition_end = (depth == 0);
                    if (definition_end)
                    {
                        break;
                    }
                }

                const bool comment_block_start = (line_view == COMMENT_BLOCK_HEADER);
                if (comment_block_start)
                {
                    skip_comment_block(false);
                    continue;
                }
            }

            const bool second_line = (first_line_length == 0) && !m_block_buffer.empty();
            if (second_line)
            {
                first_line_length = m_block_buffer.size();
            }

            if (ix_starts_with(m_line_buffer.data(), header.data()))
            {
                depth += 1;
            }

            m_block_buffer.push(m_line_buffer.data(), m_line_buffer.size());
        }

        if (!m_block_buffer.empty())
        {
            // Remove the last newline.
            m_block_buffer.pop_back(1);
        }

        const size_t body_length = m_block_buffer.size();
        const char *body = arena.push(m_block_buffer.data(), body_length);
        macros.emplace(ix_StringView{name, name_length}, Macro{body_length, first_line_length, body});
    }

    void read_and_eval_lua_block()
    {
        m_block_buffer.clear();
        m_block_buffer.push(LUA_RETURN.data(), LUA_RETURN.length());

        while (true)
        {
            m_line_buffer.clear();
            load_next_line(true);

            if (m_line_buffer.empty())
            {
                return;
            }

            do
            {
                m_redo_macro_expansion = false;
                expand_non_lazy_macros();
                if (m_current_line_has_lazy_call)
                {
                    expand_lazy_macros();
                }
            } while (m_redo_macro_expansion);

            const char *line_start = m_line_buffer.data();

            if (*line_start == '#')
            {
                const ix_StringView line_view(line_start, m_line_buffer.size());

                const bool block_end = (line_view == LUA_BLOCK_FOOTER);
                if (block_end)
                {
                    break;
                }

                const bool comment_block_start = (line_view == COMMENT_BLOCK_HEADER);
                if (comment_block_start)
                {
                    skip_comment_block(true);
                    continue;
                }
            }

            m_block_buffer.push(m_line_buffer.data(), m_line_buffer.size());
        }

        const char *output;
        size_t output_length;
        m_block_buffer.push_char('\0');
        eval_lua_fragment(m_block_buffer.data(), m_block_buffer.size(), &output, &output_length);

        if (output_length != 0)
        {
            m_block_buffer.clear();
            m_block_buffer.push(output, output_length);
            m_block_buffer.push_char('\n');
            m_block_buffer.push(m_secondary_input_buffer.data() + m_secondary_input_offset,
                                m_secondary_input_buffer.size() - m_secondary_input_offset);
            m_secondary_input_buffer.clear();
            m_secondary_input_buffer.push(m_block_buffer.data(), m_block_buffer.size());
            m_secondary_input_offset = 0;
        }

        lua_settop(m_lua_state, 0);
    }

    void skip_comment_block(bool expand_lazy_calls)
    {
        size_t depth = 1;
        m_lua_enabled = false;

        while (true)
        {
            m_line_buffer.clear();
            load_next_line(true);

            if (m_line_buffer.empty())
            {
                break;
            }

            do
            {
                m_redo_macro_expansion = false;
                expand_non_lazy_macros();
                if (expand_lazy_calls && m_current_line_has_lazy_call)
                {
                    expand_lazy_macros();
                }
            } while (m_redo_macro_expansion);

            const char *line_start = m_line_buffer.data();

            if (*line_start != '#')
            {
                continue;
            }

            const ix_StringView line_view(line_start, m_line_buffer.size());
            const bool block_end = (line_view == COMMENT_BLOCK_FOOTER);
            if (block_end)
            {
                depth -= 1;
                const bool comment_end = (depth == 0);
                if (comment_end)
                {
                    break;
                }
            }

            const bool block_begin = (line_view == COMMENT_BLOCK_HEADER);
            if (block_begin)
            {
                depth += 1;
            }
        }

        m_lua_enabled = true;
    }

    void eval_lua_fragment(const char *fragment, size_t fragment_length, const char **output, size_t *output_length)
    {
        if (ix_UNLIKELY(m_lua_state == nullptr))
        {
            m_lua_state = luaL_newstate();
            luaL_checkversion(m_lua_state);
            lua_gc(m_lua_state, LUA_GCGEN, 0, 0);
            luaL_openlibs(m_lua_state);
        }

        ix_ASSERT(ix_memcmp(fragment, LUA_RETURN.data(), LUA_RETURN.length()) == 0);
        ix_ASSERT(fragment[fragment_length - 1] == '\0');

        // With "return".
        const int result = eval_lua_program(m_lua_state, fragment, fragment_length - 1, nullptr);

        if (result != LUA_OK)
        {
            // Without "return".
            lua_settop(m_lua_state, 0);
            constexpr size_t OFFSET = LUA_RETURN.length();
            eval_lua_program(m_lua_state, fragment + OFFSET, fragment_length - 1 - OFFSET, m_err_handle);
        }

        *output = lua_tolstring(m_lua_state, 1, output_length);
        if (*output == nullptr)
        {
            // The program's return value is not an integer nor a string.
            *output = "";
            *output_length = 0;
        }
    }
};

GokuraiContext gokurai_context_create(const ix_FileHandle *out_handle, const ix_FileHandle *err_handle)
{
    return ix_new<GokuraiContextImpl>(out_handle, err_handle);
}

void gokurai_context_clear(GokuraiContext ctx)
{
    auto *impl = static_cast<GokuraiContextImpl *>(ctx);
    impl->clear();
}

void gokurai_context_destroy(GokuraiContext ctx)
{
    auto *impl = static_cast<GokuraiContextImpl *>(ctx);
    ix_delete(impl);
}

void gokurai_context_feed_input(GokuraiContext ctx, const char *input, size_t input_length)
{
    auto *impl = static_cast<GokuraiContextImpl *>(ctx);
    impl->feed_input(input, input_length);
}

void gokurai_context_feed_str(GokuraiContext ctx, const char *str)
{
    gokurai_context_feed_input(ctx, str, ix_strlen(str));
}

void gokurai_context_end_input(GokuraiContext ctx, GokuraiResult result)
{
    GokuraiContextImpl *ctx_impl = static_cast<GokuraiContextImpl *>(ctx);
    auto *result_impl = static_cast<GokuraiResultImpl *>(result);
    ctx_impl->end_input(result_impl);
}

GokuraiResult gokurai_result_create()
{
    return ix_new<GokuraiResultImpl>();
}

void gokurai_result_destroy(GokuraiResult result)
{
    auto *impl = static_cast<GokuraiResultImpl *>(result);
    ix_delete(impl);
}

const char *gokurai_result_get_output(GokuraiResult result)
{
    const auto *impl = static_cast<GokuraiResultImpl *>(result);
    return impl->data().get();
}

size_t gokurai_result_get_output_length(GokuraiResult result)
{
    const auto *impl = static_cast<GokuraiResultImpl *>(result);
    return impl->size();
}

[[maybe_unused]] static GokuraiResultImpl gokurai(const char *input, size_t input_length,
                                                  const ix_FileHandle *out_handle, const ix_FileHandle *err_handle)
{
    GokuraiContextImpl ctx(out_handle, err_handle);
    ctx.feed_input(input, input_length);
    GokuraiResultImpl result;
    ctx.end_input(&result);
    return result;
}

[[maybe_unused]] static GokuraiResultImpl gokurai_str(const char *input)
{
    return gokurai(input, ix_strlen(input), nullptr, &ix_FileHandle::of_stderr());
}

#if !ix_MEASURE_COVERAGE
[[maybe_unused]] inline doctest::String toString(const GokuraiResultImpl &res)
{
    if (res.size() == 0)
    {
        return "";
    }

    return res.data().get();
}
#endif

#define test_gokurai(input, expected)                     \
    do                                                    \
    {                                                     \
        const GokuraiResultImpl res = gokurai_str(input); \
        if (res.size() == 0)                              \
        {                                                 \
            ix_EXPECT(ix_strlen(expected) == 0);          \
        }                                                 \
        else                                              \
        {                                                 \
            ix_EXPECT_EQSTR(res.data().get(), expected);  \
        }                                                 \
    } while (0)

ix_TEST_CASE("gokurai: plain")
{
    test_gokurai("", "");
    test_gokurai("\n", "\n");
    test_gokurai("\n\n\n", "\n\n\n");
    test_gokurai("hello world", "hello world");
    test_gokurai("hello world\n", "hello world\n");

    test_gokurai("hello world\n"
                 "hello world\n"
                 "hello world\n",
                 "hello world\n"
                 "hello world\n"
                 "hello world\n");

    test_gokurai("hello world\n"
                 "hello world\n"
                 "hello world",
                 "hello world\n"
                 "hello world\n"
                 "hello world");

    const char *long_msg = "01234567890123456789012345678901234567890123456789012345678901234567890123456789"
                           "01234567890123456789012345678901234567890123456789012345678901234567890123456789"
                           "01234567890123456789012345678901234567890123456789012345678901234567890123456789"
                           "01234567890123456789012345678901234567890123456789012345678901234567890123456789"
                           "01234567890123456789012345678901234567890123456789012345678901234567890123456789"
                           "01234567890123456789012345678901234567890123456789012345678901234567890123456789"
                           "01234567890123456789012345678901234567890123456789012345678901234567890123456789";
    test_gokurai(long_msg, long_msg);
}

ix_TEST_CASE("gokurai: macro definition")
{
    test_gokurai("#+MACRO foo FOO\n", "");

    test_gokurai("#+MACRO foo FOO\n"
                 "hello world\n"
                 "#+MACRO bar Bar\n",
                 "hello world\n");

    test_gokurai("#+MACRO foo FOO\n"
                 "hello world\n"
                 "#+MACRO bar Bar",
                 "hello world\n");
}

ix_TEST_CASE("gokurai: undefined macro")
{
    test_gokurai("hello [[[world]]]", "hello ");
    test_gokurai("hello [[[world]]]\n", "hello \n");
    test_gokurai("hello [[[world]]]\n"
                 "hello [[[world]]]\n"
                 "hello [[[world]]]\n",
                 "hello \n"
                 "hello \n"
                 "hello \n");
}

ix_TEST_CASE("gokurai: constant macro")
{
    test_gokurai("#+MACRO world WORLD\n"
                 "hello [[[world]]]\n",
                 "hello WORLD\n");

    test_gokurai("#+MACRO world WORLD\n"
                 "hello [[[world]]]\n"
                 "hello [[[world]]]\n"
                 "hello [[[world]]]\n",
                 "hello WORLD\n"
                 "hello WORLD\n"
                 "hello WORLD\n");

    test_gokurai("#+MACRO world WORLD\n"
                 "hello [[[world]]] [[[world]]]\n",
                 "hello WORLD WORLD\n");

    test_gokurai("#+MACRO world WORLD\n"
                 "hello [[[world]]] [[[world]]]\n"
                 "hello [[[world]]] [[[world]]]\n"
                 "hello [[[world]]] [[[world]]]\n",
                 "hello WORLD WORLD\n"
                 "hello WORLD WORLD\n"
                 "hello WORLD WORLD\n");

    test_gokurai("#+MACRO world WORLD\n"
                 "hello [[[world]]] [[[world]]]\n"
                 "hello [[[world]]] [[[world]]]\n"
                 "hello [[[world]]] [[[world]]]",
                 "hello WORLD WORLD\n"
                 "hello WORLD WORLD\n"
                 "hello WORLD WORLD");

    test_gokurai("#+MACRO a A\n"
                 "#+MACRO b B\n"
                 "#+MACRO c C\n"
                 "[[[a]]] [[[b]]] [[[c]]]\n",
                 "A B C\n");

    test_gokurai("#+MACRO long loooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooong\n"
                 "[[[long]]]\n"
                 "[[[long]]]\n"
                 "[[[long]]]\n",
                 "loooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooong\n"
                 "loooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooong\n"
                 "loooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooong\n");

    test_gokurai("#+MACRO long loooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooong\n"
                 "[[[long]]]---[[[long]]]---[[[long]]]\n",
                 "loooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooong"
                 "---"
                 "loooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooong"
                 "---"
                 "loooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooong\n");

    test_gokurai("#+MACRO foo FOO\n"
                 "[[[[foo]]]]\n",
                 "[FOO]\n");

    test_gokurai("#+MACRO foo FOO\n"
                 "[[[[foo]]][[[foo]]]]]]\n",
                 "[FOOFOO]]]\n");
}

ix_TEST_CASE("gokurai: macro with arguments")
{
    test_gokurai("#+MACRO foo FOO $1 FOO\n"
                 "#+MACRO bar BAR\n"
                 "[[[foo([[[bar]]])]]]",
                 "FOO BAR FOO");

    test_gokurai("#+MACRO foo FOO $1-$2-$3 FOO\n"
                 "[[[foo(one,two,three)]]]",
                 "FOO one-two-three FOO");

    test_gokurai("#+MACRO foo FOO $1-$2-$3 FOO\n"
                 "[[[foo(one,two,three,four,five)]]]",
                 "FOO one-two-three FOO");

    test_gokurai("#+MACRO foo FOO $1-$2-$3 FOO\n"
                 "[[[foo(one\\,two,three,four\\,five)]]]",
                 "FOO one,two-three-four,five FOO");

    test_gokurai("#+MACRO foo FOO $0 FOO\n"
                 "[[[foo(one,two,three,four,five)]]]",
                 "FOO one,two,three,four,five FOO");

    test_gokurai("#+MACRO dollar $$1\n"
                 "[[[dollar(10)]]], [[[dollar(100)]]], [[[dollar(1000)]]]",
                 "$10, $100, $1000");

    test_gokurai("#+MACRO foo [[[LEFT]]]$1[[[RIGHT]]]\n"
                 "[[[foo(x)]]]",
                 "x");

    test_gokurai("#+MACRO foo $1-$2-$3-$4-$5-$6-$7-$8-$9\n"
                 "[[[foo(11,22,33,44,55,66,77,88,99)]]]",
                 "11-22-33-44-55-66-77-88-99");

    test_gokurai("#+MACRO foo $1-$2-$3-$4-$5-$6-$7-$8-$9\n"
                 "[[[foo(11,22,33,44,55,66,77,88,99)]]]",
                 "11-22-33-44-55-66-77-88-99");

    test_gokurai("#+MACRO foo $1-$2-$3-$4-$5-$6-$7-$8-$9\n"
                 "[[[foo(11,22,33,44,55,66,77,88,99,1010,1111)]]]",
                 "11-22-33-44-55-66-77-88-99,1010,1111");

    test_gokurai("#+MACRO foo $1-$2\n"
                 "[[[foo(11,22,33,44)]]]",
                 "11-22");
}

ix_TEST_CASE("gokurai: local macro")
{
    test_gokurai("#+LOCAL_MACRO foo FOO\n"
                 "[[[foo]]]\n"
                 "[[[foo]]]\n",
                 "FOO\n"
                 "\n");

    test_gokurai("#+LOCAL_MACRO foo FOO\n"
                 "#+LOCAL_MACRO bar BAR\n"
                 "[[[foo]]][[[bar]]]\n"
                 "[[[foo]]][[[bar]]]\n",
                 "FOOBAR\n"
                 "\n");

    test_gokurai("#+LOCAL_MACRO foo FOO\n"
                 "#+LOCAL_MACRO bar BAR\n"
                 "[[[foo]]][[[bar]]]\n"
                 "[[[foo]]][[[bar]]]\n",
                 "FOOBAR\n"
                 "\n");

    test_gokurai("#+MACRO foo FOO\n"
                 "#+LOCAL_MACRO foo FOOFOO\n"
                 "[[[foo]]]\n"
                 "[[[foo]]]\n",
                 "FOOFOO\n"
                 "FOO\n");
}

ix_TEST_CASE("gokurai: nested constructs")
{
    test_gokurai("#+MACRO f f\n"
                 "#+MACRO o o\n"
                 "#+MACRO foo FOO\n"
                 "[[[[[[f]]][[[o]]][[[o]]]]]]",
                 "FOO");

    test_gokurai("#+MACRO f f\n"
                 "#+MACRO o o\n"
                 "#+MACRO < [\n"
                 "#+MACRO > ]\n"
                 "#+MACRO foo FOO\n"
                 "[[[<]]][[[<]]][[[<]]][[[f]]][[[o]]][[[o]]][[[>]]][[[>]]][[[>]]]",
                 "FOO");
}

ix_TEST_CASE("gokurai: ill-formed macro call")
{
    test_gokurai("[[[foo]]][[[foo[[[foo]]]", "[[[foo");
    test_gokurai("[[[foo]]][[[foo][[[foo]]]", "[[[foo]");
    test_gokurai("[[[foo]]][[[foo]][[[foo]]]", "[[[foo]]");
    test_gokurai("[[[foo]]][[foo]]][[[foo]]]", "[[foo]]]");
    test_gokurai("[[[foo]]][foo]]][[[foo]]]", "[foo]]]");
    test_gokurai("[[[foo]]]foo]]][[[foo]]]", "foo]]]");
    test_gokurai("]]]]]]]]]]]]]]]]]]]]", "]]]]]]]]]]]]]]]]]]]]");
    test_gokurai("[[[[[[[[[[[[[[[[[[[[", "[[[[[[[[[[[[[[[[[[[[");

    test_gokurai("#+LOCAL_MACRO foo FOO\n"
                 "foo[[[foo]]]]foo]]]",
                 "fooFOO]foo]]]");
}

ix_TEST_CASE("gokurai: ill-formed macro call with arguments")
{
    test_gokurai("[[[foo]]][[[foo(]]][[[foo]]]", "");
    test_gokurai("[[[foo]]][[[foo(1,2,3,4,5]]][[[foo]]]", "");
    test_gokurai("[[[foo]]][[[foo(1,2,3,)4,5]]][[[foo]]]", "");
}

ix_TEST_CASE("gokurai: tests from gokuro")
{
    // empty.gokuro
    test_gokurai("\n", "\n");

    // no_newline.gokuro
    test_gokurai("", "");

    // hello.gokuro
    test_gokurai("hello world!\n", "hello world!\n");

    // multi_line.gokuro
    test_gokurai("line 1\n"
                 "line 2\n"
                 "line 3\n"
                 "line 4\n",
                 "line 1\n"
                 "line 2\n"
                 "line 3\n"
                 "line 4\n");

    // long_line.gokuro
    const char *msg =
        "hello world! hello world! hello world! hello world! hello world! hello world! hello world! hello world! "
        "hello world! hello world! hello world! hello world! hello world! hello world! hello world! hello world! "
        "hello world! hello world! hello world! hello world! hello world! hello world! hello world! hello world! "
        "hello world! hello world! hello world! hello world! hello world! hello world! hello world! hello world! "
        "hello world! hello world! hello world! hello world! hello world! hello world! hello world! hello world! "
        "hello world! hello world! hello world! hello world! hello world! hello world! hello world! hello world! "
        "hello world! hello world! hello world! hello world! hello world! hello world! hello world! hello world! "
        "hello world! hello world! hello world! hello world! hello world! hello world! hello world! hello world! "
        "hello world! hello world! hello world! hello world! hello world! hello world! hello world! hello world! "
        "hello world! hello world! hello world! hello world! hello world! hello world! hello world! hello world! "
        "hello world! hello world! hello world! hello world! hello world! hello world! hello world! hello world! "
        "hello world! hello world! hello world! hello world! hello world! hello world! hello world! hello world! "
        "hello world! hello world! hello world! hello world! hello world! hello world! hello world! hello world! "
        "hello world! hello world! hello world! hello world! hello world! hello world! hello world! hello world! "
        "hello world! hello world! hello world! hello world! hello world! hello world! hello world! hello world! "
        "hello world! hello world! hello world! hello world! hello world! hello world! hello world! hello world! "
        "hello world! hello world! hello world! hello world! hello world! hello world! hello world! hello world! "
        "hello world! hello world! hello world! hello world! hello world! hello world! hello world! hello world! "
        "hello world! hello world! hello world! hello world! hello world! hello world! hello world! hello world! "
        "hello world! hello world! hello world! hello world! hello world! hello world! hello world! hello world!";

    test_gokurai(msg, msg);

    // macro.gokuro
    test_gokurai(R"(
#+MACRO value1 1
#+MACRO value22 22
#+MACRO value333 333
#+MACRO value4444 4444
#+MACRO value55555 55555
#+MACRO value666666 666666
#+MACRO value0
[[[value1]]]
[[[value22]]]
[[[value333]]]
[[[value4444]]]
[[[value55555]]]
[[[value666666]]]
value1 is [[[value1]]].
value22 is [[[value22]]].
value333 is [[[value333]]].
value4444 is [[[value4444]]].
value55555 is [[[value55555]]].
value666666 is [[[value666666]]].
[[[value1]]][[[value22]]][[[value333]]][[[value4444]]][[[value55555]]][[[value666666]]]
)",
                 R"(
1
22
333
4444
55555
666666
value1 is 1.
value22 is 22.
value333 is 333.
value4444 is 4444.
value55555 is 55555.
value666666 is 666666.
122333444455555666666
)");

    // macro_local.gokuro
    test_gokurai(R"(
#+MACRO macro $1-^[[[A]]]-^[[[B]]]-^[[[C]]]-$2
[[[macro]]]
[[[macro()]]]
[[[macro(AAA,BBB)]]]
#+LOCAL_MACRO A A
#+LOCAL_MACRO B B
#+LOCAL_MACRO C C
[[[macro(0,9)]]]
[[[macro(AAA,BBB)]]]
)",
                 R"(
$1----$2
----
AAA----BBB
0-A-B-C-9
AAA----BBB
)");

    // macro_args.gokuro
    test_gokurai(R"(
#+MACRO verbatim $0
#+MACRO paren ($0)
#+MACRO macro5 ($1,$2,$3,$4,$5)
#+MACRO macro9 $1-$2-$3-$4-$5-$6-$7-$8-$9
text [[[verbatim(text)]]] text
text [[[verbatim()]]] text
text [[[verbatim(,)]]] text
text [[[paren(some text)]]] text
text [[[macro5(1,2,3,4,5)]]] text
text [[[macro5(a,bb,ccc,dddd,eeeee,ffffff)]]] text
text [[[macro9(1,2,3,4,5,6,7,8,9)]]] text
text [[[macro9(a,bb,ccc,dddd,eeeee,ffffff,ggggggg,hhhhhhhh,iiiiiiiii)]]] text
)",
                 R"(
text text text
text  text
text , text
text (some text) text
text (1,2,3,4,5) text
text (a,bb,ccc,dddd,eeeee) text
text 1-2-3-4-5-6-7-8-9 text
text a-bb-ccc-dddd-eeeee-ffffff-ggggggg-hhhhhhhh-iiiiiiiii text
)");

    // macro_combined.gokuro
    test_gokurai(R"(
#+MACRO value 1
#+MACRO left [
#+MACRO right ]
[[[left]]][[[left]]][[[left]]]value[[[right]]][[[right]]][[[right]]]
)",
                 R"(
1
)");

    // macro_defined_by_macro.gokuro
    test_gokurai(R"(
#+MACRO global #+MACRO $1 $2
[[[global(macro1,Hello world!)]]]
[[[macro1]]]

#+MACRO text ^[[[LEFT]]]$0^[[[RIGHT]]]
#+MACRO local #+LOCAL_MACRO $1 $2
[[[local(LEFT,])]]]
[[[local(RIGHT,[)]]]
[[[text(Hello world!)]]]
)",
                 R"(
Hello world!

]Hello world![
)");

    // macro_gokuro.gokuro
    test_gokurai(R"(
#+MACRO message The price is $0.
]]][[[message($1)]]][[[
]]][[[message($$1)]]][[[
)",
                 R"(
]]]The price is $1.[[[
]]]The price is $$1.[[[
)");

    // macro_error.gokuro
    test_gokurai(R"(
#+MACRO value 1
#+MACRO verbatim $0
#+MACRO verbatim2 $1-$2
[[[hoge]]]
[[[
]]]
[[[hoge[[[value]]]]]]
[[[value]]]]]]
[[[hoge(fugafuga]]]
[[[value(1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1)]]]
[[[verbatim(,,,,,)]]]
[[[verbatim2(,,,,,)]]]
[[[verbatim2((),())]]]
[[[verbatim([[][]])]]]
]]][[[value]]][[[
[[[verbatim(FOO]]]
)",
                 R"(

[[[
]]]

1]]]

1
,,,,,
-
()-()
[[][]]
]]]1[[[

)");

    // macro_escaped_comma.gokuro
    test_gokurai(R"(
#+MACRO macro $1|$2|$3
#+MACRO verbatim $0
]]][[[macro(1\,2,3\,4,5\,6)]]][[[
[[[verbatim(\,)]]]
)",
                 R"(
]]]1,2|3,4|5,6[[[
\,
)");

    // macro_japanese.gokuro
    test_gokurai(R"(
[[[こんにちは]]]
[[[無理難題]]]
)",
                 R"(


)");

    // math.gokuro
    test_gokurai(R"(
#+MACRO math $$0$
[[[math(x)]]]
[[[math(1)]]]
[[[math(10)]]]
[[[math(100)]]]
#+MACRO math1 $$1$
[[[math1(1)]]]
[[[math1(2)]]]
#+MACRO math2 $$2$
[[[math2(FOO,1)]]]
[[[math2(FOO,2)]]]
#+MACRO mathmath $$$0$$
[[[mathmath(x)]]]
[[[mathmath(1)]]]
[[[mathmath(10)]]]
[[[mathmath(100)]]]
#+MACRO dollardollar $$
[[[dollardollar]]]
)",
                 R"(
$x$
$1$
$10$
$100$
$1$
$2$
$1$
$2$
$$x$$
$$1$$
$$10$$
$$100$$
$$
)");
}

ix_TEST_CASE("gokurai: lua")
{
    // With return.
    test_gokurai("[[[__LUA__(return 3)]]]", "3");
    test_gokurai("[[[__LUA__(return 1 + 2)]]]", "3");
    test_gokurai("[[[__LUA__(return 1 + 2;)]]]", "3");
    test_gokurai("[[[__LUA__(return math.abs(-1);)]]]", "1");
    test_gokurai("[[[__LUA__(x = 1;)]]]\n"
                 "[[[__LUA__(return x;)]]]\n",
                 "\n"
                 "1\n");

    // Without return.
    test_gokurai("[[[__LUA__(3)]]]", "3");
    test_gokurai("[[[__LUA__(1 + 2)]]]", "3");
    test_gokurai("[[[__LUA__(math.abs(-1))]]]", "1");
    test_gokurai("[[[__LUA__(x = 1;)]]]\n"
                 "[[[__LUA__(x)]]]\n",
                 "\n"
                 "1\n");

    // Returned value is not convertible to a string.
    test_gokurai("[[[__LUA__()]]]", "");
    test_gokurai("[[[__LUA__(return nil;)]]]", "");
    test_gokurai("[[[__LUA__(return {1,2,3};)]]]", "");

    // Parse error.
    {
        ix_TempFileW out;
        ix_TempFileW err;
        const char *src = "[[[__LUA__(return 1 + ;)]]]";
        gokurai(src, ix_strlen(src), &out.file_handle(), &err.file_handle());
        ix_EXPECT_EQSTR(out.data(), "[string \"return 1 + ;\"]:1: unexpected symbol near ';'");
        ix_EXPECT_EQSTR(err.data(), "Lua load failed: [string \"return 1 + ;\"]:1: unexpected symbol near ';'\n");
    }

    // Parse error without stderr.
    {
        ix_TempFileW out;
        const char *src = "[[[__LUA__(return 1 + ;)]]]";
        gokurai(src, ix_strlen(src), &out.file_handle(), nullptr);
        ix_EXPECT_EQSTR(out.data(), "[string \"return 1 + ;\"]:1: unexpected symbol near ';'");
    }

    // Eval error.
    {
        ix_TempFileW out;
        ix_TempFileW err;
        const char *src = "[[[__LUA__(return nil + nil;)]]]";
        gokurai(src, ix_strlen(src), &out.file_handle(), &err.file_handle());
        ix_EXPECT_EQSTR(out.data(), "[string \"return nil + nil;\"]:1: attempt to perform arithmetic on a nil value");
        ix_EXPECT_EQSTR(err.data(),
                        "Lua evaluation failed: "
                        "[string \"return nil + nil;\"]:1: attempt to perform arithmetic on a nil value\n");
    }

    // Eval error without stderr.
    {
        ix_TempFileW out;
        const char *src = "[[[__LUA__(return nil + nil;)]]]";
        gokurai(src, ix_strlen(src), &out.file_handle(), nullptr);
        ix_EXPECT_EQSTR(out.data(), "[string \"return nil + nil;\"]:1: attempt to perform arithmetic on a nil value");
    }
}

ix_TEST_CASE("gokurai: global block macro")
{
    // Basic global block macro definition.
    test_gokurai("#+MACRO_BEGIN foo\n"
                 "#+MACRO_END\n",
                 "");
    test_gokurai("#+MACRO_BEGIN foo\n"
                 "#+MACRO_END:",
                 "");

    // Ill-formed global block macro definition.
    test_gokurai("#+MACRO_BEGIN foo\n"
                 "bar\n",
                 "");
    test_gokurai("#+MACRO_BEGIN foo\n", "");
    test_gokurai("#+MACRO_BEGIN \n"
                 "foo\n"
                 "bar\n",
                 "");

    // Global block macro definition between normal lines.
    test_gokurai("foo\n"
                 "#+MACRO_BEGIN macro\n"
                 "#+MACRO_END\n"
                 "bar\n",
                 "foo\n"
                 "bar\n");

    // Long global block macro definition.
    test_gokurai("foo\n"
                 "#+MACRO_BEGIN macro\n"
                 "one\n"
                 "two\n"
                 "three\n"
                 "#+MACRO_END\n"
                 "bar\n",
                 "foo\n"
                 "bar\n");

    // Call an empty global block macro.
    test_gokurai("#+MACRO_BEGIN macro\n"
                 "#+MACRO_END\n"
                 "[[[macro]]]\n",
                 "\n");

    // Call a normal global block macro.
    test_gokurai("#+MACRO_BEGIN macro\n"
                 "one\n"
                 "two\n"
                 "three\n"
                 "#+MACRO_END\n"
                 "[[[macro]]]\n",
                 "one\n"
                 "two\n"
                 "three\n");

    // Call an empty global block macro multiple times.
    test_gokurai("#+MACRO_BEGIN macro\n"
                 "#+MACRO_END\n"
                 "foo[[[macro]]][[[macro]]][[[macro]]][[[macro]]][[[macro]]][[[macro]]]bar\n",
                 "foobar\n");

    // Call an normal global block macro multiple times.
    test_gokurai("#+MACRO_BEGIN macro\n"
                 "one\n"
                 "two\n"
                 "three\n"
                 "#+MACRO_END\n"
                 "foo[[[macro]]][[[macro]]]bar\n",
                 "fooone\n"
                 "two\n"
                 "threeone\n"
                 "two\n"
                 "threebar\n");
    test_gokurai("#+MACRO_BEGIN macro\n"
                 "one\n"
                 "two\n"
                 "three\n"
                 "#+MACRO_END\n"
                 "foo\n"
                 "[[[macro]]]\n"
                 "[[[macro]]]\n"
                 "bar\n",
                 "foo\n"
                 "one\n"
                 "two\n"
                 "three\n"
                 "one\n"
                 "two\n"
                 "three\n"
                 "bar\n");
    test_gokurai("#+MACRO_BEGIN macro\n"
                 "one\n"
                 "two\n"
                 "three\n"
                 "#+MACRO_END\n"
                 "foo\n"
                 "[[[macro]]]XXXXXXXXXXXXXXXXXXXXXXX[[[macro]]]\n"
                 "bar\n",
                 "foo\n"
                 "one\n"
                 "two\n"
                 "threeXXXXXXXXXXXXXXXXXXXXXXXone\n"
                 "two\n"
                 "three\n"
                 "bar\n");

    // Call a global block macro from a global block macro.
    test_gokurai("#+MACRO_BEGIN macro1\n"
                 "one\n"
                 "two\n"
                 "three\n"
                 "#+MACRO_END\n"
                 "#+MACRO_BEGIN macro2\n"
                 "zero\n"
                 "[[[macro1]]]\n"
                 "four\n"
                 "#+MACRO_END\n"
                 "[[[macro2]]]\n",
                 "zero\n"
                 "one\n"
                 "two\n"
                 "three\n"
                 "four\n");

    // Global block macro right after a local macro.
    test_gokurai("#+LOCAL_MACRO foo FOO\n"
                 "#+MACRO_BEGIN macro\n"
                 "one\n"
                 "two\n"
                 "three\n"
                 "#+MACRO_END\n"
                 "[[[foo]]]",
                 "");

    // Macro definition in a global block macro.
    //   Local macro.
    test_gokurai("#+MACRO_BEGIN macro\n"
                 "one\n"
                 "#+LOCAL_MACRO two 222\n"
                 "^[[[two]]]\n"
                 "three\n"
                 "#+MACRO_END\n"
                 "[[[macro]]]\n",
                 "one\n"
                 "222\n"
                 "three\n");
    //   Global macro.
    test_gokurai("#+MACRO_BEGIN macro\n"
                 "one\n"
                 "#+MACRO two 222\n"
                 "^[[[two]]]\n"
                 "three\n"
                 "#+MACRO_END\n"
                 "[[[macro]]]\n"
                 "[[[two]]]\n",
                 "one\n"
                 "222\n"
                 "three\n"
                 "222\n");
    //   Global block macro.
    test_gokurai("#+MACRO_BEGIN macro\n"
                 "one\n"
                 "#+MACRO_BEGIN two\n"
                 "2\n"
                 "2\n"
                 "2\n"
                 "#+MACRO_END\n"
                 "^[[[two]]]\n"
                 "three\n"
                 "#+MACRO_END\n"
                 "[[[macro]]]\n"
                 "[[[two]]]\n",
                 "one\n"
                 "2\n"
                 "2\n"
                 "2\n"
                 "three\n"
                 "2\n"
                 "2\n"
                 "2\n");
    //   Local block macro.
    test_gokurai("#+MACRO_BEGIN macro\n"
                 "one\n"
                 "#+LOCAL_MACRO_BEGIN two\n"
                 "2\n"
                 "2\n"
                 "2\n"
                 "#+LOCAL_MACRO_END\n"
                 "^[[[two]]]\n"
                 "three\n"
                 "#+MACRO_END\n"
                 "[[[macro]]]\n"
                 "[[[two]]]\n",
                 "one\n"
                 "2\n"
                 "2\n"
                 "2\n"
                 "three\n"
                 "\n");

    // Global block macro with argumetns.
    test_gokurai("#+MACRO_BEGIN macro\n"
                 "$1\n"
                 "$2-$2\n"
                 "$3-$3-$3\n"
                 "#+MACRO_END\n"
                 "[[[macro(1,22,333)]]]\n"
                 "[[[macro(1,22,333)]]]\n",
                 "1\n"
                 "22-22\n"
                 "333-333-333\n"
                 "1\n"
                 "22-22\n"
                 "333-333-333\n");
}

ix_TEST_CASE("gokurai: local block macro")
{
    // Basic local block macro definition.
    test_gokurai("#+LOCAL_MACRO_BEGIN foo\n"
                 "#+LOCAL_MACRO_END\n",
                 "");
    test_gokurai("#+LOCAL_MACRO_BEGIN foo\n"
                 "#+LOCAL_MACRO_END:",
                 "");

    // Ill-formed local block macro definition.
    test_gokurai("#+LOCAL_MACRO_BEGIN foo\n"
                 "bar\n",
                 "");
    test_gokurai("#+LOCAL_MACRO_BEGIN foo\n", "");
    test_gokurai("#+LOCAL_MACRO_BEGIN \n"
                 "foo\n"
                 "bar\n",
                 "");

    // Local macro definition between normal lines.
    test_gokurai("foo\n"
                 "#+LOCAL_MACRO_BEGIN macro\n"
                 "#+LOCAL_MACRO_END\n"
                 "bar\n",
                 "foo\n"
                 "bar\n");

    // Long long block macro definition.
    test_gokurai("foo\n"
                 "#+LOCAL_MACRO_BEGIN macro\n"
                 "one\n"
                 "two\n"
                 "three\n"
                 "#+LOCAL_MACRO_END\n"
                 "bar\n",
                 "foo\n"
                 "bar\n");

    // Call an empty local block macro.
    test_gokurai("#+LOCAL_MACRO_BEGIN macro\n"
                 "#+LOCAL_MACRO_END\n"
                 "[[[macro]]]\n",
                 "\n");

    // Call a normal local block macro.
    test_gokurai("#+LOCAL_MACRO_BEGIN macro\n"
                 "one\n"
                 "two\n"
                 "three\n"
                 "#+LOCAL_MACRO_END\n"
                 "[[[macro]]]\n",
                 "one\n"
                 "two\n"
                 "three\n");

    // Call an empty local block macro multiple times.
    test_gokurai("#+LOCAL_MACRO_BEGIN macro\n"
                 "#+LOCAL_MACRO_END\n"
                 "foo[[[macro]]][[[macro]]][[[macro]]][[[macro]]][[[macro]]][[[macro]]]bar\n",
                 "foobar\n");

    // Call an normal block macro multiple times.
    test_gokurai("#+LOCAL_MACRO_BEGIN macro\n"
                 "one\n"
                 "two\n"
                 "three\n"
                 "#+LOCAL_MACRO_END\n"
                 "foo[[[macro]]][[[macro]]]bar\n",
                 "fooone\n"
                 "two\n"
                 "threeone\n"
                 "two\n"
                 "threebar\n");
    test_gokurai("#+LOCAL_MACRO_BEGIN macro\n"
                 "one\n"
                 "two\n"
                 "three\n"
                 "#+LOCAL_MACRO_END\n"
                 "[[[macro]]]\n"
                 "[[[macro]]]\n"
                 "bar\n",
                 "one\n"
                 "two\n"
                 "three\n"
                 "\n"
                 "bar\n");

    // Call a local block macro from a global block macro.
    test_gokurai("#+MACRO_BEGIN macro1\n"
                 "one\n"
                 "two\n"
                 "three\n"
                 "#+MACRO_END\n"
                 "#+LOCAL_MACRO_BEGIN macro2\n"
                 "zero\n"
                 "[[[macro1]]]\n"
                 "four\n"
                 "#+LOCAL_MACRO_END\n"
                 "[[[macro2]]]\n",
                 "zero\n"
                 "one\n"
                 "two\n"
                 "three\n"
                 "four\n");

    // Call a local block macro from a local block macro.
    test_gokurai("#+LOCAL_MACRO_BEGIN macro1\n"
                 "one\n"
                 "two\n"
                 "three\n"
                 "#+LOCAL_MACRO_END\n"
                 "#+LOCAL_MACRO_BEGIN macro2\n"
                 "[[[macro1]]]\n"
                 "four\n"
                 "#+LOCAL_MACRO_END\n"
                 "[[[macro2]]]\n",
                 "one\n"
                 "two\n"
                 "three\n"
                 "four\n");

    // Local block macro right after a local macro.
    test_gokurai("#+LOCAL_MACRO foo FOO\n"
                 "#+LOCAL_MACRO_BEGIN macro\n"
                 "one\n"
                 "two\n"
                 "three\n"
                 "#+LOCAL_MACRO_END\n"
                 "[[[foo]]]",
                 "FOO");

    // Macro definition in a local block macro.
    //   Local macro.
    test_gokurai("#+LOCAL_MACRO_BEGIN macro\n"
                 "one\n"
                 "#+LOCAL_MACRO two 222\n"
                 "^[[[two]]]\n"
                 "three\n"
                 "#+LOCAL_MACRO_END\n"
                 "[[[macro]]]\n",
                 "one\n"
                 "222\n"
                 "three\n");
    //   Global macro.
    test_gokurai("#+LOCAL_MACRO_BEGIN macro\n"
                 "one\n"
                 "#+MACRO two 222\n"
                 "^[[[two]]]\n"
                 "three\n"
                 "#+LOCAL_MACRO_END\n"
                 "[[[macro]]]\n"
                 "[[[two]]]\n",
                 "one\n"
                 "222\n"
                 "three\n"
                 "222\n");
    //   Global block macro.
    test_gokurai("#+LOCAL_MACRO_BEGIN macro\n"
                 "one\n"
                 "#+MACRO_BEGIN two\n"
                 "2\n"
                 "2\n"
                 "2\n"
                 "#+MACRO_END\n"
                 "^[[[two]]]\n"
                 "three\n"
                 "#+LOCAL_MACRO_END\n"
                 "[[[macro]]]\n"
                 "[[[two]]]\n",
                 "one\n"
                 "2\n"
                 "2\n"
                 "2\n"
                 "three\n"
                 "2\n"
                 "2\n"
                 "2\n");
    //   Local block macro.
    test_gokurai("#+LOCAL_MACRO_BEGIN macro\n"
                 "one\n"
                 "#+LOCAL_MACRO_BEGIN two\n"
                 "2\n"
                 "2\n"
                 "2\n"
                 "#+LOCAL_MACRO_END\n"
                 "^[[[two]]]\n"
                 "three\n"
                 "#+LOCAL_MACRO_END\n"
                 "[[[macro]]]\n"
                 "[[[two]]]\n",
                 "one\n"
                 "2\n"
                 "2\n"
                 "2\n"
                 "three\n"
                 "\n");

    // Local macro with argumetns.
    test_gokurai("#+LOCAL_MACRO_BEGIN macro\n"
                 "$1\n"
                 "$2-$2\n"
                 "$3-$3-$3\n"
                 "#+LOCAL_MACRO_END\n"
                 "[[[macro(1,22,333)]]]\n"
                 "[[[macro(1,22,333)]]]\n",
                 "1\n"
                 "22-22\n"
                 "333-333-333\n"
                 "\n");
}

ix_TEST_CASE("gokurai: macro with an empty name")
{
    test_gokurai("#+MACRO_BEGIN \n"
                 "two\n"
                 "three\n"
                 "#+MACRO_END\n"
                 "one\n"
                 "[[[]]]\n"
                 "four\n",
                 "one\n"
                 "two\n"
                 "three\n"
                 "four\n");

    test_gokurai("#+LOCAL_MACRO_BEGIN \n"
                 "foo\n"
                 "bar\n"
                 "#+LOCAL_MACRO_END\n"
                 "[[[]]]\n",
                 "foo\n"
                 "bar\n");
}

ix_TEST_CASE("gokurai: macro with an empty body")
{
    test_gokurai("#+MACRO empty \n"
                 "foo\n"
                 "[[[empty]]]\n"
                 "bar\n",
                 "foo\n"
                 "\n"
                 "bar\n");

    test_gokurai("#+LOCAL_MACRO empty \n"
                 "[[[empty]]]\n"
                 "bar\n",
                 "\n"
                 "bar\n");
}

ix_TEST_CASE("gokurai: lua program returns multiline string")
{
    test_gokurai("[[[__LUA__(return \"foo\\nbar\\n\")]]][[[__LUA__(return \"foo\\nbar\\n\")]]]\n", //
                 "foo\n"
                 "bar\n"
                 "foo\n"
                 "bar\n"
                 "\n");

    test_gokurai("[[[__LUA__(return \"#+MACRO bar BAR\\nfoo\")]]]\n"
                 "[[[bar]]]\n",
                 "foo\n"
                 "BAR\n");
}

ix_TEST_CASE("gokurai: lua block")
{
    // Basic lua block.
    test_gokurai(R"(#+LUA_BEGIN
x = "hello";
y = "world";
#+LUA_END
[[[__LUA__(x .. " " .. y)]]]
)",
                 "hello world\n");
    test_gokurai(R"(#+LUA_BEGIN
x = "hello";
y = "world";
return x .. " " .. y;
#+LUA_END
)",
                 "hello world\n");

    // Lua block within a block macro.
    test_gokurai(R"(#+MACRO_BEGIN macro
foo
#+LUA_BEGIN
x = "hello";
y = "world";
return x .. " " .. y;
#+LUA_END
bar
#+MACRO_END
[[[macro]]]
)",
                 "foo\n"
                 "hello world\n"
                 "bar\n");

    // Lua block with macro calls.
    test_gokurai(R"(#+MACRO foo FOO
#+LUA_BEGIN
x = "hello";
y = "world";
return x .. " [[[foo]]] " .. y;
#+LUA_END
)",
                 "hello FOO world\n");

    // Unterminated lua block.
    test_gokurai(R"(
#+LUA_BEGIN
x = "hello";
y = "world";
return x .. y;
)",
                 R"(
)");
}

ix_TEST_CASE("gokurai: quoted macro calls")
{
    // Quoted macro call starts.
    test_gokurai("'[[[bar]]]\n", "[[[bar]]]\n");
    test_gokurai("foo'[[[bar]]]foo\n", "foo[[[bar]]]foo\n");
    test_gokurai("'[[[bar]]]\n"
                 "'[[[bar]]]\n",
                 "[[[bar]]]\n"
                 "[[[bar]]]\n");
    test_gokurai("'[[[bar]]]'[[[bar]]]\n", "[[[bar]]][[[bar]]]\n");
    test_gokurai("'[[[bar]]]'[[[bar]]]'[[[bar]]]'[[[bar]]]'[[[bar]]]'[[[bar]]]\n",
                 "[[[bar]]][[[bar]]][[[bar]]][[[bar]]][[[bar]]][[[bar]]]\n");
    test_gokurai("'[[[bar]]][[[bar]]]\n", "[[[bar]]]\n");

    // Quoted macro call ends.
    test_gokurai("[[[bar']]]\n", "[[[bar]]]\n");
    test_gokurai("foo'[[[bar']]]foo\n", "foo[[[bar]]]foo\n");
    test_gokurai("[[[bar']]]\n"
                 "[[[bar']]]\n",
                 "[[[bar]]]\n"
                 "[[[bar]]]\n");
    test_gokurai("[[[bar']]][[[bar']]]\n", "[[[bar]]][[[bar]]]\n");

    test_gokurai("[[[bar']]][[[bar']]][[[bar']]][[[bar']]][[[bar']]][[[bar']]]\n",
                 "[[[bar]]][[[bar]]][[[bar]]][[[bar]]][[[bar]]][[[bar]]]\n");
    test_gokurai("[[[bar']]][[[bar]]]\n", "[[[bar]]]\n");

    // Macro calls with quotes at both sides.
    test_gokurai("'[[[bar']]]\n", "[[[bar]]]\n");
    test_gokurai("'[[[bar']]]'[[[bar']]]\n", "[[[bar]]][[[bar]]]\n");
    test_gokurai("[[[bar]]]'[[[bar']]][[[bar]]]\n", "[[[bar]]]\n");

    // Doubly quoted macro starts/ends.
    test_gokurai("''[[[bar]]]\n", "'[[[bar]]]\n");
    test_gokurai("[[[bar'']]]\n", "[[[bar']]]\n");

    // Ill-formed quoted macro calls.
    test_gokurai("'[[[bar\n", "[[[bar\n");
    test_gokurai("'[[[bar]]]]]]\n", "[[[bar]]]]]]\n");
    test_gokurai("[['[[[bar]]]\n", "[[[[[bar]]]\n");
    test_gokurai("[[[foo']]]]", "[[[foo]]]]");
    test_gokurai("[[[foo']]]]]", "[[[foo]]]]]");
    test_gokurai("[[[foo']]]]]]", "");
    test_gokurai("[[[foo']]]]]]]]]", "]]]");
    test_gokurai("'[[[[foo]]]", "[[[[foo]]]");
    test_gokurai("'[[[[[foo]]]", "[[[[[foo]]]");
    test_gokurai("'[[[[[[foo]]]", "[[[");
    test_gokurai("'[[[[[[[[[foo]]]", "[[[[[[");
    test_gokurai("[[[foo']]]]]]foo]]]", "foo]]]");

    // Macro calls that have quoted macro calls.
    test_gokurai("#+MACRO foo FOO-$1-FOO\n"
                 "[[[foo('[[[bar']]])]]]",
                 "FOO-[[[bar]]]-FOO");
    test_gokurai("#+MACRO foo FOO-$1-FOO\n"
                 "'[[[bar('[[[foo('[[[bar']]])']]])']]]",
                 "[[[bar([[[foo([[[bar]]])]]])]]]");
    test_gokurai("#+MACRO foo FOO-$1-FOO\n"
                 "[[[foo('[[[bar']]])]]]",
                 "FOO-[[[bar]]]-FOO");
    test_gokurai("#+MACRO foo FOO-$1-FOO\n"
                 "[[[foo('[[[)]]]",
                 "FOO-[[[-FOO");
    test_gokurai("#+MACRO foo FOO-$1-FOO\n"
                 "[[[foo(']]])]]]",
                 "FOO-]]]-FOO");

    // Quoted macro calls in macro definitions.
    test_gokurai("#+MACRO foo '[[[bar]]]\n"
                 "[[[foo]]]\n",
                 "[[[bar]]]\n");
}

ix_TEST_CASE("gokurai: quoted directives")
{
    test_gokurai("'#+MACRO foo FOO\n"
                 "''#+MACRO foo FOO\n"
                 "'''#+MACRO foo FOO\n",
                 "#+MACRO foo FOO\n"
                 "'#+MACRO foo FOO\n"
                 "''#+MACRO foo FOO\n");

    test_gokurai("'#+MACRO ", "#+MACRO ");
    test_gokurai("'#+MACRO_BEGIN ", "#+MACRO_BEGIN ");
    test_gokurai("'#+MACRO_END\n", "#+MACRO_END\n");
    test_gokurai("'#+LOCAL_MACRO ", "#+LOCAL_MACRO ");
    test_gokurai("'#+LOCAL_MACRO_BEGIN ", "#+LOCAL_MACRO_BEGIN ");
    test_gokurai("'#+LOCAL_MACRO_END\n", "#+LOCAL_MACRO_END\n");
    test_gokurai("'#+LUA_BEGIN", "#+LUA_BEGIN");
    test_gokurai("'#+LUA_END\n", "#+LUA_END\n");
    test_gokurai("'#+COMMENT_BEGIN", "#+COMMENT_BEGIN");
    test_gokurai("'#+COMMENT_END\n", "#+COMMENT_END\n");

    test_gokurai("''#+MACRO ", "'#+MACRO ");
    test_gokurai("''#+MACRO_BEGIN ", "'#+MACRO_BEGIN ");
    test_gokurai("''#+MACRO_END\n", "'#+MACRO_END\n");
    test_gokurai("''#+LOCAL_MACRO ", "'#+LOCAL_MACRO ");
    test_gokurai("''#+LOCAL_MACRO_BEGIN ", "'#+LOCAL_MACRO_BEGIN ");
    test_gokurai("''#+LOCAL_MACRO_END\n", "'#+LOCAL_MACRO_END\n");
    test_gokurai("''#+LUA_BEGIN", "'#+LUA_BEGIN");
    test_gokurai("''#+LUA_END\n", "'#+LUA_END\n");
    test_gokurai("''#+COMMENT_BEGIN", "'#+COMMENT_BEGIN");
    test_gokurai("''#+COMMENT_END\n", "'#+COMMENT_END\n");
}

ix_TEST_CASE("gokurai: tricky cases")
{
    test_gokurai("#+LOCAL_MACRO foo FOO\n"
                 "foo[[[foo]]]]foo]]]",
                 "fooFOO]foo]]]");

    test_gokurai("]]]]]]]]]]]]]]]]]]]]", "]]]]]]]]]]]]]]]]]]]]");
    test_gokurai("[[[[[[[[[[[[[[[[[[[[", "[[[[[[[[[[[[[[[[[[[[");
}

ix_TEST_CASE("gokurai: no newline")
{
    test_gokurai("[[[__NO_NEWLINE__]]]", "");
    test_gokurai("[[[__NO_NEWLINE__]]]\n", "\n");

    test_gokurai("hello [[[__NO_NEWLINE__]]]\n"
                 "world\n",
                 "hello world\n");

    test_gokurai("foo[[[__NO_NEWLINE__]]]\n"
                 "bar[[[__NO_NEWLINE__]]]\n"
                 "foo[[[__NO_NEWLINE__]]]\n"
                 "bar\n",
                 "foobarfoobar\n");

    test_gokurai("foo[[[__NO_NEWLINE__]]]\n"
                 "bar[[[__NO_NEWLINE__]]]\n"
                 "foo[[[__NO_NEWLINE__]]]\n"
                 "bar[[[__NO_NEWLINE__]]]\n",
                 "foobarfoobar\n");

    test_gokurai(R"(#+MACRO_BEGIN foo
FOO[[[__NO_NEWLINE__]]]
BAR[[[__NO_NEWLINE__]]]
FOO[[[__NO_NEWLINE__]]]
BAR
#+MACRO_END
[[[foo]]]
)",
                 "FOOBARFOOBAR\n");

    test_gokurai(R"(#+MACRO_BEGIN foo
FOO[[[__NO_NEWLINE__]]]
BAR[[[__NO_NEWLINE__]]]
FOO[[[__NO_NEWLINE__]]]
BAR
#+MACRO_END
[[[foo]]][[[foo]]]
)",
                 "FOOBARFOOBARFOOBARFOOBAR\n");
}

ix_TEST_CASE("gokurai: comment block")
{
    // Basic comment blocks.
    test_gokurai("#+COMMENT_BEGIN\n"
                 "#+COMMENT_END\n",
                 "");
    test_gokurai("#+COMMENT_BEGIN\n"
                 "FOO\n"
                 "BAR\n"
                 "#+COMMENT_END\n",
                 "");
    test_gokurai("foo\n"
                 "#+COMMENT_BEGIN\n"
                 "FOO\n"
                 "BAR\n"
                 "#+COMMENT_END\n"
                 "bar\n",
                 "foo\n"
                 "bar\n");

    // Unterminated comment block.
    test_gokurai("foo\n"
                 "#+COMMENT_BEGIN\n"
                 "FOO\n"
                 "BAR\n",
                 "foo\n");
    test_gokurai("foo\n"
                 "#+COMMENT_BEGIN\n"
                 "FOO\n"
                 "BAR",
                 "foo\n");

    // Comment block in a multiline macro.
    test_gokurai("#+MACRO_BEGIN macro\n"
                 "foo\n"
                 "#+COMMENT_BEGIN\n"
                 "FOO\n"
                 "BAR\n"
                 "#+COMMENT_END\n"
                 "bar\n"
                 "#+MACRO_END\n"
                 "[[[macro]]]\n",
                 "foo\n"
                 "bar\n");

    // Comment block in a lua block.
    test_gokurai(R"(#+LUA_BEGIN
x = "hello";
#+COMMENT_BEGIN
FOO
BAR
#+COMMENT_END
y = "world";
return x .. " " .. y;
#+LUA_END
)",
                 "hello world\n");

    // Nested comment block.
    test_gokurai(R"(#+COMMENT_BEGIN
foo
#+COMMENT_BEGIN
FOO
BAR
#+COMMENT_END
bar
#+COMMENT_END
hello world
)",
                 "hello world\n");

    // Comment block involving macros.
    test_gokurai(R"(#+MACRO begin #+COMMENT_BEGIN
#+MACRO end #+COMMENT_END
[[[begin]]]
foo
[[[begin]]]
FOO
BAR
[[[end]]]
bar
[[[end]]]
hello world
)",
                 "hello world\n");

    // Lazy macros in a comment block.
    test_gokurai(R"(#+MACRO begin #+COMMENT_BEGIN
#+MACRO end #+COMMENT_END
[[[begin]]]
foo
^[[[begin]]]
FOO
BAR
[[[end]]]
bar
^[[[end]]]
hello world
)",
                 "hello world\n");
}

ix_TEST_CASE("gokurai: gokurai_str")
{
    GokuraiResultImpl result0 = gokurai_str("hello world");
    ix_EXPECT_EQSTR(result0.data().get(), "hello world");
    ix_EXPECT(result0.size() == 11);
    result0 = gokurai_str("hello gokurai");
    ix_EXPECT_EQSTR(result0.data().get(), "hello gokurai");
    ix_EXPECT(result0.size() == ix_strlen("hello gokurai"));

    GokuraiResultImpl result1 = ix_move(result0);
    result1 = ix_move(result1);

    GokuraiResultImpl result2(ix_move(result1));

    ix_UniquePointer<char[]> output = result2.detach();
    ix_EXPECT_EQSTR(output.get(), "hello gokurai");
}

ix_TEST_CASE("gokurai: lua block inside multiline macro")
{
    test_gokurai(R"(#+MACRO_BEGIN term
#+LUA_BEGIN
if "$0" == "xxx" then
  return "FOOFOO"
else
  return "$0" .. "$0"
end
#+LUA_END
#+MACRO_END
[[[term(FOO)]]]
[[[term(xxx)]]]
)",
                 "FOOFOO\n"
                 "FOOFOO\n");
}

ix_TEST_CASE("gokurai: lazy macro call")
{
    // Short lazy macro.
    test_gokurai(R"(#+MACRO foo FOOFOO
[[[foo]]]
^[[[foo]]]
[[[foo]]]^[[[foo]]][[[foo]]]^[[[foo]]]
)",
                 "FOOFOO\n"
                 "FOOFOO\n"
                 "FOOFOOFOOFOOFOOFOOFOOFOO\n");

    // Long lazy macro.
    test_gokurai(R"(#+MACRO foo FOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOO
[[[foo]]]
^[[[foo]]]
[[[foo]]]^[[[foo]]][[[foo]]]^[[[foo]]]
)",
                 "FOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOO\n"
                 "FOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOO\n"
                 "FOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOO"
                 "FOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOO"
                 "FOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOO"
                 "FOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOO\n");

    // Multiline, short lazy macro.
    test_gokurai(R"(#+MACRO_BEGIN foo
foo 1
foo 2
foo 3
#+MACRO_END
^[[[foo]]]
)",
                 "foo 1\n"
                 "foo 2\n"
                 "foo 3\n");

    // Multiline, long lazy macro.
    test_gokurai(R"(#+MACRO_BEGIN foo
foo 1
foo 22222222222222222
foo 333333333333333333333333333333333333333333333333333333333
#+MACRO_END
^[[[foo]]]
)",
                 "foo 1\n"
                 "foo 22222222222222222\n"
                 "foo 333333333333333333333333333333333333333333333333333333333\n");

    // Lazy macros are expanded on the call site.
    test_gokurai(R"(#+MACRO call_foo '[[[foo]]] is expanded to "^[[[foo]]]".
[[[call_foo]]]
#+MACRO foo FOOFOO
[[[call_foo]]]
#+LOCAL_MACRO foo LOCAL_FOOFOO
[[[call_foo]]]
)",
                 R"([[[foo]]] is expanded to "".
[[[foo]]] is expanded to "FOOFOO".
[[[foo]]] is expanded to "LOCAL_FOOFOO".
)");

    // Lazy macros are expanded on the call site (complex).
    test_gokurai(R"(#+MACRO explain '[[[$0]]] is expanded to "^[[[$0]]]".
[[[explain(foo)]]]
#+MACRO foo FOOFOO
[[[explain(foo)]]]
#+LOCAL_MACRO foo LOCAL_FOOFOO
[[[explain(foo)]]]
#+LOCAL_MACRO foo foo-$1-$2-$3-foo
[[[explain(foo(one,two,three))]]]
[[[explain(explain(foo))]]]
)",
                 R"([[[foo]]] is expanded to "".
[[[foo]]] is expanded to "FOOFOO".
[[[foo]]] is expanded to "LOCAL_FOOFOO".
[[[foo(one,two,three)]]] is expanded to "foo-one-two-three-foo".
[[[explain(foo)]]] is expanded to "[[[foo]]] is expanded to "FOOFOO".".
)");

    // Lazy macros with lua code.
    test_gokurai(R"(#+MACRO counter [[[__LUA__(count = 0)]]]^[[[__LUA__(count = count + 1; return count;)]]]
[[[counter]]]
[[[counter]]]
[[[counter]]]
[[[counter]]]
[[[counter]]]-[[[counter]]]-[[[counter]]]-[[[counter]]]
[[[counter]]]-^[[[counter]]]-[[[counter]]]-^[[[counter]]]
^[[[counter]]]-[[[counter]]]-^[[[counter]]]-[[[counter]]]
)",
                 R"(1
2
3
4
8-7-6-5
12-11-10-9
16-15-14-13
)");

    // Lazy macros within lua code.
    test_gokurai(R"(#+MACRO counter [[[__LUA__(count = 0)]]]^[[[__LUA__(count = count + 1; return count;)]]]
^[[[__LUA__("[[[counter]]]" .. "-" .. "[[[counter]]]")]]]
#+LUA_BEGIN
"[[[counter]]]" .. "-" .. "[[[counter]]]";
#+LUA_END
#+LUA_BEGIN
"^[[[counter]]]" .. "-" .. "[[[counter]]]";
#+LUA_END
)",
                 R"(2-1
4-3
6-5
)");

    // Not lazy macro
    test_gokurai(R"(
#+MACRO foo foo-$1-foo
[[[foo(^[[)]]]
)",
                 R"(
foo-^[[-foo
)");
}

ix_TEST_CASE("gokurai: nested lazy macro")
{
    test_gokurai(R"(#+MACRO foo_0 ^[[[FOO]]]
#+MACRO foo_1 [[[foo_0]]]
#+MACRO foo_2 [[[foo_1]]]
#+MACRO foo_3 [[[foo_2]]]
#+MACRO foo_4 [[[foo_3]]]
[[[foo_4]]]
#+MACRO FOO FOOFOO
[[[foo_4]]]
#+LOCAL_MACRO FOO LOCAL_FOOFOO
[[[foo_4]]]
#+MACRO FOO FOOFOO_AGAIN
[[[foo_4]]]
)",
                 R"(
FOOFOO
LOCAL_FOOFOO
FOOFOO_AGAIN
)");

    test_gokurai(R"(#+MACRO foo_0 ^[[[FOO]]]
#+MACRO foo_1 ^[[[foo_0]]]
#+MACRO foo_2 ^[[[foo_1]]]
#+MACRO foo_3 ^[[[foo_2]]]
#+MACRO foo_4 ^[[[foo_3]]]
[[[foo_4]]]
#+MACRO FOO FOOFOO
[[[foo_4]]]
#+LOCAL_MACRO FOO LOCAL_FOOFOO
[[[foo_4]]]
#+MACRO FOO FOOFOO_AGAIN
[[[foo_4]]]
)",
                 R"(
FOOFOO
LOCAL_FOOFOO
FOOFOO_AGAIN
)");
}

ix_TEST_CASE("gokurai: lazy macro + no newline")
{
    test_gokurai(R"(#+MACRO NOT_NEWLINE ^[[[__NO_NEWLINE__]]]
#+MACRO f FOO
#+MACRO b BAR
[[[f]]][[[NOT_NEWLINE]]]
[[[b]]][[[NOT_NEWLINE]]]
[[[f]]][[[NOT_NEWLINE]]]
[[[b]]][[[NOT_NEWLINE]]]
[[[f]]][[[NOT_NEWLINE]]]
[[[b]]]
)",
                 R"(FOOBARFOOBARFOOBAR
)");

    test_gokurai(R"(#+MACRO NOT_NEWLINE ^[[[__NO_NEWLINE__]]]
#+MACRO f FOO
#+MACRO b BAR
#+MACRO_BEGIN foobar
[[[f]]][[[NOT_NEWLINE]]]
[[[b]]][[[NOT_NEWLINE]]]
[[[f]]][[[NOT_NEWLINE]]]
[[[b]]][[[NOT_NEWLINE]]]
[[[f]]][[[NOT_NEWLINE]]]
[[[b]]]
#+MACRO_END
[[[foobar]]]
)",
                 R"(FOOBARFOOBARFOOBAR
)");

    test_gokurai(R"(#+MACRO NOT_NEWLINE ^[[[__NO_NEWLINE__]]]
#+MACRO f FOO
#+MACRO b BAR
#+MACRO_BEGIN foobar
[[[f]]][[[NOT_NEWLINE]]]
[[[b]]][[[NOT_NEWLINE]]]
^[[[f]]][[[NOT_NEWLINE]]]
[[[b]]][[[NOT_NEWLINE]]]
[[[f]]][[[NOT_NEWLINE]]]
^[[[b]]][[[NOT_NEWLINE]]]
^[[[f]]][[[NOT_NEWLINE]]]
^[[[b]]]
#+MACRO_END
#+MACRO call_foobar ^[[[foobar]]]
#+MACRO call_call_foobar [[[call_foobar]]]
[[[call_call_foobar]]]
)",
                 R"(FOOBARFOOBARFOOBARFOOBAR
)");
}

ix_TEST_CASE("gokurai: Quote lazy calls")
{
    test_gokurai("^", "^");
    test_gokurai("'^", "'^");
    test_gokurai("'^[[[FOO]]]", "'");
    test_gokurai("'^'[[[FOO]]]", "^[[[FOO]]]");
}

ix_TEST_CASE("gokurai: Lazy call inside a normal call")
{
    test_gokurai(R"(
#+MACRO foo foo-$0-foo
#+MACRO bar [[[foo(^[[[foo(BAR)]]])]]]
[[[bar]]]
)",
                 R"(
foo-foo-BAR-foo-foo
)");
}

ix_TEST_CASE("gokurai: Deeply nested lazy and normal calls")
{
    test_gokurai(R"(
#+MACRO foo foo-$0-foo
#+MACRO bar [[[foo(^[[[foo([[[foo(^[[[foo([[[foo(BAR)]]])]]])]]])]]])]]]
[[[bar]]]
#+MACRO bar ^[[[foo([[[foo(^[[[foo([[[foo(^[[[foo(BAR)]]])]]])]]])]]])]]]
[[[bar]]]
#+MACRO bar ^[[[foo(^[[[foo([[[foo([[[foo(^[[[foo(BAR)]]])]]])]]])]]])]]]
[[[bar]]]
#+MACRO bar [[[foo(^[[[foo(^[[[foo([[[foo(^[[[foo(BAR)]]])]]])]]])]]])]]]
[[[bar]]]
)",
                 R"(
foo-foo-foo-foo-foo-BAR-foo-foo-foo-foo-foo
foo-foo-foo-foo-foo-BAR-foo-foo-foo-foo-foo
foo-foo-foo-foo-foo-BAR-foo-foo-foo-foo-foo
foo-foo-foo-foo-foo-BAR-foo-foo-foo-foo-foo
)");
}

ix_TEST_CASE("gokurai: Lazy calls inside normal call")
{
    test_gokurai(R"(
#+MACRO foo foo-$0-foo
#+MACRO bar [[[foo(BAR-^[[[foo(BAR)]]]-^[[[foo(BAR)]]]-^[[[foo(BAR)]]]-BAR)]]]
[[[bar]]]
)",
                 R"(
foo-BAR-foo-BAR-foo-foo-BAR-foo-foo-BAR-foo-BAR-foo
)");
}

ix_TEST_CASE("gokurai: Lazy call followed by quoted macro close")
{
    test_gokurai(R"(
#+MACRO foo foo-$0-foo
#+MACRO bar [[[foo(^[[[foo(BAR)]]]']]])]]]
[[[bar]]]
#+MACRO bar [[[foo(^[[[foo(BAR)]]]']]]']]]]']]]]])]]]
[[[bar]]]
)",
                 R"(
foo-foo-BAR-foo]]]-foo
foo-foo-BAR-foo]]]]]]]]]]]]-foo
)");
}

ix_TEST_CASE("gokurai: Directive after lazy macro expansion")
{
    test_gokurai(R"(
#+MACRO m #+MACRO
^[[[m]]] foo foofoo
[[[foo]]]
)",
                 R"(
foofoo
)");
}

ix_TEST_CASE("gokurai: Lazy macro calls in arguments")
{
    test_gokurai(R"(
#+MACRO foo foo-$1-$2-$3-foo
#+MACRO bar BAR
[[[foo([[[bar]]],^[[[bar]]],[[[bar]]])]]]
)",
                 R"(
foo-BAR-BAR-BAR-foo
)");
}

ix_TEST_CASE("gokurai: Quoted comma in lazy macro calls in arguments")
{
    test_gokurai(R"(
#+MACRO foo foo-$1-$2-$3-foo
#+MACRO bar bar-$1,$2,$3-bar
[[[foo(BAR,^[[[bar(one,two\,three,four)]]],BAR)]]]
)",
                 R"(
foo-BAR-bar-one,two,three,four-bar-BAR-foo
)");
}

ix_TEST_CASE("gokurai: tricky lazy macro")
{
    test_gokurai(R"(
#+MACRO bar BARBAR
#+MACRO foo bar
^[[[[[[foo]]]]]]
)",
                 R"(
BARBAR
)");

    test_gokurai(R"(
#+MACRO foo bar
#+MACRO bar foobar
#+MACRO foobar foofoobarbar
#+MACRO foofoobarbar x
^[[[^[[[[[[foo]]]]]]]]]
)",
                 R"(
foofoobarbar
)");

    test_gokurai(R"(
#+MACRO bar BARBAR
#+MACRO foo bar
[[[^[[[foo]]]]]]
)",
                 R"(

)");
}

ix_TEST_CASE("gokurai: Backslashes in a marco argument")
{
    test_gokurai(R"(
#+MACRO m mmm-$1-$2-$3-mmm
[[[m(foo,\,bar)]]]
)",
                 R"(
mmm-foo-,bar--mmm
)");

    test_gokurai(R"(
#+MACRO m mmm-$1-$2-$3-mmm
[[[m(foo,\\,bar)]]]
)",
                 R"(
mmm-foo-\-bar-mmm
)");

    test_gokurai(R"(
#+MACRO m mmm-$1-$2-$3-mmm
[[[m(foo,\\\,bar)]]]
)",
                 R"(
mmm-foo-\,bar--mmm
)");

    test_gokurai(R"(
#+MACRO m mmm-$1-$2-$3-mmm
[[[m(foo,\\\\,bar)]]]
)",
                 R"(
mmm-foo-\\-bar-mmm
)");

    test_gokurai(R"(
#+MACRO m mmm-$1-$2-$3-mmm
[[[m(foo,\\\\\,bar)]]]
)",
                 R"(
mmm-foo-\\,bar--mmm
)");

    test_gokurai(R"(
#+MACRO m mmm-$1-$2-$3-mmm
[[[m(foo,a\b,bar)]]]
)",
                 R"(
mmm-foo-a\b-bar-mmm
)");

    test_gokurai(R"(
#+MACRO m mmm-$1-$2-$3-mmm
[[[m(foo,a\\b,bar)]]]
)",
                 R"(
mmm-foo-a\\b-bar-mmm
)");

    test_gokurai(R"(
#+MACRO m mmm-$1-$2-$3-mmm
[[[m(foo,a\\\b,bar)]]]
)",
                 R"(
mmm-foo-a\\\b-bar-mmm
)");

    test_gokurai(R"(
#+MACRO m mmm-$1-$2-$3-mmm
[[[m(foo,a\\\\b,bar)]]]
)",
                 R"(
mmm-foo-a\\\\b-bar-mmm
)");
}

ix_TEST_CASE("gokurai: not a directive")
{
    test_gokurai(R"(
#
#+MA
#+COMM
)",
                 R"(
#
#+MA
#+COMM
)");
}

ix_TEST_CASE("gokurai: INPUT_LINE_NUMBER")
{
    test_gokurai(R"([[[__INPUT_LINE_NUMBER__]]]
[[[__INPUT_LINE_NUMBER__]]]
[[[__INPUT_LINE_NUMBER__]]]
)",
                 R"(1
2
3
)");

    test_gokurai(R"([[[__INPUT_LINE_NUMBER__]]]
#+COMMENT_BEGIN
foo
bar
foobar
#+COMMENT_END
[[[__INPUT_LINE_NUMBER__]]]
)",
                 R"(1
7
)");

    test_gokurai(R"([[[__INPUT_LINE_NUMBER__]]]
#+MACRO_BEGIN foo
f
o
o
#+MACRO_END
[[[foo]]]
[[[__INPUT_LINE_NUMBER__]]]
[[[foo]]]
[[[__INPUT_LINE_NUMBER__]]]
)",
                 R"(1
f
o
o
8
f
o
o
10
)");
}

ix_TEST_CASE("gokurai: OUTPUT_LINE_NUMBER")
{
    test_gokurai(R"([[[__OUTPUT_LINE_NUMBER__]]]
[[[__OUTPUT_LINE_NUMBER__]]]
[[[__OUTPUT_LINE_NUMBER__]]]
)",
                 R"(1
2
3
)");

    test_gokurai(R"([[[__OUTPUT_LINE_NUMBER__]]]
#+COMMENT_BEGIN
foo
bar
foobar
#+COMMENT_END
[[[__OUTPUT_LINE_NUMBER__]]]
)",
                 R"(1
2
)");

    test_gokurai(R"([[[__OUTPUT_LINE_NUMBER__]]]
#+MACRO_BEGIN foo
f
o
o
#+MACRO_END
[[[foo]]]
[[[__OUTPUT_LINE_NUMBER__]]]
[[[foo]]]
[[[__OUTPUT_LINE_NUMBER__]]]
)",
                 R"(1
f
o
o
5
f
o
o
9
)");
}

ix_TEST_CASE("gokurai: lua code inside a comment block")
{
    test_gokurai(R"(
[[[__LUA__(x = 1)]]]
#+COMMENT_BEGIN
[[[__LUA__(x = 100)]]]
#+COMMENT_END
[[[__LUA__(x)]]]
)",
                 R"(

1
)");
}

ix_TEST_CASE("gokurai: enable/disable lua")
{
    test_gokurai(R"(
[[[__LUA__(x = 1)]]]
[[[__DISABLE_LUA__]]]
[[[__LUA__(x = 2)]]]
[[[__LUA__(x)]]]
[[[__ENABLE_LUA__]]]
[[[__LUA__(x)]]]
)",
                 R"(





1
)");

    test_gokurai(R"(
[[[__LUA__(x = 1)]]]
#+COMMENT_BEGIN
[[[__ENABLE_LUA__]]]
[[[__LUA__(x = 100)]]]
#+COMMENT_END
[[[__LUA__(x)]]]
)",
                 R"(

100
)");
}

ix_TEST_CASE("gokurai: lazy lua fragment returns a macro call")
{
    test_gokurai(R"(
#+MACRO FOO foo
^[[[__LUA__("[" .. "[[FOO]]" .. "]")]]]
)",
                 R"(
foo
)");

    test_gokurai(R"(
#+MACRO end #+COMMENT_END
#+COMMENT_BEGIN
foo
bar
^[[[__LUA__("[" .. "[[end]]" .. "]")]]]
)",
                 R"(
)");

    test_gokurai(R"(
#+MACRO FOO foo
#+MACRO end #+LUA_END
#+LUA_BEGIN
return "[" .. "[[FOO]]" .. "]"
^[[[__LUA__("[" .. "[[end]]" .. "]")]]]
)",
                 R"(
foo
)");
}

ix_TEST_CASE("gokurai: multiline macro preceded by local macros")
{
    test_gokurai(R"(
#+MACRO_BEGIN show-foo
^[[[foo]]]
^[[[foo]]]
^[[[foo]]]
#+MACRO_END
#+LOCAL_MACRO foo FOO
[[[show-foo]]]
)",
                 R"(
FOO
FOO
FOO
)");

    test_gokurai(R"(
#+MACRO_BEGIN show-foo-bar-foobar
^[[[foo]]]
^[[[bar]]]
^[[[foobar]]]
#+MACRO_END
#+LOCAL_MACRO foo FOO
#+LOCAL_MACRO bar BAR
#+LOCAL_MACRO foobar FOOBAR
[[[show-foo-bar-foobar]]]
)",
                 R"(
FOO
BAR
FOOBAR
)");
}

ix_TEST_CASE("gokurai: incorrectly placed NO_NEWLINE")
{
    test_gokurai(R"(
foo [[[__NO_NEWLINE__]]] bar
foo ^[[[__NO_NEWLINE__]]] bar
)",
                 R"(
foo  bar
foo  bar
)");
}

ix_TEST_CASE("gokurai: Lazy NO_NEWLINE inside a comment block within a block macro definition")
{
    test_gokurai(R"(
#+MACRO_BEGIN foo
foo^[[[__NO_NEWLINE__]]]
#+COMMENT_BEGIN
bar^[[[__NO_NEWLINE__]]]
#+COMMENT_END
foo
#+MACRO_END
[[[foo]]]
)",
                 R"(
foofoo
)");
}

ix_TEST_CASE("gokurai: unbalanced lazy macro")
{
    test_gokurai(R"(
OK
[[[(^[[[FOO]]]
OK
)",
                 R"(
OK
[[[(
OK
)");

    test_gokurai(R"(
OK
^[[[FOO]]])]]]
OK
)",
                 R"(
OK
)]]]
OK
)");
}

ix_TEST_CASE("gokurai: do not clear local macros when a line is loaded by [[[__NO_NEWLINE__]]]")
{
    test_gokurai(R"(
#+LOCAL_MACRO foo FOO
foo-[[[__NO_NEWLINE__]]]
[[[foo]]][[[__NO_NEWLINE__]]]
-foo
)",
                 R"(
foo-FOO-foo
)");
}

ix_TEST_CASE("gokurai: macro defined by an expanded macro")
{
    test_gokurai(R"(
#+MACRO_BEGIN foo
1
^[[[two]]]
#+MACRO two 2
3
#+MACRO_END
[[[foo]]]
[[[foo]]]
)",
                 R"(
1

3
1
2
3
)");

    test_gokurai(R"(
#+MACRO_BEGIN foo
1
^[[[two]]]
#+MACRO_BEGIN two
2
#+MACRO_END
3
#+MACRO_END
[[[foo]]]
[[[foo]]]
)",
                 R"(
1

3
1
2
3
)");
}

ix_TEST_CASE("public api")
{
    GokuraiContext ctx = gokurai_context_create(nullptr, &ix_FileHandle::of_stderr());
    GokuraiResult result = gokurai_result_create();

    {
        gokurai_context_feed_str(ctx, "hello world\n");
        gokurai_context_feed_str(ctx, "bye world\n");
        gokurai_context_end_input(ctx, result);
        ix_EXPECT_EQSTR(gokurai_result_get_output(result), "hello world\n"
                                                           "bye world\n");
        ix_EXPECT(gokurai_result_get_output_length(result) == 22);

        gokurai_context_clear(ctx);
        gokurai_context_feed_str(ctx, "hello again world\n");
        gokurai_context_feed_str(ctx, "bye again world\n");
        gokurai_context_end_input(ctx, result);
        ix_EXPECT_EQSTR(gokurai_result_get_output(result), "hello again world\n"
                                                           "bye again world\n");
        ix_EXPECT(gokurai_result_get_output_length(result) == 34);
    }

    {
        gokurai_context_clear(ctx);
        gokurai_context_feed_str(ctx, "#+MACRO foo FOO-FOO\n");
        gokurai_context_feed_str(ctx, "foo-[[[foo]]]-foo\n");
        gokurai_context_end_input(ctx, result);
        ix_EXPECT_EQSTR(gokurai_result_get_output(result), "foo-FOO-FOO-foo\n");
        ix_EXPECT(gokurai_result_get_output_length(result) == 16);

        gokurai_context_clear(ctx);
        gokurai_context_feed_str(ctx, "foo-[[[foo]]]-foo\n");
        gokurai_context_end_input(ctx, result);
        ix_EXPECT_EQSTR(gokurai_result_get_output(result), "foo--foo\n");
        ix_EXPECT(gokurai_result_get_output_length(result) == 9);
    }

    {
        gokurai_context_clear(ctx);
        gokurai_context_feed_str(ctx, "#+LOCAL_MACRO foo FOO-FOO\n");
        gokurai_context_feed_str(ctx, "foo-[[[foo]]]-foo\n");
        gokurai_context_end_input(ctx, result);
        ix_EXPECT_EQSTR(gokurai_result_get_output(result), "foo-FOO-FOO-foo\n");
        ix_EXPECT(gokurai_result_get_output_length(result) == 16);

        gokurai_context_feed_str(ctx, "#+LOCAL_MACRO foo FOO-FOO\n");
        gokurai_context_clear(ctx);
        gokurai_context_feed_str(ctx, "foo-[[[foo]]]-foo\n");
        gokurai_context_end_input(ctx, result);
        ix_EXPECT_EQSTR(gokurai_result_get_output(result), "foo--foo\n");
        ix_EXPECT(gokurai_result_get_output_length(result) == 9);
    }

    {
        gokurai_context_clear(ctx);
        gokurai_context_feed_str(ctx, "[[[__LUA__(x = 1)]]]\n");
        gokurai_context_feed_str(ctx, "x is [[[__LUA__(x)]]]\n");
        gokurai_context_end_input(ctx, result);
        ix_EXPECT_EQSTR(gokurai_result_get_output(result), "\n"
                                                           "x is 1\n");
        ix_EXPECT(gokurai_result_get_output_length(result) == 8);

        gokurai_context_clear(ctx);
        gokurai_context_feed_str(ctx, "x is [[[__LUA__(x)]]]\n");
        gokurai_context_end_input(ctx, result);
        ix_EXPECT_EQSTR(gokurai_result_get_output(result), "x is \n");
        ix_EXPECT(gokurai_result_get_output_length(result) == 6);
    }

    gokurai_context_destroy(ctx);
    gokurai_result_destroy(result);
}

ix_TEST_CASE("gokurai: The last line is a multiline macro and has no newline at the end.")
{
    test_gokurai(R"(
#+MACRO_BEGIN foo
FOO
BAR
FOOBAR
#+MACRO_END
[[[foo]]])",
                 R"(
FOO
BAR
FOOBAR)");
}

ix_TEST_CASE("gokurai: __INPUT_LINE_NUMBER__ and __OUTPUTPUT_LINE_NUMBER__ after clear()")
{
    GokuraiContext ctx = gokurai_context_create(nullptr, &ix_FileHandle::of_stderr());
    GokuraiResult result = gokurai_result_create();

    {
        gokurai_context_feed_str(ctx, "#+MACRO foo FOO\n");
        gokurai_context_feed_str(ctx, "[[[foo]]]\n");
        gokurai_context_feed_str(ctx, "[[[__INPUT_LINE_NUMBER__]]]\n");
        gokurai_context_feed_str(ctx, "[[[__OUTPUT_LINE_NUMBER__]]]\n");
        gokurai_context_end_input(ctx, result);
        ix_EXPECT_EQSTR(gokurai_result_get_output(result), "FOO\n"
                                                           "3\n"
                                                           "3\n");
        ix_EXPECT(gokurai_result_get_output_length(result) == 8);
    }

    gokurai_context_clear(ctx);
    {
        gokurai_context_feed_str(ctx, "#+MACRO foo FOO\n");
        gokurai_context_feed_str(ctx, "[[[foo]]]\n");
        gokurai_context_feed_str(ctx, "[[[__INPUT_LINE_NUMBER__]]]\n");
        gokurai_context_feed_str(ctx, "[[[__OUTPUT_LINE_NUMBER__]]]\n");
        gokurai_context_end_input(ctx, result);
        ix_EXPECT_EQSTR(gokurai_result_get_output(result), "FOO\n"
                                                           "3\n"
                                                           "3\n");
        ix_EXPECT(gokurai_result_get_output_length(result) == 8);
    }

    gokurai_context_destroy(ctx);
    gokurai_result_destroy(result);
}

ix_TEST_CASE("gokurai: teasing string")
{
    test_gokurai("#+FOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOO", "#+FOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOO");
    test_gokurai("'#+FOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOO", "'#+FOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOO");
}
