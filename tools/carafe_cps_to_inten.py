#!/usr/bin/env python3
"""
Build a Comet Carafe intensity file (.carafe_inten) from a compact prediction store
(.cps -- tools/carafe_cps.py) plus the target .idx's variant map -- the search-time input
for the predicted-intensity PSM score (docs/20260903_IntensityScore_design.md Section 2.2,
Phase 0).

Where tools/carafe_cps_to_fi_mask.py reduces each variant's predictions to a keep/drop
bit per fragment, this keeps the predicted intensity VALUES themselves: per Comet
peptide-index variant, a sparse list of (fragment position, quantized sqrt relative
intensity) peaks, bound to one specific .idx exactly the way a .fi_mask is. Same join,
same parallel byte-range streaming of the variant map, same worker-side sort + parent
k-way merge, same strictly-increasing post-write verification -- those helpers are imported
from carafe_cps_to_fi_mask rather than re-implemented.

Usage:
  carafe_cps_to_inten.py <idx_file> <variant_map_tsv> <cps_file> <out_inten_file>
      [--ignore-modloss] [--min-relative-intensity F] [--max-peaks N]
      [--verify-out-tsv PATH] [--workers N]

  idx_file          the .idx this file will be searched with (fingerprint + raw-peptide
                    count baked into the header)
  variant_map_tsv   tools/idx_to_carafe.py's sidecar for THAT .idx (supplies the
                    (iWhichPeptide, modNumIdx, cNtermMod, cCtermMod) tuples, the row_index
                    mapping into the store, and the VarModConfig header line)
  cps_file          the compact prediction store (may come from a different .idx flavor's
                    export -- the store is keyed by row_index over the shared peptide
                    population)
  --ignore-modloss  build a general-mode file from a phospho-mode store: modloss channels
                    dropped, threshold reference = first-4-channel base peak (same
                    semantics as the mask builder's flag)

File layout (little-endian throughout):

  magic line   b"Comet Carafe intensity v3\\n"
  header       "Key: Value\\n" ASCII lines, terminated by one blank line:
                 SourceIdxFingerprint     zlib CRC-32 of the .idx [pep_pos, footer_pos)
                                          (carafe_ms2_to_fi_mask.idx_fingerprint(), the
                                          same value CometPredictedMask.cpp computes)
                 SourceIdxNumRawPeptides  u64 decimal, from the raw-peptide table
                 SourceIdxPath            informational
                 SourceCpsPath            informational
                 VarModConfig             the exact string comet.exe -x emitted, verbatim
                 Mode                     phospho | general
                 Channels                 comma-joined "code=name" pairs for the channels this
                                          file can contain, e.g. 0=b_z1,1=y_z1,4=b_z2,5=y_z2;
                                          codes are fixed per name (CHANNEL_NAMES index), so a
                                          general-mode or z1-only file simply omits codes
                 Transform                sqrt   (q encodes sqrt(relative intensity))
                 Quant                    u8
                 MinRelativeIntensity     peak-keep threshold (relative to the base peak)
                 MaxPeaks                 per-variant cap on stored peaks
                 PerCharge                0 | 1 -- see "Per-charge records" below
  u64          entry count
  entries      variable length, sorted strictly increasing by
               (iWhichPeptide, modNumIdx, cNtermMod, cCtermMod, charge):
                 u32 iWhichPeptide, i32 modNumIdx, i8 cNtermMod, i8 cCtermMod   (KEY_FMT)
                 u8  charge   -- precursor charge this record was predicted for; 0 = the
                                 max-merge over every predicted charge (PerCharge: 0 files)
                 u8  nPeaks
                 nPeaks x { u16 code, u8 q }                                     (PEAK_FMT)
                   code = (channel << 8) | ladderPos     -- see peak_code(); channel codes:
                                                            0 b_z1, 1 y_z1, 2 b_modloss_z1,
                                                            3 y_modloss_z1, 4 b_z2, 5 y_z2,
                                                            6 b_modloss_z2, 7 y_modloss_z2
                   q    = round(255 * sqrt(rel))         -- rel = intensity / base_peak
               peaks within an entry are sorted by code (ascending), so a consumer can
               scatter them into a dense [channel][ladderPos] array in one pass.

Ladder-position convention (must match the C++ scorer, and is the SAME coordinate the
.fi_mask bit index uses before its "-2" shift): Comet's per-peptide fragment loop index i
(0-based) scores b-ion length i+1 AND y-ion length i+1 at each iteration, so

  - b_{L}  (AlphaBase row r's b column, L = r+1)        -> ladderPos = r
  - y_{L}  (AlphaBase row r's y column, L = nAA-r-1)    -> ladderPos = nAA-2-r   (mirrored)
  - modloss channels: identical mapping from the modloss columns at the same rows.

Unlike the FI mask there is no "i > 1" gate: lengths 1 and 2 are scoreable fragments in
XcorrScore*'s ladder and are kept here when they pass the threshold. ladderPos ranges over
0..nAA-2 (nAA <= MAX_PEPTIDE_LEN 51), so it fits the 8 bits reserved for it.

Doubly-charged fragments (v2, 2026-09-03): a v2 .cps store (carafe_pred_to_cps.py
--channels 8) carries b_z2/y_z2 (and modloss z2) predictions; those become channel codes
4-7 with the same ladder-position mapping as their z1 counterparts. From a v1 (z1-only)
store the z2 codes are simply absent from Channels. At score time Comet only reads fragment
charges up to the spectrum's max fragment charge (precursor charge - 1, capped by
max_fragment_charge), so z2 predicted peaks are ignored entirely -- excluded from |p| as
well -- for 2+ precursors, exactly as XCorr never scores z2 fragments there.

Per-charge records (v3, 2026-09-03): the store holds one row per predicted precursor
charge of each variant (idx_to_carafe.py --charges). By default (PerCharge: 0) those rows
are max-merged into one record per variant with charge byte 0, the FI-mask convention.
With --per-charge (needs --out-tsv, the <flavor>.carafe_peptides.tsv whose charge column,
in row_index order, tells each store row's precursor charge) one record is written per
(variant, charge), and the C++ scorer picks the record matching the spectrum's precursor
charge -- falling back to a charge-0 record, else the nearest lower predicted charge, else
the lowest higher one. Records of one variant are consecutive in the file (sorted by
charge within the key), which is what lets the loader keep one offset per variant.

Peak selection per variant: predictions are max-merged across the variant's charge rows
(unless --per-charge), scaled by the base peak (all-8-
channel base8 in phospho mode, first-4-channel base4 in general/--ignore-modloss mode --
the same reference the mask builder uses), peaks with rel >= MinRelativeIntensity are kept
up to MaxPeaks (highest rel first; ties broken by code for determinism), quantized as
round(255*sqrt(rel)), and any peak quantizing to 0 is dropped. Modloss channels are skipped
for unmodified variants (modNumIdx == -1) since they cannot carry a neutral loss. An entry
may legitimately have zero peaks (all-zero prediction); it is still written so coverage is
exact -- the C++ side treats "record present, no peaks" as score 0 without a "missing
record" warning.

Why sparse + sqrt + u8: ~75-80% of the store's dense slots are zero and masking already
showed the signal lives in the top ~6-20 peaks; sqrt is the transform the score compares
against (Comet's SP-score array holds binned sqrt intensities), and 8 bits of sqrt(rel)
resolves rel down to ~1.5e-5 near zero and ~0.8% steps near 1.0 -- far below prediction
error. Estimated sizes: OxMet ~0.25 GB, Phospho-large ~2.5 GB (vs 8.7 GB .cps).
"""

