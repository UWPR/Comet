# Comet Test Suite Summary

Three independent test suites live under `tests/`, each with its own runner and
purpose, plus one supporting driver:

| Subdirectory | Purpose | Runner(s) |
|---|---|---|
| `unit/` | Index-building correctness, byte-level format checks, end-to-end search regressions for specific fixed bugs (80 Python test IDs), and 72 C++ unit tests of `CometSearch`/`CometPreprocess`/`ModificationsPermuter` internals | `run_tests.py`, `test_il_sequence.py`, `CometUnitTests.exe` (built from `CometUnitTests.vcxproj`) |
| `regression/` | Compare the current build against a tagged release binary on real MS data (timing, PSM counts, PSM agreement); also verifies Windows `.raw` file support | `setup_baselines.py`, `run_regression.py`, `test_raw_vs_mzxml.py` |
| `perf/` | Wall-clock time and peak memory benchmarks across search modes | `run_perf.py` |
| `rts_repro/` | Thermo-independent, Linux-buildable driver for the real-time search (RTS) single-spectrum path; not a test by itself, used by T22 | `rts_repro.cpp`, `ms2_to_fixture.py` |

Test counts as of `v2026.02.3` (2026-09-17): `run_tests.py` registers 59 named
tests plus 21 generated `t21_*` legacy cases (80 IDs, 10 of them integration-only);
`CometUnitTests.exe` has 72 `TEST_F` cases.

See `CLAUDE.md` for the canonical invocation examples. This document summarizes
what each individual test actually checks.

---

## `tests/unit/`

```bash
python tests/unit/run_tests.py --comet /mnt/c/Work/Comet-master/comet.exe
python tests/unit/run_tests.py --comet /mnt/c/Work/Comet-master/comet.exe t13
python tests/unit/run_tests.py --comet /mnt/c/Work/Comet-master/comet.exe --integration
```

T1-T18 build a plain-peptide `.idx` via `Comet.exe -i` against a small
crafted FASTA in `tests/unit/data/`, then parse the resulting `.idx` directly
(`parse_idx()`) and check specific peptides, masses, flanking residues, or
protein-list contents -- except T13 (pure Python, no Comet invocation) and
T17/T18 (integration, require `data/human.small.fasta`). T19 onward are
end-to-end search regressions: each builds an FI_DB (`-i`) and/or PI_DB (`-j`)
index, or searches the plain FASTA, against a small `.ms2`/`.mzXML` fixture and
asserts on the `.txt` (and, where relevant, pepXML/mzIdentML) output. Always
pass `--comet` as a full path; the default `../../comet.exe` only resolves
correctly when invoked from inside `tests/unit/`.

Integration-only IDs (need `--integration`, some also `--bigdata`): T17, T18,
`t22_rts_fi`, `t22_rts_pi`, `t22_rts_fi_protterm`, `t22_rts_pi_protterm`,
`t23_decoy_modes`, `t24_index_parity`, `t24_internal_decoy_parity`,
`t44_termmod_parity_bigdata` (`INTEGRATION_TESTS` in `run_tests.py`). T8-T10 do
not exist.

