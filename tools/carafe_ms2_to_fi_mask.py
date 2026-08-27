#!/usr/bin/env python3
"""
Turn Carafe's predicted MS2 spectra into a per-variant fragment-ion keep/drop mask for
Comet's fragment ion index (FI) -- Phase 2a/2c of docs/20260805_carafe.md. Phase 2a covered
"general" mode (b_z1/b_z2/y_z1/y_z2 only); Phase 2c adds phospho mode's modloss channels
(b_modloss_z1/b_modloss_z2/y_modloss_z1/y_modloss_z2), gated on Phase 2b having added a real
neutral-loss ion class to the FI itself (CometFragmentIndex.cpp's AddFragments(), Section 6.6).

Inputs (four files from a single tools/idx_to_carafe.py export + Carafe ai_pred.py run against
it -- NOT row-position-aligned with each other; see "Row-order correctness" below):

  1. <out_tsv>               -- tools/idx_to_carafe.py's main output: sequence/mods/mod_sites/
     charge, one row per (peptide, charge) combination, the direct input fed to ai_pred.py.
     row_index below is the 0-based data-row position in THIS file.
  2. <out_tsv>.variants.tsv  -- tools/idx_to_carafe.py's sidecar: row_index -> the FI variant
     identity tuple (iWhichPeptide, modNumIdx, cNtermMod, cCtermMod). Multiple row_index values
     can belong to one tuple (one per predicted charge state -- idx_to_carafe.py emits one
     out_tsv row per configured charge); multiple tuples can point at the SAME row_index
     (idx_to_carafe.py's dedup collapsing identical sequence/mods/mod_sites/charge rows).
  3. <prefix>_ms2_df.tsv     -- Carafe's echoed-back input rows (sequence/mods/mod_sites/charge
     verbatim, PLUS nAA and frag_start_idx/frag_stop_idx it adds). AlphaBase convention: a row's
     fragments occupy [frag_start_idx, frag_stop_idx) in ms2_pred.tsv, one row per cleavage
     site, nAA-1 rows.
  4. <prefix>_ms2_pred.tsv   -- the flat fragment table. Columns b_z1/b_z2/y_z1/y_z2 always;
     additionally b_modloss_z1/b_modloss_z2/y_modloss_z1/y_modloss_z2 when Carafe was run with
     `--mode phosphorylation` (`ai_pred.py`'s `mode_type != 'general'` output shape, Section
     2.3). Mode is auto-detected here from this file's header -- no CLI flag needed (matches
     Section 8 item 6's minimal-surface philosophy): the file itself unambiguously says which
     channels it has.

Row-order correctness: an earlier version of this script assumed <prefix>_ms2_df.tsv's Nth
data row was out_tsv's Nth row (a claim docs/20260805_carafe.md Section 6.3 believed it had
empirically confirmed). That is FALSE at real scale: AlphaBase's ModelInterface.predict() (the
base class Carafe's MS2/RT models share) does `precursor_df.sort_values('nAA', inplace=True)`
+ `reset_index(drop=True, inplace=True)` on the caller's own dataframe whenever the input lacks
a pre-existing 'nAA' column -- and since ai_pred.py passes its input dataframe by reference and
later writes that SAME (now sorted) object out as _ms2_df.tsv, the echoed file ends up ordered
by peptide length, not input order. Confirmed broken at every real scale this project has
tested (92,741 / 197,782 / 7,130,366-row runs); only the original 250-row Phase 0 spike ever
looked order-preserving, because it happened to check exclusively row 0, which a stable sort
trivially leaves in place. Fixed here by joining <out_tsv> and <prefix>_ms2_df.tsv on the
(sequence, mods, mod_sites, charge) content tuple instead of file position -- that tuple is
unique across out_tsv by tools/idx_to_carafe.py's own dedup step (confirmed empirically:
zero collisions at all three scales above) and Carafe echoes those four columns verbatim, so
this join is correct regardless of whatever internal reordering ai_pred.py does. See
read_out_tsv()/read_ms2_df() below.

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

--ignore-modloss / building a "without neutral loss" mask from a phospho-mode Carafe run
(2026-08-14): a phospho analysis wants both a "with NL" and a "without NL" comparison, but
running Carafe's expensive `--mode phosphorylation` inference twice (once for real, once just
to get a general-mode-shaped prediction of the identical peptide population) is pure waste --
the modloss channels are simply unused input for a "without NL" mask, not a different
prediction. `--ignore-modloss` makes this script treat a phospho-mode ms2_pred_tsv as if it
were general-mode: base_peak/threshold computed from the 4 unshifted channels only (see the
has_modloss gate added to compute_variant_mask()'s base_peak calculation below -- without this
gate, a genuinely high-intensity modloss prediction could inflate/deflate the threshold used for
the UNSHIFTED masks too, which would make "with NL" and "without NL" not actually comparable to
a true general-mode baseline), modloss masks always 0, GeneralMode: 1 in the output header --
byte-for-byte what a real `--mode general` Carafe run against the same peptides would produce
(confirmed: the two differ only in wasted modloss compute, never in the resulting mask). One
Carafe inference run therefore feeds two `carafe_ms2_to_fi_mask.py` invocations (one plain, one
with --ignore-modloss) to produce both masks. Important companion requirement, NOT solved by
this flag alone: CometFragmentIndex.cpp's `bUseFragmentNeutralLoss` is baked into a .idx's own
header from its variable_mod config's neutral-loss field at BUILD time (ParsePeptideIndexHeader(),
docs/20260811_restore_idx_header_mods.md) and CometPredictedMask::Load()'s VarModConfig check
(ComputeVarModConfigString(), which includes dNeutralLoss per slot) will hard-reject a mask
whose embedded VarModConfig doesn't match the live search's -- so a genuine "without NL" search
needs its OWN .idx build (same mod residues/mass/max-per-mod, neutral_loss field zeroed) and its
own `comet.exe -x` / idx_to_carafe.py export (for the matching VarModConfig string), even though
peptide/modNumIdx enumeration and the Carafe predictions themselves are identical between the two
-- only the .idx build and export are cheap enough (minutes, not hours) to duplicate; Carafe
inference is not, hence this flag.

Memory note: this script still loads the full <prefix>_ms2_pred.tsv into memory (no
pandas/numpy dependency -- matching tools/idx_to_carafe.py's zero-external-dependency
convention), but as of the row-order fix above (2026-08-12) stores it as one array.array per
channel (FragmentTable below) rather than a Python list of 8-tuples. The original list-of-
tuples form swap-thrashed a 31GB-RAM machine to a near-standstill on the ~87M-fragment-row
target-only full-proteome oxmet run (~28GB of pure per-row Python-object overhead for data
that's ~5.5GB as raw float64); the array-of-columns form (initially float64, 'd') fixed that
run. Re-run 2026-08-13 at ~174M fragment rows (Section 6.16/6.18's target+decoy population,
2x the target-only run), the double-precision array itself pushed total process memory back
past 31GB during the mask-computation loop that follows the read -- switched to float32 ('f')
at that point (see FragmentTable's own docstring for why the precision loss is safe here),
roughly halving that structure's footprint. A genuinely larger (e.g. multi-hundred-million-row)
run beyond that would still eventually need a chunked/streaming rewrite of the ms2_pred.tsv
read itself, and/or reducing the other loaded structures' overhead (out_tsv_rows,
ms2_df_by_content) -- not attempted here.
"""