import argparse
import heapq
import math
import multiprocessing
import os
import struct
import sys
import time
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import carafe_cps  # noqa: E402
import carafe_cps_to_fi_mask as cfm  # noqa: E402
import carafe_ms2_to_fi_mask as fi_mask  # noqa: E402

INTEN_FILE_MAGIC = b"Comet Carafe intensity v3\n"

KEY_FMT = "<Iibb"
KEY_SIZE = struct.calcsize(KEY_FMT)        # 10
CHARGE_FMT = "<B"                          # per-entry precursor charge (0 = merged)
COUNT_FMT = "<B"
PEAK_FMT = "<HB"
PEAK_SIZE = struct.calcsize(PEAK_FMT)      # 3

# Channel code -> name; the code is FIXED per name (a file lists the subset it carries).
CHANNEL_NAMES = ("b_z1", "y_z1", "b_modloss_z1", "y_modloss_z1",
                 "b_z2", "y_z2", "b_modloss_z2", "y_modloss_z2")
CH_B, CH_Y, CH_B_ML, CH_Y_ML, CH_B2, CH_Y2, CH_B2_ML, CH_Y2_ML = range(8)
# index of each channel code's value within a fi_mask.CHANNELS-order 8-tuple
# (b_z1, b_z2, y_z1, y_z2, b_modloss_z1, b_modloss_z2, y_modloss_z1, y_modloss_z2)
ROW8_INDEX = {CH_B: 0, CH_B2: 1, CH_Y: 2, CH_Y2: 3, CH_B_ML: 4, CH_B2_ML: 5, CH_Y_ML: 6, CH_Y2_ML: 7}
Y_CHANNELS = (CH_Y, CH_Y_ML, CH_Y2, CH_Y2_ML)


