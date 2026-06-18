# Comet Test Suite Summary

Three independent test suites live under `tests/`, each with its own runner and
purpose:

| Subdirectory | Purpose | Runner(s) |
|---|---|---|
| `unit/` | Index-building correctness, byte-level format checks, regression tests for specific fixed bugs | `run_tests.py`, `test_il_sequence.py` |
| `regression/` | Compare the current build against a tagged release binary on real MS data (timing, PSM counts, PSM agreement); also verifies Windows `.raw` file support | `setup_baselines.py`, `run_regression.py`, `test_raw_vs_mzxml.py` |
| `perf/` | Wall-clock time and peak memory benchmarks across search modes | `run_perf.py` |

See `CLAUDE.md` for the canonical invocation examples. This document summarizes
what each individual test actually checks.

---

## `tests/unit/`

```bash
python tests/unit/run_tests.py --comet /mnt/c/Work/Comet-master/comet.exe
python tests/unit/run_tests.py --comet /mnt/c/Work/Comet-master/comet.exe t13
python tests/unit/run_tests.py --comet /mnt/c/Work/Comet-master/comet.exe --integration
```

All tests build a plain-peptide `.idx` via `Comet.exe -i` against a small
crafted FASTA in `tests/unit/data/`, then parse the resulting `.idx` directly
(`parse_idx()`) and check specific peptides, masses, flanking residues, or
protein-list contents -- except T13 (pure Python, no Comet invocation) and
T17/T18 (integration, require `data/human.small.fasta`). Always pass `--comet`
as a full path; the default `../../comet.exe` only resolves correctly when
invoked from inside `tests/unit/`.

### `run_tests.py` -- T1-T20

| ID | Summary |
|---|---|
| **T1** | Basic peptide generation: single short protein `ACDEFGHIKL`, no-enzyme, length 8-10 -- verifies the exact set of sliding-window substrings and that each maps to exactly 1 protein. |
| **T2** | Within-protein deduplication: `AAAKAAAKAAAK`, length 8 -- a repeated 8-mer inside one protein must collapse to a single index entry. |
| **T3** | Cross-protein deduplication: two different proteins sharing the identical sequence `ACDEFGHIKL` -- each unique peptide must map to both proteins. |
| **T4a** (`t4_il_true`) | `equal_I_and_L=1` merges `PEPTIRDE`/`PEPTLRDE` into one canonical (I-form) entry mapping to 2 proteins. |
| **T4b** (`t4_il_false`) | `equal_I_and_L=0` keeps `PEPTIRDE`/`PEPTLRDE` as two distinct single-protein entries. |
| **T5a** (`t5_noenz`) | No-enzyme digestion of `MAKRPEPTIDEKGASTMVR` -- every length-8 substring must appear. |
| **T5b** (`t5_trypsin_0mc`) | Trypsin, 0 missed cleavages -- only `RPEPTIDEK` (>= 8 AA) survives; shorter tryptic fragments are correctly excluded. |
| **T5c** (`t5_trypsin_1mc`) | Trypsin, 1 missed cleavage -- `RPEPTIDEK` plus the two one-missed-cleavage neighbors. |
| **T6** | Flanking-residue (`cPrevAA`/`cNextAA`) correctness for every length-8 window of a 14-AA protein, including `-` at the protein termini. |
| **T7** | Mass accuracy: `PEPTIDE` embedded in a longer sequence, no static mods -- computed MH+ mass must be within 0.002 Da of the known monoisotopic value (800.36722). |
| **T11** | Edge case: a 4-AA protein, too short for the configured minimum peptide length -- must produce an empty index or a graceful "no peptides" error, never a crash. |
| **T12** | Edge case: an 8-AA protein exactly at the minimum peptide length -- exactly 1 peptide, with `-` flanking on both sides. |
| **T13** | Pure-Python round-trip test of the 5-bit peptide packing scheme (`PackPeptide`/`UnpackPeptide` in `CometDataInternal.h`), reimplemented in Python: round-trips for all 20 AAs and lengths 8-12; I/L collapse correctly when `bIL=True` and stay distinct when `bIL=False`; integer sort order matches lexicographic order; one known fixed-value encoding check. No Comet binary is invoked. |
| **T14** | Boundary between the short (<=12, packed) and long (>=13, plain-string) index paths: a 14-AA protein, length range 12-13 -- verifies the exact expected peptide sets on both sides of the boundary. |
| **T15a** (`t15_il_short`) | I/L canonicalization on the short (packed, <=12 AA) path: length-8 peptides differing only by I-vs-L merge under `equal_IL=1` and stay separate under `equal_IL=0`. |
| **T15b** (`t15_il_long`) | Same as T15a but on the long (plain-string, >=13 AA) path, length 13. |
| **T16** | Cross-path protein-list correctness: two identical 13-AA proteins, length range 8-13, so generated peptides span both the short and long index paths -- every peptide (either path) must map to both proteins. |
| **T17** *(integration)* | Builds `data/human.small.fasta` with no-enzyme, length 8-13, `equal_IL=1` and checks the peptide count falls in `[8,800,000, 9,100,000]` and that peptide count equals protein-list count. Cross-version byte-exact comparison against the v2026.01.1 baseline is unreliable (known I/L long-path dedup differences), so this test uses a count-stability range instead -- see the comment block above the test in `run_tests.py` for the full rationale. Requires `--integration` and `data/human.small.fasta` (not in the repo). |
| **T18** *(integration)* | Determinism: two independent builds of `data/human.small.fasta` (same no-enzyme len 8-13 config) must produce byte-identical `.idx` files. Requires `--integration` and `data/human.small.fasta`. |
| **T19** | Regression test for the AScorePro + FI_DB mod-ordering bug (`docs/20260617_codereview3.md` issue 2a): builds an FI_DB index with a real phospho-S variable mod baked into the `.idx` header, then searches with `print_ascorepro_score=1` and a deliberately *blank* `variable_mod01` in the search-time params (the normal real-world case for FI_DB searches). Asserts the single expected PSM (`ACDEFGSK`, phospho at position 7) is found and `ascorepro > 0` -- i.e. AScore actually used the index's mod instead of being silently skipped because of the blank search-time mod. |
| **T20** | Regression test for the PI_DB batch-search crash (`_pQueries` never assigned in `CometSearch::SearchPeptideIndex(ThreadPool*, vector<Query*>&)`, which segfaulted inside `BinarySearchMass()` on the first scored candidate). Reuses T19's phospho fixture but builds a PI_DB (`-j`, peptide index) instead of an FI_DB (`-i`, fragment index), then asserts the search exits cleanly (`rc=0`) and produces the correct PSM, rather than crashing silently after the "`- searching ...`" progress message. |

