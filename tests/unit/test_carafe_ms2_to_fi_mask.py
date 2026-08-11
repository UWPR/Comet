#!/usr/bin/env python3
"""
Correctness unit test for tools/carafe_ms2_to_fi_mask.py -- Phase 2a's general-mode masking
plus Phase 2c's modloss-channel extension (docs/20260805_carafe.md Section 4.2). Pure Python,
no comet.exe/Carafe/.idx dependency -- exercises the module's functions directly against
hand-constructed inputs with independently-computable expected masks, per Section 4.2's "this
is the step that needs a dedicated correctness unit test before it touches real data."

Does NOT need REPO/COMET setup (unlike test_il_sequence.py) since nothing here shells out to
comet.exe -- this is pure in-process Python logic.
"""

import os
import struct
import sys
import tempfile
from pathlib import Path

TOOLS_DIR = Path(__file__).resolve().parent.parent.parent / "tools"
sys.path.insert(0, str(TOOLS_DIR))
import carafe_ms2_to_fi_mask as m  # noqa: E402


# ---------------------------------------------------------------------------
# Helpers
# ---------------------------------------------------------------------------

def bit(i):
    """i = Comet loop index (length - 1). Returns the mask bit for it, per the module's
    documented bit-index convention (bit (i-2), bit 0 == i==2 == length 3)."""
    return 1 << (i - m.MIN_ION_LENGTH + 1)


def row8(b_z1=0.0, b_z2=0.0, y_z1=0.0, y_z2=0.0,
          b_ml=0.0, b_ml2=0.0, y_ml=0.0, y_ml2=0.0):
    return (b_z1, b_z2, y_z1, y_z2, b_ml, b_ml2, y_ml, y_ml2)


def check(cond, msg, failures):
    if not cond:
        failures.append(msg)
    return cond


# ---------------------------------------------------------------------------
# Tests
# ---------------------------------------------------------------------------

def test_max_across_charges_generalizes_to_8_channels(failures):
    """Phase 2c generalized max_across_charges() from hardcoded 4 fields to N -- confirm it
    still does an honest elementwise max across all 8, per-charge, per-row."""
    charge2 = [row8(1.0, 0.1, 2.0, 0.2, 3.0, 0.3, 4.0, 0.4),
               row8(0.5, 0.9, 0.5, 0.9, 0.5, 0.9, 0.5, 0.9)]
    charge3 = [row8(1.5, 0.0, 1.0, 0.0, 2.5, 0.0, 5.0, 0.0),
               row8(0.1, 1.0, 0.1, 1.0, 0.1, 1.0, 0.1, 1.0)]
    merged = m.max_across_charges([charge2, charge3])
    check(merged[0] == (1.5, 0.1, 2.0, 0.2, 3.0, 0.3, 5.0, 0.4),
          f"row 0 elementwise max wrong: {merged[0]}", failures)
    check(merged[1] == (0.5, 1.0, 0.5, 1.0, 0.5, 1.0, 0.5, 1.0),
          f"row 1 elementwise max wrong: {merged[1]}", failures)


