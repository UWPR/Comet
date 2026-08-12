#!/usr/bin/env python3
"""
Phase 5 alignment-correctness test (docs/20260805_carafe.md Section 7, item 1): formalizes
Phase 0's original hand-run validation (Section 6.1, "a standalone scratch script ... not
committed") of pitfall 2 -- the claim that AlphaBase row r's b column is Comet AddFragments()
ladder-index i=r's b-ion directly, while AlphaBase row r's y column is Comet ladder-index
i=(nAA-2-r)'s y-ion (mirrored, not row-for-row) -- as a permanent, automated, committed test.

Pure Python, no comet.exe/Carafe/.idx/torch dependency (matches
test_carafe_ms2_to_fi_mask.py's own philosophy): both ion-mass formulas below are independently
re-derived from real source (CometFragmentIndex.cpp's AddFragments() loop for Comet's side;
AlphaBase's mass_calc.py structure, as fetched and confirmed in Phase 0, for the other), not
imported from either the mask builder under test or Comet itself -- an independent
re-implementation is the only way this test can actually catch a remapping regression rather
than just re-asserting whatever carafe_ms2_to_fi_mask.py already assumes.

Two layers:
  1. compute_comet_b/y_ion_masses() vs. compute_alphabase_row_b/y_masses(): two independently
     written mass ladders, cross-checked to agree exactly (to floating-point noise) at every
     ladder index for several real peptides, including MIN_ION_LENGTH boundary cases -- this is
     Phase 0's original numeric check, now permanent.
  2. build_alphabase_shaped_rows(): builds a synthetic-but-real-formula Carafe ms2_pred-shaped
     row matrix (using the SAME independently-derived AlphaBase convention as layer 1, with a
     distinctive per-length intensity marker instead of a real predicted value) and feeds it
     through the ACTUAL production function under test,
     tools/carafe_ms2_to_fi_mask.compute_variant_mask(), confirming the bMask/yMask bits it
     returns select exactly the lengths this test independently expects -- proving the
     remapping is correct not just in isolation but through the real code path a Carafe run
     would exercise.
"""

import os
import sys
from pathlib import Path

TOOLS_DIR = Path(__file__).resolve().parent.parent.parent / "tools"
sys.path.insert(0, str(TOOLS_DIR))
import carafe_ms2_to_fi_mask as m  # noqa: E402


# ---------------------------------------------------------------------------
# Monoisotopic residue masses -- independently re-derived from
# CometSearch/CometMassSpecUtils.cpp's InitializeMassTables() elemental formulas and
# CometSearch/CometMassSpecUtils.h's element constants, not imported from anywhere in the repo.
# ---------------------------------------------------------------------------

_H = 1.007825035
_O = 15.99491463
_C = 12.00000000
_N = 14.0030740
_S = 31.9720707

RESIDUE_MASS = {
    "G": _C*2  + _H*3  + _N   + _O,
    "A": _C*3  + _H*5  + _N   + _O,
    "S": _C*3  + _H*5  + _N   + _O*2,
    "P": _C*5  + _H*7  + _N   + _O,
    "V": _C*5  + _H*9  + _N   + _O,
    "T": _C*4  + _H*7  + _N   + _O*2,
    "C": _C*3  + _H*5  + _N   + _O   + _S,
    "L": _C*6  + _H*11 + _N   + _O,
    "I": _C*6  + _H*11 + _N   + _O,
    "N": _C*4  + _H*6  + _N*2 + _O*2,
    "D": _C*4  + _H*5  + _N   + _O*3,
    "Q": _C*5  + _H*8  + _N*2 + _O*2,
    "K": _C*6  + _H*12 + _N*2 + _O,
    "E": _C*5  + _H*7  + _N   + _O*3,
    "M": _C*5  + _H*9  + _N   + _O   + _S,
    "H": _C*6  + _H*7  + _N*3 + _O,
    "F": _C*9  + _H*9  + _N   + _O,
    "R": _C*6  + _H*12 + _N*4 + _O,
    "Y": _C*9  + _H*9  + _N   + _O*2,
    "W": _C*11 + _H*10 + _N*2 + _O,
}

