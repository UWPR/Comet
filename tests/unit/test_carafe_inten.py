#!/usr/bin/env python3
"""
Correctness unit tests for tools/carafe_cps_to_inten.py (the .carafe_inten builder --
docs/20260903_IntensityScore_design.md Phase 0). Pure in-process Python, no comet.exe/.idx
dependency, matching test_carafe_cps.py's pattern; run standalone or via run_tests.py T38.

Covers: peak-code packing, sqrt quantization endpoints, the AlphaBase-row -> Comet-ladder
position mapping (b direct, y mirrored) including lengths 1-2 that the FI mask excludes,
threshold / max-peaks / zero-q dropping, modloss handling in all (has_modloss x is_modified)
combinations and the general-mode base4 reference, multi-charge max-merge, entry
(de)serialization incl. the zero-peak entry, the worker path over a real tiny .cps + variant
map, the written-file verifier (accepts a good file, rejects an unsorted one), and the
k-way merge restoring global key order across overlapping ranges.
"""

import os
import struct
import sys
import tempfile
from pathlib import Path

TOOLS_DIR = Path(__file__).resolve().parent.parent.parent / "tools"
sys.path.insert(0, str(TOOLS_DIR))
import carafe_cps as cps  # noqa: E402
import carafe_cps_to_fi_mask as cfm  # noqa: E402
import carafe_cps_to_inten as inten  # noqa: E402


def check(cond, msg, failures):
    if not cond:
        failures.append(msg)
    return cond


def _code(ch, pos):
    return inten.peak_code(ch, pos)


def _make_vmap(path, groups, var_mod_config="15.994900M--0.000000|0.000000X--0.000000"):
    """groups: [(key_tuple, [row_index, ...]), ...] written in the given order (which need
    NOT be key order -- mirroring the real map)."""
    with open(path, "wb") as f:
        f.write(f"# VarModConfig: {var_mod_config}\n".encode("ascii"))
        f.write(b"row_index\tiWhichPeptide\tmodNumIdx\tcNtermMod\tcCtermMod\n")
        for key, rows in groups:
            for ri in rows:
                f.write(f"{ri}\t{key[0]}\t{key[1]}\t{key[2]}\t{key[3]}\n".encode("ascii"))


# ---------------------------------------------------------------------------

def test_peak_code_roundtrip(failures):
    for ch in (0, 1, 2, 3, 7):
        for pos in (0, 1, 49, 255):
            c = inten.peak_code(ch, pos)
            check(inten.decode_peak_code(c) == (ch, pos),
                  f"peak_code({ch},{pos}) did not round-trip", failures)
    check(inten.peak_code(1, 5) == (1 << 8) | 5, "code layout is channel<<8 | pos", failures)
    for bad in ((-1, 0), (0, 256), (256, 0)):
        try:
            inten.peak_code(*bad)
            check(False, f"peak_code{bad} should raise", failures)
        except ValueError:
            pass


def test_quantize_sqrt(failures):
    check(inten.quantize_sqrt(0.0) == 0, "q(0) must be 0", failures)
    check(inten.quantize_sqrt(-0.5) == 0, "negative rel clamps to 0", failures)
    check(inten.quantize_sqrt(1.0) == 255, "q(1) must be 255", failures)
    check(inten.quantize_sqrt(1.7) == 255, "rel > 1 clamps to 255", failures)
    check(inten.quantize_sqrt(0.25) == 128, f"q(0.25)=round(127.5)=128, got "
          f"{inten.quantize_sqrt(0.25)}", failures)
    check(inten.quantize_sqrt(0.01) == 26, f"q(0.01)=round(25.5)=26, got "
          f"{inten.quantize_sqrt(0.01)}", failures)
    check(abs(inten.dequantize_sqrt(255) - 1.0) < 1e-12, "dequant(255) == 1.0", failures)
    # the smallest nonzero q is 1 -> rel ~1.5e-5: below any sensible threshold, so a kept
    # peak (rel >= 0.01 by default) can never quantize to 0 at default settings
    check(inten.quantize_sqrt(inten.DEFAULT_MIN_RELATIVE_INTENSITY) >= 1,
          "default threshold must not quantize to 0", failures)


