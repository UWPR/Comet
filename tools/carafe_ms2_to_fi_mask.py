#!/usr/bin/env python3
"""
Turn Carafe's predicted MS2 spectra into a per-variant fragment-ion keep/drop mask for
Comet's fragment ion index (FI) -- Phase 2a of docs/20260805_carafe.md ("general" mode only;
phospho modloss channels are Phase 2c, gated on Phase 2b adding a modloss ion class to the FI
itself, which does not exist yet).

Inputs (three files from a single tools/idx_to_carafe.py export + Carafe ai_pred.py run
against it, all still row-aligned with each other -- see docs/20260805_carafe.md Section 6.3
for the empirical confirmation that Carafe preserves row order end-to-end):

  1. <out_tsv>.variants.tsv  -- tools/idx_to_carafe.py's sidecar: row_index -> the FI variant
     identity tuple (iWhichPeptide, modNumIdx, cNtermMod, cCtermMod). Multiple row_index values
     can belong to one tuple (one per predicted charge state -- idx_to_carafe.py emits one
     out_tsv row per configured charge); multiple tuples can point at the SAME row_index
     (idx_to_carafe.py's dedup collapsing identical sequence/mods/mod_sites/charge rows).
  2. <prefix>_ms2_df.tsv     -- Carafe's echoed-back input rows, augmented with nAA and
     frag_start_idx/frag_stop_idx (AlphaBase convention: row_index's fragments occupy
     [frag_start_idx, frag_stop_idx) in ms2_pred.tsv, one row per cleavage site, nAA-1 rows).
  3. <prefix>_ms2_pred.tsv   -- the flat fragment table, columns b_z1/b_z2/y_z1/y_z2, relative
     intensities, row-sliced per (2) above.

For every FI variant tuple, across however many charge states were predicted for it:

  1. Take the per-fragment MAX intensity across those charge states (docs/20260805_carafe.md
     Section 8 item 1) -- one "best case" nAA-1 x 4 matrix per variant.
  2. Base peak = the single largest value anywhere in that matrix (all 4 channels; Section 8
     item 3 -- NOT restricted to the b_z1/y_z1 channels the FI can actually insert, so a
     genuinely charge-2-dominant peptide's charge-1 ions get thresholded against the real
     spectrum's shape, not an artificially FI-scoped one).
  3. Candidate-keep each b/y ion of length >= 3 (lengths 1-2 are never indexed by Comet's FI
     regardless of prediction -- CometFragmentIndex.cpp's `if (i > 1)` gate) whose b_z1/y_z1
     intensity clears --min-relative-intensity * base_peak.
  4. Minimum-fragment floor (Section 8 item 4): if fewer than --min-kept-peaks candidates
     cleared the threshold, top up by intensity from the combined pool of length>=3 b/y
     candidates (not a fixed b/y split) until either the floor is reached or every eligible
     candidate is kept, whichever comes first.
  5. Pack the result as two bitmasks (b-kept, y-kept) -- see "Bit-index convention" below.

Bit-index convention (must match CometFragmentIndex.cpp's consumer exactly -- Phase 3):
Comet's AddFragments() loop index i (0-indexed) computes b-ion length i+1 AND y-ion length i+1
together at each iteration (a forward+backward two-pointer sweep, NOT the complementary
cleavage-site pairing AlphaBase uses -- see docs/20260805_carafe.md Section 5 pitfall 2), and
only ever indexes i > 1 (length >= 3). So:

  - bMask bit (i-2) set  <=>  keep b-ion of length i+1.  AlphaBase row i's b column IS b_{i+1}
    directly (same row, no remap -- AlphaBase row r's b column is always b_{r+1}).
  - yMask bit (i-2) set  <=>  keep y-ion of length i+1.  AlphaBase row r's y column is
    y_{nAA-r-1} (length nAA-r-1), so y_{i+1} (length i+1) lives at row (nAA-2-i), NOT row i --
    confirmed both structurally (from AlphaBase's mass_calc.py source) and numerically
    (docs/20260805_carafe.md Section 6.1) before this script was written.

Bit 0 = i=2 (the shortest indexable ion, length 3), packing from the bottom so the C++ side
can test with `mask & (1ULL << (i - 2))`. MAX_PEPTIDE_LEN is 51 (CometSearch/core/Constants.h),
so i ranges over at most 0..48 after the -2 shift -- comfortably inside one uint64_t; no need
for two words per mask.

Known Phase 2a scale limitation: this script loads the full <prefix>_ms2_pred.tsv into memory
(plain Python lists, no pandas/numpy dependency -- matching tools/idx_to_carafe.py's
zero-external-dependency convention). Fine at the scale tested so far; a genuinely
hundreds-of-millions-of-row FASTA-scale run (docs/20260805_carafe.md Section 7's own noted
scale concern for the Carafe inference step itself) would need a chunked/streaming rewrite of
the ms2_pred.tsv read. Not attempted here -- revisit if Phase 5 benchmarking shows it matters.
"""

