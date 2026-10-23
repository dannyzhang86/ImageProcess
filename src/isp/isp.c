#include "isp/isp.h"

#include "core/errors.h"

#include <ctype.h>
#include <stddef.h>

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

const char *cfa_name(cfa_t cfa)
{
    switch (cfa) {
    case CFA_BGGR:
        return "BGGR";
    case CFA_RGGB:
        return "RGGB";
    case CFA_GBRG:
        return "GBRG";
    case CFA_GRBG:
        return "GRBG";
    default:
        return "unknown";
    }
}

int cfa_parse(const char *text, cfa_t *cfa)
{
    static const cfa_t phases[] = { CFA_BGGR, CFA_RGGB, CFA_GBRG, CFA_GRBG };
    size_t i;

    if (text == NULL || cfa == NULL) {
        return ISP_E_ARG;
    }
    for (i = 0; i < sizeof(phases) / sizeof(phases[0]); i++) {
        if (equals_ignore_case(text, cfa_name(phases[i]))) {
            *cfa = phases[i];
            return ISP_OK;
        }
    }
    return ISP_E_ARG;
}

int isp_process(const image_t *raw, const isp_config_t *config, image_t **out)
{
    image_t *linear = NULL;
    image_t *rgb16 = NULL;
    int err;

    if (out == NULL) {
        return ISP_E_ARG;
    }
    *out = NULL;
    isp_clear_error();

    if (!image_valid(raw) || raw->format != PIX_BAYER16) {
        return ISP_E_FORMAT;
    }
    if (config == NULL) {
        return ISP_E_ARG;
    }
    if (raw->width < 4 || raw->height < 4 ||
        (raw->width & 1) != 0 || (raw->height & 1) != 0) {
        return ISP_E_SIZE;
    }
    if (config->black_level < 0 || config->white_level <= config->black_level ||
        config->white_level > 65535) {
        return ISP_E_ARG;
    }

    err = isp_unpack(raw, config->black_level, config->white_level, &linear);
    if (err != ISP_OK) {
        goto done;
    }

    err = isp_demosaic(linear, config->cfa, &rgb16);
    if (err != ISP_OK) {
        goto done;
    }

    err = isp_gamma(rgb16, config->gamma, out);

done:
    image_free(linear);
    image_free(rgb16);
    return err;
}
