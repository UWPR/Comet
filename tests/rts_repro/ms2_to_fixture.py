#!/usr/bin/env python3
"""
Convert a .ms2 file into rts_repro's SPECTRUM fixture format.

rts_repro.cpp's fixture_spectra.txt was originally extracted from a Windows .raw
file via a temporary env-var-gated dump in SearchMS1MS2.cs (see rts_repro/README.md).
This script gets the same fixture format directly from any .ms2 file already sitting
in the repo (no Windows/.raw dependency), so RTS-vs-batch comparisons can reuse the
exact same spectra a batch comet.exe search runs against.

Each Z line in the source .ms2 produces one SPECTRUM block (a scan with multiple Z
lines -- multiple candidate charge states -- produces multiple blocks, all sharing
the S line's precursor m/z).

Usage:
    python3 ms2_to_fixture.py input.ms2 > fixture.txt
"""

import sys


def convert(path):
    scan = None
    mz = None
    charges = []
    peaks = []
    out = []

    def flush():
        for charge in charges:
            out.append(f"SPECTRUM {scan} {charge} {mz} {len(peaks)}")
            for m, i in peaks:
                out.append(f"{m} {i}")

    with open(path) as f:
        for line in f:
            parts = line.split()
            if not parts:
                continue
            tag = parts[0]
            if tag == "S":
                if scan is not None and charges:
                    flush()
                scan = int(parts[1])
                mz = float(parts[3])
                charges = []
                peaks = []
            elif tag == "Z":
                charges.append(int(parts[1]))
            elif tag in ("H", "I", "D"):
                continue
            else:
                if scan is not None and len(parts) >= 2:
                    peaks.append((parts[0], parts[1]))
    if scan is not None and charges:
        flush()
    return out


def main():
    if len(sys.argv) != 2:
        print(f"usage: {sys.argv[0]} input.ms2 > fixture.txt", file=sys.stderr)
        sys.exit(1)
    for line in convert(sys.argv[1]):
        print(line)


if __name__ == "__main__":
    main()