import argparse
import csv
import hashlib
import os
import struct
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import idx_to_carafe as itc

MAX_PEPTIDE_LEN = 51   # CometSearch/core/Constants.h:24 -- one more than the actual max # of AA
MIN_ION_LENGTH = 3      # CometFragmentIndex.cpp AddFragments()'s "if (i > 1)" gate -- lengths
                         # 1 and 2 are never indexed regardless of any prediction

DEFAULT_MIN_RELATIVE_INTENSITY = 0.10
# Mirrors the FRAGINDEX_CARAFE_MIN_KEPT_PEAKS constant Phase 3 will add to
# CometSearch/core/Constants.h (docs/20260805_carafe.md Section 4.2 step 4) -- keep the two
# defaults in sync if either changes.
DEFAULT_MIN_KEPT_PEAKS = 6

MASK_FILE_MAGIC = b"Comet Carafe FI mask v1\n"


# ---------------------------------------------------------------------------
# .idx fingerprint -- ties a mask file to one specific already-built .idx (Section 4.3;
# iWhichPeptide/modNumIdx numbering is only stable across reads of one built .idx file, not
# across independent rebuilds of the same FASTA -- pitfall 4).
# ---------------------------------------------------------------------------

def idx_fingerprint(idx_path, chunk_size=1 << 20):
    """SHA-256 over the .idx byte range [pep_pos, var_pos) -- the raw peptide table, protein
    list, and mod-permutation tables, i.e. exactly the content that determines iWhichPeptide/
    modNumIdx numbering when CometFragmentIndex.cpp regenerates the FI from this file. The
    trailing compact variant array is deliberately excluded: FI_DB never reads it (Section 2.2),
    so its content is irrelevant to whether a mask keyed by this fingerprint still applies."""
    reader = itc.IdxReader(idx_path)
    f = reader.f
    f.seek(reader.pep_pos)
    remaining = reader.var_pos - reader.pep_pos
    h = hashlib.sha256()
    while remaining > 0:
        n = min(chunk_size, remaining)
        buf = f.read(n)
        if not buf:
            break
        h.update(buf)
        remaining -= len(buf)

    # Cheap, human-readable cross-check alongside the hash: NumRawPeptides, read directly
    # from the raw-peptide-table's leading count(u64) rather than via the heavier
    # read_raw_peptides() (which materializes every peptide just to count them).
    f.seek(reader.pep_pos)
    (num_raw,) = struct.unpack("<Q", f.read(8))

    return h.hexdigest(), num_raw


# ---------------------------------------------------------------------------
# Input readers
# ---------------------------------------------------------------------------

def read_variant_map(path):
    """row_index -> list of (iWhichPeptide, modNumIdx, cNtermMod, cCtermMod) tuples, grouped
    the other way from how tools/idx_to_carafe.py wrote it (one line per (row_index, tuple)
    pair) -- callers here want tuple -> [row_index, ...] instead, built by the caller from this."""
    rows = []   # (row_index, tuple)
    with open(path, "r", newline="") as f:
        reader = csv.DictReader(f, delimiter="\t")
        for row in reader:
            row_index = int(row["row_index"])
            key = (int(row["iWhichPeptide"]), int(row["modNumIdx"]),
                   int(row["cNtermMod"]), int(row["cCtermMod"]))
            rows.append((row_index, key))
    return rows


