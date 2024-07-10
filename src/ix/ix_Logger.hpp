#pragma once

#include "ix.hpp"
#include "ix_Mutex.hpp"
#include "ix_Writer.hpp"
#include "ix_file.hpp"

ix_Result ix_global_logger_init();
ix_Result ix_global_logger_deinit();

ix_PRINTF_FORMAT(1, 2) void ix_log_fatal(ix_FORMAT_ARG const char *format, ...);
ix_PRINTF_FORMAT(1, 2) void ix_log_error(ix_FORMAT_ARG const char *format, ...);
ix_PRINTF_FORMAT(1, 2) void ix_log_warning(ix_FORMAT_ARG const char *format, ...);
ix_PRINTF_FORMAT(1, 2) void ix_log_info(ix_FORMAT_ARG const char *format, ...);
// ix_PRINTF_FORMAT(1, 2) void ix_log_debug(ix_FORMAT_ARG const char *format, ...); // Declared in ix.hpp.
ix_PRINTF_FORMAT(1, 2) void ix_log_verbose(ix_FORMAT_ARG const char *format, ...);

enum ix_LoggerSeverity : uint8_t
{
    ix_LOGGER_VERBOSE = 0,
    ix_LOGGER_DEBUG,
    ix_LOGGER_INFO,
    ix_LOGGER_WARNING,
    ix_LOGGER_ERROR,
    ix_LOGGER_FATAL,
    ix_LOGGER_INFINITE,
};

void ix_global_logger_set_min_severity(ix_LoggerSeverity severity);
ix_LoggerSeverity ix_global_logger_get_min_severity();

class ix_Logger
{
    ix_Writer m_writer;
    ix_Mutex m_mutex;
    ix_LoggerSeverity m_min_severity;
    const char *m_header;

    uint64_t m_start_tick;
    uint64_t m_previous_tick;

  public:
    ix_Logger(ix_LoggerSeverity min_severity, const ix_FileHandle *file, const char *header);
    void set_min_severity(ix_LoggerSeverity new_min_severity);
    ix_LoggerSeverity get_min_severity() const;

    ix_PRINTF_FORMAT(3, 0) void log(ix_LoggerSeverity severity, ix_FORMAT_ARG const char *format, va_list args);
    ix_PRINTF_FORMAT(2, 3) void log_fatal(ix_FORMAT_ARG const char *format, ...);
    ix_PRINTF_FORMAT(2, 3) void log_error(ix_FORMAT_ARG const char *format, ...);
    ix_PRINTF_FORMAT(2, 3) void log_warning(ix_FORMAT_ARG const char *format, ...);
    ix_PRINTF_FORMAT(2, 3) void log_info(ix_FORMAT_ARG const char *format, ...);
    ix_PRINTF_FORMAT(2, 3) void log_debug(ix_FORMAT_ARG const char *format, ...);
    ix_PRINTF_FORMAT(2, 3) void log_verbose(ix_FORMAT_ARG const char *format, ...);

    const char *debug_get_data();

  private:
    ix_PRINTF_FORMAT(3, 0) void print_main(ix_LoggerSeverity severity, const char *format, va_list args);
};