import argparse
import array
import csv
import os
import struct
import sys
import zlib

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

# Bumped from v1 (Phase 2a, general-mode only) to v2 (Phase 2c: on-disk entry struct gained
# two u64 fields for bModlossMask/yModlossMask) to v3 (Phase 3, docs/20260805_carafe.md
# Section 8 item 13: the fingerprint algorithm changed from SHA-256 to CRC-32 -- see idx_fingerprint()
# below for why -- a second breaking change to what SourceIdxFingerprint means, not just to
# the entry byte layout). Each bump means a stale-version reader must not silently
# misinterpret newer bytes (or vice versa), hence the magic-line version bump rather than a
# header flag alone.
MASK_FILE_MAGIC = b"Comet Carafe FI mask v3\n"


# ---------------------------------------------------------------------------
# .idx fingerprint -- ties a mask file to one specific already-built .idx (Section 4.3;
# iWhichPeptide/modNumIdx numbering is only stable across reads of one built .idx file, not
# across independent rebuilds of the same FASTA -- pitfall 4).
# ---------------------------------------------------------------------------

def idx_fingerprint(idx_path, chunk_size=1 << 20):
    """CRC-32 (zlib.crc32(), matching CometPredictedMask.cpp's ComputeIdxFingerprint() on the
    C++ side -- Phase 3, docs/20260805_carafe.md Section 8 items 12-14) over the .idx byte range
    [pep_pos, footer_pos) -- the raw peptide table and protein list, i.e. everything the v4
    unified .idx format persists at that fixed byte range (Section 6.9: Phase 0.5 dropped
    MOD_NUMBERS/MOD_SEQS/the compact variant array from disk entirely -- still true post-
    docs/20260811_restore_idx_header_mods.md, which restored mod identity to the TEXT header,
    not the variant array). Switched from SHA-256 (Phase 2a/2c) to CRC-32 when Phase 3 needed to
    compute the identical fingerprint on the C++ side too: this is a "did I point the mask at
    the wrong .idx" sanity check, not a security boundary, and zlib is already a linked
    dependency of Comet's C++ build (Python's zlib module wraps the same C library), whereas
    matching a from-scratch/vendored SHA-256 implementation bit-for-bit across two independent
    languages would be new correctness-critical code for a check that doesn't need
    cryptographic strength.

    This fully determines iWhichPeptide numbering, but -- unlike the v1 format this was
    originally written against -- NOT modNumIdx numbering any more: that now also depends on
    which variable mods were baked into this .idx's own header at BUILD time (`comet.exe -j`/
    `-i`, docs/20260811_restore_idx_header_mods.md), since ReadPeptideIndex() overwrites live
    g_staticParams.variableModParameters from the .idx before `comet.exe -x` ever runs -- the
    -P comet.params passed to -x no longer has any influence on this. The peptide/protein table
    this fingerprint covers can still be byte-identical across two builds of the same FASTA that
    used DIFFERENT variable mods (e.g. re-running -j with an edited comet.params over the same
    output filename), so a mask built against an EARLIER build of this .idx can still pass this
    fingerprint check today while carrying stale/meaningless modNumIdx keys -- CometPredictedMask::
    Load()'s VarModConfig comparison (Section 8 item 13) and Lookup()'s "not found -> fully
    unfiltered" fallback (Section 8 item 2) are Phase 3's actual guards against that, not this
    fingerprint."""
    reader = itc.IdxReader(idx_path)
    f = reader.f
    f.seek(reader.pep_pos)
    remaining = reader.footer_pos - reader.pep_pos
    crc = 0
    while remaining > 0:
        n = min(chunk_size, remaining)
        buf = f.read(n)
        if not buf:
            break
        crc = zlib.crc32(buf, crc)
        remaining -= len(buf)

    # Cheap, human-readable cross-check alongside the hash: NumRawPeptides, read directly
    # from the raw-peptide-table's leading count(u64) rather than via the heavier
    # read_raw_peptides() (which materializes every peptide just to count them).
    f.seek(reader.pep_pos)
    (num_raw,) = struct.unpack("<Q", f.read(8))

    return f"{crc & 0xffffffff:08x}", num_raw