def read_ms2_df(path):
    """row_index -> (nAA, frag_start_idx, frag_stop_idx). row_index is the 0-based data-row
    position in this file, matching tools/idx_to_carafe.py's out_tsv row order exactly
    (docs/20260805_carafe.md Section 6.3 confirms Carafe preserves this end to end)."""
    out = []
    with open(path, "r", newline="") as f:
        reader = csv.DictReader(f, delimiter="\t")
        for row in reader:
            out.append((int(row["nAA"]), int(row["frag_start_idx"]), int(row["frag_stop_idx"])))
    return out


def read_ms2_pred(path):
    """Flat fragment table -> list of (b_z1, b_z2, y_z1, y_z2) floats, one per cleavage-site
    row, in file order (sliced per-precursor via ms2_df's frag_start_idx/frag_stop_idx)."""
    out = []
    with open(path, "r", newline="") as f:
        reader = csv.DictReader(f, delimiter="\t")
        for row in reader:
            out.append((float(row["b_z1"]), float(row["b_z2"]),
                        float(row["y_z1"]), float(row["y_z2"])))
    return out


# ---------------------------------------------------------------------------
# Per-variant mask computation
# ---------------------------------------------------------------------------

def max_across_charges(frag_rows_per_charge):
    """frag_rows_per_charge: list of per-charge fragment-row lists, each a list of
    (b_z1, b_z2, y_z1, y_z2) of the SAME length (nAA-1). Returns the elementwise-max matrix,
    same shape -- Section 8 item 1's "per-fragment max intensity across charges"."""
    n_rows = len(frag_rows_per_charge[0])
    merged = []
    for r in range(n_rows):
        b_z1 = max(rows[r][0] for rows in frag_rows_per_charge)
        b_z2 = max(rows[r][1] for rows in frag_rows_per_charge)
        y_z1 = max(rows[r][2] for rows in frag_rows_per_charge)
        y_z2 = max(rows[r][3] for rows in frag_rows_per_charge)
        merged.append((b_z1, b_z2, y_z1, y_z2))
    return merged


def compute_variant_mask(merged_rows, nAA, min_relative_intensity, min_kept_peaks):
    """merged_rows: nAA-1 rows of (b_z1, b_z2, y_z1, y_z2), already max-across-charges.
    Returns (bMask, yMask, n_candidates, n_kept, base_peak)."""
    if nAA - 1 != len(merged_rows):
        raise ValueError(f"nAA={nAA} implies {nAA - 1} fragment rows, got {len(merged_rows)}")

    # Base peak: max over every predicted channel (Section 8 item 3), not just b_z1/y_z1.
    base_peak = max(v for row in merged_rows for v in row) if merged_rows else 0.0

    # AlphaBase row r's own columns give b_{r+1} and y_{nAA-r-1} directly -- relabel into
    # length-indexed dicts so the Comet loop-index remap (b: same length index i; y: mirrored)
    # falls out as a simple lookup rather than needing to juggle row indices later.
    b_by_length = {}   # length -> b_z1 intensity
    y_by_length = {}   # length -> y_z1 intensity
    for r, (b_z1, _b_z2, y_z1, _y_z2) in enumerate(merged_rows):
        b_by_length[r + 1] = b_z1
        y_by_length[nAA - r - 1] = y_z1

    threshold = min_relative_intensity * base_peak

    # Candidate pool: every length>=3 b/y ion, tagged so the floor top-up (step below) can
    # pull from the combined pool rather than a fixed b/y split.
    candidates = []   # (intensity, 'b'|'y', length)
    for length, intensity in b_by_length.items():
        if length >= MIN_ION_LENGTH:
            candidates.append((intensity, "b", length))
    for length, intensity in y_by_length.items():
        if length >= MIN_ION_LENGTH:
            candidates.append((intensity, "y", length))

    candidates.sort(key=lambda c: c[0], reverse=True)
    n_above_threshold = sum(1 for c in candidates if c[0] >= threshold)
    n_keep = max(n_above_threshold, min(min_kept_peaks, len(candidates)))
    kept = candidates[:n_keep]

    bMask = 0
    yMask = 0
    for _intensity, ion_type, length in kept:
        i = length - 1   # Comet loop index i, since length == i+1 for both b and y at index i
        bit = i - MIN_ION_LENGTH + 1  # bit (i-2): bit 0 == i==2 (length 3)
        if ion_type == "b":
            bMask |= (1 << bit)
        else:
            yMask |= (1 << bit)

    return bMask, yMask, len(candidates), len(kept), base_peak