### `run_tests.py` -- T1-T51

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
| **T17** *(integration)* | Builds `data/human.small.fasta` with no-enzyme, length 8-13, `equal_IL=1` and checks the peptide count falls in `[8,800,000, 9,100,000]` and that peptide count equals protein-list count. An exact cross-version byte comparison is unreliable here (known I/L long-path dedup differences between versions), so this test uses a count-stability range instead -- see the comment block above the test in `run_tests.py` for the full rationale. Requires `--integration` and `data/human.small.fasta` (not in the repo). |
| **T18** *(integration)* | Determinism: two independent builds of `data/human.small.fasta` (same no-enzyme len 8-13 config) must produce byte-identical `.idx` files. Requires `--integration` and `data/human.small.fasta`. |
| **T19** | Regression test for the AScorePro + FI_DB mod-ordering bug (`docs/20260617_codereview3.md` issue 2a): builds an FI_DB index with a real phospho-S variable mod baked into the `.idx` header, then searches with `print_ascorepro_score=1` and a deliberately *blank* `variable_mod01` in the search-time params (the normal real-world case for FI_DB searches). Asserts the single expected PSM (`ACDEFGSK`, phospho at position 7) is found and `ascorepro > 0` -- i.e. AScore actually used the index's mod instead of being silently skipped because of the blank search-time mod. |
| **T20** | Regression test for the PI_DB batch-search crash (`_pQueries` never assigned in `CometSearch::SearchPeptideIndex(ThreadPool*, vector<Query*>&)`, which segfaulted inside `BinarySearchMass()` on the first scored candidate). Reuses T19's phospho fixture but builds a PI_DB (`-j`, peptide index) instead of an FI_DB (`-i`, fragment index), then asserts the search exits cleanly (`rc=0`) and produces the correct PSM, rather than crashing silently after the "`- searching ...`" progress message. |
| **T21** (`t21_*`, one per case) | Migrates the ~21 hand-run functional-correctness cases from `/mnt/c/Work/20130226-comet-tests/runall.sh` (originally judged by eye against each case's README). Fixtures live in `tests/unit/data/legacy/`; params are generated at runtime from `legacy_cases.py`'s template. No assertion checks an absolute xcorr/e-value/deltaCn/ion-count (those legitimately drift across versions) -- checks are limited to peptide identity, protein identity, modification presence, hit counts, and relative score comparisons. See `legacy_cases.py`'s module docstring for the full case table. |
| **T22** *(integration)* (`t22_rts_fi`, `t22_rts_pi`) | Exercises the real-time search (RTS) single-spectrum path via `tests/rts_repro/` against an FI_DB and a PI_DB built from a small unambiguous fixture -- no C++/CLI or Thermo dependency, so it runs on Linux. Checks RTS finds the correct peptide, and that 1-thread and 8-thread runs over 197 real spectra are byte-identical (the determinism guarantee from `tests/rts_repro/README.md`). |
| **T22b** *(integration)* (`t22_rts_fi_protterm`, `t22_rts_pi_protterm`) | Same RTS 1-vs-8-thread determinism check with a `^` (protein-N-terminal) acetyl variable mod in the index, on FI_DB and PI_DB. |
| **T23** *(integration, `--bigdata`)* (`t23_decoy_modes`) | Migrates `comet-debug3`'s full-scale search (~177 MB mzXML + FASTAs). Checks internal-decoy and target-decoy searches agree on PSM counts at 1% FDR within 5%, plus a cross-version comparison (PSM count within 10%, wall-clock within 25%) against the pinned `BASELINE_TAG` release binary, auto-downloaded via `setup_baselines.py`. Skips cleanly if `--bigdata DIR` isn't present or the baseline can't be fetched. |
| **T24** *(integration, `--bigdata`)* (`t24_index_parity`) | Migrates `comet-debug4`'s full-scale search. Checks plain-FASTA, FI_DB, and PI_DB searches agree on PSM counts at 1% FDR (within a few percent of each other), plus the same `BASELINE_TAG` cross-version comparison as T23 for all three modes (each rebuilt fresh with the baseline binary, since `.idx` formats aren't guaranteed compatible across versions). Same skip behavior as T23. |
| **T24b** *(integration, `--bigdata`)* (`t24_internal_decoy_parity`) | Same full-scale 1% FDR parity check as T24 but with Comet's internal decoys (`decoy_search=1`) on all three paths -- plain FASTA, FI_DB and PI_DB -- now that FI_DB supports internal decoys (`docs/20260914_FI_internal_decoys.md`). |
| **T25** (`t25_fi_mod_slot_gap`, `t25_fi_mod_slot_ambig`) | FI_DB variable-mod-slot regression: a mod configured in `variable_mod02` (slot 1) with `variable_mod01` (slot 0) left unused must resolve to the correct slot, not silently misread as slot 0. The `_ambig` variant forces a genuinely ambiguous second modifiable site (2 candidate S residues, `max_variable_mods_in_peptide=1`) to additionally exercise `AddFragments()`'s combination-enumeration path. |
| **T26** (`t26_b1_fasta_decoy`, `t26_b2_fi_nl_order`) | B1/B2 regressions: FASTA-path decoy fragment ladder must not abort early on a phospho+NL residue, and FI_DB's neutral-loss running-count carry-forward must use the loop's own index rather than a stale outer-scope variable. |
| **T27** (`t27_modcap_fasta`, `t27_modcap_fi`) | B3/B4 regressions: a 3-mod-type combination on variable-mod slots 10-15 that exceeds `max_variable_mods_in_peptide` must be rejected, on both the FASTA and FI_DB paths. |
| **T28** (`t28_idx_cterm_mod`) | B5 regression: an `.idx`-only search (no search-time `variable_mod`) with a C-term variable mod baked into the header must still apply it -- confirms the header-restore path derives the terminal-mod flags rather than leaving them unset. |
| **T29** (`t29_decoyprefix`) | B14/B15 regression: a target-decoy PI_DB `.idx` search with the correct `decoy_prefix` must classify the decoy row correctly, and the internally-generated on-the-fly reversed decoy must not leak into rank-1 output. |
| **T30** (`t30_mass_boundary`) | C1/C2 regression: a precursor sitting exactly at the `digest_mass_range` upper boundary must be included and correctly identified, exercising the array-sizing fix at that boundary. |
| **T31** (`t31_speclib_sizing`) | C5 regression: a minimal `.msp` spectral-library MS2 run must complete without the `std::out_of_range` crash from the precursor-index sizing bug. |
| **T32** (`t32_bad_enzyme_number`) | B11 regression: `search_enzyme_number = 99` (undefined) must produce a params-file error, not silently proceed. |
| **T33** (`t33_param_robustness`) | C10 regression: a params file with a 600-char value (`szParamVal` is 512 bytes) and a malformed `mass_offsets` entry must error or otherwise handle gracefully, not stack-smash or hang. |
| **T34** (`t34_internal_decoys_fi`, `t34_internal_decoys_pi`) | Comet internal decoys (`decoy_search=1`/`2`) on FI_DB and PI_DB: pseudo-reversal of the sequence, mod-site mirroring onto the reversed sequence, palindromic peptides merging into a single entry, and separate-list (`decoy_search=2`) output. Same fixture and assertions for both index types. |
| **T35** (`t35_ascore_crossmode`) | AScorePro scores must be identical across the plain-FASTA, PI_DB and FI_DB paths -- guards the double-applied static mod (a score regression) and exercises the >150-peak normalized peak-list branch. |
| **T36** (`t36_decoy_norelocalize`) | AScorePro reports a site score for an internal-decoy PSM but never relocalizes the decoy's mod site (plain FASTA and PI_DB, real scan 42900). |
| **T37** (`t37_protein_term_plain`) | `^`/`$` protein-terminal variable mods (`docs/20260915_permuter_terminal_mods.md`) on the plain-FASTA path: applied only where the peptide is actually protein-N-/C-terminal. |
| **T38** (`t38_protein_term_index`) | `^`/`$` protein-terminal variable mods on FI_DB and PI_DB; asserts the `.idx` header is format v5. |
| **T39** (`t39_termmod_cap_index`) | Terminal mods count toward `max_variable_mods_in_peptide` on FI_DB/PI_DB exactly as on the plain-FASTA path: a 3-mod permutation (n-term + c-term + M oxidation) must vanish at cap 2 and return at cap 3. |
| **T40** (`t40_internal_decoys_protterm`) | FI_DB internal decoys combined with a `^` mod: decoy PSMs keep the terminal mod on the N-terminus of the reversed sequence, and target rows match the `decoy_search=0` run. |
| **T41** (`t41_termmod_deprecation`) | `variable_mod` fields 5/6 (term_distance, n/c-term) are deprecated: the legacy protein-terminus idiom is bridged to `^`/`$` with a warning; other non-default values warn and are ignored. |
| **T42** (`t42_static_protein_nterm_fidb`) | Static `add_Nterm_protein`/`add_Cterm_protein` masses are applied on FI_DB exactly as on PI_DB and plain FASTA, and a peptide that is protein-terminal in one protein and internal in another is attributed only to the protein where the static applies. |
| **T43** (`t43_v4_index_rejected`) | A v4 `.idx` (frozen pre-Phase-2 fixture) is refused with the "rebuild the index" message instead of being misread. |
| **T44** *(integration, `--bigdata`)* (`t44_termmod_parity_bigdata`) | Full-scale 1% FDR parity, plain FASTA vs FI_DB vs PI_DB, with `n` and `^` acetyl and `$` amidation variable mods configured. |
| **T45** (`t45_mixed_terminal_codes`) | One slot mixing residue letters with terminal codes (`n^K`, `c$K`), and two same-mass slots that merge after the deprecation bridge -- on plain FASTA, FI_DB and PI_DB. |
| **T46** (`t46_shared_peptide_attribution`) | Protein-terminus attribution for a peptide shared by proteins with different terminal context: the index keeps one raw-peptide row per sequence but each protein occurrence carries `PROT_NTERM_HERE`/`PROT_CTERM_HERE` context bits, so a `^` variant is emitted only if some occurrence is protein-N-terminal and the reported protein list is filtered to the supporting occurrences. FI_DB and PI_DB must match the plain-FASTA attribution exactly. |
| **T47** (`t47_terminal_mod_xml_annotations`) | pepXML `<terminal_modification protein_terminus>` and mzIdentML `<SearchModification>` specificity CV terms for the four terminal codes `n`, `^`, `c`, `$`; residue declarations appear once, a slot holding both codes for one terminus is declared once as peptide-scoped, `^` resolves to its UNIMOD entry; FI_DB/PI_DB internal-decoy searches produce resolvable `PeptideEvidence`. |
| **T48** (`t48_v5_missing_context_bytes_rejected`) | A v5 `.idx` whose protein list lacks the per-occurrence context bytes fails cleanly at load. |
| **T49** (`t49_bridge_edge_cases`) | Legacy `nK 0 3 0 0` / `cM 0 3 0 1` equal explicit `^K` / `$M` (with a warning) on plain FASTA, FI_DB and PI_DB; `n` and `^` in different slots coexist without cross-talk. |
| **T50** (`t50_idx_protein_context_bytes`) | The v5 protein-list context bytes on disk equal the FASTA-derived per-protein context for every peptide (repeated-in-one-protein, shared-with-different-context, plain internal), checked on a fresh build of `t50_context.fasta` and on the committed t2/t3/t6 fixtures. |
| **T51** (`t51_ascorepro_with_protein_term_mods`) | `print_ascorepro_score=1` with `^`/`$` protein-terminal variable mods configured: a pure `^`/`$` mod (no residues) is not registered with AScorePro, every path runs with AScorePro on and off with the same (peptide, protein) result set, and the `.txt` carries a numeric `ascorepro` value on the terminally-modified hit. |

