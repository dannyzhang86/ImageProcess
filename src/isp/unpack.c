#include "isp/isp.h"

#include "core/errors.h"

#include <stdint.h>
#include <stdlib.h>

int isp_unpack(const image_t *raw, int black_level, int white_level,
               image_t **out)
{
    image_t *linear;
    uint64_t range;
    int x;
    int y;

    if (out == NULL) {
        return ISP_E_ARG;
    }
    *out = NULL;

    if (!image_valid(raw) || raw->format != PIX_BAYER16) {
        return ISP_E_FORMAT;
    }
    if (black_level < 0 || white_level <= black_level || white_level > 65535) {
        return ISP_E_ARG;
    }

    linear = image_create(raw->width, raw->height, PIX_BAYER16);
    if (linear == NULL) {
        return ISP_E_NOMEM;
    }

    range = (uint64_t)(white_level - black_level);

    for (y = 0; y < raw->height; y++) {
        const uint16_t *src = image_row16(raw, y);
        uint16_t *dst = image_row16(linear, y);

        for (x = 0; x < raw->width; x++) {
            uint64_t value = src[x];

            if (value <= (uint64_t)black_level) {
                dst[x] = 0;
            } else {
                value -= (uint64_t)black_level;
                if (value >= range) {
                    dst[x] = UINT16_MAX;
                } else {
                    dst[x] = (uint16_t)((value * 65535u + range / 2u) / range);
                }
            }
        }
    }

    *out = linear;
    return ISP_OK;
}
