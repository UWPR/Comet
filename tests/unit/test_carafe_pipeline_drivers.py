#!/usr/bin/env python3
"""
Correctness unit tests for the Python ports of the Carafe pipeline drivers
(tools/carafe_chunk_common.py, carafe_prerun.py, and the tools/carafe.py umbrella
CLI) -- pure in-process Python, no comet.exe/.idx/Carafe-venv dependency, matching
test_carafe_ms2_to_fi_mask.py's pattern.

The drivers were originally bash+awk (WSL/Linux-only); these tests pin the pieces of
the port where the bash/awk semantics were subtle enough to get wrong silently:

- split_tsv_with_header(): the `head -1` + `tail -n +2 | split -l N` + reassemble
  sequence -- header prepended to every chunk, exact row boundaries, chunk_%05d
  naming, byte fidelity (CRLF data must survive un-translated), empty-body input.
- split_variant_map(): the split_variant_map_for_chunks.awk port -- GLOBAL row_index
  rewritten chunk-local (subtract chunk_index * chunk_size), comment+header lines
  reproduced at the top of every chunk, chunks with no variant rows producing NO file,
  ordering reliance.
- carafe_prerun helpers: the "Wrote N rows" log parse, the VarModConfig neutral-loss
  detection regex (drives --ignore-modloss auto-detection -- a false negative would
  silently build the wrong kind of mask), params_with()'s whole-line rewrite
  including its must-already-have-the-key failure mode and CRLF preservation,
  get_param()'s threshold-key extraction, and apply_params_shorthand()'s --params
  single-flavor resolution precedence (CLI flag > params key > default; formerly the
  standalone params_to_fi_mask.sh).
- tools/carafe.py: every subcommand maps to an existing module exposing main(argv).

Not covered here (needs comet.exe + the Carafe venv): the real end-to-end pipeline --
that is exercised by the full-scale runs documented in docs/20260826_carafe.md.
"""

import argparse
import os
import sys
import tempfile
from pathlib import Path

TOOLS_DIR = Path(__file__).resolve().parent.parent.parent / "tools"
sys.path.insert(0, str(TOOLS_DIR))
import carafe  # noqa: E402
import carafe_chunk_common as common  # noqa: E402
import carafe_prerun  # noqa: E402


def check(cond, msg, failures):
    if not cond:
        failures.append(msg)
    return cond


def _write_bytes(path, data):
    with open(path, "wb") as f:
        f.write(data)


def _read_bytes(path):
    with open(path, "rb") as f:
        return f.read()


# ---------------------------------------------------------------------------
# split_tsv_with_header
# ---------------------------------------------------------------------------

def test_split_tsv_boundaries_and_header(failures):
    with tempfile.TemporaryDirectory() as tmp:
        src = os.path.join(tmp, "in.tsv")
        header = b"sequence\tmods\tmod_sites\tcharge\n"
        rows = [f"PEP{i}\t\t\t2\n".encode() for i in range(7)]
        _write_bytes(src, header + b"".join(rows))

        chunk_dir = os.path.join(tmp, "chunks")
        n = common.split_tsv_with_header(src, chunk_dir, 3)
        check(n == 3, f"7 rows @ chunk 3 -> expected 3 chunks, got {n}", failures)

        names = sorted(os.listdir(chunk_dir))
        check(names == ["chunk_00000.tsv", "chunk_00001.tsv", "chunk_00002.tsv"],
              f"chunk naming wrong: {names}", failures)

        check(_read_bytes(os.path.join(chunk_dir, "chunk_00000.tsv"))
              == header + b"".join(rows[0:3]),
              "chunk 0 content wrong", failures)
        check(_read_bytes(os.path.join(chunk_dir, "chunk_00001.tsv"))
              == header + b"".join(rows[3:6]),
              "chunk 1 content wrong", failures)
        check(_read_bytes(os.path.join(chunk_dir, "chunk_00002.tsv"))
              == header + b"".join(rows[6:7]),
              "last (short) chunk content wrong", failures)