def test_ladder_mapping_phospho_modified(failures):
    """nAA=5 -> 4 AlphaBase rows r=0..3. b at row r is b_{r+1} -> ladderPos r; y at row r
    is y_{4-r} -> ladderPos 3-r. Values are chosen so every peak is distinct and above the
    threshold; base8 = 1.0 so rel == value."""
    nAA = 5
    rows4 = [
        # (b_z1, y_z1, b_ml, y_ml)
        (0.10, 0.20, 0.30, 0.40),   # r=0: b1@pos0, y4@pos3, bml1@pos0, yml4@pos3
        (0.11, 0.21, 0.31, 0.41),   # r=1: b2@pos1, y3@pos2
        (0.12, 0.22, 0.32, 0.42),   # r=2: b3@pos2, y2@pos1
        (0.13, 0.23, 0.33, 0.43),   # r=3: b4@pos3, y1@pos0
    ]
    peaks = inten.compute_variant_intensity([rows4], nAA, [1.0], [1.0], 0.01, 32,
                                            has_modloss=True, is_modified=True)
    got = dict(peaks)
    want = {
        _code(inten.CH_B, 0): 0.10, _code(inten.CH_B, 1): 0.11,
        _code(inten.CH_B, 2): 0.12, _code(inten.CH_B, 3): 0.13,
        _code(inten.CH_Y, 3): 0.20, _code(inten.CH_Y, 2): 0.21,
        _code(inten.CH_Y, 1): 0.22, _code(inten.CH_Y, 0): 0.23,
        _code(inten.CH_B_ML, 0): 0.30, _code(inten.CH_B_ML, 1): 0.31,
        _code(inten.CH_B_ML, 2): 0.32, _code(inten.CH_B_ML, 3): 0.33,
        _code(inten.CH_Y_ML, 3): 0.40, _code(inten.CH_Y_ML, 2): 0.41,
        _code(inten.CH_Y_ML, 1): 0.42, _code(inten.CH_Y_ML, 0): 0.43,
    }
    check(len(peaks) == 16, f"expected 16 peaks, got {len(peaks)}", failures)
    check(set(got) == set(want), f"peak code set mismatch: {sorted(got)} vs {sorted(want)}",
          failures)
    for code, rel in want.items():
        if code in got:
            check(got[code] == inten.quantize_sqrt(rel),
                  f"code {code:#06x}: q {got[code]} != quantize({rel})", failures)
    check([c for c, _ in peaks] == sorted(c for c, _ in peaks),
          "peaks must be sorted by code", failures)
    # lengths 1 and 2 (ladderPos 0 and 1) ARE present -- no FI-mask "i > 1" gate here
    check(_code(inten.CH_B, 0) in got and _code(inten.CH_Y, 0) in got,
          "b1/y1 (ladderPos 0) must be scoreable", failures)


def test_threshold_maxpeaks_and_zero_q(failures):
    nAA = 4
    rows4 = [(0.9, 0.005, 0.0, 0.0), (0.5, 0.02, 0.0, 0.0), (0.011, 0.0, 0.0, 0.0)]
    # threshold 0.01 drops 0.005 and the zeros; keeps 0.9, 0.5, 0.02, 0.011
    peaks = inten.compute_variant_intensity([rows4], nAA, [1.0], [1.0], 0.01, 32,
                                            has_modloss=False, is_modified=False)
    check(len(peaks) == 4, f"threshold: expected 4 peaks, got {len(peaks)} {peaks}", failures)
    # max_peaks=2 keeps the two most intense (0.9 b1@0, 0.5 b2@1)
    peaks = inten.compute_variant_intensity([rows4], nAA, [1.0], [1.0], 0.01, 2,
                                            has_modloss=False, is_modified=False)
    check(sorted(c for c, _ in peaks) == [_code(inten.CH_B, 0), _code(inten.CH_B, 1)],
          f"max_peaks=2 kept wrong peaks: {peaks}", failures)
    # max_peaks=0 -> empty entry
    peaks = inten.compute_variant_intensity([rows4], nAA, [1.0], [1.0], 0.01, 0,
                                            has_modloss=False, is_modified=False)
    check(peaks == [], "max_peaks=0 must yield no peaks", failures)
    # a threshold of 0 admits tiny values, but those quantizing to 0 are dropped
    rows4 = [(1.0, 1e-9, 0.0, 0.0)]
    peaks = inten.compute_variant_intensity([rows4], 2, [1.0], [1.0], 0.0, 32,
                                            has_modloss=False, is_modified=False)
    check(peaks == [(_code(inten.CH_B, 0), 255)], f"zero-q peak must be dropped: {peaks}",
          failures)
    # tie-break: equal rel -> lower code wins the max_peaks cut (deterministic)
    rows4 = [(0.5, 0.5, 0.0, 0.0), (0.5, 0.5, 0.0, 0.0)]
    peaks = inten.compute_variant_intensity([rows4], 3, [1.0], [1.0], 0.01, 1,
                                            has_modloss=False, is_modified=False)
    check(peaks == [(_code(inten.CH_B, 0), inten.quantize_sqrt(0.5))],
          f"tie-break must keep the lowest code: {peaks}", failures)