def channels_present(has_modloss, has_z2):
    """The channel codes a file built under these settings can contain, in code order."""
    codes = [CH_B, CH_Y]
    if has_modloss:
        codes += [CH_B_ML, CH_Y_ML]
    if has_z2:
        codes += [CH_B2, CH_Y2]
        if has_modloss:
            codes += [CH_B2_ML, CH_Y2_ML]
    return sorted(codes)


def channels_header_value(codes):
    return ",".join(f"{c}={CHANNEL_NAMES[c]}" for c in codes)

QMAX = 255
MAX_LADDER_POS = 255      # 8 bits; Comet's MAX_PEPTIDE_LEN 51 -> ladderPos <= 49
MAX_PEAKS_LIMIT = 255     # nPeaks is a u8

DEFAULT_MIN_RELATIVE_INTENSITY = 0.01
DEFAULT_MAX_PEAKS = 32
EXCLUDED_CHARGE = 255     # row_charges marker for rows dropped by --only-charges


# ---------------------------------------------------------------------------
# Peak coding / quantization
# ---------------------------------------------------------------------------

def peak_code(channel, ladder_pos):
    if not (0 <= channel <= 0xFF) or not (0 <= ladder_pos <= MAX_LADDER_POS):
        raise ValueError(f"peak code out of range: channel={channel} ladder_pos={ladder_pos}")
    return (channel << 8) | ladder_pos


def decode_peak_code(code):
    return code >> 8, code & 0xFF


def quantize_sqrt(rel):
    """q = round(255 * sqrt(rel)), rel clamped to [0, 1]."""
    if rel <= 0.0:
        return 0
    if rel >= 1.0:
        return QMAX
    return int(math.sqrt(rel) * QMAX + 0.5)


def dequantize_sqrt(q):
    """The value the C++ scorer uses for this peak: sqrt(rel) = q / 255."""
    return q / QMAX


# ---------------------------------------------------------------------------
# Per-variant computation
# ---------------------------------------------------------------------------