# ---------------------------------------------------------------------------
# Input readers
# ---------------------------------------------------------------------------

def read_variant_map_var_mod_config(path):
    """Reads the leading "# VarModConfig: <string>" comment line tools/idx_to_carafe.py
    propagates from comet.exe -x's own export (see that script's read_var_mod_config()) --
    embedded verbatim in this mask's own file header so Phase 3's CometPredictedMask::Load()
    can reject a mask built against different variable mods than are live in the search
    consuming it (docs/20260805_carafe.md Section 8 items 12-14). Returns None if absent (older
    idx_to_carafe.py, or its own export lacked the line -- see that script's warning)."""
    with open(path, "r", newline="") as f:
        first = f.readline().rstrip("\r\n")
    prefix = "# VarModConfig: "
    if first.startswith(prefix):
        return first[len(prefix):]
    return None


def read_variant_map(path):
    """row_index -> list of (iWhichPeptide, modNumIdx, cNtermMod, cCtermMod) tuples, grouped
    the other way from how tools/idx_to_carafe.py wrote it (one line per (row_index, tuple)
    pair) -- callers here want tuple -> [row_index, ...] instead, built by the caller from this."""
    rows = []   # (row_index, tuple)
    with open(path, "r", newline="") as f:
        first_line = f.readline()
        if not first_line.startswith("# VarModConfig: "):
            f.seek(0)   # no comment line -- let DictReader see the real header itself
        reader = csv.DictReader(f, delimiter="\t")
        for row in reader:
            row_index = int(row["row_index"])
            key = (int(row["iWhichPeptide"]), int(row["modNumIdx"]),
                   int(row["cNtermMod"]), int(row["cCtermMod"]))
            rows.append((row_index, key))
    return rows


