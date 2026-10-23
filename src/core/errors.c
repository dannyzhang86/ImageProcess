#include "core/errors.h"

#include <stdarg.h>
#include <stdio.h>

static char error_text[256];

void isp_clear_error(void)
{
    error_text[0] = '\0';
}

void isp_set_error(const char *format, ...)
{
    va_list args;

    if (format == NULL) {
        error_text[0] = '\0';
        return;
    }
    va_start(args, format);
#if defined(__GNUC__) || defined(__clang__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat-nonliteral"
#endif
    (void)vsnprintf(error_text, sizeof(error_text), format, args);
#if defined(__GNUC__) || defined(__clang__)
#pragma GCC diagnostic pop
#endif
    va_end(args);
}

const char *isp_last_error(void)
{
    return error_text;
}

const char *isp_strerror(int err)
{
    switch (err) {
    case ISP_OK:
        return "ok";
    case ISP_E_ARG:
        return "invalid argument";
    case ISP_E_IO:
        return "I/O error";
    case ISP_E_FORMAT:
        return "unsupported pixel format";
    case ISP_E_NOMEM:
        return "out of memory";
    case ISP_E_SIZE:
        return "unsupported image size";
    case ISP_E_UNSUPPORTED:
        return "operation not supported";
    default:
        return "unknown error";
    }
}
