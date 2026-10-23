#include "core/image.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

size_t image_bpp(pixel_format_t format)
{
    switch (format) {
    case PIX_BAYER16:
        return 2;
    case PIX_RGB16:
        return 6;
    case PIX_RGB8:
        return 3;
    default:
        return 0;
    }
}

int image_channels(pixel_format_t format)
{
    switch (format) {
    case PIX_BAYER16:
        return 1;
    case PIX_RGB16:
    case PIX_RGB8:
        return 3;
    default:
        return 0;
    }
}

uint16_t *image_row16(const image_t *img, int y)
{
    return (uint16_t *)(void *)(img->data + (size_t)y * img->stride);
}

uint8_t *image_row8(const image_t *img, int y)
{
    return img->data + (size_t)y * img->stride;
}

image_t *image_create(int width, int height, pixel_format_t format)
{
    size_t bpp;
    size_t stride;
    image_t *img;

    if (width <= 0 || height <= 0) {
        return NULL;
    }

    bpp = image_bpp(format);
    if (bpp == 0) {
        return NULL;
    }

    if ((size_t)width > SIZE_MAX / bpp) {
        return NULL;
    }
    stride = (size_t)width * bpp;

    if ((size_t)height > SIZE_MAX / stride) {
        return NULL;
    }

    img = (image_t *)calloc(1, sizeof(*img));
    if (img == NULL) {
        return NULL;
    }

    img->data = (uint8_t *)calloc((size_t)height, stride);
    if (img->data == NULL) {
        free(img);
        return NULL;
    }

    img->width = width;
    img->height = height;
    img->stride = stride;
    img->format = format;
    return img;
}

void image_free(image_t *img)
{
    if (img == NULL) {
        return;
    }
    free(img->data);
    free(img);
}

int image_valid(const image_t *img)
{
    size_t bpp;

    if (img == NULL || img->data == NULL) {
        return 0;
    }
    if (img->width <= 0 || img->height <= 0) {
        return 0;
    }
    bpp = image_bpp(img->format);
    if (bpp == 0) {
        return 0;
    }
    if (img->stride < (size_t)img->width * bpp) {
        return 0;
    }
    return 1;
}
