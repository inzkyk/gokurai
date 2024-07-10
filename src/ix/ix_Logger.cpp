#include "ix_Logger.hpp"
#include "ix_HollowValue.hpp"
#include "ix_doctest.hpp"
#include "ix_string.hpp"

#include <sokol_time.h>

static ix_HollowValue<ix_Logger> g_logger;

ix_Result ix_global_logger_init()
{
    g_logger.construct(ix_LOGGER_VERBOSE, &ix_FileHandle::of_stderr(), "ix_Logger");
    return ix_OK;
}

ix_Result ix_global_logger_deinit()
{
    g_logger.destruct();
    return ix_OK;
}

void ix_global_logger_set_min_severity(ix_LoggerSeverity severity)
{
    g_logger.get().set_min_severity(severity);
}

ix_LoggerSeverity ix_global_logger_get_min_severity()
{
    return g_logger.get().get_min_severity();
}

ix_PRINTF_FORMAT(1, 2) void ix_log_fatal(ix_FORMAT_ARG const char *format, ...)
{
    va_list args;
    va_start(args, format);
    g_logger.get().log(ix_LOGGER_FATAL, format, args);
    va_end(args);
}

ix_PRINTF_FORMAT(1, 2) void ix_log_error(ix_FORMAT_ARG const char *format, ...)
{
    va_list args;
    va_start(args, format);
    g_logger.get().log(ix_LOGGER_ERROR, format, args);
    va_end(args);
}

ix_PRINTF_FORMAT(1, 2) void ix_log_warning(ix_FORMAT_ARG const char *format, ...)
{
    va_list args;
    va_start(args, format);
    g_logger.get().log(ix_LOGGER_WARNING, format, args);
    va_end(args);
}

ix_PRINTF_FORMAT(1, 2) void ix_log_info(ix_FORMAT_ARG const char *format, ...)
{
    va_list args;
    va_start(args, format);
    g_logger.get().log(ix_LOGGER_INFO, format, args);
    va_end(args);
}

ix_PRINTF_FORMAT(1, 2) void ix_log_debug(const char *format, ...)
{
    va_list args;
    va_start(args, format);
    g_logger.get().log(ix_LOGGER_DEBUG, format, args);
    va_end(args);
}

ix_PRINTF_FORMAT(1, 2) void ix_log_verbose(ix_FORMAT_ARG const char *format, ...)
{
    va_list args;
    va_start(args, format);
    g_logger.get().log(ix_LOGGER_VERBOSE, format, args);
    va_end(args);
}

static constexpr const char *severity_strings[] = {
    "verbose", //
    "debug",   //
    "info",    //
    "warning", //
    "error",   //
    "fatal",   //
};

ix_Logger::ix_Logger(ix_LoggerSeverity min_severity, const ix_FileHandle *file, const char *header)
    : m_writer(0, file),
      m_min_severity(min_severity),
      m_header(header)
{
    if (file == nullptr)
    {
        m_writer.reserve_buffer_capacity(4096);
    }

    m_start_tick = stm_now();
    m_previous_tick = stm_now();
}

void ix_Logger::set_min_severity(ix_LoggerSeverity new_min_severity)
{
    m_min_severity = new_min_severity;
}

ix_LoggerSeverity ix_Logger::get_min_severity() const
{
    return m_min_severity;
}

ix_PRINTF_FORMAT(3, 0) void ix_Logger::log(ix_LoggerSeverity severity, ix_FORMAT_ARG const char *format, va_list args)
{
    if (severity < m_min_severity)
    {
        return;
    }

    print_main(severity, format, args);
}

ix_PRINTF_FORMAT(2, 3) void ix_Logger::log_fatal(ix_FORMAT_ARG const char *format, ...)
{
    if (ix_LOGGER_FATAL < m_min_severity)
    {
        return;
    }

    va_list args;
    va_start(args, format);
    print_main(ix_LOGGER_FATAL, format, args);
    va_end(args);
}

ix_PRINTF_FORMAT(2, 3) void ix_Logger::log_error(ix_FORMAT_ARG const char *format, ...)
{
    if (ix_LOGGER_ERROR < m_min_severity)
    {
        return;
    }

    va_list args;
    va_start(args, format);
    print_main(ix_LOGGER_ERROR, format, args);
    va_end(args);
}

ix_PRINTF_FORMAT(2, 3) void ix_Logger::log_warning(ix_FORMAT_ARG const char *format, ...)
{
    if (ix_LOGGER_WARNING < m_min_severity)
    {
        return;
    }

    va_list args;
    va_start(args, format);
    print_main(ix_LOGGER_WARNING, format, args);
    va_end(args);
}