TOLERANCE = 1e-9  # both formulas below use the identical residue-mass table, so agreement
                   # should be to floating-point noise, not measurement precision


def check(cond, msg, failures):
    if not cond:
        failures.append(msg)
    return cond


# ---------------------------------------------------------------------------
# Layer 1: two independent ladders
# ---------------------------------------------------------------------------

def compute_comet_ladder(peptide):
    """Re-implements CometFragmentIndex.cpp's AddFragments() b/y accumulation exactly
    (CometFragmentIndex.cpp:673-678: dBion/dYion accumulated via a forward+backward two-pointer
    sweep over the SAME loop index i, unmodified-peptide case only -- no variable mods here).
    Returns {i: (b_ion_mass, y_ion_mass)} for every ladder index i in [0, nAA-2) (Comet's
    `iEndPos = nAA - 1`), i.e. one entry per (b-ion length i+1, y-ion length i+1) pair."""
    n = len(peptide)
    end_pos = n - 1
    ladder = {}
    b_ion = 0.0
    y_ion = 0.0
    for i in range(end_pos):
        pos_reverse = end_pos - i
        b_ion += RESIDUE_MASS[peptide[i]]
        y_ion += RESIDUE_MASS[peptide[pos_reverse]]
        ladder[i] = (b_ion, y_ion)
    return ladder


def compute_alphabase_rows(peptide):
    """Independently re-implements AlphaBase's row convention as structurally confirmed in
    Phase 0 (docs/20260805_carafe.md Section 6.1) directly from AlphaBase's mass_calc.py source:
    row r's b_mass = sum of the first r+1 residues; row r's y_mass = total peptide residue mass
    minus b_mass[r] (the complementary cleavage-site pair), for r in [0, nAA-2). Deliberately
    computed via prefix sums from scratch here rather than reusing compute_comet_ladder()'s
    cumulative accumulator, even though they're mathematically the same recurrence -- the point
    of this test is that two independently-written implementations agree, not that one calls
    the other."""
    n = len(peptide)
    total = sum(RESIDUE_MASS[c] for c in peptide)
    rows = {}
    for r in range(n - 1):
        b_mass = sum(RESIDUE_MASS[c] for c in peptide[:r + 1])
        y_mass = total - b_mass
        rows[r] = (b_mass, y_mass)
    return rows


def test_comet_ladder_matches_alphabase_rows_numerically(failures):
    """Layer 1: Phase 0's original hand-verification (5 peptides including length-2/3 edge
    cases), now automated. For every Comet ladder index i (only i > 1 is ever actually indexed
    by AddFragments()'s masking gate, but the mapping must hold at every i for the remapping
    itself to be well-defined): b-ion mass at Comet index i == AlphaBase row i's b_mass
    (same-row, no remap); y-ion mass at Comet index i == AlphaBase row (nAA-2-i)'s y_mass
    (mirrored row)."""
    peptides = ["ACDEFGHIKL", "SAMPLER", "PEPTIDE", "AK", "MNQ", "ACDSEFGHIK", "GG"]
    for pep in peptides:
        n = len(pep)
        comet = compute_comet_ladder(pep)
        alphabase = compute_alphabase_rows(pep)

        for i, (b_comet, y_comet) in comet.items():
            b_alpha, _y_alpha_same_row = alphabase[i]
            check(abs(b_comet - b_alpha) < TOLERANCE,
                  f"{pep}: b-ion at Comet index {i}: comet={b_comet:.6f} vs "
                  f"alphabase row {i}={b_alpha:.6f} (same-row mapping)", failures)

            mirrored_row = n - 2 - i
            _b_alpha_mirrored, y_alpha = alphabase[mirrored_row]
            check(abs(y_comet - y_alpha) < TOLERANCE,
                  f"{pep}: y-ion at Comet index {i}: comet={y_comet:.6f} vs "
                  f"alphabase row {mirrored_row}={y_alpha:.6f} (mirrored-row mapping, "
                  f"nAA-2-i = {n}-2-{i} = {mirrored_row})", failures)