def compute_variant_intensity(rows8_per_charge, nAA, base8_per_charge, base4_per_charge,
                              min_relative_intensity, max_peaks, has_modloss, is_modified,
                              has_z2=True):
    """rows8_per_charge: one list of (nAA-1) 8-tuples in fi_mask.CHANNELS order (b_z1, b_z2,
    y_z1, y_z2, b_modloss_z1, b_modloss_z2, y_modloss_z1, y_modloss_z2) per predicted-charge
    row sharing this variant (CpsReader.read_row8() output); base8/base4 likewise per
    charge. has_z2=False (a v1 store) skips the z2 channels even if nonzero values are
    passed. Returns the entry's peak list [(code, q), ...] sorted by code, per the module
    docstring's selection rules."""
    n_pos = nAA - 1
    if n_pos < 1:
        raise ValueError(f"nAA={nAA}: no fragment positions")
    if n_pos - 1 > MAX_LADDER_POS:
        raise ValueError(f"nAA={nAA} exceeds the ladder-position range")
    for rows8 in rows8_per_charge:
        if len(rows8) != n_pos:
            raise ValueError(f"nAA={nAA} implies {n_pos} rows, got {len(rows8)}")
        if rows8 and len(rows8[0]) != 8:
            raise ValueError(f"expected 8-channel rows (CpsReader.read_row8), got {len(rows8[0])}")
    if max_peaks < 0 or max_peaks > MAX_PEAKS_LIMIT:
        raise ValueError(f"max_peaks must be in [0, {MAX_PEAKS_LIMIT}], got {max_peaks}")

    merged = rows8_per_charge[0]
    if len(rows8_per_charge) > 1:
        merged = [tuple(max(vals) for vals in zip(*rows_at_r))
                  for rows_at_r in zip(*rows8_per_charge)]

    base_peak = max(base8_per_charge) if has_modloss else max(base4_per_charge)
    if base_peak <= 0.0:
        return []
    inv_base = 1.0 / base_peak
    codes = channels_present(has_modloss and is_modified, has_z2)

    candidates = []   # (rel, code)
    for r, row in enumerate(merged):
        pos_b = r               # b_{r+1}
        pos_y = nAA - 2 - r     # y_{nAA-r-1}
        for ch in codes:
            pos = pos_y if ch in Y_CHANNELS else pos_b
            candidates.append((row[ROW8_INDEX[ch]] * inv_base, peak_code(ch, pos)))

    kept = [c for c in candidates if c[0] >= min_relative_intensity]
    kept.sort(key=lambda c: (-c[0], c[1]))
    kept = kept[:max_peaks]

    peaks = []
    for rel, code in kept:
        q = quantize_sqrt(rel)
        if q > 0:
            peaks.append((code, q))
    peaks.sort()
    return peaks


def peak_norm(peaks):
    """|p| over an entry's dequantized peaks -- what the C++ side recomputes at decode
    time (not stored: it is a pure function of the stored bytes)."""
    return math.sqrt(sum(dequantize_sqrt(q) ** 2 for _code, q in peaks))


# ---------------------------------------------------------------------------
# Entry (de)serialization
# ---------------------------------------------------------------------------

def pack_entry(key, peaks, charge=0):
    """key: (iWhichPeptide, modNumIdx, cNtermMod, cCtermMod); peaks: [(code, q), ...]
    already sorted by code; charge: precursor charge of this record (0 = merged)."""
    if len(peaks) > MAX_PEAKS_LIMIT:
        raise ValueError(f"{len(peaks)} peaks exceeds the u8 count field")
    if not (0 <= charge <= 255):
        raise ValueError(f"charge {charge} out of u8 range")
    parts = [struct.pack(KEY_FMT, *key), struct.pack(CHARGE_FMT, charge),
             struct.pack(COUNT_FMT, len(peaks))]
    parts.extend(struct.pack(PEAK_FMT, code, q) for code, q in peaks)
    return b"".join(parts)


def unpack_entry_at(buf, off):
    """Parse one entry starting at buf[off]. Returns (key, charge, peaks, next_off)."""
    key = struct.unpack_from(KEY_FMT, buf, off)
    off += KEY_SIZE
    (charge,) = struct.unpack_from(CHARGE_FMT, buf, off)
    off += 1
    (n,) = struct.unpack_from(COUNT_FMT, buf, off)
    off += 1
    peaks = list(struct.iter_unpack(PEAK_FMT, buf[off:off + n * PEAK_SIZE]))
    if len(peaks) != n:
        raise ValueError(f"truncated entry at offset {off - KEY_SIZE - 2}")
    return key, charge, peaks, off + n * PEAK_SIZE


