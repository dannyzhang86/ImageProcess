#ifndef ISP_IO_BMP_H
#define ISP_IO_BMP_H

#include "core/image.h"

/*
 * Write a 24-bit uncompressed BMP (bottom-up, 4-byte aligned rows).
 * The headers are serialized field by field, so no struct packing or
 * platform-dependent type widths are involved.
 */
int bmp_write(const char *path, const image_t *rgb8);

#endif /* ISP_IO_BMP_H */
