#ifndef ISP_ISP_H
#define ISP_ISP_H

#include "core/image.h"

/* Bayer colour filter array phase of the top-left 2x2 block. */
typedef enum {
    CFA_BGGR = 0,
    CFA_RGGB,
    CFA_GBRG,
    CFA_GRBG
} cfa_t;

typedef struct {
    int black_level;  /* per-channel black offset (uniform for now) */
    int white_level;  /* saturation level of the source format       */
    cfa_t cfa;
    float gamma;      /* display gamma; <= 0 disables the stage       */
} isp_config_t;

/* Parse "BGGR"/"RGGB"/"GBRG"/"GRBG" case-insensitively. */
int cfa_parse(const char *text, cfa_t *cfa);
const char *cfa_name(cfa_t cfa);

/*
 * Run the whole pipeline on a native Bayer image:
 *   unpack (black level + normalize) -> demosaic -> gamma -> RGB8
 * The returned image belongs to the caller and must be freed with
 * image_free().  Width and height must both be even and at least 4.
 */
int isp_process(const image_t *raw, const isp_config_t *config, image_t **out);

/* Stage entry points, exposed for unit tests and future tuning. */

/* Scale native sensor values to full-range 16-bit linear. */
int isp_unpack(const image_t *raw, int black_level, int white_level,
               image_t **out);

/* Bilinear Bayer demosaic (V4L2/libdc1394 style), PIX_BAYER16 -> PIX_RGB16. */
int isp_demosaic(const image_t *bayer, cfa_t cfa, image_t **out);

/* 16-bit linear RGB -> 8-bit display RGB, optionally gamma encoded. */
int isp_gamma(const image_t *rgb16, float gamma, image_t **out);

#endif /* ISP_ISP_H */
