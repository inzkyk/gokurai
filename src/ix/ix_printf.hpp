#pragma once

#include "ix.hpp"

#include <stdarg.h>

#if ix_COMPILER(MSVC)
#include <sal.h>
#define ix_PRINTF_FORMAT(format_idx, args_start)
#define ix_FORMAT_ARG _Printf_format_string_
#else
#define ix_PRINTF_FORMAT(format_idx, var_args_start) __attribute__((__format__(printf, format_idx, var_args_start)))
#define ix_FORMAT_ARG
#endif

ix_PRINTF_FORMAT(3, 0) int ix_vsnprintf(char *buf, size_t buf_length, const char *format, va_list args);
ix_PRINTF_FORMAT(3, 4) int ix_snprintf(char *buf, size_t buf_length, ix_FORMAT_ARG const char *format, ...);
