#!/usr/bin/env python3
"""
Merge several .carafe_inten files (tools/carafe_cps_to_inten.py, format v3) built against
the SAME .idx / VarModConfig / channel layout into one -- docs/20260903_IntensityScore_design.md
Phase 1d.

The use case is per-charge records whose predictions were produced in separate Carafe runs:
e.g. an existing 2+ store and a later 3+-only run (predicting only the new charge saves a
full re-run of the expensive inference). Each run yields its own --per-charge file; this
tool k-way merges them by (iWhichPeptide, modNumIdx, cNtermMod, cCtermMod, charge) into one
file the C++ loader can bind in one go.

Usage:
  carafe_inten_merge.py <out_inten_file> <in_1.carafe_inten> <in_2.carafe_inten> [...]

Checks: identical SourceIdxFingerprint, SourceIdxNumRawPeptides, VarModConfig, Channels,
Transform and Quant across inputs (a mismatch is a hard error -- the files describe
different peptide universes); every input must be PerCharge: 1 (merging merged charge-0
records from different runs would be meaningless); the output is re-read and verified
strictly increasing by (key, charge), so a duplicate (variant, charge) across inputs is a
loud abort, not a silent double record. Header fields other than the checked ones are taken
from the first input; SourceCpsPath lists every input's value.
"""

import argparse
import heapq
import struct
import sys
import time
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import carafe_cps_to_inten as inten  # noqa: E402

MUST_MATCH = ("SourceIdxFingerprint", "SourceIdxNumRawPeptides", "VarModConfig", "Channels",
              "Transform", "Quant", "PerCharge")


def iter_file_entries(path):
    """Stream ((key, charge), entry_bytes) from a .carafe_inten file, in file order."""
    with open(path, "rb") as f:
        _header, count = inten.read_header(f)
        carry = b""
        n = 0
        while True:
            chunk = f.read(1 << 24)
            if not chunk and not carry:
                break
            buf = carry + chunk
            off = 0
            while True:
                if off + inten.KEY_SIZE + 2 > len(buf):
                    break
                (npk,) = struct.unpack_from(inten.COUNT_FMT, buf, off + inten.KEY_SIZE + 1)
                entry_len = inten.KEY_SIZE + 2 + npk * inten.PEAK_SIZE
                if off + entry_len > len(buf):
                    break
                key, charge, _peaks, nxt = inten.unpack_entry_at(buf, off)
                yield (key, charge), buf[off:nxt]
                n += 1
                off = nxt
            carry = buf[off:]
            if not chunk:
                if carry:
                    raise ValueError(f"{path!r}: {len(carry)} trailing bytes after entry {n}")
                break
        if n != count:
            raise ValueError(f"{path!r}: header count {count} != entries read {n}")


def read_header_only(path):
    with open(path, "rb") as f:
        header, count = inten.read_header(f)
    return header, count


def merge(out_path, in_paths):
    headers = [read_header_only(p) for p in in_paths]
    ref = headers[0][0]
    for p, (h, _c) in zip(in_paths, headers):
        for k in MUST_MATCH:
            if h.get(k) != ref.get(k):
                raise ValueError(f"{p!r}: header {k}={h.get(k)!r} differs from {in_paths[0]!r}'s "
                                 f"{ref.get(k)!r}; these files do not describe the same peptide universe")
        if h.get("PerCharge") != "1":
            raise ValueError(f"{p!r}: PerCharge must be 1 for merging (got {h.get('PerCharge')!r})")
    total = sum(c for _h, c in headers)

    hdr_lines = []
    for k, v in ref.items():
        if k == "SourceCpsPath":
            v = " | ".join(read_header_only(p)[0].get("SourceCpsPath", "") for p in in_paths)
        if k == "SourceIdxPath":
            pass
        hdr_lines.append(f"{k}: {v}")
    if "SourceCpsPath" not in ref:
        hdr_lines.append("SourceCpsPath: " + " | ".join(in_paths))

    merged = (e for _kz, e in heapq.merge(*(iter_file_entries(p) for p in in_paths),
                                          key=lambda kp: kp[0]))
    inten.write_inten_file(out_path, hdr_lines, total, merged)
    return total


def main(argv=None):
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("out_inten_file")
    ap.add_argument("in_inten_files", nargs="+")
    args = ap.parse_args(argv)
    if len(args.in_inten_files) < 2:
        sys.exit("need at least two input files")
    t0 = time.time()
    total = merge(args.out_inten_file, args.in_inten_files)
    n, n_peaks = inten.verify_written_inten_sorted(args.out_inten_file)
    if n != total:
        raise ValueError(f"wrote {n} entries, expected {total}")
    print(f"Done: {args.out_inten_file!r}, {n} entries, {n_peaks} peaks, from {len(args.in_inten_files)} "
          f"files, {time.time() - t0:.0f}s", file=sys.stderr)


if __name__ == "__main__":
    main()