def test_same_row_y_mapping_would_be_wrong(failures):
    """Negative control: confirms the mirrored-row mapping isn't a no-op -- i.e. this test
    would actually catch pitfall 2 regressing back to the naive (wrong) same-row y mapping.
    Skipped for peptides/indices where the peptide happens to be a palindrome-like mass
    sequence (mirrored_row == i), which would make same-row and mirrored-row identical by
    coincidence rather than by bug."""
    pep = "ACDEFGHIKL"   # 10 residues, no accidental b/y mass symmetry
    n = len(pep)
    comet = compute_comet_ladder(pep)
    alphabase = compute_alphabase_rows(pep)
    found_a_real_mismatch = False
    for i, (_b_comet, y_comet) in comet.items():
        mirrored_row = n - 2 - i
        if mirrored_row == i:
            continue
        _b_alpha_same_row, y_alpha_same_row = alphabase[i]
        if abs(y_comet - y_alpha_same_row) > TOLERANCE:
            found_a_real_mismatch = True
    check(found_a_real_mismatch,
          f"expected the naive same-row y mapping to disagree with Comet's real y-ion mass at "
          f"at least one index for {pep!r} -- if it never does, this test can't distinguish "
          f"correct (mirrored) from buggy (same-row) mapping and layer 1 above is not a "
          f"meaningful check", failures)


# ---------------------------------------------------------------------------
# Layer 2: feed real-shaped rows through the actual production function under test
# ---------------------------------------------------------------------------

def bit_for_length(length):
    i = length - 1
    return 1 << (i - m.MIN_ION_LENGTH + 1)


def _build_rows(pep, b_by_length, y_by_length):
    """Builds an nAA-1-row, 8-channel matrix in compute_variant_mask()'s expected shape (CHANNELS
    order: b_z1, b_z2, y_z1, y_z2, 4 always-zero modloss channels), using the SAME
    independently-confirmed AlphaBase row convention as layer 1 above: row r's b_z1 belongs to
    b-ion length r+1 (same-row); row r's y_z1 belongs to y-ion length nAA-r-1 (mirrored-row) --
    real AlphaBase output would place values this same way, so a row built like this is what
    compute_variant_mask() actually receives in production."""
    n = len(pep)
    rows = []
    for r in range(n - 1):
        b_length = r + 1
        y_length = n - r - 1
        rows.append((b_by_length[b_length], 0.0, y_by_length[y_length], 0.0, 0.0, 0.0, 0.0, 0.0))
    return rows


def _run_variant_mask(pep, b_by_length, y_by_length):
    rows = _build_rows(pep, b_by_length, y_by_length)
    (bMask, yMask, bModlossMask, yModlossMask,
     n_candidates, n_kept, n_modloss_candidates, n_modloss_kept, base_peak) = m.compute_variant_mask(
        rows, len(pep), min_relative_intensity=0.50, min_kept_peaks=6,
        has_modloss=False, is_modified=False)
    return bMask, yMask, bModlossMask, yModlossMask, base_peak


