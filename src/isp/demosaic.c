/*
 * Bilinear Bayer demosaic, derived from the V4L2 libv4l / libdc1394 code
 * (itself based on OpenCV's Bayer decoding).  The original routines write
 * BGR; this port writes RGB and uses 16-bit samples.
 *
 * The row buffers are assumed to be tightly packed (stride == width), which
 * is guaranteed by image_create().
 */
#include "isp/isp.h"

#include "core/errors.h"

#include <stdint.h>

static void bayer_line_to_rgb(const uint16_t *bayer, const uint16_t *adjacent,
                              uint16_t *rgb, int width, int start_with_green,
                              int blue_line)
{
    int t0;
    int t1;

    if (start_with_green) {
        /* First pixel */
        if (blue_line) {
            *rgb++ = adjacent[0];
            *rgb++ = bayer[0];
            *rgb++ = bayer[1];
        } else {
            *rgb++ = bayer[1];
            *rgb++ = bayer[0];
            *rgb++ = adjacent[0];
        }
        /* Second pixel */
        t0 = (bayer[0] + bayer[2] + adjacent[1] + 1) / 3;
        t1 = (adjacent[0] + adjacent[2] + 1) >> 1;
        if (blue_line) {
            *rgb++ = t1;
            *rgb++ = t0;
            *rgb++ = bayer[1];
        } else {
            *rgb++ = bayer[1];
            *rgb++ = t0;
            *rgb++ = t1;
        }
        bayer++;
        adjacent++;
        width -= 2;
    } else {
        /* First pixel */
        t0 = (bayer[1] + adjacent[0] + 1) >> 1;
        if (blue_line) {
            *rgb++ = adjacent[1];
            *rgb++ = t0;
            *rgb++ = bayer[0];
        } else {
            *rgb++ = bayer[0];
            *rgb++ = t0;
            *rgb++ = adjacent[1];
        }
        width--;
    }

    if (blue_line) {
        for (; width > 2; width -= 2) {
            t0 = (bayer[0] + bayer[2] + 1) >> 1;
            *rgb++ = adjacent[1];
            *rgb++ = bayer[1];
            *rgb++ = t0;
            bayer++;
            adjacent++;

            t0 = (bayer[0] + bayer[2] + adjacent[1] + 1) / 3;
            t1 = (adjacent[0] + adjacent[2] + 1) >> 1;
            *rgb++ = t1;
            *rgb++ = t0;
            *rgb++ = bayer[1];
            bayer++;
            adjacent++;
        }
    } else {
        for (; width > 2; width -= 2) {
            t0 = (bayer[0] + bayer[2] + 1) >> 1;
            *rgb++ = t0;
            *rgb++ = bayer[1];
            *rgb++ = adjacent[1];
            bayer++;
            adjacent++;

            t0 = (bayer[0] + bayer[2] + adjacent[1] + 1) / 3;
            t1 = (adjacent[0] + adjacent[2] + 1) >> 1;
            *rgb++ = bayer[1];
            *rgb++ = t0;
            *rgb++ = t1;
            bayer++;
            adjacent++;
        }
    }

    if (width == 2) {
        /* Second to last pixel */
        t0 = (bayer[0] + bayer[2] + 1) >> 1;
        if (blue_line) {
            *rgb++ = adjacent[1];
            *rgb++ = bayer[1];
            *rgb++ = t0;
        } else {
            *rgb++ = t0;
            *rgb++ = bayer[1];
            *rgb++ = adjacent[1];
        }
        /* Last pixel */
        t0 = (bayer[1] + adjacent[2] + 1) >> 1;
        if (blue_line) {
            *rgb++ = adjacent[1];
            *rgb++ = t0;
            *rgb++ = bayer[2];
        } else {
            *rgb++ = bayer[2];
            *rgb++ = t0;
            *rgb++ = adjacent[1];
        }
    } else {
        /* Last pixel */
        if (blue_line) {
            *rgb++ = adjacent[1];
            *rgb++ = bayer[1];
            *rgb++ = bayer[0];
        } else {
            *rgb++ = bayer[0];
            *rgb++ = bayer[1];
            *rgb++ = adjacent[1];
        }
    }
}

