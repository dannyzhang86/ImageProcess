#include "core/errors.h"
#include "core/image.h"
#include "io/bmp.h"
#include "io/ppm.h"
#include "io/raw.h"
#include "isp/isp.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int failures;

#define CHECK(cond)                                                          \
    do {                                                                     \
        if (!(cond)) {                                                       \
            fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond);  \
            failures++;                                                      \
        }                                                                    \
    } while (0)

#define CHECK_EQ(actual, expected)                                           \
    do {                                                                     \
        long a_ = (long)(actual);                                            \
        long e_ = (long)(expected);                                          \
        if (a_ != e_) {                                                      \
            fprintf(stderr, "FAIL %s:%d: %s == %ld, expected %ld\n",         \
                    __FILE__, __LINE__, #actual, a_, e_);                    \
            failures++;                                                      \
        }                                                                    \
    } while (0)

static uint32_t read_u32le(const uint8_t *p)
{
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16) |
           ((uint32_t)p[3] << 24);
}

static uint16_t read_u16le(const uint8_t *p)
{
    return (uint16_t)(p[0] | (p[1] << 8));
}

static uint8_t *read_whole_file(const char *path, size_t *size)
{
    FILE *file = fopen(path, "rb");
    uint8_t *data;
    long length;

    if (file == NULL) {
        return NULL;
    }
    if (fseek(file, 0, SEEK_END) != 0 || (length = ftell(file)) < 0) {
        fclose(file);
        return NULL;
    }
    rewind(file);
    data = (uint8_t *)malloc((size_t)length);
    if (data == NULL || fread(data, 1, (size_t)length, file) != (size_t)length) {
        free(data);
        fclose(file);
        return NULL;
    }
    fclose(file);
    *size = (size_t)length;
    return data;
}

static void test_image_basics(void)
{
    image_t *img = image_create(5, 3, PIX_RGB8);

    CHECK(img != NULL);
    if (img == NULL) {
        return;
    }
    CHECK_EQ(img->stride, 15);
    CHECK(image_valid(img));
    CHECK_EQ(image_bpp(PIX_RGB8), 3);
    CHECK_EQ(image_channels(PIX_RGB8), 3);
    CHECK(image_create(0, 3, PIX_RGB8) == NULL);
    CHECK(image_create(3, 3, (pixel_format_t)99) == NULL);
    image_free(img);
    image_free(NULL);
}

static void test_unpack(void)
{
    image_t *raw = image_create(4, 1, PIX_BAYER16);
    image_t *linear = NULL;
    uint16_t *src;

    CHECK(raw != NULL);
    if (raw == NULL) {
        return;
    }
    src = image_row16(raw, 0);
    src[0] = 0;    /* below black */
    src[1] = 255;  /* below black */
    src[2] = 256;  /* black */
    src[3] = 1023; /* white */

    CHECK_EQ(isp_unpack(raw, 256, 1023, &linear), ISP_OK);
    if (linear != NULL) {
        const uint16_t *dst = image_row16(linear, 0);
        CHECK_EQ(dst[0], 0);
        CHECK_EQ(dst[1], 0);
        CHECK_EQ(dst[2], 0);
        CHECK_EQ(dst[3], 65535);
        image_free(linear);
        linear = NULL;
    }

    CHECK_EQ(isp_unpack(raw, 1023, 256, &linear), ISP_E_ARG);
    CHECK_EQ(isp_unpack(raw, 0, 65536, &linear), ISP_E_ARG);
    image_free(raw);
}

static char site_char(cfa_t cfa, int x, int y)
{
    static const char *patterns[] = { "BGGR", "RGGB", "GBRG", "GRBG" };
    return patterns[cfa][(y & 1) * 2 + (x & 1)];
}

static void fill_uniform_bayer(image_t *img, cfa_t cfa, uint16_t r, uint16_t g,
                               uint16_t b)
{
    int x;
    int y;

    for (y = 0; y < img->height; y++) {
        uint16_t *row = image_row16(img, y);
        for (x = 0; x < img->width; x++) {
            switch (site_char(cfa, x, y)) {
            case 'R':
                row[x] = r;
                break;
            case 'G':
                row[x] = g;
                break;
            default:
                row[x] = b;
                break;
            }
        }
    }
}