def test_modloss_modes(failures):
    nAA = 3
    rows4 = [(0.5, 0.6, 0.7, 0.8), (0.4, 0.3, 0.2, 0.9)]
    b8, b4 = [0.9], [0.6]
    ml_codes = {_code(inten.CH_B_ML, p) for p in (0, 1)} | {_code(inten.CH_Y_ML, p) for p in (0, 1)}

    # phospho + modified: modloss channels present, base = base8 (0.9)
    p = inten.compute_variant_intensity([rows4], nAA, b8, b4, 0.01, 32, True, True)
    codes = {c for c, _ in p}
    check(ml_codes <= codes, "phospho+modified must include modloss peaks", failures)
    check(dict(p)[_code(inten.CH_Y_ML, 0)] == inten.quantize_sqrt(0.9 / 0.9),
          "phospho mode scales by base8", failures)
    check(dict(p)[_code(inten.CH_B, 0)] == inten.quantize_sqrt(0.5 / 0.9),
          "phospho mode: b1 rel = 0.5/0.9", failures)

    # phospho + unmodified: modloss skipped (cannot carry a neutral loss), base still base8
    p = inten.compute_variant_intensity([rows4], nAA, b8, b4, 0.01, 32, True, False)
    codes = {c for c, _ in p}
    check(not (ml_codes & codes), "unmodified variant must have no modloss peaks", failures)
    check(dict(p)[_code(inten.CH_B, 0)] == inten.quantize_sqrt(0.5 / 0.9),
          "phospho+unmodified still scales by base8", failures)

    # general (--ignore-modloss): no modloss regardless of is_modified, base = base4 (0.6)
    for is_mod in (True, False):
        p = inten.compute_variant_intensity([rows4], nAA, b8, b4, 0.01, 32, False, is_mod)
        codes = {c for c, _ in p}
        check(not (ml_codes & codes), f"general mode is_modified={is_mod}: no modloss",
              failures)
        check(dict(p)[_code(inten.CH_Y, 1)] == 255,
              f"general mode: y2 (0.6) / base4 (0.6) must be the base peak (q=255): {p}",
              failures)


def test_multicharge_max_and_zero_base(failures):
    nAA = 3
    z2 = [(0.2, 0.9, 0.0, 0.0), (0.5, 0.1, 0.0, 0.0)]
    z3 = [(0.8, 0.1, 0.0, 0.0), (0.1, 0.6, 0.0, 0.0)]
    p = inten.compute_variant_intensity([z2, z3], nAA, [1.0, 1.0], [1.0, 1.0], 0.01, 32,
                                        False, False)
    got = dict(p)
    check(got[_code(inten.CH_B, 0)] == inten.quantize_sqrt(0.8), "b1 = max(0.2, 0.8)", failures)
    check(got[_code(inten.CH_Y, 1)] == inten.quantize_sqrt(0.9), "y2 = max(0.9, 0.1)", failures)
    check(got[_code(inten.CH_B, 1)] == inten.quantize_sqrt(0.5), "b2 = max(0.5, 0.1)", failures)
    check(got[_code(inten.CH_Y, 0)] == inten.quantize_sqrt(0.6), "y1 = max(0.1, 0.6)", failures)
    # base peaks maxed across charges too
    p = inten.compute_variant_intensity([z2, z3], nAA, [1.0, 2.0], [1.0, 2.0], 0.01, 32,
                                        False, False)
    check(dict(p)[_code(inten.CH_B, 0)] == inten.quantize_sqrt(0.8 / 2.0),
          "base peak must be the cross-charge max", failures)
    # zero base -> empty
    p = inten.compute_variant_intensity([[(0.0, 0.0, 0.0, 0.0)]], 2, [0.0], [0.0], 0.01, 32,
                                        True, True)
    check(p == [], "zero base peak must yield an empty entry", failures)
    # row-count mismatch is loud
    try:
        inten.compute_variant_intensity([z2], 4, [1.0], [1.0], 0.01, 32, False, False)
        check(False, "nAA/rows mismatch must raise", failures)
    except ValueError:
        pass