def test_read_ms2_pred_mode_detection(failures):
    """General-mode (4-col) and phospho-mode (8-col) ms2_pred.tsv files must both parse, with
    has_modloss reflecting which; a file with only SOME modloss columns must raise."""
    with tempfile.TemporaryDirectory() as tmp:
        tmp = Path(tmp)

        general_tsv = tmp / "general.tsv"
        general_tsv.write_text("b_z1\tb_z2\ty_z1\ty_z2\n0.1\t0.2\t0.3\t0.4\n")
        rows, has_modloss = m.read_ms2_pred(str(general_tsv))
        check(has_modloss is False, "general-mode file: has_modloss should be False", failures)
        check(rows == [(0.1, 0.2, 0.3, 0.4, 0.0, 0.0, 0.0, 0.0)],
              f"general-mode row not zero-padded correctly: {rows}", failures)

        phospho_tsv = tmp / "phospho.tsv"
        phospho_tsv.write_text(
            "b_z1\tb_z2\ty_z1\ty_z2\tb_modloss_z1\tb_modloss_z2\ty_modloss_z1\ty_modloss_z2\n"
            "0.1\t0.2\t0.3\t0.4\t0.5\t0.6\t0.7\t0.8\n")
        rows, has_modloss = m.read_ms2_pred(str(phospho_tsv))
        check(has_modloss is True, "phospho-mode file: has_modloss should be True", failures)
        check(rows == [(0.1, 0.2, 0.3, 0.4, 0.5, 0.6, 0.7, 0.8)],
              f"phospho-mode row parsed wrong: {rows}", failures)

        malformed_tsv = tmp / "malformed.tsv"
        malformed_tsv.write_text(
            "b_z1\tb_z2\ty_z1\ty_z2\tb_modloss_z1\tb_modloss_z2\n0.1\t0.2\t0.3\t0.4\t0.5\t0.6\n")
        raised = False
        try:
            m.read_ms2_pred(str(malformed_tsv))
        except ValueError:
            raised = True
        check(raised, "partial modloss columns should raise ValueError, didn't", failures)


def test_general_mode_unaffected_by_phase_2c(failures):
    """Phase 2a's original general-mode behavior (no modloss data) must be byte-identical
    after Phase 2c's refactor: modloss masks always 0, unshifted masks unaffected."""
    # 6-residue peptide -> 5 fragment rows (AlphaBase rows 0..4).
    # b lengths 1..5 at rows 0..4; y lengths (per nAA-r-1, nAA=6): 5,4,3,2,1 at rows 0..4.
    merged_rows = [
        row8(b_z1=0.05, y_z1=0.02),   # b1 (len1, ineligible), y5 (len5)
        row8(b_z1=0.90, y_z1=0.10),   # b2 (len2, ineligible), y4 (len4)
        row8(b_z1=1.00, y_z1=0.30),   # b3 (len3), y3 (len3)
        row8(b_z1=0.20, y_z1=0.15),   # b4 (len4), y2 (len2, ineligible)
        row8(b_z1=0.05, y_z1=0.05),   # b5 (len5), y1 (len1, ineligible)
    ]
    result = m.compute_variant_mask(merged_rows, nAA=6, min_relative_intensity=0.10,
                                     min_kept_peaks=6, has_modloss=False, is_modified=True)
    (bMask, yMask, bModlossMask, yModlossMask,
     n_candidates, n_kept, n_modloss_candidates, n_modloss_kept, base_peak) = result

    check(base_peak == 1.00, f"base_peak should be 1.00 (the b3 peak), got {base_peak}", failures)
    check(bModlossMask == 0 and yModlossMask == 0,
          f"has_modloss=False must yield all-zero modloss masks, got "
          f"{bModlossMask:#x}/{yModlossMask:#x}", failures)
    check(n_modloss_candidates == 0 and n_modloss_kept == 0,
          "has_modloss=False must report zero modloss candidates/kept", failures)

    # Eligible (length>=3) unshifted candidates: b3=1.00, b4=0.20, b5=0.05, y5=0.02, y4=0.10,
    # y3=0.30 -- 6 total, all >= min_kept_peaks floor of 6, so all 6 are kept regardless of the
    # 0.10 threshold (threshold = 0.10 -- b5/y5 are actually right at/below it, but the floor
    # forces them in since there are exactly 6 eligible candidates total).
    check(n_candidates == 6, f"expected 6 eligible (length>=3) candidates, got {n_candidates}", failures)
    check(n_kept == 6, f"expected all 6 kept (floor==candidate count), got {n_kept}", failures)
    expected_bMask = bit(2) | bit(3) | bit(4)   # b3,b4,b5 -> i=2,3,4
    expected_yMask = bit(4) | bit(3) | bit(2)   # y5,y4,y3 -> i=4,3,2
    check(bMask == expected_bMask, f"bMask: expected {expected_bMask:#x}, got {bMask:#x}", failures)
    check(yMask == expected_yMask, f"yMask: expected {expected_yMask:#x}, got {yMask:#x}", failures)