static void test_demosaic_all_phases(void)
{
    static const cfa_t phases[] = { CFA_BGGR, CFA_RGGB, CFA_GBRG, CFA_GRBG };
    const uint16_t r = 4000;
    const uint16_t g = 8000;
    const uint16_t b = 2000;
    size_t i;

    for (i = 0; i < sizeof(phases) / sizeof(phases[0]); i++) {
        image_t *bayer = image_create(32, 32, PIX_BAYER16);
        image_t *rgb = NULL;
        int x;
        int y;

        CHECK(bayer != NULL);
        if (bayer == NULL) {
            continue;
        }
        fill_uniform_bayer(bayer, phases[i], r, g, b);
        CHECK_EQ(isp_demosaic(bayer, phases[i], &rgb), ISP_OK);
        CHECK(rgb != NULL);
        if (rgb == NULL) {
            image_free(bayer);
            continue;
        }
        CHECK_EQ(rgb->format, PIX_RGB16);
        for (y = 0; y < rgb->height; y++) {
            const uint16_t *row = image_row16(rgb, y);
            for (x = 0; x < rgb->width; x++) {
                if (row[3 * x + 0] != r || row[3 * x + 1] != g ||
                    row[3 * x + 2] != b) {
                    fprintf(stderr,
                            "FAIL demosaic %s at (%d,%d): got %u,%u,%u\n",
                            cfa_name(phases[i]), x, y,
                            (unsigned)row[3 * x],
                            (unsigned)row[3 * x + 1],
                            (unsigned)row[3 * x + 2]);
                    failures++;
                    y = rgb->height;
                    break;
                }
            }
        }
        image_free(bayer);
        image_free(rgb);
    }
}

static void test_demosaic_rejects_bad_input(void)
{
    image_t *bayer = image_create(4, 4, PIX_BAYER16);
    image_t *rgb = NULL;
    image_t *odd = image_create(5, 4, PIX_BAYER16);

    CHECK_EQ(isp_demosaic(bayer, (cfa_t)99, &rgb), ISP_E_ARG);
    CHECK_EQ(isp_demosaic(odd, CFA_RGGB, &rgb), ISP_E_SIZE);
    CHECK_EQ(isp_demosaic(NULL, CFA_RGGB, &rgb), ISP_E_FORMAT);
    image_free(bayer);
    image_free(odd);
}

static void test_demosaic_small_sizes(void)
{
    static const int sizes[][2] = {
        { 4, 4 }, { 4, 6 }, { 6, 4 }, { 6, 6 }, { 8, 8 }
    };
    size_t i;

    for (i = 0; i < sizeof(sizes) / sizeof(sizes[0]); i++) {
        image_t *bayer = image_create(sizes[i][0], sizes[i][1], PIX_BAYER16);
        image_t *rgb = NULL;
        int x;
        int y;

        CHECK(bayer != NULL);
        if (bayer == NULL) {
            continue;
        }
        for (y = 0; y < bayer->height; y++) {
            uint16_t *row = image_row16(bayer, y);
            for (x = 0; x < bayer->width; x++) {
                row[x] = (uint16_t)((x * 1237 + y * 977 + 13) & 0xffff);
            }
        }
        CHECK_EQ(isp_demosaic(bayer, CFA_RGGB, &rgb), ISP_OK);
        CHECK(rgb != NULL);
        if (rgb != NULL) {
            CHECK(image_valid(rgb));
        }
        image_free(bayer);
        image_free(rgb);
    }
}

static void test_gamma(void)
{
    image_t *rgb16 = image_create(2, 1, PIX_RGB16);
    image_t *rgb8 = NULL;
    uint16_t *src;

    CHECK(rgb16 != NULL);
    if (rgb16 == NULL) {
        return;
    }
    src = image_row16(rgb16, 0);
    src[0] = 0;
    src[1] = 32768;
    src[2] = 65535;
    src[3] = 65535;
    src[4] = 32768;
    src[5] = 0;

    CHECK_EQ(isp_gamma(rgb16, 0.0f, &rgb8), ISP_OK);
    if (rgb8 != NULL) {
        CHECK_EQ(rgb8->format, PIX_RGB8);
        CHECK_EQ(image_row8(rgb8, 0)[0], 0);
        CHECK_EQ(image_row8(rgb8, 0)[1], 128);
        CHECK_EQ(image_row8(rgb8, 0)[2], 255);
        CHECK_EQ(image_row8(rgb8, 0)[3], 255);
        CHECK_EQ(image_row8(rgb8, 0)[4], 128);
        CHECK_EQ(image_row8(rgb8, 0)[5], 0);
        image_free(rgb8);
        rgb8 = NULL;
    }

    CHECK_EQ(isp_gamma(rgb16, -1.0f, &rgb8), ISP_E_ARG);
    CHECK_EQ(isp_gamma(rgb16, 1000.0f, &rgb8), ISP_E_ARG);
    CHECK_EQ(isp_gamma(rgb16, NAN, &rgb8), ISP_E_ARG);
    image_free(rgb16);
}