def test_entry_pack_unpack(failures):
    key = (123456, -1, -1, -1)
    peaks = [(_code(0, 0), 255), (_code(1, 3), 7), (_code(3, 2), 1)]
    blob = inten.pack_entry(key, peaks)
    check(len(blob) == inten.KEY_SIZE + 1 + 3 * inten.PEAK_SIZE,
          f"entry size {len(blob)}", failures)
    k, p, nxt = inten.unpack_entry_at(blob, 0)
    check(k == key and p == peaks and nxt == len(blob), "pack/unpack round-trip", failures)
    # zero-peak entry is legal and 11 bytes
    blob0 = inten.pack_entry((7, 5, 0, -1), [])
    check(len(blob0) == 11, "empty entry is key + count only", failures)
    k, p, nxt = inten.unpack_entry_at(blob0, 0)
    check(k == (7, 5, 0, -1) and p == [] and nxt == 11, "empty entry round-trip", failures)
    # signed termini survive
    k, _, _ = inten.unpack_entry_at(inten.pack_entry((1, 2, -1, 3), []), 0)
    check(k == (1, 2, -1, 3), f"signed termini: {k}", failures)
    # iter_packed_entries walks a concatenated run
    run = blob + blob0
    keys = [k for k, _ in inten.iter_packed_entries(run)]
    check(keys == [key, (7, 5, 0, -1)], f"iter_packed_entries: {keys}", failures)
    # peak norm is a function of the stored bytes
    n = inten.peak_norm(peaks)
    want = (1.0 ** 2 + (7 / 255) ** 2 + (1 / 255) ** 2) ** 0.5
    check(abs(n - want) < 1e-12, f"peak_norm {n} != {want}", failures)
    try:
        inten.pack_entry(key, [(0, 1)] * 256)
        check(False, ">255 peaks must raise", failures)
    except ValueError:
        pass