#### Notes on T17/T18, T21 and the big-data tests (T23, T24, T24b, T44)

Moved here from `CLAUDE.md` (2026-09-17) so that file only carries the invocation rules.

- **T17 uses count-stability, not cross-version byte comparison.** The v2026.01.1
  baseline has a known I/L long-path dedup bug (byte-exact `memcmp` instead of canonical
  L==I comparison), producing ~8,102 extra entries when `equal_IL=1`. Even with
  `equal_IL=0` there is an 8-peptide algorithmic difference from the flat-sort vs
  per-length sort change. Cross-version byte-exact or count-exact comparison is therefore
  unreliable; T17 verifies that the peptide count falls in [8,800,000, 9,100,000] for a
  no-enzyme len 8-13 build on `human.small.fasta`. **T18** covers determinism instead:
  two independent builds of the same FASTA must be byte-identical.
- **`no-enzyme + len_max > 13` will time out.** No-enzyme with `len_max=25` generates a
  ~1.1 GB index and takes >300 s. Use `len_max=13` for integration tests; it covers both
  the short path (len <= 12, 5-bit packed) and the long path (len > 12, plain string)
  while building in ~110 s.
- **T21** (`t21_*`, one per case, always run) migrates the ~21 hand-run cases from
  `/mnt/c/Work/20130226-comet-tests/runall.sh`, originally judged by eye against each
  case's README. Fixtures live in `tests/unit/data/legacy/`; params are generated at
  runtime from `legacy_cases.py`'s template rather than maintaining ~15 historical
  `comet.params.YYYYNNN` copies per case. See that module's docstring for the case table.