### `test_il_sequence.py` (standalone, not part of `run_tests.py`)

```bash
python tests/unit/test_il_sequence.py
```

Verifies that with `equal_I_and_L=1`, the fragment index stores the
*original* FASTA sequence (preserving `L`) rather than the canonicalized
all-`I` form, and that the stored sequence comes from whichever protein
occurs first (smallest file offset) in the FASTA. Tests both the short
(<=12 AA: `ACLIVERK`/`ACIIVERK`) and long (>=13 AA:
`ACLIVERPEPTIDER`/`ACIIVERPEPTIDER`) index paths, plus a baseline check that
`equal_I_and_L=0` keeps all four peptides (and their tryptic sub-fragments)
as distinct entries.

### `compare_idx.py` (supporting tool, not itself a test)

```bash
python tests/unit/compare_idx.py <old.idx> <new.idx>
python tests/unit/compare_idx.py --dump <file.idx>
```

Structurally compares two plain-peptide `.idx` files: header fields
(peptide count, protein-list count, mass range), then a memory-safe
streaming comparison of every peptide entry (binary-chunk fast path first,
falling back to a record-by-record semantic comparison only if the binary
chunks differ). Designed to handle 190M-peptide files without the ~100+ GB
peak RSS that a naive full-dict comparison would need. Used internally by
T17; also runnable standalone for debugging index changes.

---

## `tests/regression/`

```bash
python tests/regression/setup_baselines.py              # fetch baseline binary
python tests/regression/run_regression.py                # run all 3 modes x all 3 decoy variants
python tests/regression/run_regression.py --modes fasta fi
python tests/regression/run_regression.py --decoy-variants nodecoy internaldecoy2
python tests/regression/run_regression.py --tags v2026.01.1
python tests/regression/test_raw_vs_mzxml.py              # Windows-only: .raw file support
```

### `setup_baselines.py`

Downloads pre-built Comet release binaries from the `UWPR/Comet` GitHub
Releases page (`comet.win64.exe` / `comet.linux.exe`) into
`baselines/<tag>/`, for use as the "before" side of a regression comparison.
Default tag list: `v2026.01.1`. `--list` shows configured tags and whether
each is already present; skips re-downloading if the binary already exists.

### `run_regression.py`

Runs three search modes against real MS data (`data/human.small.fasta` +
`data/20250520_Hela_60min_06.mzXML`) on both the baseline binary and the
current build, and compares:

