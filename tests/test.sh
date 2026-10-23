#!/bin/sh
# Build, run the unit tests, and process the bundled RAW samples.
set -eu

# Allow running this script from any directory.
root=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
cd "$root"

make test

# 10-bit RGGB synthetic gradient.
./simpleisp \
    --in tests/example/1280x720_RGGB_10_0.raw \
    --out build/sample_10bit.bmp \
    --size 1280x720 \
    --format RAW10 \
    --cfa RGGB \
    --black 256 \
    --white 1023

# 8-bit BGGR X-Rite ColorChecker capture.
./simpleisp \
    --in tests/example/2564x1926_8_0.RAW \
    --out build/colorchecker.bmp \
    --size 2564x1926 \
    --format RAW8 \
    --cfa BGGR

echo "wrote build/sample_10bit.bmp and build/colorchecker.bmp"
