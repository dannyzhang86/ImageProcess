#define _POSIX_C_SOURCE 200809L

#include "core/errors.h"
#include "core/image.h"
#include "io/bmp.h"
#include "io/ppm.h"
#include "io/raw.h"
#include "isp/isp.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

typedef struct {
    const char *input;
    const char *output;
    int width;
    int height;
    raw_format_t format;
    cfa_t cfa;
    int black;
    int white;
    float gamma;
} options_t;

static void usage(FILE *stream, const char *program)
{
    fprintf(stream,
            "simpleisp - a small RAW -> RGB image signal pipeline\n"
            "\n"
            "Usage:\n"
            "  %s --in <file> --out <file> --size <W>x<H> \\\n"
            "     --format <fmt> --cfa <phase> [--black N] "
            "[--white N] [--gamma G]\n"
            "\n"
            "Options:\n"
            "  --in <path>    input RAW file, unpacked little-endian samples\n"
            "  --out <path>   output image: .bmp (default) or .ppm\n"
            "  --size <WxH>   image dimensions, e.g. 1280x720\n"
            "  --format <f>   RAW8, RAW10, RAW12, RAW14, RAW16\n"
            "  --cfa <p>      Bayer phase: BGGR, RGGB, GBRG, GRBG\n"
            "  --black <n>    black level in source units (default 0)\n"
            "  --white <n>    saturation level (default 2^bits - 1)\n"
            "  --gamma <g>    display gamma (default 2.2, 0 = linear)\n"
            "  -h, --help     show this help\n"
            "\n"
            "Legacy form:\n"
            "  %s <W> <H> <format> <cfa> <input> <output>\n",
            program, program);
}

static int parse_long(const char *text, long min, long max, long *out)
{
    char *end = NULL;
    long value;

    errno = 0;
    value = strtol(text, &end, 10);
    if (errno != 0 || end == text || *end != '\0' || value < min || value > max) {
        return -1;
    }
    *out = value;
    return 0;
}

static int parse_size(const char *text, int *width, int *height)
{
    char extra;
    int matched = sscanf(text, "%dx%d%c", width, height, &extra);

    if (matched != 2 || *width <= 0 || *height <= 0) {
        return -1;
    }
    return 0;
}

static int ends_with_ignore_case(const char *text, const char *suffix)
{
    size_t text_len = strlen(text);
    size_t suffix_len = strlen(suffix);
    size_t i;

    if (text_len < suffix_len) {
        return 0;
    }
    text += text_len - suffix_len;
    for (i = 0; i < suffix_len; i++) {
        if (text[i] != suffix[i] &&
            !(text[i] >= 'A' && text[i] <= 'Z' &&
              text[i] - 'A' + 'a' == suffix[i])) {
            return 0;
        }
    }
    return 1;
}