def read_out_tsv(path):
    """row_index -> (sequence, mods, mod_sites, charge), exactly as tools/idx_to_carafe.py wrote
    <out_tsv> (the direct input fed to Carafe's ai_pred.py). row_index is the 0-based data-row
    position in THIS file -- the same row_index tools/idx_to_carafe.py's variant-map sidecar
    references, since both were written from the same in-memory loop in that script. Used to
    join against read_ms2_df()'s content-keyed dict below; see module docstring's "Row-order
    correctness" section for why a positional join with Carafe's own echoed file is wrong."""
    out = []
    with open(path, "r", newline="") as f:
        reader = csv.DictReader(f, delimiter="\t")
        for row in reader:
            out.append((row["sequence"], row["mods"], row["mod_sites"], int(row["charge"])))
    return out


def read_ms2_df(path):
    """(sequence, mods, mod_sites, charge) -> (nAA, frag_start_idx, frag_stop_idx), keyed by
    content rather than this file's row position -- Carafe's ai_pred.py does not preserve input
    row order in its own echo at real scale (module docstring's "Row-order correctness"
    section). Raises if the same tuple appears twice: tools/idx_to_carafe.py's out_tsv is
    deduped on exactly this tuple, so a real collision here would mean this file wasn't produced
    from that out_tsv (or the two have desynced some other way)."""
    out = {}
    with open(path, "r", newline="") as f:
        reader = csv.DictReader(f, delimiter="\t")
        for row in reader:
            key = (row["sequence"], row["mods"], row["mod_sites"], int(row["charge"]))
            if key in out:
                raise ValueError(f"{path!r}: duplicate (sequence, mods, mod_sites, charge) "
                                  f"{key!r} -- expected unique per tools/idx_to_carafe.py's own "
                                  f"out_tsv dedup guarantee; can't join unambiguously")
            out[key] = (int(row["nAA"]), int(row["frag_start_idx"]), int(row["frag_stop_idx"]))
    return out