def test_end_to_end_worker_file_and_verifier(failures):
    """Tiny real .cps + variant map through process_range(), the file writer, the reader
    and the verifier -- both modes. Store quant is u16 with base8 chosen so every value is
    exactly representable (lossless), so expected peaks can be computed from the floats."""
    qmax = 65535
    base8 = 6.5535   # q / qmax * base8 == q * 1e-4 exactly enough for float compare
    nAA = 5
    steps_rows = [
        # variant A (unmodified, one charge row): row 0
        [(60000, 30000, 0, 0), (100, 65535, 0, 0), (5000, 20, 0, 0), (10, 8000, 0, 0)],
        # variant B (modified, two charge rows): rows 1, 2
        [(200, 100, 300, 400), (600, 65535, 700, 0), (1, 2, 3, 4), (0, 0, 0, 0)],
        [(150, 250, 350, 450), (550, 650, 750, 65535), (5, 6, 7, 8), (9, 10, 11, 12)],
    ]
    with tempfile.TemporaryDirectory() as tmp:
        sp = os.path.join(tmp, "t.cps")
        w = cps.CpsWriter(sp, source_rows=0, source_head_crc="0" * 8, quant="u16",
                          mode="phospho")
        b8s, b4s = [], []
        for rows in steps_rows:
            b8 = max(s for r in rows for s in r) / qmax * base8
            b4 = max(s for r in rows for s in r[:2]) / qmax * base8   # b_z1/y_z1 only here
            b8s.append(b8)
            b4s.append(b4)
            w.append_row(nAA, b8, b4, rows)
        w.source_rows = 3
        w.finalize()

        vp = os.path.join(tmp, "v.tsv")
        # written in NON-key order on purpose: B (10,3,..) before A (10,-1,..)
        groups = [((10, 3, -1, -1), [1, 2]), ((10, -1, -1, -1), [0])]
        _make_vmap(vp, groups)
        data_start, vmc = cfm.find_data_start(vp)
        size = os.path.getsize(vp)

        reader = cps.CpsReader(sp)
        float_rows = [reader.read_row(i)[3] for i in range(3)]
        reader.close()

        for ignore_modloss in (False, True):
            has_modloss = not ignore_modloss
            _, blob, n, npk = inten.process_range(
                (0, vp, data_start, data_start, size, sp, 0.01, 32, has_modloss))
            check(n == 2, f"expected 2 entries, got {n}", failures)
            entries = list(inten.iter_packed_entries(blob))
            keys = [k for k, _ in entries]
            check(keys == [(10, -1, -1, -1), (10, 3, -1, -1)],
                  f"worker must sort its range by key: {keys}", failures)
            total_peaks = 0
            for key, rows in groups:
                want = inten.compute_variant_intensity(
                    [float_rows[ri] for ri in rows], nAA,
                    [b8s[ri] for ri in rows], [b4s[ri] for ri in rows], 0.01, 32,
                    has_modloss=has_modloss, is_modified=(key[1] != -1))
                total_peaks += len(want)
                got = dict(entries)[key]
                _, got_peaks, _ = inten.unpack_entry_at(got, 0)
                check(got_peaks == want,
                      f"ignore_modloss={ignore_modloss} key={key}: {got_peaks} != {want}",
                      failures)
                if not ignore_modloss and key[1] != -1:
                    check(any(inten.decode_peak_code(c)[0] >= 2 for c, _ in got_peaks),
                          "modified variant in phospho mode should carry modloss peaks",
                          failures)
            check(npk == total_peaks, f"worker peak count {npk} != {total_peaks}", failures)

            out = os.path.join(tmp, f"t{int(ignore_modloss)}.carafe_inten")
            hdr = inten.header_lines("0123abcd", 42, "x.idx", sp, vmc,
                                     general_mode=ignore_modloss, min_rel=0.01, max_peaks=32)
            inten.write_inten_file(out, hdr, n, inten.merge_sorted_runs([blob]))
            nv, npv = inten.verify_written_inten_sorted(out)
            check((nv, npv) == (2, total_peaks), f"verifier counts {(nv, npv)}", failures)
            header, read_entries = inten.read_inten_file(out)
            check(header["SourceIdxFingerprint"] == "0123abcd" and
                  header["SourceIdxNumRawPeptides"] == "42" and
                  header["VarModConfig"] == vmc and
                  header["Mode"] == ("general" if ignore_modloss else "phospho") and
                  header["Channels"] == ",".join(
                      inten.CHANNELS_GENERAL if ignore_modloss else inten.CHANNELS_PHOSPHO) and
                  header["Transform"] == "sqrt" and header["Quant"] == "u8",
                  f"header fields wrong: {header}", failures)
            check([k for k, _ in read_entries] == keys, "reader keys", failures)

        # verifier rejects an unsorted file: swap the two entries
        bad = os.path.join(tmp, "bad.carafe_inten")
        inten.write_inten_file(bad, hdr, 2, [e for _, e in reversed(entries)])
        try:
            inten.verify_written_inten_sorted(bad)
            check(False, "verifier must reject an unsorted file", failures)
        except ValueError:
            pass
        # ... and a header/entry count mismatch
        bad2 = os.path.join(tmp, "bad2.carafe_inten")
        inten.write_inten_file(bad2, hdr, 3, [e for _, e in entries])
        try:
            inten.verify_written_inten_sorted(bad2)
            check(False, "verifier must reject a count mismatch", failures)
        except ValueError:
            pass


def test_merge_restores_global_order(failures):
    def run(keys):
        return b"".join(inten.pack_entry(k, [(_code(0, 0), 9)] * (i % 3))
                        for i, k in enumerate(sorted(keys)))
    r1 = run([(10, -1, -1, -1), (40, 2, -1, -1), (40, 5, 0, -1)])
    r2 = run([(20, -1, -1, -1), (40, 3, -1, -1), (50, -1, -1, -1)])
    merged = [struct.unpack_from(inten.KEY_FMT, e, 0)
              for e in inten.merge_sorted_runs([r1, None, r2])]
    want = [(10, -1, -1, -1), (20, -1, -1, -1), (40, 2, -1, -1), (40, 3, -1, -1),
            (40, 5, 0, -1), (50, -1, -1, -1)]
    check(merged == want, f"merge order: {merged}", failures)


TESTS = [
    test_peak_code_roundtrip,
    test_quantize_sqrt,
    test_ladder_mapping_phospho_modified,
    test_threshold_maxpeaks_and_zero_q,
    test_modloss_modes,
    test_multicharge_max_and_zero_base,
    test_entry_pack_unpack,
    test_end_to_end_worker_file_and_verifier,
    test_merge_restores_global_order,
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