- **Big data.** T23/T24 (`--integration` + `--bigdata`) migrate `comet-debug3` /
  `comet-debug4`'s full-scale searches (~350 MB of real data: a 177 MB mzXML, 57 MB and
  116 MB FASTAs). `--bigdata DIR` (default: the sibling `20130226-comet-tests/` directory)
  points at this data in place; it is never copied into the repo, and every big-data test
  skips cleanly if the directory isn't present. T23 checks that internal-decoy and
  target-decoy searches agree on PSM counts at 1% FDR (via `tools/qvalue.py`) within 5%;
  T24 checks the same for plain-FASTA vs. FI_DB vs. PI_DB. All three currently pass and
  agree within a few percent (17,660 / 17,033 / 17,660 PSMs at 1% FDR respectively).
  T24b repeats T24 with internal decoys and T44 with terminal variable mods.
- **Cross-version comparison against a previous Comet release.** T23 and T24 also run
  every one of their configs (T23: both decoy modes; T24: plain-FASTA, FI_DB, PI_DB, each
  index built fresh with the baseline binary since `.idx` formats aren't guaranteed
  compatible across versions) against a pinned previous release, `BASELINE_TAG` in
  `run_tests.py` (currently `v2026.02.2`), fetched automatically on first use via
  `tests/regression/setup_baselines.py`'s download logic into
  `tests/regression/baselines/<tag>/comet` (gitignored, like all fetched baselines;
  override with `--baseline PATH`). Cross-version checks skip cleanly, without failing the
  test, if no baseline is available. Same-version comparisons (internal-vs-target-decoy,
  FI/PI-vs-plain-FASTA) use a 5% tolerance; current-vs-baseline comparisons use 10%,
  since a different Comet version legitimately identifies a somewhat different peptide
  population.