# ---------------------------------------------------------------------------
# Mask file writer
# ---------------------------------------------------------------------------

ENTRY_FMT = "<IibbQQ"   # iWhichPeptide(u32) modNumIdx(i32) cNtermMod(i8) cCtermMod(i8) bMask(u64) yMask(u64)
ENTRY_SIZE = struct.calcsize(ENTRY_FMT)


def write_mask_file(path, fingerprint, num_raw_peptides, idx_path, threshold, min_kept_peaks,
                     entries):
    """entries: iterable of (iWhichPeptide, modNumIdx, cNtermMod, cCtermMod, bMask, yMask),
    NOT required to be pre-sorted -- sorted here so the file is binary-searchable at FI build
    time (Section 4.3)."""
    entries = sorted(entries, key=lambda e: e[:4])
    with open(path, "wb") as f:
        f.write(MASK_FILE_MAGIC)
        f.write(f"SourceIdxFingerprint: {fingerprint}\n".encode("ascii"))
        f.write(f"SourceIdxNumRawPeptides: {num_raw_peptides}\n".encode("ascii"))
        f.write(f"SourceIdxPath: {idx_path}\n".encode("ascii"))
        f.write(f"MinRelativeIntensity: {threshold}\n".encode("ascii"))
        f.write(f"MinKeptPeaks: {min_kept_peaks}\n".encode("ascii"))
        f.write(f"GeneralMode: 1\n".encode("ascii"))  # Phase 2c will add phospho-mode masks
        f.write(b"\n")
        f.write(struct.pack("<Q", len(entries)))
        for e in entries:
            f.write(struct.pack(ENTRY_FMT, *e))


def read_mask_file(path):
    """Companion reader (for testing / Phase 3 prototyping) -- mirrors write_mask_file()."""
    with open(path, "rb") as f:
        magic = f.readline()
        if magic != MASK_FILE_MAGIC:
            raise ValueError(f"{path!r}: not a {MASK_FILE_MAGIC!r} mask file (got {magic!r})")
        header = {}
        while True:
            line = f.readline()
            if not line or line == b"\n":
                break
            text = line.decode("ascii").rstrip("\n")
            key, _, val = text.partition(": ")
            header[key] = val

        (count,) = struct.unpack("<Q", f.read(8))
        entries = []
        for _ in range(count):
            entries.append(struct.unpack(ENTRY_FMT, f.read(ENTRY_SIZE)))
        return header, entries


# ---------------------------------------------------------------------------
# main
# ---------------------------------------------------------------------------