def test_modloss_mask_independent_from_unshifted(failures):
    """Core Phase 2c behavior: modloss channel is thresholded/floored completely independently
    from the unshifted channel, against the shared base_peak, for a modified variant."""
    # Same 6-residue/5-row shape as above, but now with modloss columns populated too, and a
    # deliberately different keep/drop pattern between unshifted and modloss so the test can't
    # pass by the two pools accidentally coinciding.
    merged_rows = [
        row8(b_z1=0.05, y_z1=0.02, b_ml=0.00, y_ml=1.00),   # row0: b1/y5 -- y5 modloss is the base peak
        row8(b_z1=0.90, y_z1=0.10, b_ml=0.00, y_ml=0.00),   # row1: b2/y4
        row8(b_z1=1.00, y_z1=0.30, b_ml=0.05, y_ml=0.00),   # row2: b3/y3 -- unshifted base peak
        row8(b_z1=0.20, y_z1=0.15, b_ml=0.60, y_ml=0.00),   # row3: b4/y2
        row8(b_z1=0.05, y_z1=0.05, b_ml=0.02, y_ml=0.00),   # row4: b5/y1
    ]
    # base_peak = max over ALL 8 channels = 1.00 (tie between unshifted b3 and modloss y5 -- both 1.00).
    result = m.compute_variant_mask(merged_rows, nAA=6, min_relative_intensity=0.50,
                                     min_kept_peaks=1, has_modloss=True, is_modified=True)
    (bMask, yMask, bModlossMask, yModlossMask,
     n_candidates, n_kept, n_modloss_candidates, n_modloss_kept, base_peak) = result

    check(base_peak == 1.00, f"base_peak should be 1.00, got {base_peak}", failures)
    # threshold = 0.50 * 1.00 = 0.50. Unshifted eligible (length>=3): b3=1.00(pass), b4=0.20(fail),
    # b5=0.05(fail), y5=0.02(fail), y4=0.10(fail), y3=0.30(fail) -- only b3 clears; floor=1 so
    # exactly b3 is kept (floor already satisfied by the 1 that cleared).
    check(n_candidates == 6, f"expected 6 unshifted candidates, got {n_candidates}", failures)
    check(n_kept == 1, f"expected 1 unshifted kept (only b3 clears 0.50 threshold), got {n_kept}", failures)
    check(bMask == bit(2), f"bMask should be only bit(2) (b3), got {bMask:#x}", failures)
    check(yMask == 0, f"yMask should be empty, got {yMask:#x}", failures)

    # Modloss eligible (length>=3): b_modloss3=0.05(fail), b_modloss4=0.60(pass),
    # b_modloss5=0.02(fail), y_modloss5=1.00(pass), y_modloss4=0.00(fail), y_modloss3=0.00(fail)
    # -- 2 clear the threshold; floor=1 already satisfied.
    check(n_modloss_candidates == 6, f"expected 6 modloss candidates, got {n_modloss_candidates}", failures)
    check(n_modloss_kept == 2, f"expected 2 modloss kept (b4-modloss, y5-modloss), got {n_modloss_kept}", failures)
    check(bModlossMask == bit(3), f"bModlossMask should be only bit(3) (b4), got {bModlossMask:#x}", failures)
    check(yModlossMask == bit(4), f"yModlossMask should be only bit(4) (y5), got {yModlossMask:#x}", failures)

    # And the unshifted result must be UNCHANGED by the modloss pool's presence -- re-run the
    # exact same rows with has_modloss=False and confirm bMask/yMask/n_candidates/n_kept match.
    result_no_ml = m.compute_variant_mask(merged_rows, nAA=6, min_relative_intensity=0.50,
                                           min_kept_peaks=1, has_modloss=False, is_modified=True)
    check(result_no_ml[0] == bMask and result_no_ml[1] == yMask,
          "unshifted bMask/yMask must be independent of has_modloss", failures)
    check(result_no_ml[4] == n_candidates and result_no_ml[5] == n_kept,
          "unshifted n_candidates/n_kept must be independent of has_modloss", failures)
    # base_peak is purely a function of merged_rows' own contents (max over all 8 fields,
    # unconditionally) -- NOT gated on has_modloss -- per Section 8 item 3's "the same
    # reference regardless of search mode". In real usage this is moot (read_ms2_pred always
    # zero-pads modloss fields when has_modloss is False, so there's no real signal to
    # differ over); this direct call artificially keeps the modloss values present in
    # merged_rows while flipping the flag, so base_peak legitimately stays 1.00 either way --
    # confirming has_modloss only gates the modloss POOL's threshold/floor computation, not
    # the base_peak reference itself.
    check(result_no_ml[8] == base_peak,
          f"base_peak must be identical regardless of has_modloss (both derived from the same "
          f"merged_rows): with-flag={base_peak}, without-flag={result_no_ml[8]}", failures)