def iter_packed_entries(blob):
    """Yield ((key, charge), entry_bytes) from one packed run of variable-length entries."""
    off = 0
    end = len(blob)
    while off < end:
        key, charge, _peaks, nxt = unpack_entry_at(blob, off)
        yield (key, charge), blob[off:nxt]
        off = nxt


def merge_sorted_runs(blobs):
    """k-way merge of per-range sorted packed-entry runs -> entry_bytes in global key
    order (streaming; memory = the blobs + the heap)."""
    return (e for _keyz, e in heapq.merge(
        *(iter_packed_entries(b) for b in blobs if b), key=lambda kp: kp[0]))


def load_row_charges(out_tsv_path):
    """Precursor charge per out_tsv row_index (the 4th column of <flavor>.carafe_peptides.tsv,
    row order == row_index order) as a bytes object indexable by row_index."""
    charges = bytearray()
    with open(out_tsv_path, "rb") as f:
        header = f.readline().rstrip(b"\r\n").split(b"\t")
        try:
            ci = header.index(b"charge")
        except ValueError:
            raise ValueError(f"{out_tsv_path!r}: no 'charge' column in header {header!r}")
        for line in f:
            parts = line.rstrip(b"\r\n").split(b"\t")
            if len(parts) <= ci:
                raise ValueError(f"{out_tsv_path!r}: short line {line!r}")
            charges.append(int(parts[ci]))
    return bytes(charges)


# ---------------------------------------------------------------------------
# Worker
# ---------------------------------------------------------------------------

def process_range(job):
    """Worker: one variant-map byte range -> (range_index, packed_entries SORTED by key,
    n_entries, n_peaks). Mirrors carafe_cps_to_fi_mask.process_range()."""
    (range_index, vmap_path, data_start, byte_start, byte_end, cps_path,
     min_rel, max_peaks, has_modloss, row_charges, per_charge) = job

    reader = carafe_cps.CpsReader(cps_path)
    has_z2 = reader.channels == carafe_cps.CHANNELS_V2
    keyed = []   # ((key, charge), packed)
    n_peaks = 0
    for key, row_indices in cfm.iter_vmap_groups(vmap_path, data_start, byte_start, byte_end):
        if row_charges is not None:
            # EXCLUDED_CHARGE marks rows dropped by --only-charges
            row_indices = [ri for ri in row_indices if row_charges[ri] != EXCLUDED_CHARGE]
            if not row_indices:
                continue   # variant has no row at a kept charge -> no record (scores 0.0)
        if not per_charge:
            groups = [(0, row_indices)]           # merged: one record, charge 0
        else:
            by_z = {}
            for ri in row_indices:
                by_z.setdefault(row_charges[ri], []).append(ri)
            groups = sorted(by_z.items())
        for charge, ris in groups:
            rows8_pc, b8s, b4s, nAA_seen = [], [], [], None
            for ri in ris:
                nAA, b8, b4, rows8 = reader.read_row8(ri)
                nAA_seen = nAA
                rows8_pc.append(rows8)
                b8s.append(b8)
                b4s.append(b4)
            peaks = compute_variant_intensity(rows8_pc, nAA_seen, b8s, b4s, min_rel, max_peaks,
                                              has_modloss=has_modloss, is_modified=(key[1] != -1),
                                              has_z2=has_z2)
            n_peaks += len(peaks)
            keyed.append(((key, charge), pack_entry(key, peaks, charge)))
    reader.close()
    keyed.sort(key=lambda kp: kp[0])
    return range_index, b"".join(p for _, p in keyed), len(keyed), n_peaks


# ---------------------------------------------------------------------------
# File writer / reader / verifier
# ---------------------------------------------------------------------------