def test_split_tsv_exact_multiple_and_small(failures):
    with tempfile.TemporaryDirectory() as tmp:
        src = os.path.join(tmp, "in.tsv")
        header = b"h1\th2\n"
        rows = [f"a{i}\tb{i}\n".encode() for i in range(6)]
        _write_bytes(src, header + b"".join(rows))

        n = common.split_tsv_with_header(src, os.path.join(tmp, "c1"), 3)
        check(n == 2, f"6 rows @ 3 -> expected exactly 2 chunks, got {n}", failures)

        n = common.split_tsv_with_header(src, os.path.join(tmp, "c2"), 100)
        check(n == 1, f"6 rows @ 100 -> expected 1 chunk, got {n}", failures)
        check(_read_bytes(os.path.join(tmp, "c2", "chunk_00000.tsv"))
              == header + b"".join(rows),
              "single-chunk split must reproduce the whole file", failures)


def test_split_tsv_empty_body(failures):
    with tempfile.TemporaryDirectory() as tmp:
        src = os.path.join(tmp, "in.tsv")
        _write_bytes(src, b"only\theader\n")
        chunk_dir = os.path.join(tmp, "chunks")
        n = common.split_tsv_with_header(src, chunk_dir, 3)
        check(n == 0, f"header-only input -> expected 0 chunks, got {n}", failures)
        check(common.list_chunk_tsvs(chunk_dir) == [],
              "header-only input must produce no chunk files", failures)


def test_split_tsv_preserves_crlf_bytes(failures):
    # The splitter is binary I/O so platform newline translation can never corrupt
    # data -- a CRLF input must come back out CRLF byte-for-byte.
    with tempfile.TemporaryDirectory() as tmp:
        src = os.path.join(tmp, "in.tsv")
        header = b"h1\th2\r\n"
        rows = [b"a\t1\r\n", b"b\t2\r\n", b"c\t3\r\n"]
        _write_bytes(src, header + b"".join(rows))
        chunk_dir = os.path.join(tmp, "chunks")
        n = common.split_tsv_with_header(src, chunk_dir, 2)
        check(n == 2, f"expected 2 chunks, got {n}", failures)
        check(_read_bytes(os.path.join(chunk_dir, "chunk_00000.tsv"))
              == header + rows[0] + rows[1],
              "CRLF chunk 0 not byte-identical", failures)
        check(_read_bytes(os.path.join(chunk_dir, "chunk_00001.tsv"))
              == header + rows[2],
              "CRLF chunk 1 not byte-identical", failures)


def test_count_data_rows(failures):
    with tempfile.TemporaryDirectory() as tmp:
        p = os.path.join(tmp, "t.tsv")
        _write_bytes(p, b"h\n" + b"r\n" * 5)
        check(common.count_data_rows(p) == 5, "count_data_rows wrong", failures)
        _write_bytes(p, b"h\n")
        check(common.count_data_rows(p) == 0, "header-only should count 0", failures)


# ---------------------------------------------------------------------------
# split_variant_map (the awk port)
# ---------------------------------------------------------------------------

VMC = "# VarModConfig: 79.966331STY--97.976896|0.0X--0.0"
VMAP_HEADER = "row_index\tiWhichPeptide\tmodNumIdx\tcNtermMod\tcCtermMod"


def _make_vmap(path, row_indices):
    lines = [VMC, VMAP_HEADER]
    for ri in row_indices:
        lines.append(f"{ri}\t{ri * 10}\t{ri % 3}\t-\t-")
    with open(path, "w", newline="\n") as f:
        f.write("\n".join(lines) + "\n")


