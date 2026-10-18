"""Compares two PFM images from a capture run.

Usage: python tools/scripts/pfmdiff.py a.pfm b.pfm [threshold]

Prints the largest and the mean absolute difference over all channels, and
how many pixels differ by more than threshold (default 0.01) in any
channel. Exit code 1 if any pixel is over threshold.
"""

import array
import sys


def read_pfm(path):
    with open(path, "rb") as f:
        kind = f.readline().strip()
        width, height = (int(v) for v in f.readline().split())
        scale = float(f.readline())
        channels = 3 if kind == b"PF" else 1
        data = array.array("f")
        data.frombytes(f.read(width * height * channels * 4))
    if (scale < 0) != (sys.byteorder == "little"):
        data.byteswap()
    return width, height, channels, data


def main():
    wa, ha, ca, a = read_pfm(sys.argv[1])
    wb, hb, cb, b = read_pfm(sys.argv[2])
    if (wa, ha, ca) != (wb, hb, cb):
        sys.exit(f"size differs: {wa}x{ha}x{ca} vs {wb}x{hb}x{cb}")
    threshold = float(sys.argv[3]) if len(sys.argv) > 3 else 0.01
    largest = 0.0
    total = 0.0
    over = 0
    for p in range(wa * ha):
        pixel_over = False
        for c in range(ca):
            d = abs(a[p * ca + c] - b[p * ca + c])
            total += d
            largest = max(largest, d)
            pixel_over = pixel_over or d > threshold
        over += pixel_over
    print(f"max={largest:.6f} mean={total / len(a):.6f} "
          f"pixels over {threshold}: {over} of {wa * ha}")
    sys.exit(1 if over else 0)


if __name__ == "__main__":
    main()