def header_lines(fingerprint, num_raw_peptides, idx_path, cps_path, var_mod_config,
                 general_mode, min_rel, max_peaks, has_z2=True, per_charge=False):
    if var_mod_config is None:
        raise ValueError("var_mod_config is required -- the variant map has no "
                         "'# VarModConfig:' line; rebuild the export with a current comet.exe -x")
    channels = channels_header_value(channels_present(not general_mode, has_z2))
    return [
        f"SourceIdxFingerprint: {fingerprint}",
        f"SourceIdxNumRawPeptides: {num_raw_peptides}",
        f"SourceIdxPath: {idx_path}",
        f"SourceCpsPath: {cps_path}",
        f"VarModConfig: {var_mod_config}",
        f"Mode: {'general' if general_mode else 'phospho'}",
        f"Channels: {channels}",
        "Transform: sqrt",
        "Quant: u8",
        f"MinRelativeIntensity: {min_rel}",
        f"MaxPeaks: {max_peaks}",
        f"PerCharge: {1 if per_charge else 0}",
    ]


def write_inten_file(path, hdr_lines, total, entry_bytes_iter):
    """entry_bytes_iter: packed entries in global key order (merge_sorted_runs())."""
    with open(path, "wb") as f:
        f.write(INTEN_FILE_MAGIC)
        for line in hdr_lines:
            f.write((line + "\n").encode("ascii"))
        f.write(b"\n")
        f.write(struct.pack("<Q", total))
        buf = []
        buf_bytes = 0
        for e in entry_bytes_iter:
            buf.append(e)
            buf_bytes += len(e)
            if buf_bytes >= (1 << 22):
                f.write(b"".join(buf))
                buf = []
                buf_bytes = 0
        if buf:
            f.write(b"".join(buf))


def read_header(f):
    """Consume magic + header from an open binary file; returns (header_dict, count)."""
    magic = f.readline()
    if magic != INTEN_FILE_MAGIC:
        raise ValueError(f"not a {INTEN_FILE_MAGIC!r} file (got {magic!r})")
    header = {}
    while True:
        line = f.readline()
        if not line:
            raise ValueError("unterminated header")
        if line == b"\n":
            break
        k, _, v = line.decode("ascii").rstrip("\n").partition(": ")
        header[k] = v
    (count,) = struct.unpack("<Q", f.read(8))
    return header, count


def read_inten_file(path):
    """Companion reader (tests / prototyping): returns (header, [(key, charge, peaks), ...])."""
    with open(path, "rb") as f:
        header, count = read_header(f)
        blob = f.read()
    entries = []
    off = 0
    while off < len(blob):
        key, charge, peaks, off = unpack_entry_at(blob, off)
        entries.append((key, charge, peaks))
    if len(entries) != count:
        raise ValueError(f"header count {count} != entries read {len(entries)}")
    return header, entries


def verify_written_inten_sorted(path):
    """Stream the finished file: entries strictly increasing by (key, charge) (sort order AND
    uniqueness), peak codes within an entry strictly increasing, count matches the header.
    Returns (n_entries, n_peaks)."""
    with open(path, "rb") as f:
        _header, count = read_header(f)
        prev = None
        n = 0
        n_peaks = 0
        carry = b""
        while True:
            chunk = f.read(1 << 24)
            if not chunk and not carry:
                break
            buf = carry + chunk
            off = 0
            while True:
                # need key + charge + count to know the entry length
                if off + KEY_SIZE + 2 > len(buf):
                    break
                (npk,) = struct.unpack_from(COUNT_FMT, buf, off + KEY_SIZE + 1)
                entry_len = KEY_SIZE + 2 + npk * PEAK_SIZE
                if off + entry_len > len(buf):
                    break
                key, charge, peaks, nxt = unpack_entry_at(buf, off)
                keyz = (key, charge)
                if prev is not None and keyz <= prev:
                    raise ValueError(f"entries not strictly increasing at entry {n}: "
                                     f"{prev} -> {keyz}")
                prev_code = -1
                for code, _q in peaks:
                    if code <= prev_code:
                        raise ValueError(f"entry {n} key {key}: peak codes not strictly "
                                         f"increasing")
                    prev_code = code
                prev = keyz
                n += 1
                n_peaks += len(peaks)
                off = nxt
            carry = buf[off:]
            if not chunk:
                if carry:
                    raise ValueError(f"{len(carry)} trailing bytes after entry {n}")
                break
        if n != count:
            raise ValueError(f"header count {count} != streamed entries {n}")
        return n, n_peaks