def test_compute_variant_mask_remaps_real_rows_correctly(failures):
    """Layer 2: the actual carafe_ms2_to_fi_mask.compute_variant_mask() function, fed a row
    matrix built with the SAME independently-confirmed AlphaBase row convention as layer 1 above
    -- not a hand-picked bit pattern like test_carafe_ms2_to_fi_mask.py's existing tests -- must
    keep exactly the lengths an independent length->intensity marker assignment says it should,
    for BOTH channels independently. Two sub-cases (b-heavy, y-heavy) so a b/y channel mixup is
    also caught, not just a length mixup. The y-heavy case is THE pitfall-2 regression check:
    each row r's y_z1 is placed at length nAA-r-1 (mirrored) when building the input here,
    matching real AlphaBase output -- if compute_variant_mask()'s y_by_length dict regressed to
    same-row indexing, the high markers would land on the wrong lengths and be caught.

    Threshold (50% of the 100.0 base peak = 50.0) is chosen so n_above_threshold (6, lengths
    3-8) already equals min_kept_peaks (6) -- the floor never needs to pad beyond the literal
    >=-threshold set, keeping this test's independently-computed expectation a plain threshold
    comparison rather than a re-implementation of _threshold_and_floor_pool()'s combined-pool
    top-N logic (already covered by test_carafe_ms2_to_fi_mask.py's own
    test_floor_never_pads_beyond_available_candidates)."""
    pep = "ACDSEFGHIK"   # 10 residues -> lengths 3..9 indexable (i=2..8, MIN_ION_LENGTH=3);
                         # lengths 1-2 are still present as rows (build_candidates() filters
                         # them out by length, not by absence), so both dicts need entries for
                         # every length 1..9 or the lookup below raises KeyError.
    heavy = {1: 0.0, 2: 0.0, 3: 100.0, 4: 90.0, 5: 80.0, 6: 70.0, 7: 60.0, 8: 50.0, 9: 5.0}
    light = {1: 0.0, 2: 0.0, 3: 1.0, 4: 1.0, 5: 1.0, 6: 1.0, 7: 1.0, 8: 1.0, 9: 1.0}
    expected_heavy_lengths = {3, 4, 5, 6, 7, 8}
    expected_mask = 0
    for length in expected_heavy_lengths:
        expected_mask |= bit_for_length(length)

    # --- b-heavy: b markers distinctive, y uniformly low ---
    bMask, yMask, bModlossMask, yModlossMask, base_peak = _run_variant_mask(pep, heavy, light)
    check(base_peak == 100.0, f"b-heavy case: expected base_peak=100.0, got {base_peak}", failures)
    check(bModlossMask == 0 and yModlossMask == 0,
          f"b-heavy case: has_modloss=False must produce all-zero modloss masks, got "
          f"bModlossMask={bModlossMask:#x} yModlossMask={yModlossMask:#x}", failures)
    check(bMask == expected_mask,
          f"b-heavy case: expected bMask bits for lengths {sorted(expected_heavy_lengths)} "
          f"({expected_mask:#x}), got {bMask:#x} -- catches a b-channel length<->bit-index "
          f"remapping regression (b_by_length[r+1] = b_z1, same-row)", failures)
    check(yMask == 0,
          f"b-heavy case: expected yMask=0 (y markers all below threshold), got {yMask:#x} -- "
          f"catches a b/y channel mixup", failures)

    # --- y-heavy: y markers distinctive (via the true mirrored-row length), b uniformly low ---
    bMask2, yMask2, bModlossMask2, yModlossMask2, base_peak2 = _run_variant_mask(pep, light, heavy)
    check(base_peak2 == 100.0, f"y-heavy case: expected base_peak=100.0, got {base_peak2}", failures)
    check(yMask2 == expected_mask,
          f"y-heavy case: expected yMask bits for lengths {sorted(expected_heavy_lengths)} "
          f"({expected_mask:#x}), got {yMask2:#x} -- this is THE pitfall-2 regression check: "
          f"real AlphaBase output places these markers at the mirrored row; if "
          f"compute_variant_mask()'s y_by_length dict regressed to same-row indexing, the high "
          f"markers would land on the wrong lengths and this assertion would catch it", failures)
    check(bMask2 == 0,
          f"y-heavy case: expected bMask=0 (b markers all below threshold), got {bMask2:#x} -- "
          f"catches a b/y channel mixup", failures)


# ---------------------------------------------------------------------------
# Runner (matches test_carafe_ms2_to_fi_mask.py's own standalone-script convention)
# ---------------------------------------------------------------------------

TESTS = [
    test_comet_ladder_matches_alphabase_rows_numerically,
    test_same_row_y_mapping_would_be_wrong,
    test_compute_variant_mask_remaps_real_rows_correctly,
]


def run_test():
    all_failures = []
    for test_fn in TESTS:
        failures = []
        test_fn(failures)
        status = "PASS" if not failures else "FAIL"
        print(f"  [{status}] {test_fn.__name__}")
        for f in failures:
            print(f"         - {f}")
        all_failures.extend(failures)

    if all_failures:
        print(f"\n{len(all_failures)} failure(s)")
        return False
    print("\nPASS")
    return True


if __name__ == "__main__":
    ok = run_test()
    sys.exit(0 if ok else 1)