def test_modloss_skipped_for_unmodified_variant(failures):
    """A variant with modNumIdx == -1 (is_modified=False) must get zero modloss masks even
    when has_modloss=True and the modloss columns themselves have high intensity -- ties to
    CometFragmentIndex.cpp's bFragmentNL gate (modNumIdx >= 0 required for ANY NL-shifted FI
    entry), so computing/floor-padding a modloss mask here would be meaningless dead data."""
    merged_rows = [
        row8(b_z1=0.9, y_z1=0.9, b_ml=0.9, y_ml=0.9),
        row8(b_z1=0.9, y_z1=0.9, b_ml=0.9, y_ml=0.9),
        row8(b_z1=0.9, y_z1=0.9, b_ml=0.9, y_ml=0.9),
        row8(b_z1=0.9, y_z1=0.9, b_ml=0.9, y_ml=0.9),
    ]
    result = m.compute_variant_mask(merged_rows, nAA=5, min_relative_intensity=0.10,
                                     min_kept_peaks=6, has_modloss=True, is_modified=False)
    (bMask, yMask, bModlossMask, yModlossMask,
     n_candidates, n_kept, n_modloss_candidates, n_modloss_kept, base_peak) = result
    check(bModlossMask == 0 and yModlossMask == 0,
          f"unmodified variant must have zero modloss masks even with has_modloss=True, got "
          f"{bModlossMask:#x}/{yModlossMask:#x}", failures)
    check(n_modloss_candidates == 0 and n_modloss_kept == 0,
          "unmodified variant must report zero modloss candidates/kept", failures)
    # Unshifted masking must still proceed normally for an unmodified (but otherwise eligible)
    # variant -- is_modified only gates the modloss pool, nothing else.
    check(n_candidates > 0 and n_kept > 0,
          "unshifted masking should still happen for an unmodified variant", failures)


def test_floor_never_pads_beyond_available_candidates(failures):
    """If a pool has fewer than min_kept_peaks eligible candidates, keep all of them -- no
    padding beyond what exists (Section 8 item 4's explicit "does not pad to 6" requirement),
    checked here for the modloss pool specifically (Phase 2c's new pool)."""
    # nAA=4 -> 3 fragment rows -> b lengths 1,2,3; y lengths 3,2,1. Only length>=3 survives:
    # b3 (row2) and y3 (row0) -- exactly 2 modloss candidates available, floor asks for 6.
    merged_rows = [
        row8(b_z1=0.01, y_z1=0.01, b_ml=0.01, y_ml=0.40),
        row8(b_z1=0.01, y_z1=0.01, b_ml=0.01, y_ml=0.01),
        row8(b_z1=0.01, y_z1=0.01, b_ml=0.30, y_ml=0.01),
    ]
    result = m.compute_variant_mask(merged_rows, nAA=4, min_relative_intensity=0.99,
                                     min_kept_peaks=6, has_modloss=True, is_modified=True)
    (bMask, yMask, bModlossMask, yModlossMask,
     n_candidates, n_kept, n_modloss_candidates, n_modloss_kept, base_peak) = result
    check(n_modloss_candidates == 2, f"expected 2 modloss candidates total, got {n_modloss_candidates}", failures)
    check(n_modloss_kept == 2, f"floor should keep all available (2), not pad beyond, got {n_modloss_kept}", failures)
    check(bModlossMask == bit(2) and yModlossMask == bit(2),
          f"both the only b3/y3 modloss candidates should be kept, got "
          f"bModlossMask={bModlossMask:#x} yModlossMask={yModlossMask:#x}", failures)


