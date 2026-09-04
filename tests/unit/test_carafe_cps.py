#!/usr/bin/env python3
"""
Correctness unit tests for tools/carafe_cps.py (the compact prediction store, M2 of
docs/20260822_carafe_prerun.md) -- pure in-process Python, no comet.exe/.idx dependency,
matching test_carafe_ms2_to_fi_mask.py's pattern.

Covers: writer/reader roundtrip (values, offsets, both quantizations), header/row-count
self-consistency rejection, source-CRC mispairing rejection, dequantization math, zero-
base-peak handling, and -- the load-bearing one -- compute_variant_mask_from_cps()
agreeing EXACTLY with carafe_ms2_to_fi_mask.compute_variant_mask() on the same values when
quantization is lossless (values chosen as exact multiples of base_peak/qmax), in all four
mode combinations (has_modloss x is_modified) and across multi-charge merging.

Does NOT substitute for the real-data rebuild-diff experiment (Section 5.4's quantization-
granularity decision) -- that runs against real chunk predictions and the 2,498 committed-
on-disk ground-truth chunk masks, and is reported in the plan doc, not here.
"""

import os
import struct
import sys
import tempfile
from pathlib import Path

TOOLS_DIR = Path(__file__).resolve().parent.parent.parent / "tools"
sys.path.insert(0, str(TOOLS_DIR))
import carafe_cps as cps  # noqa: E402
import carafe_ms2_to_fi_mask as fi_mask  # noqa: E402


def check(cond, msg, failures):
    if not cond:
        failures.append(msg)
    return cond


def _write_store(path, rows, quant="u8", src_crc="deadbeef", mode="phospho"):
    """rows: list of (nAA, base8, base4, quantized_rows)."""
    w = cps.CpsWriter(path, source_rows=0, source_head_crc=src_crc, quant=quant, mode=mode)
    for nAA, base8, base4, q in rows:
        w.append_row(nAA, base8, base4, q)
    w.source_rows = len(rows)
    w.finalize()


