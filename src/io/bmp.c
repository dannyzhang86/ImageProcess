#include "io/bmp.h"

#include "core/errors.h"

#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define BMP_FILE_HEADER_SIZE 14u
#define BMP_INFO_HEADER_SIZE 40u
#define BMP_HEADER_SIZE (BMP_FILE_HEADER_SIZE + BMP_INFO_HEADER_SIZE)

static void put_u16le(uint8_t *dst, uint16_t value)
{
    dst[0] = (uint8_t)(value & 0xffu);
    dst[1] = (uint8_t)((value >> 8) & 0xffu);
}

static void put_u32le(uint8_t *dst, uint32_t value)
{
    dst[0] = (uint8_t)(value & 0xffu);
    dst[1] = (uint8_t)((value >> 8) & 0xffu);
    dst[2] = (uint8_t)((value >> 16) & 0xffu);
    dst[3] = (uint8_t)((value >> 24) & 0xffu);
}

int bmp_write(const char *path, const image_t *rgb8)
{
    FILE *file;
    uint8_t file_header[BMP_FILE_HEADER_SIZE];
    uint8_t info_header[BMP_INFO_HEADER_SIZE];
    uint8_t *row;
    size_t stride;
    size_t image_size;
    size_t file_size;
    int err = ISP_OK;
    int y;

    if (path == NULL || !image_valid(rgb8) || rgb8->format != PIX_RGB8) {
        return ISP_E_ARG;
    }
    isp_clear_error();

    stride = ((size_t)rgb8->width * 3u + 3u) & ~(size_t)3u;
    if ((size_t)rgb8->height > (SIZE_MAX - BMP_HEADER_SIZE) / stride) {
        isp_set_error("image too large for the BMP format");
        return ISP_E_SIZE;
    }
    image_size = stride * (size_t)rgb8->height;
    file_size = BMP_HEADER_SIZE + image_size;

    if (file_size > UINT32_MAX) {
        isp_set_error("image too large for the BMP format");
        return ISP_E_SIZE;
    }

    memset(file_header, 0, sizeof(file_header));
    file_header[0] = 'B';
    file_header[1] = 'M';
    put_u32le(file_header + 2, (uint32_t)file_size);
    put_u32le(file_header + 10, BMP_HEADER_SIZE);

    memset(info_header, 0, sizeof(info_header));
    put_u32le(info_header + 0, BMP_INFO_HEADER_SIZE);
    put_u32le(info_header + 4, (uint32_t)rgb8->width);
    put_u32le(info_header + 8, (uint32_t)rgb8->height);
    put_u16le(info_header + 12, 1);  /* planes */
    put_u16le(info_header + 14, 24); /* bits per pixel */
    put_u32le(info_header + 16, 0);  /* BI_RGB */
    put_u32le(info_header + 20, (uint32_t)image_size);
    put_u32le(info_header + 24, 2835); /* 72 DPI */
    put_u32le(info_header + 28, 2835);

    file = fopen(path, "wb");
    if (file == NULL) {
        isp_set_error("cannot open '%s' for writing: %s", path,
                      strerror(errno));
        return ISP_E_IO;
    }

    row = (uint8_t *)calloc(stride, 1);
    if (row == NULL) {
        isp_set_error("out of memory writing '%s'", path);
        fclose(file);
        return ISP_E_NOMEM;
    }

    if (fwrite(file_header, 1, sizeof(file_header), file) !=
            sizeof(file_header) ||
        fwrite(info_header, 1, sizeof(info_header), file) !=
            sizeof(info_header)) {
        isp_set_error("failed to write BMP headers to '%s'", path);
        err = ISP_E_IO;
        goto done;
    }

    /* BMP stores rows bottom-up and pixels as BGR. */
    for (y = rgb8->height - 1; y >= 0 && err == ISP_OK; y--) {
        const uint8_t *src = image_row8(rgb8, y);
        int x;

        memset(row, 0, stride);
        for (x = 0; x < rgb8->width; x++) {
            row[3 * x + 0] = src[3 * x + 2]; /* B */
            row[3 * x + 1] = src[3 * x + 1]; /* G */
            row[3 * x + 2] = src[3 * x + 0]; /* R */
        }
        if (fwrite(row, 1, stride, file) != stride) {
            isp_set_error("failed to write BMP pixels to '%s'", path);
            err = ISP_E_IO;
        }
    }

done:
    free(row);
    if (fclose(file) != 0 && err == ISP_OK) {
        isp_set_error("failed to close '%s'", path);
        err = ISP_E_IO;
    }
    return err;
}
