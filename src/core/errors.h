#ifndef ISP_CORE_ERRORS_H
#define ISP_CORE_ERRORS_H

enum {
    ISP_OK = 0,
    ISP_E_ARG = -1,        /* bad argument / configuration            */
    ISP_E_IO = -2,         /* file could not be opened or read/written */
    ISP_E_FORMAT = -3,     /* wrong image format for this stage        */
    ISP_E_NOMEM = -4,      /* allocation failure                      */
    ISP_E_SIZE = -5,       /* unsupported image dimensions            */
    ISP_E_UNSUPPORTED = -6 /* valid request, not implemented yet      */
};

/* Human readable text for an ISP error code. */
const char *isp_strerror(int err);

/*
 * Optional detail for the last failure ("'x.raw' is too small ...").
 * The buffer is process-global, so this is not thread-safe; the CLI is
 * single-threaded and library code only sets it on the failing path.
 */
void isp_clear_error(void);
#if defined(__GNUC__) || defined(__clang__)
void isp_set_error(const char *format, ...) __attribute__((format(printf, 1, 2)));
#else
void isp_set_error(const char *format, ...);
#endif
const char *isp_last_error(void);

#endif /* ISP_CORE_ERRORS_H */