class FragmentTable:
    """Compact column-oriented storage for read_ms2_pred()'s flat fragment table -- one
    array.array('f') per channel instead of a Python list of 8-tuples. A list of N 8-tuples of
    Python floats costs ~300+ bytes/row in pure object/container overhead; N float32s packed 8
    per row in array.array cost 32 bytes/row -- roughly a 10x reduction, the difference between
    fitting a real ms2_pred.tsv in RAM and swap-thrashing a 31GB machine to a standstill
    (confirmed empirically twice, 2026-08-12/13: the original double-precision ('d') version
    handled the ~87M-row target-only oxmet run fine, but the ~174M-row target+decoy run --
    Section 6.16/6.18-style full-database coverage, twice the rows -- pushed total process
    memory (FragmentTable plus the other loaded structures) past 31GB and into real swap
    thrashing during the mask-computation loop that follows the read; switching to float32 here
    was the fix). float32 loses precision below ~7 significant digits, which does not matter for
    this data: predicted intensities are normalized 0..1 fractions thresholded at 10%-relative-
    intensity granularity (DEFAULT_MIN_RELATIVE_INTENSITY), many orders of magnitude coarser
    than float32's resolution -- no threshold/floor decision can flip because of this. Not
    threaded through to any other float64 data in this script (out_tsv/variant-map/mask-file
    fields are all integers or already-quantized bits, not float32-sensitive).
    table[start:stop] still returns a plain list of CHANNELS-order tuples -- same external shape
    callers (e.g. max_across_charges(), compute_variant_mask()) already expect -- built on
    demand only for the (small, per-variant) slice asked for, not materialized up front."""
    __slots__ = ("_cols", "n_channels")

    def __init__(self, n_channels):
        self.n_channels = n_channels
        self._cols = [array.array('f') for _ in range(n_channels)]

    def append(self, values):
        for col, v in zip(self._cols, values):
            col.append(v)

    def __len__(self):
        return len(self._cols[0])

    def __getitem__(self, sl):
        if isinstance(sl, slice):
            return list(zip(*(col[sl] for col in self._cols)))
        return tuple(col[sl] for col in self._cols)


def read_ms2_pred(path):
    """Flat fragment table -> (table, has_modloss). table is a FragmentTable (see above);
    table[start:stop] returns a list of 8-tuples in CHANNELS order, one per cleavage-site row,
    in file order (sliced per-precursor via ms2_df's frag_start_idx/frag_stop_idx). has_modloss
    is True iff the file's header has all four modloss columns (Carafe run with
    `--mode phosphorylation`); when False, the modloss positions of every returned tuple are 0.0
    (never read -- see compute_variant_mask's modloss-skip logic, which gates on has_modloss
    rather than on these zeros)."""
    with open(path, "r", newline="") as f:
        reader = csv.DictReader(f, delimiter="\t")
        fieldnames = set(reader.fieldnames or ())
        n_modloss_present = sum(1 for c in MODLOSS_CHANNELS if c in fieldnames)
        if n_modloss_present not in (0, len(MODLOSS_CHANNELS)):
            missing = [c for c in MODLOSS_CHANNELS if c not in fieldnames]
            raise ValueError(f"{path!r}: has {n_modloss_present}/{len(MODLOSS_CHANNELS)} "
                              f"modloss columns -- expected all or none (missing: {missing})")
        has_modloss = n_modloss_present == len(MODLOSS_CHANNELS)

        table = FragmentTable(len(CHANNELS))
        zero_modloss = (0.0, 0.0, 0.0, 0.0)
        for row in reader:
            if has_modloss:
                table.append(tuple(float(row[c]) for c in CHANNELS))
            else:
                table.append(tuple(float(row[c]) for c in CHANNELS[:4]) + zero_modloss)
    return table, has_modloss


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
    # present -- not just the 4 (or up to 4 more) channels the FI can actually insert. Gated on
    # has_modloss (2026-08-14, --ignore-modloss below): a caller building a mask meant to behave
    # like a genuine general-mode Carafe run (no modloss data at all) must also exclude modloss
    # VALUES from this reference, not just skip the modloss pool -- otherwise a real, high-
    # intensity modloss prediction could inflate the threshold and silently drop unshifted ions
    # a true general-mode run would have kept (or the reverse). For an actually general-mode
    # source this is a no-op: read_ms2_pred() zero-pads modloss channels when has_modloss is
    # False, and 0.0 can never win a max() against non-negative real intensities.
    base_rows = merged_rows if has_modloss else [row[:4] for row in merged_rows]
    base_peak = max((v for row in base_rows for v in row), default=0.0)
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
                     general_mode, var_mod_config, entries):
    """entries: iterable of (iWhichPeptide, modNumIdx, cNtermMod, cCtermMod, bMask, yMask,
    bModlossMask, yModlossMask), NOT required to be pre-sorted -- sorted here so the file is
    binary-searchable at FI build time (Section 4.3). general_mode: True iff the source
    prediction had no modloss columns at all (i.e. every entry's modloss masks are 0).
    var_mod_config: the "# VarModConfig: ..." string read from the variant map (see
    read_variant_map_var_mod_config()) -- required (not optional) so Phase 3's
    CometPredictedMask::Load() always has it to check against live comet.params (Section 8 items 12-14);
    a caller with None here (an export from an old comet.exe -x) gets a loud error instead of
    silently producing a mask Phase 3 will refuse to load anyway."""
    if var_mod_config is None:
        raise ValueError(
            "var_mod_config is required (docs/20260805_carafe.md Section 8 items 12-14) -- the variant "
            "map has no '# VarModConfig:' line, meaning it came from an older comet.exe -x. "
            "Rebuild the export with the current comet.exe.")
    entries = sorted(entries, key=lambda e: e[:4])
    with open(path, "wb") as f:
        f.write(MASK_FILE_MAGIC)
        f.write(f"SourceIdxFingerprint: {fingerprint}\n".encode("ascii"))
        f.write(f"SourceIdxNumRawPeptides: {num_raw_peptides}\n".encode("ascii"))
        f.write(f"SourceIdxPath: {idx_path}\n".encode("ascii"))
        f.write(f"MinRelativeIntensity: {threshold}\n".encode("ascii"))
        f.write(f"MinKeptPeaks: {min_kept_peaks}\n".encode("ascii"))
        f.write(f"GeneralMode: {1 if general_mode else 0}\n".encode("ascii"))
        f.write(f"VarModConfig: {var_mod_config}\n".encode("ascii"))
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