def test_vmap_split_rewrites_row_index(failures):
    with tempfile.TemporaryDirectory() as tmp:
        vmap = os.path.join(tmp, "v.tsv")
        # rows spanning chunks 0, 1 and 3 (chunk 2 deliberately empty)
        _make_vmap(vmap, [0, 1, 2, 3, 4, 5, 9, 10, 11])
        out = os.path.join(tmp, "chunks")
        n_rows, n_chunks = common.split_variant_map(vmap, out, 3, log=None)
        check(n_rows == 9, f"expected 9 data rows, got {n_rows}", failures)
        check(n_chunks == 4, f"highest chunk 3 -> n_chunks 4, got {n_chunks}", failures)

        names = sorted(os.listdir(out))
        check(names == ["chunk_00000.tsv", "chunk_00001.tsv", "chunk_00003.tsv"],
              f"empty chunk 2 must produce no file: {names}", failures)

        with open(os.path.join(out, "chunk_00001.tsv")) as f:
            lines = f.read().splitlines()
        check(lines[0] == VMC, "chunk must start with the VarModConfig comment",
              failures)
        check(lines[1] == VMAP_HEADER, "chunk line 2 must be the header", failures)
        # global row_index 3,4,5 -> local 0,1,2; other columns untouched
        got = [(l.split("\t")[0], l.split("\t")[1]) for l in lines[2:]]
        check(got == [("0", "30"), ("1", "40"), ("2", "50")],
              f"chunk 1 row_index rewrite wrong: {got}", failures)

        with open(os.path.join(out, "chunk_00003.tsv")) as f:
            lines = f.read().splitlines()
        got = [l.split("\t")[0] for l in lines[2:]]
        check(got == ["0", "1", "2"],
              f"chunk 3 (after a gap) row_index rewrite wrong: {got}", failures)


def test_vmap_split_empty_map(failures):
    with tempfile.TemporaryDirectory() as tmp:
        vmap = os.path.join(tmp, "v.tsv")
        _make_vmap(vmap, [])
        out = os.path.join(tmp, "chunks")
        n_rows, n_chunks = common.split_variant_map(vmap, out, 3, log=None)
        check((n_rows, n_chunks) == (0, 0),
              f"empty map -> (0, 0), got {(n_rows, n_chunks)}", failures)
        check(common.list_chunk_tsvs(out) == [],
              "empty map must produce no chunk files", failures)


def test_vmap_split_boundary_rows(failures):
    # row_index exactly at a chunk boundary must open the NEXT chunk with local
    # index 0 (integer-division boundary, the awk `int(row_index / CHUNK_SIZE)`).
    with tempfile.TemporaryDirectory() as tmp:
        vmap = os.path.join(tmp, "v.tsv")
        _make_vmap(vmap, [2, 3])
        out = os.path.join(tmp, "chunks")
        common.split_variant_map(vmap, out, 3, log=None)
        with open(os.path.join(out, "chunk_00000.tsv")) as f:
            c0 = [l.split("\t")[0] for l in f.read().splitlines()[2:]]
        with open(os.path.join(out, "chunk_00001.tsv")) as f:
            c1 = [l.split("\t")[0] for l in f.read().splitlines()[2:]]
        check(c0 == ["2"], f"row 2 belongs to chunk 0 as local 2: {c0}", failures)
        check(c1 == ["0"], f"row 3 belongs to chunk 1 as local 0: {c1}", failures)


# ---------------------------------------------------------------------------
# carafe_prerun helpers
# ---------------------------------------------------------------------------

def test_parse_rows_written(failures):
    check(carafe_prerun.parse_rows_written("blah\nWrote 12345 rows to x.tsv\n") == 12345,
          "parse_rows_written should find the count", failures)
    check(carafe_prerun.parse_rows_written("Wrote 1 rows\nWrote 2 rows\n") == 1,
          "parse_rows_written must take the FIRST match (bash used head -1)", failures)
    check(carafe_prerun.parse_rows_written("no rows here") is None,
          "parse_rows_written must return None when absent", failures)