def test_roundtrip_u8_and_u16(failures):
    for quant, qmax in (("u8", 255), ("u16", 65535)):
        with tempfile.TemporaryDirectory() as tmp:
            p = os.path.join(tmp, "t.cps")
            rows = [
                (4, 2.0, 1.5, [(qmax, 0, qmax // 2, 1), (3, 4, 5, 6), (0, 0, 0, 0)]),
                (2, 0.0, 0.0, [(0, 0, 0, 0)]),
            ]
            _write_store(p, rows, quant=quant)
            r = cps.CpsReader(p)
            check(r.row_count == 2, f"{quant}: row_count 2 != {r.row_count}", failures)
            check(r.quant == quant, f"{quant}: header quant mismatch", failures)

            nAA, base8, base4, rows4 = r.read_row(0)
            check((nAA, base8, base4) == (4, 2.0, 1.5),
                  f"{quant}: row0 scalars {(nAA, base8, base4)}", failures)
            check(abs(rows4[0][0] - 2.0) < 1e-9,
                  f"{quant}: qmax should dequantize to base8 exactly, got {rows4[0][0]}",
                  failures)
            check(rows4[0][1] == 0.0, f"{quant}: q=0 must dequantize to 0.0", failures)
            expect = (qmax // 2) / qmax * 2.0
            check(abs(rows4[0][2] - expect) < 1e-9,
                  f"{quant}: mid-scale dequant {rows4[0][2]} != {expect}", failures)

            nAA, base8, base4, rows4 = r.read_row(1)
            check(base8 == 0.0 and rows4 == [(0.0, 0.0, 0.0, 0.0)],
                  f"{quant}: zero-base row must dequantize all-zero, got {rows4}", failures)
            r.close()


def test_v2_store_read_row_compat(failures):
    """A v2 (8-channel) store must serve read_row() exactly like a v1 store holding the same
    z1 channels, so the mask path is unaffected; read_row8() on a v1 store zero-fills z2."""
    with tempfile.TemporaryDirectory() as tmp:
        p1 = os.path.join(tmp, "v1.cps")
        p2 = os.path.join(tmp, "v2.cps")
        rows4 = [(100, 50, 25, 10), (7, 8, 9, 11)]
        rows8 = [(100, 60, 50, 40, 25, 30, 10, 20), (7, 1, 8, 2, 9, 3, 11, 4)]
        _write_store(p1, [(3, 2.0, 1.0, rows4)], quant="u8")
        w = cps.CpsWriter(p2, source_rows=0, source_head_crc="deadbeef", quant="u8", mode="phospho",
                          channels=8)
        w.append_row(3, 2.0, 1.0, rows8)
        w.source_rows = 1
        w.finalize()
        r1, r2 = cps.CpsReader(p1), cps.CpsReader(p2)
        check((r1.version, r1.channels, r2.version, r2.channels) == (1, 4, 2, 8),
              "version/channels detection", failures)
        check(r1.read_row(0) == r2.read_row(0), "read_row identical across v1/v2", failures)
        _, _, _, v1r8 = r1.read_row8(0)
        _, _, _, v2r8 = r2.read_row8(0)
        check(all(v1r8[i][j] == 0.0 for i in range(2) for j in (1, 3, 5, 7)),
              "v1 read_row8 zero-fills z2 columns", failures)
        check(abs(v2r8[0][1] - 60 / 255 * 2.0) < 1e-9 and abs(v2r8[1][7] - 4 / 255 * 2.0) < 1e-9,
              f"v2 read_row8 z2 values: {v2r8}", failures)
        r1.close(); r2.close()
        try:
            cps.pack_row(3, 1.0, 1.0, rows4, "B", 8)
            check(False, "pack_row must reject 4-value rows for an 8-channel store", failures)
        except ValueError:
            pass


def test_append_packed_identical_to_append_row(failures):
    """append_packed() (the worker-packed bulk path -- the fix for the 44GB parent-RSS
    Pool.imap buffering incident) must produce a byte-identical store to per-row
    append_row() over the same rows."""
    rows = [
        (4, 2.0, 1.5, [(255, 0, 127, 1), (3, 4, 5, 6), (0, 0, 0, 0)]),
        (3, 1.0, 1.0, [(9, 8, 7, 6), (5, 4, 3, 2)]),
        (2, 0.0, 0.0, [(0, 0, 0, 0)]),
    ]
    with tempfile.TemporaryDirectory() as tmp:
        p_row = os.path.join(tmp, "row.cps")
        p_packed = os.path.join(tmp, "packed.cps")
        _write_store(p_row, rows, quant="u8")

        w = cps.CpsWriter(p_packed, source_rows=0, source_head_crc="deadbeef",
                           quant="u8", mode="phospho")
        blobs, sizes = [], []
        for nAA, b8, b4, q in rows:
            packed = cps.pack_row(nAA, b8, b4, q, "B")
            blobs.append(packed)
            sizes.append(len(packed))
        w.append_packed(b"".join(blobs), sizes)
        w.source_rows = len(rows)
        w.finalize()

        check(Path(p_row).read_bytes() == Path(p_packed).read_bytes(),
              "append_packed store differs from append_row store byte-for-byte", failures)

        # And the size-sum guard must fire on a lying row_sizes list.
        w2 = cps.CpsWriter(os.path.join(tmp, "bad.cps"), source_rows=0,
                            source_head_crc="deadbeef", quant="u8", mode="phospho")
        try:
            w2.append_packed(b"".join(blobs), [s + 1 for s in sizes])
            check(False, "append_packed must reject mismatched row_sizes sum", failures)
        except ValueError:
            pass


def test_row_count_mismatch_refused(failures):
    with tempfile.TemporaryDirectory() as tmp:
        p = os.path.join(tmp, "t.cps")
        w = cps.CpsWriter(p, source_rows=5, source_head_crc="00000000",
                           quant="u8", mode="general")
        w.append_row(3, 1.0, 1.0, [(1, 2, 3, 4), (5, 6, 7, 8)])
        try:
            w.finalize()
            check(False, "finalize() must refuse header-row-count mismatch", failures)
        except ValueError:
            pass
        check(not os.path.exists(p), "no store file may exist after refused finalize",
              failures)


def test_truncated_store_detected(failures):
    with tempfile.TemporaryDirectory() as tmp:
        p = os.path.join(tmp, "t.cps")
        _write_store(p, [(3, 1.0, 1.0, [(1, 2, 3, 4), (5, 6, 7, 8)])])
        # Corrupt: flip the binary row_count to disagree with the header line.
        data = bytearray(Path(p).read_bytes())
        idx = data.index(b"\n\n") + 2
        data[idx:idx + 8] = struct.pack("<Q", 99)
        Path(p).write_bytes(bytes(data))
        try:
            cps.CpsReader(p)
            check(False, "reader must reject header/binary row-count disagreement", failures)
        except ValueError:
            pass


def test_verify_source_crc(failures):
    with tempfile.TemporaryDirectory() as tmp:
        src = os.path.join(tmp, "out.tsv")
        Path(src).write_bytes(b"sequence\tmods\tmod_sites\tcharge\r\nAAAK\t\t\t2\r\n")
        other = os.path.join(tmp, "other.tsv")
        Path(other).write_bytes(b"sequence\tmods\tmod_sites\tcharge\r\nPEPK\t\t\t2\r\n")
        p = os.path.join(tmp, "t.cps")
        _write_store(p, [(3, 1.0, 1.0, [(0, 0, 0, 0), (0, 0, 0, 0)])],
                     src_crc=cps.out_tsv_head_crc32(src))
        r = cps.CpsReader(p)
        r.verify_source(src, n_data_rows=1)   # must not raise
        try:
            r.verify_source(other)
            check(False, "verify_source must reject a different out_tsv", failures)
        except ValueError:
            pass
        try:
            r.verify_source(src, n_data_rows=2)
            check(False, "verify_source must reject a row-count mismatch", failures)
        except ValueError:
            pass
        r.close()


def test_from_cps_matches_tsv_path_when_lossless(failures):
    """Choose intensities as exact multiples of base8/qmax so u8 quantization roundtrips
    losslessly -- then compute_variant_mask_from_cps() must agree with the TSV-path
    compute_variant_mask() bit-for-bit on every output, across all mode combinations."""
    qmax = 255
    base8 = 2.55          # so quantization step = 0.01 exactly in float32-close terms
    nAA = 6
    # Per AlphaBase row r: (b_z1, b_z2, y_z1, y_z2, b_ml_z1, b_ml_z2, y_ml_z1, y_ml_z2).
    # The global (all-8) max must live in a MODLOSS channel so base4 != base8 -- putting it
    # in any of the first 4 channels (b_z1/b_z2/y_z1/y_z2) would make the two references
    # coincide and the ignore-modloss path wouldn't be exercised.
    steps = [
        (100, 0, 200, 130, 50, 0, 30, 0),
        (90, 110, 10, 0, 60, 255, 20, 0),   # b_ml_z2=255*step = base8 -- the all-8 max
        (80, 0, 20, 0, 70, 0, 10, 0),
        (5, 0, 250, 0, 0, 0, 0, 0),
        (1, 0, 2, 0, 3, 0, 4, 0),
    ]
    merged8 = [tuple(s / qmax * base8 for s in row) for row in steps]
    base4 = max(v for row in merged8 for v in row[:4])
    rows4 = [(row[0], row[2], row[4], row[6]) for row in merged8]
    q_rows = [tuple(row[i] for i in (0, 2, 4, 6)) for row in steps]

    for has_modloss in (True, False):
        for is_modified in (True, False):
            want = fi_mask.compute_variant_mask(
                merged8, nAA, 0.10, 3, has_modloss, is_modified)
            got = cps.compute_variant_mask_from_cps(
                [rows4], nAA, [base8], [base4], 0.10, 3, has_modloss, is_modified)
            check(want == got,
                  f"from_cps disagrees (has_modloss={has_modloss}, is_modified="
                  f"{is_modified}):\n  tsv={want}\n  cps={got}", failures)
    # Sanity that the fixture actually exercises differing references:
    check(base4 != base8, "fixture must have base4 != base8 to be a real test", failures)
    _ = q_rows  # documented derivation; quantization losslessness is implied by construction


def test_from_cps_multicharge_max(failures):
    """Cross-charge max must mirror max_across_charges(): per position, per channel."""
    nAA = 4
    c1 = [(1.0, 0.0, 0.5, 0.2), (0.1, 0.9, 0.0, 0.0), (0.3, 0.3, 0.3, 0.3)]
    c2 = [(0.5, 0.8, 0.6, 0.1), (0.2, 0.1, 0.7, 0.0), (0.3, 0.3, 0.2, 0.4)]
    got = cps.compute_variant_mask_from_cps(
        [c1, c2], nAA, [1.0, 0.9], [1.0, 0.8], 0.10, 1, True, True)
    merged_expect = [(1.0, 0.8, 0.6, 0.2), (0.2, 0.9, 0.7, 0.0), (0.3, 0.3, 0.3, 0.4)]
    merged8 = [(b, 0.0, y, 0.0, bm, 0.0, ym, 0.0) for b, y, bm, ym in merged_expect]
    want = fi_mask.compute_variant_mask(merged8, nAA, 0.10, 1, True, True)
    # base_peak differs (fi_mask derives from merged8 = 1.0; cps uses max(base8s) = 1.0) --
    # identical here by construction.
    check(want == got, f"multicharge merge mismatch:\n  tsv={want}\n  cps={got}", failures)


def _make_vmap(path, groups):
    """groups: list of (key4, [row_index,...]) written in order, one line per
    (row_index, key) pair -- same shape idx_to_carafe.py writes."""
    with open(path, "wb") as f:
        f.write(b"# VarModConfig: TESTCONFIG\r\n")
        f.write(b"row_index\tiWhichPeptide\tmodNumIdx\tcNtermMod\tcCtermMod\r\n")
        for key, rows in groups:
            for ri in rows:
                f.write(f"{ri}\t{key[0]}\t{key[1]}\t{key[2]}\t{key[3]}\r\n".encode("ascii"))


def test_vmap_range_split_covers_each_group_exactly_once(failures):
    """The parallel byte-range splitter (carafe_cps_to_fi_mask.iter_vmap_groups) must yield
    every tuple-group exactly once across ANY partition of the byte range -- including
    boundaries landing mid-line and mid-group (multi-row groups)."""
    import carafe_cps_to_fi_mask as cfm

    groups = [
        ((1, -1, -1, -1), [0]),
        ((1, 0, -1, -1), [1, 2]),      # multi-row group (two charge states)
        ((2, -1, -1, -1), [3]),
        ((2, 5, -1, -1), [4, 5]),
        ((3, -1, -1, -1), [6]),
        ((7, 1, 0, -1), [7]),
    ]
    with tempfile.TemporaryDirectory() as tmp:
        vp = os.path.join(tmp, "v.tsv")
        _make_vmap(vp, groups)
        data_start, vmc = cfm.find_data_start(vp)
        check(vmc == "TESTCONFIG", f"VarModConfig parse: {vmc!r}", failures)
        size = os.path.getsize(vp)

        # Single range must reproduce the groups exactly.
        got = list(cfm.iter_vmap_groups(vp, data_start, data_start, size))
        check(got == groups, f"single-range groups mismatch:\n{got}\nvs\n{groups}", failures)

        # Every possible 2-way split point (byte granularity!) must still yield each group
        # exactly once, in order, across the two ranges combined.
        for split in range(data_start + 1, size):
            a = list(cfm.iter_vmap_groups(vp, data_start, data_start, split))
            b = list(cfm.iter_vmap_groups(vp, data_start, split, size))
            combined = a + b
            if combined != groups:
                check(False, f"split at byte {split}: groups wrong\n  a={a}\n  b={b}",
                      failures)
                break

        # And an arbitrary 3-way split.
        s1 = data_start + (size - data_start) // 3
        s2 = data_start + 2 * (size - data_start) // 3
        combined = (list(cfm.iter_vmap_groups(vp, data_start, data_start, s1))
                    + list(cfm.iter_vmap_groups(vp, data_start, s1, s2))
                    + list(cfm.iter_vmap_groups(vp, data_start, s2, size)))
        check(combined == groups, f"3-way split groups wrong: {combined}", failures)


def test_cps_to_fi_mask_end_to_end_matches_tsv_builder(failures):
    """Full pipeline equivalence on synthetic lossless data: a tiny store + variant map,
    run through carafe_cps_to_fi_mask's worker path, must produce entries identical to
    computing each variant with the TSV path's compute_variant_mask() -- in both modes."""
    import carafe_cps_to_fi_mask as cfm

    qmax = 65535
    base8 = 6.5535
    # Two variants: one unmodified single-charge (rows 0), one modified two-charge (rows 1,2)
    nAA = 5
    steps_rows = [
        [(100, 0, 900, 0, 0, 0, 0, 0), (500, 0, 400, 0, 0, 0, 0, 0),
         (50, 0, 60, 0, 0, 0, 0, 0), (10, 0, 20, 0, 0, 0, 0, 0)],
        [(200, 0, 100, 0, 300, 0, 400, 0), (600, 65535, 700, 0, 800, 0, 900, 0),
         (1, 0, 2, 0, 3, 0, 4, 0), (0, 0, 0, 0, 0, 0, 0, 0)],
        [(150, 0, 250, 0, 350, 0, 450, 0), (550, 0, 650, 0, 750, 0, 850, 0),
         (5, 0, 6, 0, 7, 0, 8, 0), (9, 0, 10, 0, 11, 0, 12, 0)],
    ]
    float_rows = [[tuple(s / qmax * base8 for s in row) for row in rows]
                  for rows in steps_rows]

    with tempfile.TemporaryDirectory() as tmp:
        sp = os.path.join(tmp, "t.cps")
        w = cps.CpsWriter(sp, source_rows=0, source_head_crc="0" * 8,
                           quant="u16", mode="phospho")
        for rows in steps_rows:
            b8 = max(s for r in rows for s in r) / qmax * base8
            b4 = max(s for r in rows for s in r[:4]) / qmax * base8
            w.append_row(nAA, b8, b4, [tuple(r[i] for i in (0, 2, 4, 6)) for r in rows])
        w.source_rows = 3
        w.finalize()

        vp = os.path.join(tmp, "v.tsv")
        groups = [((10, -1, -1, -1), [0]), ((10, 3, -1, -1), [1, 2])]
        _make_vmap(vp, groups)
        data_start, _ = cfm.find_data_start(vp)
        size = os.path.getsize(vp)

        for ignore_modloss in (False, True):
            has_modloss = not ignore_modloss
            _, blob, n = cfm.process_range(
                (0, vp, data_start, data_start, size, sp, 0.10, 2, has_modloss))
            check(n == 2, f"expected 2 entries, got {n}", failures)
            got_entries = list(struct.iter_unpack(fi_mask.ENTRY_FMT, blob))

            for (key, rows), got in zip(groups, got_entries):
                merged8 = float_rows[rows[0]]
                if len(rows) > 1:
                    merged8 = [tuple(max(vals) for vals in zip(*rs))
                               for rs in zip(*(float_rows[ri] for ri in rows))]
                if ignore_modloss:
                    merged8_for_tsv = merged8
                    want = fi_mask.compute_variant_mask(
                        [r[:4] + (0.0,) * 4 for r in merged8_for_tsv], nAA, 0.10, 2,
                        has_modloss=False, is_modified=(key[1] != -1))
                else:
                    want = fi_mask.compute_variant_mask(
                        merged8, nAA, 0.10, 2, has_modloss=True,
                        is_modified=(key[1] != -1))
                check(got[:4] == key and got[4:] == want[:4],
                      f"ignore_modloss={ignore_modloss} key={key}: entry "
                      f"{got} != want key+{want[:4]}", failures)


def test_worker_sorts_and_merge_restores_global_order(failures):
    """The real 124.8M-row variant map is NOT globally key-ordered (empirically: mod-variant
    enumeration doesn't nest inside peptide order), so workers must sort their own ranges
    and merge_sorted_runs() must restore global order across ranges whose key spans
    OVERLAP. Fixture: two ranges with interleaved keys, out of order within each range."""
    import carafe_cps_to_fi_mask as cfm

    nAA = 3
    with tempfile.TemporaryDirectory() as tmp:
        sp = os.path.join(tmp, "t.cps")
        w = cps.CpsWriter(sp, source_rows=0, source_head_crc="0" * 8,
                           quant="u16", mode="phospho")
        for _ in range(4):
            w.append_row(nAA, 1.0, 1.0, [(65535, 65535, 0, 0), (65535, 65535, 0, 0)])
        w.source_rows = 4
        w.finalize()

        # Range A gets keys (50,...) then (10,...); range B gets (40,...) then (20,...) --
        # each internally unsorted, spans overlapping.
        vp_a = os.path.join(tmp, "a.tsv")
        _make_vmap(vp_a, [((50, -1, -1, -1), [0]), ((10, -1, -1, -1), [1])])
        vp_b = os.path.join(tmp, "b.tsv")
        _make_vmap(vp_b, [((40, 2, -1, -1), [2]), ((20, -1, -1, -1), [3])])

        blobs = []
        for vp in (vp_a, vp_b):
            ds, _ = cfm.find_data_start(vp)
            _, blob, n = cfm.process_range(
                (0, vp, ds, ds, os.path.getsize(vp), sp, 0.10, 1, True))
            check(n == 2, f"{vp}: expected 2 entries, got {n}", failures)
            keys = [k for k, _ in cfm.iter_packed_entries(blob)]
            check(keys == sorted(keys), f"worker output not sorted: {keys}", failures)
            blobs.append(blob)

        merged_keys = [struct.unpack_from("<Iibb", e)[0]
                       for e in cfm.merge_sorted_runs(blobs)]
        check(merged_keys == [10, 20, 40, 50],
              f"merged order wrong: {merged_keys}", failures)


TESTS = [
    test_roundtrip_u8_and_u16,
    test_v2_store_read_row_compat,
    test_append_packed_identical_to_append_row,
    test_row_count_mismatch_refused,
    test_truncated_store_detected,
    test_verify_source_crc,
    test_from_cps_matches_tsv_path_when_lossless,
    test_from_cps_multicharge_max,
    test_vmap_range_split_covers_each_group_exactly_once,
    test_cps_to_fi_mask_end_to_end_matches_tsv_builder,
    test_worker_sorts_and_merge_restores_global_order,
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
