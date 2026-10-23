#ifndef ISP_IO_RAW_H
#define ISP_IO_RAW_H

#include "core/image.h"

/* Unpacked (one sample per element) little-endian RAW layouts. */
typedef enum {
    RAW_FMT_8 = 8,
    RAW_FMT_10 = 10,
    RAW_FMT_12 = 12,
    RAW_FMT_14 = 14,
    RAW_FMT_16 = 16
} raw_format_t;

/* Bits per sample of a valid format, 0 otherwise. */
int raw_format_bits(raw_format_t format);

/* Bytes per sample in the file (1 for RAW8, 2 for the others). */
int raw_format_bpp(raw_format_t format);

/* Parse "RAW8".."RAW16" case-insensitively.  Returns ISP_OK or ISP_E_ARG. */
int raw_format_parse(const char *text, raw_format_t *format);

/* Canonical name, e.g. "RAW10". */
const char *raw_format_name(raw_format_t format);

/*
 * Load an unpacked RAW file into a tightly packed PIX_BAYER16 image.
 *
 * Samples are kept at their native value range (0..255 for RAW8,
 * 0..1023 for RAW10, ...); black-level subtraction happens in the ISP
 * unpack stage.  The file must contain at least width * height * bpp bytes;
 * trailing bytes are ignored with a warning.
 */
int raw_load(const char *path, int width, int height, raw_format_t format,
             image_t **out);

#endif /* ISP_IO_RAW_H */