int isp_demosaic(const image_t *bayer, cfa_t cfa, image_t **out)
{
    image_t *rgb;
    const uint16_t *b;
    uint16_t *dst;
    int width;
    int height;
    int start_with_green;
    int blue_line;
    int remaining;

    if (out == NULL) {
        return ISP_E_ARG;
    }
    *out = NULL;

    if (!image_valid(bayer) || bayer->format != PIX_BAYER16) {
        return ISP_E_FORMAT;
    }

    width = bayer->width;
    height = bayer->height;
    if (width < 4 || height < 4 || (width & 1) != 0 || (height & 1) != 0) {
        return ISP_E_SIZE;
    }
    if (bayer->stride != (size_t)width * sizeof(uint16_t)) {
        return ISP_E_FORMAT;
    }

    switch (cfa) {
    case CFA_BGGR:
        start_with_green = 0;
        blue_line = 1;
        break;
    case CFA_RGGB:
        start_with_green = 0;
        blue_line = 0;
        break;
    case CFA_GBRG:
        start_with_green = 1;
        blue_line = 1;
        break;
    case CFA_GRBG:
        start_with_green = 1;
        blue_line = 0;
        break;
    default:
        return ISP_E_ARG;
    }

    rgb = image_create(width, height, PIX_RGB16);
    if (rgb == NULL) {
        return ISP_E_NOMEM;
    }

    b = (const uint16_t *)(const void *)bayer->data;
    dst = (uint16_t *)(void *)rgb->data;

    /* Render the first line from the first two Bayer rows. */
    bayer_line_to_rgb(b, b + width, dst, width, start_with_green, blue_line);
    dst += (size_t)width * 3u;

    /* Interior lines; b points at the row above the line being rendered. */
    for (remaining = height - 2; remaining > 0; remaining--) {
        const uint16_t *bayer_end = b + (width - 2);
        int t0;
        int t1;

        if (start_with_green) {
            t0 = (b[1] + b[width * 2 + 1] + 1) >> 1;
            t1 = (b[0] + b[width * 2] + b[width + 1] + 1) / 3;
            if (blue_line) {
                dst[0] = b[width];
                dst[1] = (uint16_t)t1;
                dst[2] = (uint16_t)t0;
            } else {
                dst[0] = (uint16_t)t0;
                dst[1] = (uint16_t)t1;
                dst[2] = b[width];
            }

            t1 = (b[width] + b[width + 2] + 1) >> 1;
            if (blue_line) {
                dst[3] = (uint16_t)t1;
                dst[4] = b[width + 1];
                dst[5] = (uint16_t)t0;
            } else {
                dst[3] = (uint16_t)t0;
                dst[4] = b[width + 1];
                dst[5] = (uint16_t)t1;
            }
            b++;
            dst += 6;
        } else {
            t0 = (b[0] + b[width * 2] + 1) >> 1;
            if (blue_line) {
                dst[0] = b[width + 1];
                dst[1] = b[width];
                dst[2] = (uint16_t)t0;
            } else {
                dst[0] = (uint16_t)t0;
                dst[1] = b[width];
                dst[2] = b[width + 1];
            }
            dst += 3;
        }

        if (blue_line) {
            for (; b <= bayer_end - 2; b += 2) {
                t0 = (b[0] + b[2] + b[width * 2] +
                      b[width * 2 + 2] + 2) >> 2;
                t1 = (b[1] + b[width] + b[width + 2] +
                      b[width * 2 + 1] + 2) >> 2;
                dst[0] = b[width + 1];
                dst[1] = (uint16_t)t1;
                dst[2] = (uint16_t)t0;
                dst += 3;

                t0 = (b[2] + b[width * 2 + 2] + 1) >> 1;
                t1 = (b[width + 1] + b[width + 3] + 1) >> 1;
                dst[0] = (uint16_t)t1;
                dst[1] = b[width + 2];
                dst[2] = (uint16_t)t0;
                dst += 3;
            }
        } else {
            for (; b <= bayer_end - 2; b += 2) {
                t0 = (b[0] + b[2] + b[width * 2] +
                      b[width * 2 + 2] + 2) >> 2;
                t1 = (b[1] + b[width] + b[width + 2] +
                      b[width * 2 + 1] + 2) >> 2;
                dst[0] = (uint16_t)t0;
                dst[1] = (uint16_t)t1;
                dst[2] = b[width + 1];
                dst += 3;

                t0 = (b[2] + b[width * 2 + 2] + 1) >> 1;
                t1 = (b[width + 1] + b[width + 3] + 1) >> 1;
                dst[0] = (uint16_t)t0;
                dst[1] = b[width + 2];
                dst[2] = (uint16_t)t1;
                dst += 3;
            }
        }

        if (b < bayer_end) {
            /* Second to last pixel */
            t0 = (b[0] + b[2] + b[width * 2] +
                  b[width * 2 + 2] + 2) >> 2;
            t1 = (b[1] + b[width] + b[width + 2] +
                  b[width * 2 + 1] + 2) >> 2;
            if (blue_line) {
                dst[0] = b[width + 1];
                dst[1] = (uint16_t)t1;
                dst[2] = (uint16_t)t0;
            } else {
                dst[0] = (uint16_t)t0;
                dst[1] = (uint16_t)t1;
                dst[2] = b[width + 1];
            }
            dst += 3;

            /* Last pixel */
            t0 = (b[2] + b[width * 2 + 2] + 1) >> 1;
            if (blue_line) {
                dst[0] = b[width + 1];
                dst[1] = b[width + 2];
                dst[2] = (uint16_t)t0;
            } else {
                dst[0] = (uint16_t)t0;
                dst[1] = b[width + 2];
                dst[2] = b[width + 1];
            }
            dst += 3;
            b++;
        } else {
            /* Last pixel */
            t0 = (b[0] + b[width * 2] + 1) >> 1;
            t1 = (b[1] + b[width * 2 + 1] + b[width] + 1) / 3;
            if (blue_line) {
                dst[0] = b[width + 1];
                dst[1] = (uint16_t)t1;
                dst[2] = (uint16_t)t0;
            } else {
                dst[0] = (uint16_t)t0;
                dst[1] = (uint16_t)t1;
                dst[2] = b[width + 1];
            }
            dst += 3;
        }

        /* Skip the two border pixels to start the next row. */
        b += 2;

        blue_line = !blue_line;
        start_with_green = !start_with_green;
    }

    /* Render the last line. */
    bayer_line_to_rgb(b + width, b, dst, width, !start_with_green, !blue_line);

    *out = rgb;
    return ISP_OK;
}