def main(argv=None):
    ap = argparse.ArgumentParser(
        description="Build a predicted-fragment keep/drop mask for Comet's FI from Carafe's "
                     "MS2 prediction output.",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog=__doc__)
    ap.add_argument("idx_file", help="The same Comet .idx file idx_to_carafe.py exported from")
    ap.add_argument("out_tsv", help="idx_to_carafe.py's main output (sequence/mods/mod_sites/"
                                     "charge) -- the file fed directly to Carafe's ai_pred.py; "
                                     "needed to join against ms2_df_tsv by content since Carafe "
                                     "doesn't preserve row order at scale (see module docstring)")
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
    ap.add_argument("--ignore-modloss", action="store_true",
                     help="Build a mask that ignores Carafe's modloss (neutral-loss) "
                          "predictions even when ms2_pred_tsv has modloss columns (phospho "
                          "mode) -- produces a mask functionally equivalent to running Carafe "
                          "in general mode against the same peptide population (base_peak/"
                          "threshold computed from the unshifted channels only, modloss masks "
                          "always 0, GeneralMode: 1 in the output header). Lets one phospho-"
                          "mode Carafe run feed both a 'with neutral loss' and a 'without "
                          "neutral loss' mask build without re-running Carafe -- pair with an "
                          "idx_file/variant_map_tsv built from a .idx whose neutral-loss mod "
                          "config was disabled (dNeutralLoss 0.0), since CometPredictedMask::"
                          "Load()'s VarModConfig check will otherwise reject a mask whose "
                          "embedded VarModConfig doesn't match the live search's.")
    ap.add_argument("-v", "--verbose", action="store_true")
    args = ap.parse_args(argv)

    if args.verbose:
        print(f"Fingerprinting {args.idx_file} ...", file=sys.stderr)
    fingerprint, num_raw_peptides = idx_fingerprint(args.idx_file)
    if args.verbose:
        print(f"  fingerprint {fingerprint}, {num_raw_peptides} raw peptides", file=sys.stderr)

    if args.verbose:
        print(f"Reading {args.out_tsv} ...", file=sys.stderr)
    out_tsv_rows = read_out_tsv(args.out_tsv)   # row_index -> (sequence, mods, mod_sites, charge)

    if args.verbose:
        print(f"Reading {args.variant_map_tsv} ...", file=sys.stderr)
    var_mod_config = read_variant_map_var_mod_config(args.variant_map_tsv)
    if var_mod_config is None:
        print(f"WARNING: {args.variant_map_tsv!r} has no '# VarModConfig:' line -- was "
              f"tools/idx_to_carafe.py run against an older comet.exe -x export? The output "
              f"mask will fail to write (Section 8 items 12-14's VarModConfig guard is required).",
              file=sys.stderr)
    vmap_rows = read_variant_map(args.variant_map_tsv)

    if args.verbose:
        print(f"Reading {args.ms2_df_tsv} ...", file=sys.stderr)
    ms2_df_by_content = read_ms2_df(args.ms2_df_tsv)
    if len(ms2_df_by_content) != len(out_tsv_rows):
        print(f"WARNING: {args.out_tsv!r} has {len(out_tsv_rows)} rows but "
              f"{args.ms2_df_tsv!r} has {len(ms2_df_by_content)} distinct rows -- expected "
              f"equal counts (every out_tsv row should be echoed back exactly once)",
              file=sys.stderr)

    if args.verbose:
        print(f"Reading {args.ms2_pred_tsv} ...", file=sys.stderr)
    ms2_pred, has_modloss = read_ms2_pred(args.ms2_pred_tsv)
    if args.verbose:
        print(f"  mode: {'phospho (modloss channels present)' if has_modloss else 'general'}",
              file=sys.stderr)

    # --ignore-modloss: mask computation below treats the source as general-mode regardless of
    # what read_ms2_pred() actually detected -- see the flag's own help text. has_modloss itself
    # (above) still reflects the true file contents for the printed message; effective_has_modloss
    # is what actually drives masking/output from here on.
    effective_has_modloss = has_modloss and not args.ignore_modloss
    if args.ignore_modloss and has_modloss and args.verbose:
        print("  --ignore-modloss: building a general-mode-equivalent mask from this "
              "phospho-mode prediction (modloss values excluded from base_peak too, not just "
              "from the modloss pool)", file=sys.stderr)

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
            content_key = out_tsv_rows[row_index]
            ms2_entry = ms2_df_by_content.get(content_key)
            if ms2_entry is None:
                print(f"WARNING: out_tsv row_index {row_index} {content_key!r} (tuple {key}) "
                      f"not found in {args.ms2_df_tsv!r} -- skipping", file=sys.stderr)
                frag_rows_per_charge = None
                break
            nAA, start, stop = ms2_entry
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
            args.min_relative_intensity, args.min_kept_peaks, effective_has_modloss, is_modified)

        n_variants += 1
        n_candidates_total += n_candidates
        n_kept_total += n_kept
        entries.append((key[0], key[1], key[2], key[3], bMask, yMask, bModlossMask, yModlossMask))

        if effective_has_modloss and is_modified:
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
                     general_mode=not effective_has_modloss, var_mod_config=var_mod_config,
                     entries=entries)

    avg_kept = (n_kept_total / n_variants) if n_variants else 0.0
    avg_candidates = (n_candidates_total / n_variants) if n_variants else 0.0
    print(f"Wrote {len(entries)} variant masks to {args.out_mask_file}", file=sys.stderr)
    print(f"Average {avg_kept:.2f} of {avg_candidates:.2f} eligible fragments kept per variant "
          f"({100.0 * n_kept_total / n_candidates_total:.1f}% overall)"
          if n_candidates_total else "No eligible fragments seen", file=sys.stderr)
    if effective_has_modloss:
        avg_modloss_kept = (n_modloss_kept_total / n_modloss_variants) if n_modloss_variants else 0.0
        avg_modloss_candidates = (n_modloss_candidates_total / n_modloss_variants) if n_modloss_variants else 0.0
        print(f"Modloss: average {avg_modloss_kept:.2f} of {avg_modloss_candidates:.2f} eligible "
              f"fragments kept per modified variant ({n_modloss_variants}/{n_variants} variants "
              f"had a modloss pool"
              + (f", {100.0 * n_modloss_kept_total / n_modloss_candidates_total:.1f}% overall)"
                 if n_modloss_candidates_total else ")"), file=sys.stderr)


if __name__ == "__main__":
    main()