def main():
    ap = argparse.ArgumentParser(
        description="Build a predicted-fragment keep/drop mask for Comet's FI from Carafe's "
                     "MS2 prediction output.",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog=__doc__)
    ap.add_argument("idx_file", help="The same Comet .idx file idx_to_carafe.py exported from")
    ap.add_argument("variant_map_tsv", help="idx_to_carafe.py's --variant-map output")
    ap.add_argument("ms2_df_tsv", help="Carafe ai_pred.py's <prefix>_ms2_df.tsv output")
    ap.add_argument("ms2_pred_tsv", help="Carafe ai_pred.py's <prefix>_ms2_pred.tsv output")
    ap.add_argument("out_mask_file", help="Output mask file path")
    ap.add_argument("--min-relative-intensity", type=float, default=DEFAULT_MIN_RELATIVE_INTENSITY,
                     help=f"Keep a b/y ion if its intensity is >= this fraction of the "
                          f"variant's predicted base peak (default: {DEFAULT_MIN_RELATIVE_INTENSITY})")
    ap.add_argument("--min-kept-peaks", type=int, default=DEFAULT_MIN_KEPT_PEAKS,
                     help=f"Always keep at least this many of a variant's most intense "
                          f"length>=3 candidates, even if below the threshold (default: "
                          f"{DEFAULT_MIN_KEPT_PEAKS}; mirrors the eventual "
                          f"FRAGINDEX_CARAFE_MIN_KEPT_PEAKS C++ constant)")
    ap.add_argument("-v", "--verbose", action="store_true")
    args = ap.parse_args()

    if args.verbose:
        print(f"Fingerprinting {args.idx_file} ...", file=sys.stderr)
    fingerprint, num_raw_peptides = idx_fingerprint(args.idx_file)
    if args.verbose:
        print(f"  fingerprint {fingerprint}, {num_raw_peptides} raw peptides", file=sys.stderr)

    if args.verbose:
        print(f"Reading {args.variant_map_tsv} ...", file=sys.stderr)
    vmap_rows = read_variant_map(args.variant_map_tsv)

    if args.verbose:
        print(f"Reading {args.ms2_df_tsv} ...", file=sys.stderr)
    ms2_df = read_ms2_df(args.ms2_df_tsv)

    if args.verbose:
        print(f"Reading {args.ms2_pred_tsv} ...", file=sys.stderr)
    ms2_pred = read_ms2_pred(args.ms2_pred_tsv)

    # Group variant-map rows by tuple -> [row_index, ...] (one row_index per predicted charge
    # state that ended up sharing this tuple's (sequence, mods, mod_sites) -- see module
    # docstring). A dict preserves first-seen order, which is nice for --verbose output but not
    # otherwise relied on.
    tuple_to_rows = {}
    for row_index, key in vmap_rows:
        tuple_to_rows.setdefault(key, []).append(row_index)

    n_variants = 0
    n_candidates_total = 0
    n_kept_total = 0
    entries = []

    for key, row_indices in tuple_to_rows.items():
        frag_rows_per_charge = []
        nAA_seen = None
        for row_index in row_indices:
            nAA, start, stop = ms2_df[row_index]
            if nAA_seen is None:
                nAA_seen = nAA
            elif nAA_seen != nAA:
                # Defensive hardening check, matching idx_to_carafe.py's own style -- every
                # charge-row of the same tuple must be the same peptide, hence same nAA. A
                # mismatch would mean the variant map and ms2_df.tsv have desynced.
                print(f"WARNING: tuple {key} has mismatched nAA across its charge rows "
                      f"({nAA_seen} vs {nAA} at row_index {row_index}) -- skipping", file=sys.stderr)
                frag_rows_per_charge = None
                break
            frag_rows_per_charge.append(ms2_pred[start:stop])

        if not frag_rows_per_charge:
            continue

        bMask, yMask, n_candidates, n_kept, base_peak = compute_variant_mask(
            max_across_charges(frag_rows_per_charge), nAA_seen,
            args.min_relative_intensity, args.min_kept_peaks)

        n_variants += 1
        n_candidates_total += n_candidates
        n_kept_total += n_kept
        entries.append((key[0], key[1], key[2], key[3], bMask, yMask))

        if args.verbose and n_variants <= 5:
            print(f"  variant {key}: nAA={nAA_seen}, base_peak={base_peak:.4g}, "
                  f"{n_kept}/{n_candidates} kept, bMask={bMask:#x}, yMask={yMask:#x}",
                  file=sys.stderr)

    write_mask_file(args.out_mask_file, fingerprint, num_raw_peptides, args.idx_file,
                     args.min_relative_intensity, args.min_kept_peaks, entries)

    avg_kept = (n_kept_total / n_variants) if n_variants else 0.0
    avg_candidates = (n_candidates_total / n_variants) if n_variants else 0.0
    print(f"Wrote {len(entries)} variant masks to {args.out_mask_file}", file=sys.stderr)
    print(f"Average {avg_kept:.2f} of {avg_candidates:.2f} eligible fragments kept per variant "
          f"({100.0 * n_kept_total / n_candidates_total:.1f}% overall)"
          if n_candidates_total else "No eligible fragments seen", file=sys.stderr)


if __name__ == "__main__":
    main()
