#include "io/raw.h"

#include "core/errors.h"

#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int raw_format_bits(raw_format_t format)
{
    switch (format) {
    case RAW_FMT_8:
    case RAW_FMT_10:
    case RAW_FMT_12:
    case RAW_FMT_14:
    case RAW_FMT_16:
        return (int)format;
    default:
        return 0;
    }
}

int raw_format_bpp(raw_format_t format)
{
    switch (format) {
    case RAW_FMT_8:
        return 1;
    case RAW_FMT_10:
    case RAW_FMT_12:
    case RAW_FMT_14:
    case RAW_FMT_16:
        return 2;
    default:
        return 0;
    }
}

const char *raw_format_name(raw_format_t format)
{
    switch (format) {
    case RAW_FMT_8:
        return "RAW8";
    case RAW_FMT_10:
        return "RAW10";
    case RAW_FMT_12:
        return "RAW12";
    case RAW_FMT_14:
        return "RAW14";
    case RAW_FMT_16:
        return "RAW16";
    default:
        return "unknown";
    }
}

static int equals_ignore_case(const char *a, const char *b)
{
    while (*a != '\0' && *b != '\0') {
        if (tolower((unsigned char)*a) != tolower((unsigned char)*b)) {
            return 0;
        }
        a++;
        b++;
    }
    return *a == '\0' && *b == '\0';
}

int raw_format_parse(const char *text, raw_format_t *format)
{
    static const raw_format_t formats[] = {
        RAW_FMT_8, RAW_FMT_10, RAW_FMT_12, RAW_FMT_14, RAW_FMT_16
    };
    size_t i;

    if (text == NULL || format == NULL) {
        return ISP_E_ARG;
    }

    for (i = 0; i < sizeof(formats) / sizeof(formats[0]); i++) {
        if (equals_ignore_case(text, raw_format_name(formats[i]))) {
            *format = formats[i];
            return ISP_OK;
        }
    }
    return ISP_E_ARG;
}

static int read_file(const char *path, uint8_t **data, size_t *size)
{
    FILE *file;
    long length;
    uint8_t *buffer;
    size_t got;

    *data = NULL;
    *size = 0;

    file = fopen(path, "rb");
    if (file == NULL) {
        isp_set_error("cannot open '%s': %s", path, strerror(errno));
        return ISP_E_IO;
    }

    if (fseek(file, 0, SEEK_END) != 0) {
        isp_set_error("cannot seek '%s'", path);
        fclose(file);
        return ISP_E_IO;
    }
    length = ftell(file);
    if (length < 0) {
        isp_set_error("cannot determine size of '%s'", path);
        fclose(file);
        return ISP_E_IO;
    }
    rewind(file);

    buffer = (uint8_t *)malloc((size_t)length > 0 ? (size_t)length : 1);
    if (buffer == NULL) {
        isp_set_error("out of memory reading '%s'", path);
        fclose(file);
        return ISP_E_NOMEM;
    }

    got = fread(buffer, 1, (size_t)length, file);
    fclose(file);
    if (got != (size_t)length) {
        isp_set_error("short read on '%s': %zu of %ld bytes", path, got,
                      length);
        free(buffer);
        return ISP_E_IO;
    }

    *data = buffer;
    *size = (size_t)length;
    return ISP_OK;
}

int raw_load(const char *path, int width, int height, raw_format_t format,
             image_t **out)
{
    const int bpp = raw_format_bpp(format);
    uint64_t need64;
    size_t need;
    size_t size;
    uint8_t *file_data;
    image_t *img;
    int err;
    int x;
    int y;

    if (out == NULL) {
        return ISP_E_ARG;
    }
    *out = NULL;
    isp_clear_error();

    if (path == NULL || width <= 0 || height <= 0 || bpp == 0) {
        return ISP_E_ARG;
    }

    need64 = (uint64_t)width * (uint64_t)height * (uint64_t)bpp;
    if (need64 > (uint64_t)SIZE_MAX) {
        isp_set_error("image dimensions are too large");
        return ISP_E_SIZE;
    }
    need = (size_t)need64;

    err = read_file(path, &file_data, &size);
    if (err != ISP_OK) {
        return err;
    }

    if (size < need) {
        isp_set_error("'%s' is too small: %zu bytes, expected %zu "
                      "(%dx%d %s)",
                      path, size, need, width, height, raw_format_name(format));
        free(file_data);
        return ISP_E_SIZE;
    }
    if (size > need) {
        fprintf(stderr, "raw: warning: ignoring %zu trailing bytes in '%s'\n",
                size - need, path);
    }

    img = image_create(width, height, PIX_BAYER16);
    if (img == NULL) {
        isp_set_error("out of memory allocating a %dx%d image", width, height);
        free(file_data);
        return ISP_E_NOMEM;
    }

    for (y = 0; y < height; y++) {
        uint16_t *row = image_row16(img, y);
        const uint8_t *src = file_data + (size_t)y * (size_t)width * (size_t)bpp;

        if (bpp == 1) {
            for (x = 0; x < width; x++) {
                row[x] = src[x];
            }
        } else {
            for (x = 0; x < width; x++) {
                row[x] = (uint16_t)(src[2 * x] | (src[2 * x + 1] << 8));
            }
        }
    }

    free(file_data);
    *out = img;
    return ISP_OK;
}