- **Runtime regression check.** Each search and index build is timed, and every
  current-vs-baseline pairing asserts current isn't more than `TIMING_NOISE_TOLERANCE`
  (currently 25%) slower than the baseline's wall-clock time for the same operation, via
  `_check_timing()`. The threshold is deliberately generous: these are multi-minute,
  single-sample wall-clock measurements on real (possibly shared) hardware, and run-to-run
  variance from machine noise alone can plausibly reach 10-20% with no code change. Treat
  one `_check_timing` failure as "worth re-running to confirm", not proof of a regression;
  a *repeated* failure across runs is the real signal.
- **`std::length_error` on a full-scale FI_DB.** While developing T24, one manual
  (non-harness) attempt to search a full-scale target-decoy FI_DB crashed with
  `std::length_error: cannot create std::vector larger than max_size()`. That manual build
  had been interrupted by a shell timeout mid-write, almost certainly leaving a truncated
  `.idx` on disk; under T24's own clean build-then-search sequence FI_DB has run correctly
  every time. See the comment above `test_t24_index_parity` in `run_tests.py` if this ever
  resurfaces.

`legacy_cases.py` holds the T21 case table; `MiniTest.h` is the C++ harness
used by `CometUnitTests` below.

### `CometUnitTests` (C++, `CometUnitTests.vcxproj`)

```bat
x64\Release\CometUnitTests.exe
```

