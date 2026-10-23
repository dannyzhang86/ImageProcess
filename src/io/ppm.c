#include "io/ppm.h"

#include "core/errors.h"

#include <errno.h>
#include <stdio.h>
#include <string.h>

int ppm_write(const char *path, const image_t *rgb8)
{
    FILE *file;
    int err = ISP_OK;
    int y;

    if (path == NULL || !image_valid(rgb8) || rgb8->format != PIX_RGB8) {
        return ISP_E_ARG;
    }
    isp_clear_error();

    file = fopen(path, "wb");
    if (file == NULL) {
        isp_set_error("cannot open '%s' for writing: %s", path,
                      strerror(errno));
        return ISP_E_IO;
    }

    if (fprintf(file, "P6\n%d %d\n255\n", rgb8->width, rgb8->height) < 0) {
        isp_set_error("failed to write PPM header to '%s'", path);
        err = ISP_E_IO;
    }

    for (y = 0; y < rgb8->height && err == ISP_OK; y++) {
        const uint8_t *src = image_row8(rgb8, y);
        if (fwrite(src, 1, (size_t)rgb8->width * 3u, file) !=
            (size_t)rgb8->width * 3u) {
            isp_set_error("failed to write PPM pixels to '%s'", path);
            err = ISP_E_IO;
        }
    }

    if (fclose(file) != 0 && err == ISP_OK) {
        isp_set_error("failed to close '%s'", path);
        err = ISP_E_IO;
    }
    return err;
}