def test_varmodconfig_has_nl(failures):
    # withNL: phospho with the -98 neutral loss
    check(carafe_prerun.varmodconfig_has_nl(
              "# VarModConfig: 79.966331STY--97.976896"),
          "97.976896 delta must be detected as NL", failures)
    # noNL flavors: all deltas zero, in the zero spellings Comet actually emits
    for zeros in ("79.966331STY--0.0", "79.966331STY--0", "79.966331STY--0.000000",
                  "15.994915M--0.0|79.966331STY--0.0"):
        check(not carafe_prerun.varmodconfig_has_nl(f"# VarModConfig: {zeros}"),
              f"all-zero deltas must NOT be detected as NL: {zeros}", failures)
    # mixed: any one nonzero delta makes the flavor withNL
    check(carafe_prerun.varmodconfig_has_nl(
              "# VarModConfig: 15.994915M--0.0|79.966331STY--97.976896"),
          "mixed zero/nonzero deltas must be detected as NL", failures)
    # sub-1.0 delta (zero integer part, nonzero fraction)
    check(carafe_prerun.varmodconfig_has_nl("# VarModConfig: 1.0S--0.5"),
          "0.5 delta must be detected as NL", failures)
    # the mod MASS being nonzero must not trip NL detection on its own
    check(not carafe_prerun.varmodconfig_has_nl("# VarModConfig: 79.966331STY--0.0"),
          "nonzero mod mass with zero delta must not read as NL", failures)


def test_params_with(failures):
    with tempfile.TemporaryDirectory() as tmp:
        src = os.path.join(tmp, "src.params")
        dst = os.path.join(tmp, "dst.params")
        # CRLF source, exactly like a real comet.params in this repo
        _write_bytes(src, b"# comment\r\ndatabase_name = old.fasta\r\nnum_threads = 0\r\n")
        carafe_prerun.params_with(src, dst, "database_name", "new.fasta")
        got = _read_bytes(dst)
        check(got == b"# comment\r\ndatabase_name = new.fasta\r\nnum_threads = 0\r\n",
              f"params_with rewrite/CRLF preservation wrong: {got!r}", failures)

        # missing key must fail loudly, not silently write an unchanged copy
        try:
            carafe_prerun.params_with(src, dst, "no_such_key", "x")
            check(False, "params_with must raise when the key is absent", failures)
        except ValueError:
            pass

        # key must match as a whole word at line start ('database_name2 =' is not
        # 'database_name =')
        _write_bytes(src, b"database_name2 = other\r\ndatabase_name = old\r\n")
        carafe_prerun.params_with(src, dst, "database_name", "new")
        check(_read_bytes(dst) == b"database_name2 = other\r\ndatabase_name = new\r\n",
              "params_with must not rewrite a longer key sharing the prefix", failures)


# ---------------------------------------------------------------------------
# carafe_prerun.get_param / apply_params_shorthand (the --params single-flavor mode)
# ---------------------------------------------------------------------------

def test_get_param(failures):
    with tempfile.TemporaryDirectory() as tmp:
        p = os.path.join(tmp, "c.params")
        _write_bytes(p,
                     b"# header comment\r\n"
                     b"carafe_mask_min_relative_intensity = 0.15   # trailing comment\r\n"
                     b"carafe_mask_min_peaks=7\r\n"
                     b"other_key = x\r\n")
        check(carafe_prerun.get_param(p, "carafe_mask_min_relative_intensity")
              == "0.15",
              "value must stop before whitespace/comment", failures)
        check(carafe_prerun.get_param(p, "carafe_mask_min_peaks") == "7",
              "no-spaces 'key=value' form must parse", failures)
        check(carafe_prerun.get_param(p, "absent_key") is None,
              "absent key must return None", failures)


def _shorthand_args(**kw):
    a = argparse.Namespace(params="", flavors=None,
                           min_relative_intensity=None, min_kept_peaks=None)
    vars(a).update(kw)
    return a