| Mode | What it does |
|---|---|
| `fasta` | Regular FASTA database search (no index build). |
| `fi` | Builds a fresh fragment-ion index (`-i`) with *each* binary separately, then searches it. |
| `pi` | Builds a fresh peptide index (`-j`) with each binary separately, then searches it. |

Each mode is additionally run under one or more **decoy variants**, each
backed by its own params file with `decoy_search` baked in:

| Variant | `decoy_search` | Params file | Modes |
|---|---|---|---|
| `nodecoy` | 0 | `data/comet_phospho.params` | fasta, fi, pi |
| `internaldecoy1` | 1 (internal decoy, concatenated) | `data/comet_phospho_internaldecoy1.params` | fasta, pi |
| `internaldecoy2` | 2 (internal decoy, separate) | `data/comet_phospho_internaldecoy2.params` | fasta, pi |

`internaldecoy1`/`internaldecoy2` are automatically skipped for `fi` -- FI
does not support Comet's internal (on-the-fly) decoy generation -- and the
report shows an explicit `SKIPPED` line for that combination rather than
silently omitting it.

For each mode/variant it records: index build time (fi/pi only), search
wall-clock time, PSM count above `xcorr >= 2.5`, and the fraction of common
(scan, charge) pairs where both binaries agree on the top peptide
(I/L-normalized comparison). For `internaldecoy2`, which makes Comet write a
separate `<basename>.decoy.txt` output file, the same PSM count/agreement
comparison is run again on that decoy-only file. Writes both a human-readable
`report.txt` and a machine-readable `report.json` (covering every
variant x mode combination for that baseline tag) under
`results/<timestamp>_<tag>/`, with raw per-run Comet output kept under
`results/<timestamp>_<tag>/<variant>/<mode>/`.

### `test_raw_vs_mzxml.py`

```bash
python tests/regression/test_raw_vs_mzxml.py
python tests/regression/test_raw_vs_mzxml.py --comet ../../x64/Release/Comet.exe
```

Windows-only test: only the Windows release reads `.raw` files directly
(Thermo vendor library), so this confirms that support actually works by
running the *same* Windows `Comet.exe` against `data/20250520_Hela_60min_06.mzXML`
and `data/20250520_Hela_60min_06.raw` -- the same underlying acquisition, two
different file formats -- with all 5 output formats enabled
(`output_txtfile`, `output_sqtfile`, `output_pepxmlfile`,
`output_mzidentmlfile`, `output_percolatorfile`) and comparing each pair:

| Format | Comparison |
|---|---|
| `.txt` | Full PSM-level comparison via the same `parse_txt()`/`compare_results()` used by `run_regression.py`: PSM count at `xcorr >= 2.5`, top-peptide agreement on common scans (must be >= 99%), and the PSM counts themselves (must be within 5% of each other -- looser than the other formats because this count is sensitive to borderline scores flipping across the threshold from tiny numeric differences between vendor-raw and converted-mzXML peaks). |
| `.sqt`, `.pep.xml`, `.mzid`, `.pin` | Lightweight record-count comparison (`S\t` lines, `<spectrum_query >` tags, `<SpectrumIdentificationResult id=` tags, and data rows respectively) -- must be within 1% of each other. Each file is also checked for existence and non-zero size. |

The test is automatically **skipped** (not failed) if given a non-Windows
binary (checked via the PE `MZ` header, not the host OS -- the script can run
from WSL/Linux and still invoke a Windows `Comet.exe` via interop) or if
`data/20250520_Hela_60min_06.raw` is absent (it's gitignored and not always
present). Output files are moved out from next to each input file into
`results/<timestamp>_raw_vs_mzxml/{mzxml,raw}.<ext>` immediately after each
search, both to allow comparison and to avoid leaving Comet's side-effect
output files sitting in `data/`.

---

## `tests/perf/`

```bash
python tests/perf/run_perf.py
python tests/perf/run_perf.py --comet ../../x64/Release/Comet.exe
python tests/perf/run_perf.py --baseline ../regression/baselines/v2026.01.1/Comet.exe
python tests/perf/run_perf.py --runs 3
```

### `run_perf.py`

**Not yet implemented** -- the script currently only parses its arguments and
prints them; the body is a documented `TODO`. Its intended scope (per its
module docstring) is wall-clock time and peak RSS memory across the same
three search modes as `run_regression.py` (FASTA, fragment-ion index,
peptide index), optionally repeated `--runs N` times with the median taken,
optionally compared against a `--baseline` binary, with results written to
`reports/<timestamp>.json` plus a human-readable text summary. Memory
collection is intended to use `/usr/bin/time -v` on Linux and `tasklist` on
Windows.
