# Legacy sources

This directory keeps the original single-file demo for reference:

- `ImageProcess.c` — CLI and fixed-size stack buffers
- `RAW2RGB24.c` — unpacking and the V4L2-derived demosaic (BGR, 8-bit)
- `Gamma.c` — gamma table
- `BMPOperation.c` — BMP writer with platform-dependent structs
- `LUT10_8.h` … `LUT16_8.h` — generated log/gamma lookup tables (unused by
  the new pipeline)

The active implementation lives under `src/`.  These files are not compiled by
the Makefile and are kept only so the original work and its attribution stay
available.