static int parse_options(int argc, char **argv, options_t *options)
{
    int i;

    memset(options, 0, sizeof(*options));
    options->gamma = 2.2f;
    options->white = -1;
    options->format = RAW_FMT_8;
    options->cfa = CFA_RGGB;

    if (argc == 7 && argv[1][0] != '-') {
        long width;
        long height;

        if (parse_long(argv[1], 1, 65535, &width) != 0 ||
            parse_long(argv[2], 1, 65535, &height) != 0) {
            fprintf(stderr, "simpleisp: invalid image size '%s' x '%s'\n",
                    argv[1], argv[2]);
            return -1;
        }
        options->width = (int)width;
        options->height = (int)height;
        if (raw_format_parse(argv[3], &options->format) != ISP_OK) {
            fprintf(stderr, "simpleisp: unknown format '%s'\n", argv[3]);
            return -1;
        }
        if (cfa_parse(argv[4], &options->cfa) != ISP_OK) {
            fprintf(stderr, "simpleisp: unknown Bayer phase '%s'\n", argv[4]);
            return -1;
        }
        options->input = argv[5];
        options->output = argv[6];
        options->white = (1 << raw_format_bits(options->format)) - 1;
        return 0;
    }

    for (i = 1; i < argc; i++) {
        const char *arg = argv[i];

        if (strcmp(arg, "-h") == 0 || strcmp(arg, "--help") == 0) {
            usage(stdout, argv[0]);
            exit(0);
        } else if (strcmp(arg, "--in") == 0) {
            if (++i >= argc) {
                fprintf(stderr, "simpleisp: --in needs a value\n");
                return -1;
            }
            options->input = argv[i];
        } else if (strcmp(arg, "--out") == 0) {
            if (++i >= argc) {
                fprintf(stderr, "simpleisp: --out needs a value\n");
                return -1;
            }
            options->output = argv[i];
        } else if (strcmp(arg, "--size") == 0) {
            if (++i >= argc || parse_size(argv[i], &options->width,
                                          &options->height) != 0) {
                fprintf(stderr, "simpleisp: --size expects <W>x<H>\n");
                return -1;
            }
        } else if (strcmp(arg, "--format") == 0) {
            if (++i >= argc || raw_format_parse(argv[i], &options->format) !=
                                   ISP_OK) {
                fprintf(stderr, "simpleisp: --format expects RAW8..RAW16\n");
                return -1;
            }
        } else if (strcmp(arg, "--cfa") == 0) {
            if (++i >= argc || cfa_parse(argv[i], &options->cfa) != ISP_OK) {
                fprintf(stderr, "simpleisp: --cfa expects BGGR, RGGB, "
                                "GBRG or GRBG\n");
                return -1;
            }
        } else if (strcmp(arg, "--black") == 0) {
            long value;
            if (++i >= argc || parse_long(argv[i], 0, 65535, &value) != 0) {
                fprintf(stderr, "simpleisp: --black expects 0..65535\n");
                return -1;
            }
            options->black = (int)value;
        } else if (strcmp(arg, "--white") == 0) {
            long value;
            if (++i >= argc || parse_long(argv[i], 1, 65535, &value) != 0) {
                fprintf(stderr, "simpleisp: --white expects 1..65535\n");
                return -1;
            }
            options->white = (int)value;
        } else if (strcmp(arg, "--gamma") == 0) {
            char *end = NULL;
            float value;
            if (++i >= argc) {
                fprintf(stderr, "simpleisp: --gamma needs a value\n");
                return -1;
            }
            errno = 0;
            value = strtof(argv[i], &end);
            if (errno != 0 || end == argv[i] || *end != '\0' || value < 0.0f) {
                fprintf(stderr, "simpleisp: --gamma expects a non-negative "
                                "number\n");
                return -1;
            }
            options->gamma = value;
        } else {
            fprintf(stderr, "simpleisp: unknown option '%s'\n", arg);
            return -1;
        }
    }

    if (options->input == NULL || options->output == NULL ||
        options->width <= 0 || options->height <= 0) {
        fprintf(stderr, "simpleisp: missing required arguments\n");
        return -1;
    }

    if (options->white < 0) {
        options->white = (1 << raw_format_bits(options->format)) - 1;
    }
    return 0;
}

static double elapsed_ms(const struct timespec *start,
                         const struct timespec *end)
{
    return (double)(end->tv_sec - start->tv_sec) * 1000.0 +
           (double)(end->tv_nsec - start->tv_nsec) / 1000000.0;
}

static void report_error(const char *what, int err)
{
    const char *detail = isp_last_error();

    if (detail != NULL && detail[0] != '\0') {
        fprintf(stderr, "simpleisp: %s: %s (%s)\n", what, isp_strerror(err),
                detail);
    } else {
        fprintf(stderr, "simpleisp: %s: %s\n", what, isp_strerror(err));
    }
}

int main(int argc, char **argv)
{
    options_t options;
    isp_config_t config;
    image_t *raw = NULL;
    image_t *rgb = NULL;
    struct timespec start;
    struct timespec end;
    int err;

    if (parse_options(argc, argv, &options) != 0) {
        usage(stderr, argv[0]);
        return 2;
    }

    fprintf(stderr,
            "simpleisp: %dx%d %s %s black=%d white=%d gamma=%.2f\n",
            options.width, options.height,
            raw_format_name(options.format), cfa_name(options.cfa),
            options.black, options.white, options.gamma);

    err = raw_load(options.input, options.width, options.height,
                   options.format, &raw);
    if (err != ISP_OK) {
        report_error("load failed", err);
        return 1;
    }

    config.black_level = options.black;
    config.white_level = options.white;
    config.cfa = options.cfa;
    config.gamma = options.gamma;

    clock_gettime(CLOCK_MONOTONIC, &start);
    err = isp_process(raw, &config, &rgb);
    clock_gettime(CLOCK_MONOTONIC, &end);

    if (err != ISP_OK) {
        report_error("processing failed", err);
        image_free(raw);
        return 1;
    }

    if (ends_with_ignore_case(options.output, ".ppm") ||
        ends_with_ignore_case(options.output, ".pnm")) {
        err = ppm_write(options.output, rgb);
    } else {
        err = bmp_write(options.output, rgb);
    }
    if (err != ISP_OK) {
        report_error("write failed", err);
        image_free(raw);
        image_free(rgb);
        return 1;
    }

    {
        double ms = elapsed_ms(&start, &end);
        double megapixels = (double)options.width * options.height / 1e6;

        printf("wrote %s (%dx%d, %.1f ms, %.1f MP/s)\n", options.output,
               rgb->width, rgb->height, ms,
               ms > 0.0 ? megapixels * 1000.0 / ms : 0.0);
    }

    image_free(raw);
    image_free(rgb);
    return 0;
}
