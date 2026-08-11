#!/usr/bin/env python3
"""
Turn Carafe's predicted MS2 spectra into a per-variant fragment-ion keep/drop mask for
Comet's fragment ion index (FI) -- Phase 2a/2c of docs/20260805_carafe.md. Phase 2a covered
"general" mode (b_z1/b_z2/y_z1/y_z2 only); Phase 2c adds phospho mode's modloss channels
(b_modloss_z1/b_modloss_z2/y_modloss_z1/y_modloss_z2), gated on Phase 2b having added a real
neutral-loss ion class to the FI itself (CometFragmentIndex.cpp's AddFragments(), Section 6.6).

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
  3. <prefix>_ms2_pred.tsv   -- the flat fragment table. Columns b_z1/b_z2/y_z1/y_z2 always;
     additionally b_modloss_z1/b_modloss_z2/y_modloss_z1/y_modloss_z2 when Carafe was run with
     `--mode phosphorylation` (`ai_pred.py`'s `mode_type != 'general'` output shape, Section
     2.3). Mode is auto-detected here from this file's header -- no CLI flag needed (matches
     Section 8 item 6's minimal-surface philosophy): the file itself unambiguously says which
     channels it has.

For every FI variant tuple, across however many charge states were predicted for it:

  1. Take the per-fragment MAX intensity across those charge states (docs/20260805_carafe.md
     Section 8 item 1) -- one "best case" nAA-1 x (4 or 8) matrix per variant.
  2. Base peak = the single largest value anywhere in that matrix, across EVERY predicted
     channel including modloss (Section 8 item 3; Section 4.2 step 2 explicitly calls out
     "plus b_modloss_z1/z2/y_modloss_z1/z2 in phospho mode") -- NOT restricted to the b_z1/y_z1/
     b_modloss_z1/y_modloss_z1 channels the FI can actually insert, so a genuinely charge-2- or
     modloss-dominant peptide's insertable ions still get thresholded against the real
     spectrum's shape, not an artificially FI-scoped one.
  3. Candidate-keep each UNSHIFTED b/y ion of length >= 3 whose b_z1/y_z1 intensity clears
     --min-relative-intensity * base_peak, exactly as Phase 2a.
  4. Phase 2c, new: independently candidate-keep each MODLOSS b/y ion of length >= 3 whose
     b_modloss_z1/y_modloss_z1 intensity clears the SAME threshold (against the SAME base
     peak) -- a completely separate pool from step 3's, per Section 8 item 11 ("the FI inserts
     both the unshifted and the loss-shifted fragment as separate entries... so Phase 2c's
     masking can independently keep/drop each"). Skipped entirely (both modloss masks left at
     0, no candidates counted) when either: (a) the input has no modloss columns at all
     (general-mode prediction -- there is nothing to threshold), or (b) this variant is
     unmodified (modNumIdx == -1) -- CometFragmentIndex.cpp's `bFragmentNL` gate
     (`bUseFragmentNeutralLoss && modNumIdx >= 0`) means an unmodified variant can *never* get
     an NL-shifted FI entry regardless of what any mask says, so computing a modloss mask for
     one would be pure dead weight (and, worse, would apply the floor below to a pool that is
     definitionally without real signal, forcing meaningless zero-intensity picks into the mask
     file). A modified-but-non-lossy variant (e.g. oxidation only, no phospho) is NOT special-
     cased beyond this -- Carafe's own modloss prediction is expected to already read near-zero
     for such variants (empirically confirmed non-eligible-fragment modloss channels predict
     exactly 0.0 in ad hoc testing), so the threshold alone should naturally exclude them; only
     the floor could still force a few zero-intensity entries in, which is harmless dead data
     since CometFragmentIndex.cpp's own `iPositionNLB`/`iPositionNLY` eligibility gate (a
     property of the peptide's actual mod combination, independent of any mask) will never
     consult those mask bits for a variant with no lossy mod slot active.
  5. Minimum-fragment floor (Section 8 item 4), applied INDEPENDENTLY to the unshifted pool
     (step 3) and, when computed, the modloss pool (step 4): if fewer than --min-kept-peaks
     candidates cleared the threshold in that pool, top up by intensity from that pool's own
     combined b/y candidates (not a fixed b/y split, and not mixed across the unshifted/modloss
     pools) until either the floor is reached or every eligible candidate in that pool is kept,
     whichever comes first. This is a Phase 2c design decision, not explicitly pre-decided in
     Section 8 (those items predate modloss channels entering the tool's scope) -- reusing the
     SAME --min-relative-intensity/--min-kept-peaks values for both pools (no new CLI flags)
     keeps the two ion classes symmetric and matches Section 8 item 5/6's minimal-surface intent.
  6. Pack the result as FOUR bitmasks (b-kept, y-kept, b-modloss-kept, y-modloss-kept) -- see
     "Bit-index convention" below. The unshifted and modloss masks use the identical bit
     convention (same ladder index i, same "if (i > 1)" gate) since
     CometFragmentIndex.cpp's neutral-loss-shifted-variant insertion runs inside the very same
     `for (int i = 0; i < iEndPos; ++i)` loop and index as the unshifted insertion immediately
     above it (CometFragmentIndex.cpp:693-732) -- there is no separate remap to derive for the
     modloss case.

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
  - bModlossMask/yModlossMask bit (i-2): same convention, sourced from b_modloss_z1/
    y_modloss_z1 at the same rows instead of b_z1/y_z1.

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

# Column layout every fragment row is normalized to internally, regardless of whether the
# source ms2_pred.tsv had 4 (general mode) or 8 (phospho mode) columns -- see read_ms2_pred().
CHANNELS = ("b_z1", "b_z2", "y_z1", "y_z2",
            "b_modloss_z1", "b_modloss_z2", "y_modloss_z1", "y_modloss_z2")
MODLOSS_CHANNELS = CHANNELS[4:]

# Bumped from v1 (Phase 2a) since the on-disk entry struct gained two u64 fields -- a v1 reader
# must not silently misinterpret v2 bytes (or vice versa), hence the magic-line version bump
# rather than a header flag alone.
MASK_FILE_MAGIC = b"Comet Carafe FI mask v2\n"


# ---------------------------------------------------------------------------
# .idx fingerprint -- ties a mask file to one specific already-built .idx (Section 4.3;
# iWhichPeptide/modNumIdx numbering is only stable across reads of one built .idx file, not
# across independent rebuilds of the same FASTA -- pitfall 4).
# ---------------------------------------------------------------------------

def idx_fingerprint(idx_path, chunk_size=1 << 20):
    """SHA-256 over the .idx byte range [pep_pos, footer_pos) -- the raw peptide table and
    protein list, i.e. everything the v2 unified .idx format still persists (docs/20260805_
    carafe.md Section 6.9: Phase 0.5 dropped MOD_NUMBERS/MOD_SEQS/the compact variant array
    from disk entirely). This fully determines iWhichPeptide numbering, but -- unlike the v1
    format this was originally written against -- NOT modNumIdx numbering any more: that now
    also depends on whichever comet.params variable mods were live when `comet.exe -x`
    produced the variant export this mask's tuples were built from (docs/20260805_carafe.md
    Section 9). A mask built against this exact .idx but a DIFFERENT comet.params would still
    pass this fingerprint check while carrying stale/meaningless modNumIdx keys -- Phase 3
    (not yet built) will need its own guard for that (e.g. verifying a looked-up mask entry's
    tuple still resolves to the same sequence it's about to mask, mirroring the sequence
    cross-check idx_to_carafe.py now does against its own export file) rather than relying on
    this fingerprint alone."""
    reader = itc.IdxReader(idx_path)
    f = reader.f
    f.seek(reader.pep_pos)
    remaining = reader.footer_pos - reader.pep_pos
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
    """Flat fragment table -> (rows, has_modloss). rows is a list of 8-tuples in CHANNELS
    order, one per cleavage-site row, in file order (sliced per-precursor via ms2_df's
    frag_start_idx/frag_stop_idx). has_modloss is True iff the file's header has all four
    modloss columns (Carafe run with `--mode phosphorylation`); when False, the modloss
    positions of every returned tuple are 0.0 (never read -- see compute_variant_mask's
    modloss-skip logic, which gates on has_modloss rather than on these zeros)."""
    with open(path, "r", newline="") as f:
        reader = csv.DictReader(f, delimiter="\t")
        fieldnames = set(reader.fieldnames or ())
        n_modloss_present = sum(1 for c in MODLOSS_CHANNELS if c in fieldnames)
        if n_modloss_present not in (0, len(MODLOSS_CHANNELS)):
            missing = [c for c in MODLOSS_CHANNELS if c not in fieldnames]
            raise ValueError(f"{path!r}: has {n_modloss_present}/{len(MODLOSS_CHANNELS)} "
                              f"modloss columns -- expected all or none (missing: {missing})")
        has_modloss = n_modloss_present == len(MODLOSS_CHANNELS)

        out = []
        for row in reader:
            if has_modloss:
                out.append(tuple(float(row[c]) for c in CHANNELS))
            else:
                out.append(tuple(float(row[c]) for c in CHANNELS[:4]) + (0.0, 0.0, 0.0, 0.0))
    return out, has_modloss


# ---------------------------------------------------------------------------
# Per-variant mask computation
# ---------------------------------------------------------------------------

def max_across_charges(frag_rows_per_charge):
    """frag_rows_per_charge: list of per-charge fragment-row lists, each a list of 8-tuples
    (CHANNELS order) of the SAME length (nAA-1). Returns the elementwise-max matrix, same
    shape -- Section 8 item 1's "per-fragment max intensity across charges", generalized over
    all 8 channels (not just the original 4) so it works unchanged for phospho-mode input."""
    n_rows = len(frag_rows_per_charge[0])
    n_channels = len(frag_rows_per_charge[0][0])
    merged = []
    for r in range(n_rows):
        merged.append(tuple(
            max(rows[r][c] for rows in frag_rows_per_charge)
            for c in range(n_channels)
        ))
    return merged


def _threshold_and_floor_pool(candidates, threshold, min_kept_peaks):
    """Shared threshold+floor logic for one candidate pool (either the unshifted b/y pool or
    the modloss b/y pool -- Section 8 item 4's floor, applied per-pool per Phase 2c's docstring
    note above). candidates: list of (intensity, 'b'|'y', length). Returns (kept, n_candidates)."""
    candidates = sorted(candidates, key=lambda c: c[0], reverse=True)
    n_above_threshold = sum(1 for c in candidates if c[0] >= threshold)
    n_keep = max(n_above_threshold, min(min_kept_peaks, len(candidates)))
    return candidates[:n_keep], len(candidates)


def _pack_mask(kept):
    """kept: list of (intensity, 'b'|'y', length). Returns (bMask, yMask) packed per the
    bit-index convention (bit (i-2), i = length-1) documented in the module docstring."""
    bMask = 0
    yMask = 0
    for _intensity, ion_type, length in kept:
        i = length - 1   # Comet loop index i, since length == i+1 for both b and y at index i
        bit = i - MIN_ION_LENGTH + 1  # bit (i-2): bit 0 == i==2 (length 3)
        if ion_type == "b":
            bMask |= (1 << bit)
        else:
            yMask |= (1 << bit)
    return bMask, yMask


def compute_variant_mask(merged_rows, nAA, min_relative_intensity, min_kept_peaks,
                          has_modloss, is_modified):
    """merged_rows: nAA-1 rows of 8-tuples (CHANNELS order), already max-across-charges.
    has_modloss: whether the source prediction had modloss columns at all (False for
    general-mode input). is_modified: whether this variant's modNumIdx >= 0 -- an unmodified
    variant can never carry an NL-shifted FI entry (CometFragmentIndex.cpp's `bFragmentNL`
    gate), so its modloss pool is skipped unconditionally regardless of has_modloss.

    Returns (bMask, yMask, bModlossMask, yModlossMask, n_candidates, n_kept,
             n_modloss_candidates, n_modloss_kept, base_peak)."""
    if nAA - 1 != len(merged_rows):
        raise ValueError(f"nAA={nAA} implies {nAA - 1} fragment rows, got {len(merged_rows)}")

    # Base peak: max over every predicted channel (Section 8 item 3), including modloss when
    # present -- not just the 4 (or up to 4 more) channels the FI can actually insert.
    base_peak = max((v for row in merged_rows for v in row), default=0.0)
    threshold = min_relative_intensity * base_peak

    # AlphaBase row r's own columns give b_{r+1} and y_{nAA-r-1} directly -- relabel into
    # length-indexed dicts so the Comet loop-index remap (b: same length index i; y: mirrored)
    # falls out as a simple lookup rather than needing to juggle row indices later.
    b_by_length = {}            # length -> b_z1 intensity
    y_by_length = {}            # length -> y_z1 intensity
    b_modloss_by_length = {}    # length -> b_modloss_z1 intensity
    y_modloss_by_length = {}    # length -> y_modloss_z1 intensity
    for r, row in enumerate(merged_rows):
        b_z1, _b_z2, y_z1, _y_z2, b_modloss_z1, _b_modloss_z2, y_modloss_z1, _y_modloss_z2 = row
        b_by_length[r + 1] = b_z1
        y_by_length[nAA - r - 1] = y_z1
        b_modloss_by_length[r + 1] = b_modloss_z1
        y_modloss_by_length[nAA - r - 1] = y_modloss_z1

    def build_candidates(by_length_b, by_length_y):
        candidates = []   # (intensity, 'b'|'y', length)
        for length, intensity in by_length_b.items():
            if length >= MIN_ION_LENGTH:
                candidates.append((intensity, "b", length))
        for length, intensity in by_length_y.items():
            if length >= MIN_ION_LENGTH:
                candidates.append((intensity, "y", length))
        return candidates

    unshifted_candidates = build_candidates(b_by_length, y_by_length)
    kept, n_candidates = _threshold_and_floor_pool(unshifted_candidates, threshold, min_kept_peaks)
    bMask, yMask = _pack_mask(kept)
    n_kept = len(kept)

    # Modloss pool: independent threshold+floor against the SAME base_peak/threshold (Section
    # 8 item 11 -- "independently keep/drop each"), skipped entirely per the module docstring's
    # reasoning (step 4) when there's nothing meaningful to threshold.
    if has_modloss and is_modified:
        modloss_candidates = build_candidates(b_modloss_by_length, y_modloss_by_length)
        modloss_kept, n_modloss_candidates = _threshold_and_floor_pool(
            modloss_candidates, threshold, min_kept_peaks)
        bModlossMask, yModlossMask = _pack_mask(modloss_kept)
        n_modloss_kept = len(modloss_kept)
    else:
        bModlossMask = yModlossMask = 0
        n_modloss_candidates = n_modloss_kept = 0

    return (bMask, yMask, bModlossMask, yModlossMask,
            n_candidates, n_kept, n_modloss_candidates, n_modloss_kept, base_peak)


# ---------------------------------------------------------------------------
# Mask file writer
# ---------------------------------------------------------------------------

# iWhichPeptide(u32) modNumIdx(i32) cNtermMod(i8) cCtermMod(i8)
# bMask(u64) yMask(u64) bModlossMask(u64) yModlossMask(u64)
# The last two are always present (v2) but are 0 for every entry in a general-mode-only mask
# file (GeneralMode: 1 in the header) -- Phase 3's consumer can skip the modloss lookup
# entirely when GeneralMode is set, or just harmlessly look up an always-empty mask either way.
ENTRY_FMT = "<IibbQQQQ"
ENTRY_SIZE = struct.calcsize(ENTRY_FMT)


def write_mask_file(path, fingerprint, num_raw_peptides, idx_path, threshold, min_kept_peaks,
                     general_mode, entries):
    """entries: iterable of (iWhichPeptide, modNumIdx, cNtermMod, cCtermMod, bMask, yMask,
    bModlossMask, yModlossMask), NOT required to be pre-sorted -- sorted here so the file is
    binary-searchable at FI build time (Section 4.3). general_mode: True iff the source
    prediction had no modloss columns at all (i.e. every entry's modloss masks are 0)."""
    entries = sorted(entries, key=lambda e: e[:4])
    with open(path, "wb") as f:
        f.write(MASK_FILE_MAGIC)
        f.write(f"SourceIdxFingerprint: {fingerprint}\n".encode("ascii"))
        f.write(f"SourceIdxNumRawPeptides: {num_raw_peptides}\n".encode("ascii"))
        f.write(f"SourceIdxPath: {idx_path}\n".encode("ascii"))
        f.write(f"MinRelativeIntensity: {threshold}\n".encode("ascii"))
        f.write(f"MinKeptPeaks: {min_kept_peaks}\n".encode("ascii"))
        f.write(f"GeneralMode: {1 if general_mode else 0}\n".encode("ascii"))
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
    ap.add_argument("ms2_pred_tsv", help="Carafe ai_pred.py's <prefix>_ms2_pred.tsv output "
                                          "(general mode: 4 columns; phospho mode: 8 columns "
                                          "including modloss -- auto-detected)")
    ap.add_argument("out_mask_file", help="Output mask file path")
    ap.add_argument("--min-relative-intensity", type=float, default=DEFAULT_MIN_RELATIVE_INTENSITY,
                     help=f"Keep a b/y (or modloss b/y) ion if its intensity is >= this "
                          f"fraction of the variant's predicted base peak (default: "
                          f"{DEFAULT_MIN_RELATIVE_INTENSITY})")
    ap.add_argument("--min-kept-peaks", type=int, default=DEFAULT_MIN_KEPT_PEAKS,
                     help=f"Always keep at least this many of a variant's most intense "
                          f"length>=3 candidates, even if below the threshold -- applied "
                          f"independently to the unshifted and modloss pools (default: "
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
    ms2_pred, has_modloss = read_ms2_pred(args.ms2_pred_tsv)
    if args.verbose:
        print(f"  mode: {'phospho (modloss channels present)' if has_modloss else 'general'}",
              file=sys.stderr)

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
    n_modloss_variants = 0
    n_modloss_candidates_total = 0
    n_modloss_kept_total = 0
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

        is_modified = key[1] != -1   # modNumIdx >= 0, matches CometFragmentIndex.cpp's bFragmentNL gate
        (bMask, yMask, bModlossMask, yModlossMask,
         n_candidates, n_kept, n_modloss_candidates, n_modloss_kept, base_peak) = compute_variant_mask(
            max_across_charges(frag_rows_per_charge), nAA_seen,
            args.min_relative_intensity, args.min_kept_peaks, has_modloss, is_modified)

        n_variants += 1
        n_candidates_total += n_candidates
        n_kept_total += n_kept
        entries.append((key[0], key[1], key[2], key[3], bMask, yMask, bModlossMask, yModlossMask))

        if has_modloss and is_modified:
            n_modloss_variants += 1
            n_modloss_candidates_total += n_modloss_candidates
            n_modloss_kept_total += n_modloss_kept

        if args.verbose and n_variants <= 5:
            print(f"  variant {key}: nAA={nAA_seen}, base_peak={base_peak:.4g}, "
                  f"{n_kept}/{n_candidates} kept, bMask={bMask:#x}, yMask={yMask:#x}, "
                  f"modloss {n_modloss_kept}/{n_modloss_candidates} kept, "
                  f"bModlossMask={bModlossMask:#x}, yModlossMask={yModlossMask:#x}",
                  file=sys.stderr)

    write_mask_file(args.out_mask_file, fingerprint, num_raw_peptides, args.idx_file,
                     args.min_relative_intensity, args.min_kept_peaks,
                     general_mode=not has_modloss, entries=entries)

    avg_kept = (n_kept_total / n_variants) if n_variants else 0.0
    avg_candidates = (n_candidates_total / n_variants) if n_variants else 0.0
    print(f"Wrote {len(entries)} variant masks to {args.out_mask_file}", file=sys.stderr)
    print(f"Average {avg_kept:.2f} of {avg_candidates:.2f} eligible fragments kept per variant "
          f"({100.0 * n_kept_total / n_candidates_total:.1f}% overall)"
          if n_candidates_total else "No eligible fragments seen", file=sys.stderr)
    if has_modloss:
        avg_modloss_kept = (n_modloss_kept_total / n_modloss_variants) if n_modloss_variants else 0.0
        avg_modloss_candidates = (n_modloss_candidates_total / n_modloss_variants) if n_modloss_variants else 0.0
        print(f"Modloss: average {avg_modloss_kept:.2f} of {avg_modloss_candidates:.2f} eligible "
              f"fragments kept per modified variant ({n_modloss_variants}/{n_variants} variants "
              f"had a modloss pool"
              + (f", {100.0 * n_modloss_kept_total / n_modloss_candidates_total:.1f}% overall)"
                 if n_modloss_candidates_total else ")"), file=sys.stderr)


if __name__ == "__main__":
    main()
