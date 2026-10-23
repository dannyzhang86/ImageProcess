#ifndef ISP_IO_PPM_H
#define ISP_IO_PPM_H

#include "core/image.h"

/* Write a binary PPM (P6) file.  PPM needs no headers beyond the text line. */
int ppm_write(const char *path, const image_t *rgb8);

#endif /* ISP_IO_PPM_H */
