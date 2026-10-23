#ifndef ISP_CORE_IMAGE_H
#define ISP_CORE_IMAGE_H

#include <stddef.h>
#include <stdint.h>

/*
 * Minimal image container used by every stage.
 *
 * Samples are unsigned and little-endian in memory.  A Bayer buffer has one
 * channel (16-bit), RGB buffers have three interleaved channels.  The stride
 * is expressed in bytes so callers can add padding without changing the
 * pixel format.
 */
typedef enum {
    PIX_BAYER16 = 0, /* one 16-bit sample per pixel, native sensor value */
    PIX_RGB16,       /* three interleaved 16-bit samples per pixel        */
    PIX_RGB8         /* three interleaved 8-bit samples per pixel         */
} pixel_format_t;

typedef struct {
    uint8_t *data;
    int width;
    int height;
    size_t stride; /* bytes per row, always >= width * bytes_per_pixel */
    pixel_format_t format;
} image_t;

/* Bytes per pixel of a format, 0 for an unknown format. */
size_t image_bpp(pixel_format_t format);

/* Number of interleaved channels, 0 for an unknown format. */
int image_channels(pixel_format_t format);

/* Aligned row helpers. */
uint16_t *image_row16(const image_t *img, int y);
uint8_t *image_row8(const image_t *img, int y);

/*
 * Allocate a tightly packed image.  Returns NULL on invalid dimensions,
 * overflow, or allocation failure.  The buffer is zeroed.
 */
image_t *image_create(int width, int height, pixel_format_t format);

/* Free an image allocated with image_create().  NULL is accepted. */
void image_free(image_t *img);

/* Structural sanity check (non-NULL data, positive dimensions, sane stride). */
int image_valid(const image_t *img);

#endif /* ISP_CORE_IMAGE_H */