def test_apply_params_shorthand(failures):
    with tempfile.TemporaryDirectory() as tmp:
        p = os.path.join(tmp, "c.params")
        _write_bytes(p,
                     b"carafe_mask_min_relative_intensity = 0.15\r\n"
                     b"carafe_mask_min_peaks = 7\r\n")

        # params keys fill unset thresholds; flavor becomes primary=<file>
        a = _shorthand_args(params=p)
        carafe_prerun.apply_params_shorthand(a)
        check(a.flavors == [("primary", p)],
              f"--params must become the primary flavor: {a.flavors}", failures)
        check((a.min_relative_intensity, a.min_kept_peaks) == (0.15, 7),
              f"params keys must fill thresholds: "
              f"{(a.min_relative_intensity, a.min_kept_peaks)}", failures)

        # an explicit CLI value must win over the params key
        a = _shorthand_args(params=p, min_relative_intensity=0.25)
        carafe_prerun.apply_params_shorthand(a)
        check((a.min_relative_intensity, a.min_kept_peaks) == (0.25, 7),
              "explicit CLI threshold must beat the params key", failures)

        # a params file without the keys falls through to the 0.10/6 defaults
        p2 = os.path.join(tmp, "bare.params")
        _write_bytes(p2, b"database_name = x\r\n")
        a = _shorthand_args(params=p2)
        carafe_prerun.apply_params_shorthand(a)
        check((a.min_relative_intensity, a.min_kept_peaks) == (0.10, 6),
              "missing params keys must fall back to 0.10/6", failures)

        # --flavor path untouched: defaults resolve, flavors preserved
        a = _shorthand_args(flavors=[("withnl", "w.params")])
        carafe_prerun.apply_params_shorthand(a)
        check(a.flavors == [("withnl", "w.params")]
              and (a.min_relative_intensity, a.min_kept_peaks) == (0.10, 6),
              "--flavor path must keep flavors and resolve defaults", failures)

        # --params + --flavor is an error; so is neither
        for bad in (_shorthand_args(params=p, flavors=[("x", "y")]),
                    _shorthand_args()):
            try:
                carafe_prerun.apply_params_shorthand(bad)
                check(False, "conflicting/absent flavor config must sys.exit",
                      failures)
            except SystemExit:
                pass


# ---------------------------------------------------------------------------
# tools/carafe.py umbrella CLI
# ---------------------------------------------------------------------------

def test_carafe_dispatch_table(failures):
    # Every subcommand must point at a module file that exists next to carafe.py and
    # (for the stdlib-only ones we can afford to import here) expose main(argv).
    # Import is deliberately skipped for the numpy-dependent stages -- the table
    # entries are checked on disk instead, and their main(argv) signatures are pinned
    # by grep-level convention, not import.
    import importlib
    import inspect
    stdlib_only = {"carafe_prerun", "run_carafe_chunked", "build_carafe_mask_chunked",
                   "carafe_ms2_to_fi_mask", "merge_carafe_fi_masks", "idx_to_carafe"}
    for cmd, (mod_name, desc) in carafe.COMMANDS.items():
        check((TOOLS_DIR / f"{mod_name}.py").is_file(),
              f"carafe.py maps {cmd!r} to missing module {mod_name}.py", failures)
        check(bool(desc), f"{cmd!r} has no description", failures)
        if mod_name in stdlib_only:
            mod = importlib.import_module(mod_name)
            main = getattr(mod, "main", None)
            ok = main is not None and "argv" in inspect.signature(main).parameters
            check(ok, f"{mod_name}.main(argv) missing or wrong signature", failures)


TESTS = [
    test_split_tsv_boundaries_and_header,
    test_split_tsv_exact_multiple_and_small,
    test_split_tsv_empty_body,
    test_split_tsv_preserves_crlf_bytes,
    test_count_data_rows,
    test_vmap_split_rewrites_row_index,
    test_vmap_split_empty_map,
    test_vmap_split_boundary_rows,
    test_parse_rows_written,
    test_varmodconfig_has_nl,
    test_params_with,
    test_get_param,
    test_apply_params_shorthand,
    test_carafe_dispatch_table,
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