static void test_bmp(void)
{
    image_t *img = image_create(3, 2, PIX_RGB8);
    uint8_t *pixels;
    uint8_t *file_data;
    size_t file_size = 0;
    const char *path = "build/test_generated.bmp";

    CHECK(img != NULL);
    if (img == NULL) {
        return;
    }
    pixels = img->data;
    /* Row 0: red, green, blue */
    pixels[0] = 255; pixels[1] = 0;   pixels[2] = 0;
    pixels[3] = 0;   pixels[4] = 255; pixels[5] = 0;
    pixels[6] = 0;   pixels[7] = 0;   pixels[8] = 255;
    /* Row 1: white, gray, black */
    pixels[9] = 255;  pixels[10] = 255; pixels[11] = 255;
    pixels[12] = 128; pixels[13] = 128; pixels[14] = 128;
    pixels[15] = 0;   pixels[16] = 0;   pixels[17] = 0;

    CHECK_EQ(bmp_write(path, img), ISP_OK);
    file_data = read_whole_file(path, &file_size);
    CHECK(file_data != NULL);
    if (file_data != NULL) {
        CHECK_EQ(file_size, 78); /* 54 + 12 * 2 */
        CHECK(file_data[0] == 'B' && file_data[1] == 'M');
        CHECK_EQ(read_u32le(file_data + 2), 78);
        CHECK_EQ(read_u32le(file_data + 10), 54);
        CHECK_EQ(read_u32le(file_data + 14), 40);
        CHECK_EQ(read_u32le(file_data + 18), 3);
        CHECK_EQ(read_u32le(file_data + 22), 2);
        CHECK_EQ(read_u16le(file_data + 26), 1);
        CHECK_EQ(read_u16le(file_data + 28), 24);
        CHECK_EQ(read_u32le(file_data + 30), 0);
        CHECK_EQ(read_u32le(file_data + 34), 24);
        /* Bottom row first: white, gray, black, padding. */
        CHECK_EQ(file_data[54], 255);
        CHECK_EQ(file_data[55], 255);
        CHECK_EQ(file_data[56], 255);
        CHECK_EQ(file_data[57], 128);
        CHECK_EQ(file_data[58], 128);
        CHECK_EQ(file_data[59], 128);
        CHECK_EQ(file_data[60], 0);
        CHECK_EQ(file_data[61], 0);
        CHECK_EQ(file_data[62], 0);
        CHECK_EQ(file_data[63], 0); /* row padding */
        CHECK_EQ(file_data[64], 0);
        CHECK_EQ(file_data[65], 0);
        /* Top row: red BGR 0,0,255; green 0,255,0; blue 255,0,0. */
        CHECK_EQ(file_data[54 + 12], 0);
        CHECK_EQ(file_data[54 + 13], 0);
        CHECK_EQ(file_data[54 + 14], 255);
        CHECK_EQ(file_data[54 + 15], 0);
        CHECK_EQ(file_data[54 + 16], 255);
        CHECK_EQ(file_data[54 + 17], 0);
        CHECK_EQ(file_data[54 + 18], 255);
        CHECK_EQ(file_data[54 + 19], 0);
        CHECK_EQ(file_data[54 + 20], 0);
        free(file_data);
    }
    CHECK_EQ(bmp_write(path, NULL), ISP_E_ARG);
    remove(path);
    image_free(img);
}

static void test_ppm(void)
{
    image_t *img = image_create(2, 1, PIX_RGB8);
    uint8_t *file_data;
    size_t file_size = 0;
    const char *path = "build/test_generated.ppm";

    CHECK(img != NULL);
    if (img == NULL) {
        return;
    }
    img->data[0] = 1; img->data[1] = 2; img->data[2] = 3;
    img->data[3] = 4; img->data[4] = 5; img->data[5] = 6;

    CHECK_EQ(ppm_write(path, img), ISP_OK);
    file_data = read_whole_file(path, &file_size);
    CHECK(file_data != NULL);
    if (file_data != NULL) {
        CHECK_EQ(file_size, 17);
        CHECK(memcmp(file_data, "P6\n2 1\n255\n", 11) == 0);
        CHECK_EQ(file_data[11], 1);
        CHECK_EQ(file_data[12], 2);
        CHECK_EQ(file_data[13], 3);
        CHECK_EQ(file_data[14], 4);
        CHECK_EQ(file_data[15], 5);
        CHECK_EQ(file_data[16], 6);
        free(file_data);
    }
    remove(path);
    image_free(img);
}