# ---------------------------------------------------------------------------
# CLI
# ---------------------------------------------------------------------------

def main(argv=None):
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("idx_file")
    ap.add_argument("variant_map_tsv")
    ap.add_argument("cps_file")
    ap.add_argument("out_inten_file")
    ap.add_argument("--min-relative-intensity", type=float,
                    default=DEFAULT_MIN_RELATIVE_INTENSITY,
                    help=f"keep peaks with intensity/base_peak >= F (default "
                         f"{DEFAULT_MIN_RELATIVE_INTENSITY})")
    ap.add_argument("--max-peaks", type=int, default=DEFAULT_MAX_PEAKS,
                    help=f"per-variant cap on stored peaks, highest first (default "
                         f"{DEFAULT_MAX_PEAKS}, max {MAX_PEAKS_LIMIT})")
    ap.add_argument("--ignore-modloss", action="store_true",
                    help="general-mode file from a phospho-mode store: drop modloss "
                         "channels, threshold against the first-4-channel base peak")
    ap.add_argument("--verify-out-tsv", default=None,
                    help="Optional: the original out_tsv, to check the store's provenance "
                         "head-CRC against")
    ap.add_argument("--per-charge", action="store_true",
                    help="one record per (variant, predicted precursor charge) instead of "
                         "the max-merge over charges; requires --out-tsv")
    ap.add_argument("--out-tsv", default=None,
                    help="<flavor>.carafe_peptides.tsv (row_index order) -- supplies each "
                         "store row's precursor charge for --per-charge / --only-charges "
                         "(defaults to --verify-out-tsv's value)")
    ap.add_argument("--only-charges", default=None,
                    help="comma-separated predicted precursor charges to keep (e.g. 2); other "
                         "store rows are ignored and a variant with no kept row gets no "
                         "record. Requires --out-tsv. Works with or without --per-charge.")
    ap.add_argument("--workers", type=int, default=max(1, (os.cpu_count() or 2) - 2))
    args = ap.parse_args(argv)

    if not (0.0 <= args.min_relative_intensity <= 1.0):
        sys.exit("--min-relative-intensity must be in [0, 1]")
    if not (0 <= args.max_peaks <= MAX_PEAKS_LIMIT):
        sys.exit(f"--max-peaks must be in [0, {MAX_PEAKS_LIMIT}]")
    out_tsv = args.out_tsv or args.verify_out_tsv
    if (args.per_charge or args.only_charges) and not out_tsv:
        sys.exit("--per-charge / --only-charges require --out-tsv (or --verify-out-tsv)")

    t0 = time.time()
    row_charges = None
    if args.per_charge or args.only_charges:
        print(f"Reading per-row precursor charges from {out_tsv} ...", file=sys.stderr)
        row_charges = load_row_charges(out_tsv)
        if args.only_charges:
            keep = {int(z) for z in args.only_charges.split(",")}
            if EXCLUDED_CHARGE in keep:
                sys.exit(f"charge {EXCLUDED_CHARGE} is reserved")
            row_charges = bytes(z if z in keep else EXCLUDED_CHARGE for z in row_charges)
            n_keep = sum(1 for z in row_charges if z != EXCLUDED_CHARGE)
            print(f"--only-charges {sorted(keep)}: keeping {n_keep} of {len(row_charges)} store rows",
                  file=sys.stderr)
    reader = carafe_cps.CpsReader(args.cps_file)
    store_rows = reader.row_count
    store_mode = reader.header.get("Mode", "phospho")
    store_has_z2 = reader.channels == carafe_cps.CHANNELS_V2
    store_version = reader.version
    if args.verify_out_tsv:
        reader.verify_source(args.verify_out_tsv)
        print(f"store provenance vs {args.verify_out_tsv!r}: OK", file=sys.stderr)
    if row_charges is not None and len(row_charges) != store_rows:
        sys.exit(f"--out-tsv has {len(row_charges)} data rows but the store has {store_rows}")
    reader.close()
    has_modloss = (store_mode == "phospho") and not args.ignore_modloss

    print(f"Fingerprinting {args.idx_file} ...", file=sys.stderr)
    fingerprint, num_raw_peptides = fi_mask.idx_fingerprint(args.idx_file)

    data_start, var_mod_config = cfm.find_data_start(args.variant_map_tsv)
    vmap_size = os.path.getsize(args.variant_map_tsv)
    n_ranges = max(1, args.workers * 4)
    span = max(1, (vmap_size - data_start) // n_ranges)
    jobs = []
    for i in range(n_ranges):
        byte_start = data_start + i * span
        byte_end = data_start + (i + 1) * span if i < n_ranges - 1 else vmap_size
        if byte_start >= vmap_size:
            break
        jobs.append((i, args.variant_map_tsv, data_start, byte_start, byte_end,
                     args.cps_file, args.min_relative_intensity, args.max_peaks,
                     has_modloss, row_charges, args.per_charge))

    print(f"{len(jobs)} ranges, {args.workers} workers, has_modloss={has_modloss}, "
          f"z2={store_has_z2} (store v{store_version}), per_charge={args.per_charge}, "
          f"min_rel={args.min_relative_intensity}, max_peaks={args.max_peaks}",
          file=sys.stderr)

    blobs = [None] * len(jobs)
    counts = [0] * len(jobs)
    peak_counts = [0] * len(jobs)
    n_done = 0
    pool = multiprocessing.Pool(processes=args.workers)
    try:
        for range_index, blob, n, npk in pool.imap_unordered(process_range, jobs):
            blobs[range_index] = blob
            counts[range_index] = n
            peak_counts[range_index] = npk
            n_done += 1
            if n_done % 8 == 0 or n_done == len(jobs):
                print(f"[{n_done}/{len(jobs)} ranges] {sum(counts)} entries, "
                      f"{time.time() - t0:.0f}s", file=sys.stderr)
    finally:
        pool.close()
        pool.join()

    total = sum(counts)
    total_peaks = sum(peak_counts)
    print(f"Merging {len(jobs)} sorted runs and writing {total} entries "
          f"({total_peaks} peaks) to {args.out_inten_file!r} ...", file=sys.stderr)
    hdr = header_lines(fingerprint, num_raw_peptides, args.idx_file, args.cps_file,
                       var_mod_config, general_mode=not has_modloss,
                       min_rel=args.min_relative_intensity, max_peaks=args.max_peaks,
                       has_z2=store_has_z2, per_charge=args.per_charge)
    write_inten_file(args.out_inten_file, hdr, total, merge_sorted_runs(blobs))

    n_verified, n_peaks_verified = verify_written_inten_sorted(args.out_inten_file)
    if n_peaks_verified != total_peaks:
        raise ValueError(f"peak count mismatch: workers {total_peaks}, file {n_peaks_verified}")
    size = os.path.getsize(args.out_inten_file)
    mean_peaks = (n_peaks_verified / n_verified) if n_verified else 0.0
    print(f"Done: {args.out_inten_file!r}, {n_verified} entries, {n_peaks_verified} peaks "
          f"({mean_peaks:.1f}/entry), {size:,} bytes, {time.time() - t0:.0f}s total, "
          f"store had {store_rows} rows", file=sys.stderr)


if __name__ == "__main__":
    main()