ix_PRINTF_FORMAT(2, 3) void ix_Logger::log_info(ix_FORMAT_ARG const char *format, ...)
{
    if (ix_LOGGER_INFO < m_min_severity)
    {
        return;
    }

    va_list args;
    va_start(args, format);
    print_main(ix_LOGGER_INFO, format, args);
    va_end(args);
}

ix_PRINTF_FORMAT(2, 3) void ix_Logger::log_debug(ix_FORMAT_ARG const char *format, ...)
{
    if (ix_LOGGER_DEBUG < m_min_severity)
    {
        return;
    }

    va_list args;
    va_start(args, format);
    print_main(ix_LOGGER_DEBUG, format, args);
    va_end(args);
}

ix_PRINTF_FORMAT(2, 3) void ix_Logger::log_verbose(ix_FORMAT_ARG const char *format, ...)
{
    if (ix_LOGGER_VERBOSE < m_min_severity)
    {
        return;
    }

    va_list args;
    va_start(args, format);
    print_main(ix_LOGGER_VERBOSE, format, args);
    va_end(args);
}

const char *ix_Logger::debug_get_data()
{
    m_writer.end_string();
    return m_writer.data();
}

ix_PRINTF_FORMAT(3, 0) void ix_Logger::print_main(ix_LoggerSeverity severity, const char *format, va_list args)
{
    m_mutex.lock();
    const uint64_t now_tick = stm_now();

    const double ms_from_start = stm_ms(now_tick - m_start_tick);
    const double ms_from_previous = stm_ms(now_tick - m_previous_tick);

    const char *severity_string = severity_strings[static_cast<size_t>(severity)];

    m_writer.write_stringf("[%09.4f ms] [%09.4f ms] [%s::%s] ", //
                           ms_from_start,                       //
                           ms_from_previous,                    //
                           m_header,                            //
                           severity_string);                    //
    m_writer.write_stringfv(format, args);
    m_writer.write_char('\n');
    m_mutex.unlock();

    m_previous_tick = now_tick;
}

ix_TEST_CASE("ix_Logger")
{
    {
        ix_Logger logger(ix_LOGGER_VERBOSE, nullptr, "ix_Logger");
        logger.log_fatal("fatal");
        logger.log_error("error");
        logger.log_warning("warning");
        logger.log_debug("debug");
        logger.log_info("info");
        logger.log_verbose("verbose");
        const char *s = logger.debug_get_data();
        ix_EXPECT(ix_strstr(s, "[ix_Logger::fatal] fatal\n") != nullptr);
        ix_EXPECT(ix_strstr(s, "[ix_Logger::error] error\n") != nullptr);
        ix_EXPECT(ix_strstr(s, "[ix_Logger::warning] warning\n") != nullptr);
        ix_EXPECT(ix_strstr(s, "[ix_Logger::debug] debug\n") != nullptr);
        ix_EXPECT(ix_strstr(s, "[ix_Logger::info] info\n") != nullptr);
        ix_EXPECT(ix_strstr(s, "[ix_Logger::verbose] verbose\n") != nullptr);
    }

    {
        ix_Logger logger(ix_LOGGER_VERBOSE, nullptr, "ix_Logger");
        logger.set_min_severity(ix_LOGGER_ERROR);
        logger.log_fatal("fatal");
        logger.log_error("error");
        logger.log_warning("warning");
        logger.log_debug("debug");
        logger.log_info("info");
        logger.log_verbose("verbose");
        const char *s = logger.debug_get_data();
        ix_EXPECT(ix_strstr(s, "[ix_Logger::fatal] fatal\n") != nullptr);
        ix_EXPECT(ix_strstr(s, "[ix_Logger::error] error\n") != nullptr);
    }

    {
        ix_Logger logger(ix_LOGGER_INFINITE, nullptr, "ix_Logger");
        logger.log_fatal("fatal");
        logger.log_error("error");
        logger.log_warning("warning");
        logger.log_debug("debug");
        logger.log_info("info");
        logger.log_verbose("verbose");
        const char *s = logger.debug_get_data();
        ix_EXPECT_EQSTR(s, "");
    }

    {
        const ix_LoggerSeverity old = ix_global_logger_get_min_severity();
        ix_global_logger_set_min_severity(ix_LOGGER_INFINITE);
        ix_log_fatal("fatal");
        ix_log_error("error");
        ix_log_warning("warning");
        ix_log_debug("debug");
        ix_log_info("info");
        ix_log_verbose("verbose");
        ix_global_logger_set_min_severity(old);
    }
}
