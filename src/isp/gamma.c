#include "isp/isp.h"

#include "core/errors.h"

#include <math.h>
#include <stdint.h>
#include <stdlib.h>

#define GAMMA_LUT_SIZE 65536u

int isp_gamma(const image_t *rgb16, float gamma, image_t **out)
{
    image_t *rgb8;
    uint8_t *lut;
    int i;
    int x;
    int y;

    if (out == NULL) {
        return ISP_E_ARG;
    }
    *out = NULL;

    if (!image_valid(rgb16) || rgb16->format != PIX_RGB16) {
        return ISP_E_FORMAT;
    }
    if (isnan(gamma) ||
        (gamma != 0.0f && (gamma < 0.01f || gamma > 100.0f))) {
        return ISP_E_ARG;
    }

    lut = (uint8_t *)malloc(GAMMA_LUT_SIZE);
    if (lut == NULL) {
        return ISP_E_NOMEM;
    }

    if (gamma <= 0.0f) {
        for (i = 0; i < (int)GAMMA_LUT_SIZE; i++) {
            lut[i] = (uint8_t)(((uint32_t)i * 255u + 32767u) / 65535u);
        }
    } else {
        const float exponent = 1.0f / gamma;

        for (i = 0; i < (int)GAMMA_LUT_SIZE; i++) {
            float value = ((float)i + 0.5f) / 65536.0f;
            float encoded = powf(value, exponent) * 255.0f + 0.5f;

            if (encoded < 0.0f) {
                encoded = 0.0f;
            } else if (encoded > 255.0f) {
                encoded = 255.0f;
            }
            lut[i] = (uint8_t)encoded;
        }
    }

    rgb8 = image_create(rgb16->width, rgb16->height, PIX_RGB8);
    if (rgb8 == NULL) {
        free(lut);
        return ISP_E_NOMEM;
    }

    for (y = 0; y < rgb16->height; y++) {
        const uint16_t *src = image_row16(rgb16, y);
        uint8_t *dst = image_row8(rgb8, y);

        for (x = 0; x < rgb16->width * 3; x++) {
            dst[x] = lut[src[x]];
        }
    }

    free(lut);
    *out = rgb8;
    return ISP_OK;
}
