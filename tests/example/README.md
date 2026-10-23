# Test sample data

These captures are used by `tests/test.sh` and are intentionally kept small
enough to live in the repository.

| File | Format | CFA | Content |
|---|---|---|---|
| `1280x720_RGGB_10_0.raw` | `RAW10` | `RGGB` | 1280×720 synthetic radial gradient |
| `2564x1926_8_0.RAW` | `RAW8` | `BGGR` | 2564×1926 X-Rite ColorChecker capture |

Both files store unpacked little-endian samples.  The ColorChecker capture is
useful for developing and evaluating the Phase 2 AWB/CCM stages; the 10-bit
gradient is a good unpack/gamma smoke test.