Built as part of `Comet.sln` (Release/x64) and run by the Windows CI workflow
(`.github/workflows/windows-build.yml`). Uses `MiniTest.h`, a dependency-free
in-repo harness (no gtest, no CMake/NuGet fetch), and links against
`CometSearch.lib` directly to test methods that the Python harness can only
reach through a full search:

| Source | Suite | Cases | What it covers |
|---|---|---:|---|
| `TestCometSearchAndPreprocess.cpp` | `CometSearchTest` | 28 | `CometSearch` static helpers with a minimal `g_staticParams` setup: `CheckEnzymeTermini`/`CheckEnzymeStartTermini`/`CheckEnzymeEndTermini` (tryptic rules, K-before-P, protein termini, `num_enzyme_termini=1`), `CheckMassMatchStatic` with `isotope_error` 0/1 (C13 window), `GetAA` codon translation on forward/reverse strands (stop codon, unknown codon), and `AllocateMemory`/`DeallocateMemory` idempotence. |
| `TestCometSearchAndPreprocess.cpp` | `CometPreprocessTest` | 26 | `CometPreprocess` driver helpers: `CheckExit` under every scan-selection mode (entire file, scan range, specific scan, batch size, error status), `GetMassCushion` for amu/mmu/ppm and precursor-m/z tolerances, `IsValidInputType` per file type (`.raw`/`.mzXML` true; `.ms2`/`.mgf`/`.mzML` false), `Reset`/`DoneProcessingAllSpectra`, and allocate/deallocate idempotence. |
| `TestCometSearchAndPreprocess.cpp` | `BinarySearchMassFixture` | 5 | `CheckMassMatchStatic` at and beyond the lower/upper tolerance bounds, driving the mass-window binary search used by the index paths. |
| `TestModificationsPermuter.cpp` | `PermuterTest` | 13 | `ModificationsPermuter` (P1-P13): sentinel terminal-slot positions in the mod sequence, residue-only sequences without sentinels, mod-char translation, terminal + residue entries, terminal mods counting toward the per-peptide and per-mod caps, protein-N-term-only and both-termini cases, zero-combination/overflow guards, determinism, upstream overlapping-K mods, and the `PROT_*_HERE` context-flag rule that a both-termini variant needs one occurrence carrying both bits. |

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
Default tag list: `v2026.02.2`. `--list` shows configured tags and whether
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

`internaldecoy1`/`internaldecoy2` are automatically skipped for `fi` and the
report shows an explicit `SKIPPED` line for that combination rather than
silently omitting it. That skip predates `v2026.02.3`, whose FI_DB does
support internal decoys (`docs/20260914_FI_internal_decoys.md`, covered by
T34/T40/T24b in `run_tests.py`); `run_regression.py`'s mode table has not been
widened yet, so FI internal-decoy regressions are only exercised by the unit
harness.

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

## `tests/rts_repro/`

```bash
# build: single g++ line in rts_repro/README.md (no Makefile); T22 builds it itself
./rts_repro <database.idx> <fixture_file> <num_threads> <output_file> [ascorepro:0|1] [index_search_type:0=PI_DB|1=FI_DB]
python3 tests/rts_repro/ms2_to_fixture.py in.ms2 > fixture.txt
```

Not a test suite itself but the driver T22/T22b run (T22 compiles it with
`g++` against `libcometsearch.a` and skips if that fails). `rts_repro.cpp` calls the
same `ICometSearchManager` API that `RealtimeSearch/SearchMS1MS2.cs` reaches
through `CometWrapper.dll` (`InitializeSingleSpectrumSearch()` +
`DoSingleSpectrumSearchMultiResults()`) from N worker threads pulling off a
shared queue -- the C# harness's concurrency pattern -- against 197 real
spectra in `fixture_spectra.txt` (extracted once from
`20250520_Hela_60min_06.raw`). It needs no Thermo library or C++/CLI, so the
RTS path is testable on Linux. Written for the E-value jitter investigation
(`docs/20260714_EvalueJitter.md`); see its `README.md` for the fixture format
and regeneration steps.

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