def test_mask_file_roundtrip_v3(failures):
    """write_mask_file/read_mask_file must round-trip the v3 8-field entry struct exactly,
    including nonzero modloss masks, and the magic line must be the v3 string."""
    with tempfile.TemporaryDirectory() as tmp:
        path = str(Path(tmp) / "test.mask")
        entries = [
            (5, 2, -1, -1, 0x7, 0x3, 0x1, 0x0),
            (10, -1, 0, -1, 0x0, 0x0, 0x0, 0x0),
        ]
        m.write_mask_file(path, "deadbeef" * 8, 12345, "/some/path.idx",
                           0.10, 6, general_mode=False, var_mod_config="79.966331S--0.000000",
                           entries=entries)

        with open(path, "rb") as f:
            magic = f.readline()
        check(magic == m.MASK_FILE_MAGIC, f"magic line mismatch: {magic!r}", failures)

        header, read_entries = m.read_mask_file(path)
        check(header.get("GeneralMode") == "0", f"GeneralMode header wrong: {header.get('GeneralMode')!r}", failures)
        check(header.get("SourceIdxNumRawPeptides") == "12345",
              f"SourceIdxNumRawPeptides header wrong: {header.get('SourceIdxNumRawPeptides')!r}", failures)
        check(header.get("VarModConfig") == "79.966331S--0.000000",
              f"VarModConfig header wrong: {header.get('VarModConfig')!r}", failures)
        check(sorted(read_entries) == sorted(entries),
              f"entries didn't round-trip: wrote {entries}, read {read_entries}", failures)

        # general_mode=True path too, since GeneralMode's value is derived (not just echoed).
        path2 = str(Path(tmp) / "test2.mask")
        m.write_mask_file(path2, "cafef00d" * 8, 1, "/x.idx", 0.10, 6,
                           general_mode=True, var_mod_config="",
                           entries=[(1, -1, -1, -1, 0x1, 0x1, 0x0, 0x0)])
        header2, _ = m.read_mask_file(path2)
        check(header2.get("GeneralMode") == "1", f"GeneralMode=True should write '1', got {header2.get('GeneralMode')!r}", failures)

        # var_mod_config=None must raise -- Section 8 items 12-14's guard is required, not optional.
        raised = False
        try:
            m.write_mask_file(str(Path(tmp) / "test3.mask"), "abc", 1, "/x.idx", 0.10, 6,
                               general_mode=True, var_mod_config=None, entries=[])
        except ValueError:
            raised = True
        check(raised, "var_mod_config=None should raise ValueError, didn't", failures)

        check(m.ENTRY_SIZE == struct.calcsize("<IibbQQQQ"),
              f"ENTRY_SIZE should match the 8-field v3 struct, got {m.ENTRY_SIZE}", failures)


# ---------------------------------------------------------------------------
# Runner
# ---------------------------------------------------------------------------

TESTS = [
    test_max_across_charges_generalizes_to_8_channels,
    test_read_ms2_pred_mode_detection,
    test_general_mode_unaffected_by_phase_2c,
    test_modloss_mask_independent_from_unshifted,
    test_modloss_skipped_for_unmodified_variant,
    test_floor_never_pads_beyond_available_candidates,
    test_mask_file_roundtrip_v3,
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