static void test_raw_loader(void)
{
    const char *path = "build/test_generated.raw";
    FILE *file = fopen(path, "wb");
    uint8_t bytes[] = { 0x01, 0x00, 0x02, 0x00, 0x03, 0x00,
                        0x04, 0x00, 0xff, 0x03, 0x00, 0x02,
                        0x01, 0x00, 0xaa, 0x02 };
    image_t *raw = NULL;
    uint16_t *row;

    CHECK(file != NULL);
    if (file != NULL) {
        CHECK_EQ(fwrite(bytes, 1, sizeof(bytes), file), sizeof(bytes));
        fclose(file);
    }

    CHECK_EQ(raw_load(path, 4, 2, RAW_FMT_10, &raw), ISP_OK);
    CHECK(raw != NULL);
    if (raw != NULL) {
        CHECK_EQ(raw->format, PIX_BAYER16);
        CHECK_EQ(raw->width, 4);
        CHECK_EQ(raw->height, 2);
        row = image_row16(raw, 0);
        CHECK_EQ(row[0], 1);
        CHECK_EQ(row[1], 2);
        CHECK_EQ(row[2], 3);
        CHECK_EQ(row[3], 4);
        row = image_row16(raw, 1);
        CHECK_EQ(row[0], 0x3ff);
        CHECK_EQ(row[1], 0x200);
        CHECK_EQ(row[2], 0x001);
        CHECK_EQ(row[3], 0x2aa);
        image_free(raw);
        raw = NULL;
    }

    CHECK_EQ(raw_load(path, 8, 8, RAW_FMT_10, &raw), ISP_E_SIZE);
    CHECK(strlen(isp_last_error()) > 0);
    CHECK_EQ(raw_load("build/does-not-exist.raw", 4, 2, RAW_FMT_10, &raw),
             ISP_E_IO);
    CHECK(strlen(isp_last_error()) > 0);
    remove(path);
}

static void test_pipeline(void)
{
    image_t *raw = image_create(16, 16, PIX_BAYER16);
    image_t *rgb = NULL;
    isp_config_t config;

    CHECK(raw != NULL);
    if (raw == NULL) {
        return;
    }
    fill_uniform_bayer(raw, CFA_RGGB, 0, 32768, 65535);

    config.black_level = 0;
    config.white_level = 65535;
    config.cfa = CFA_RGGB;
    config.gamma = 0.0f;

    CHECK_EQ(isp_process(raw, &config, &rgb), ISP_OK);
    CHECK(rgb != NULL);
    if (rgb != NULL) {
        const uint8_t *row = image_row8(rgb, 8);
        CHECK_EQ(rgb->format, PIX_RGB8);
        CHECK_EQ(rgb->width, 16);
        CHECK_EQ(rgb->height, 16);
        CHECK_EQ(row[3 * 8 + 0], 0);
        CHECK_EQ(row[3 * 8 + 1], 128);
        CHECK_EQ(row[3 * 8 + 2], 255);
        image_free(rgb);
    }

    config.white_level = 0;
    CHECK_EQ(isp_process(raw, &config, &rgb), ISP_E_ARG);
    image_free(raw);
}

static void test_cfa_parse(void)
{
    cfa_t cfa = CFA_RGGB;

    CHECK_EQ(cfa_parse("bggr", &cfa), ISP_OK);
    CHECK_EQ(cfa, CFA_BGGR);
    CHECK_EQ(cfa_parse("GRBG", &cfa), ISP_OK);
    CHECK_EQ(cfa, CFA_GRBG);
    CHECK_EQ(cfa_parse("XYZ", &cfa), ISP_E_ARG);
    CHECK(strcmp(cfa_name(CFA_GBRG), "GBRG") == 0);
}

int main(void)
{
    test_image_basics();
    test_unpack();
    test_demosaic_all_phases();
    test_demosaic_rejects_bad_input();
    test_demosaic_small_sizes();
    test_gamma();
    test_bmp();
    test_ppm();
    test_raw_loader();
    test_pipeline();
    test_cfa_parse();

    if (failures != 0) {
        fprintf(stderr, "%d check(s) failed\n", failures);
        return 1;
    }
    printf("all tests passed\n");
    return 0;
}
