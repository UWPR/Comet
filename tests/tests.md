# Comet Test Suite Summary

Three independent test suites live under `tests/`, each with its own runner and
purpose, plus one supporting driver:

| Subdirectory | Purpose | Runner(s) |
|---|---|---|
| `unit/` | Index-building correctness, byte-level format checks, end-to-end search regressions for specific fixed bugs (93 Python test IDs), and 82 C++ unit tests of `CometSearch`/`CometPreprocess`/`ModificationsPermuter` internals | `run_tests.py`, `test_il_sequence.py`, `CometUnitTests.exe` (built from `CometUnitTests.vcxproj`) |
| `regression/` | Compare the current build against a tagged release binary on real MS data (timing, PSM counts, PSM agreement); also verifies Windows `.raw` file support | `setup_baselines.py`, `run_regression.py`, `test_raw_vs_mzxml.py` |
| `perf/` | Wall-clock time and peak memory benchmarks across search modes | `run_perf.py` |
| `rts_repro/` | Thermo-independent, Linux-buildable driver for the real-time search (RTS) single-spectrum path; not a test by itself, used by T22 | `rts_repro.cpp`, `ms2_to_fixture.py` |

Test counts as of 2026-09-30 (2026.03 rev. 0 development, `terminalmods`: T1-T64): `run_tests.py` registers 72 named
tests plus 21 generated `t21_*` legacy cases (95 IDs, 10 of them integration-only);
`CometUnitTests.exe` has 82 `TEST_F`/`TEST` cases.

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

**Skips are counted as skips (2026-11-08, test T79).** A test that cannot run in the current
environment raises `TestSkipped(reason)`. That covers missing data, `--integration` not given, a
Windows `--comet` for a Linux-only test, `g++` not installed, and `RealtimeSearch.exe` not built.
The runner counts it as SKIPPED and lists every skipped test with its reason under the
`Results:` line. Before, tests printed "SKIP" and returned no failures, which counted as passed,
so "N passed, 0 skipped" could hide tests that never ran. Skips still do not fail the run. Two
partial skips stay printed notes inside a test that otherwise runs: T23/T24's cross-version
checks without a baseline, and T74's RTS half.

**rts_repro build (`_rts_repro_status()`).** The RTS tests (T22, T43, T67, T68, T71-T73, T75-T77,
T81-T83, and T74's RTS half) need `tests/rts_repro/rts_repro`, built once per run when it is missing or older
than `rts_repro.cpp` or any library it links: `libcometsearch.a`, `libmstoolkit.a`,
`libmstoolkitextern.a` and `libascorepro.a`. A missing `g++` is a skip. A failed build, with
`g++` present, is a **failure** of each RTS test, reported with the compiler output. Before, a
failed build, for example after an interface change that broke `rts_repro.cpp`, turned every
RTS test into a "SKIP" that counted as passed, and only `libcometsearch.a` was checked for
staleness.

**RTS test parameters are pinned (`RTS_TEST_PARAMS`, 2026-11-08).** `rts_repro`'s defaults
follow `RealtimeSearch/SearchMS1MS2.cs`. That is an example harness whose settings change as
RTS is tried in different configurations, so no test relies on those defaults. `_rts_run()`
passes `isotope_error` 2 (what RTS runs) and `max_index_runtime` 0 (no wall-clock budget, so
no timing-dependent timeouts) through `RTS_REPRO_PARAMS`. A test that needs other values sets
them itself, through its own `RTS_REPRO_PARAMS` entries, which win, or the `isotopeError`
argument. T80 reports where `rts_repro`'s defaults differ from the harness, as notes.

### `run_tests.py` -- T1-T88

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
| **T22** *(integration)* (`t22_rts_fi`, `t22_rts_pi`) | Exercises the real-time search (RTS) single-spectrum path via `tests/rts_repro/` against an FI_DB and a PI_DB built from a small unambiguous fixture -- no C++/CLI or Thermo dependency, so it runs on Linux. Checks RTS finds the correct peptide, and that 1-thread and 8-thread runs over 197 real spectra are byte-identical (the determinism guarantee from `tests/rts_repro/README.md`). The determinism check runs at `isotope_error` 2, the FI search's general multi-window path that RTS uses, and at 0, the single-window fast path (2026-11-08). |
| **T22b** *(integration)* (`t22_rts_fi_protterm`, `t22_rts_pi_protterm`) | Same RTS 1-vs-8-thread determinism check with a `^` (protein-N-terminal) acetyl variable mod in the index, on FI_DB and PI_DB. |
| **T23** *(integration, `--bigdata`)* (`t23_decoy_modes`) | Migrates `comet-debug3`'s full-scale search (~177 MB mzXML + FASTAs). Checks internal-decoy and target-decoy searches agree on PSM counts at 1% FDR within 5%, plus a cross-version comparison (PSM count within 10%, wall-clock within 25%) against the pinned `BASELINE_TAG` release binary, auto-downloaded via `setup_baselines.py`. Skips cleanly if `--bigdata DIR` isn't present or the baseline can't be fetched. |
| **T24** *(integration, `--bigdata`)* (`t24_index_parity`) | Migrates `comet-debug4`'s full-scale search. Checks plain-FASTA, FI_DB, and PI_DB searches agree on PSM counts at 1% FDR (within a few percent of each other), plus the same `BASELINE_TAG` cross-version comparison as T23 for all three modes (each rebuilt fresh with the baseline binary, since `.idx` formats aren't guaranteed compatible across versions). Same skip behavior as T23. Note (2026-11-06): `fragindex_histogram_fill` (default 100, `docs/20261106_FI_evalue_histogram_fill.md`) changed FI_DB E-values -- larger for weak hits, now matching PI_DB's on the same PSMs -- but not xcorr or ranking; T23/T24 count at 1% FDR by xcorr, so their checks and the `BASELINE_TAG` comparison are unaffected (E-value-ranked FI counts move by about +1.6% on the HeLa benchmark; set `fragindex_histogram_fill = 0` to reproduce the old E-values). Note (2026-11-08): the fragment-index charge-conversion fix (`docs/20261108_FI_fragment_charge_lookup.md`) changes FI_DB results for 3+ and higher precursors; on the HeLa benchmark FI PSMs at 1% FDR by xcorr over all charges move -0.4% (a charge-mixing artifact; flat within each charge), well inside the 10% `BASELINE_TAG` tolerance. |
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
| **T41** (`t41_termmod_fields`) | `variable_mod` fields 5/6 (term_distance, which_term) restrict mod positions on the plain-FASTA path as in v2026.02.2: the legacy protein-terminus idiom (`n 0 3 0 0` / `c 0 3 0 1`) still equals `^`/`$` and is rewritten silently; `M 0 3 0 0` / `M 0 3 2 0` (protein N-terminus) admit no internal M, `M 0 3 3 2` only M within 3 of the peptide N-terminus, `M 0 3 -2 0` no M on the peptide C-terminal residue. |
| **T42** (`t42_static_protein_nterm_fidb`) | Static `add_Nterm_protein`/`add_Cterm_protein` masses are applied on FI_DB exactly as on PI_DB and plain FASTA, and a peptide that is protein-terminal in one protein and internal in another is attributed only to the protein where the static applies. |
| **T43** (`t43_v4_index_rejected`) | A v4 `.idx` (frozen pre-Phase-2 fixture) is refused with the "rebuild the index" message instead of being misread. |
| **T44** *(integration, `--bigdata`)* (`t44_termmod_parity_bigdata`) | Full-scale 1% FDR parity, plain FASTA vs FI_DB vs PI_DB, with `n` and `^` acetyl and `$` amidation variable mods configured. |
| **T45** (`t45_mixed_terminal_codes`) | One slot mixing residue letters with terminal codes (`n^K`, `c$K`), and two same-mass slots that merge after the deprecation bridge -- on plain FASTA, FI_DB and PI_DB. |
| **T46** (`t46_shared_peptide_attribution`) | Protein-terminus attribution for a peptide shared by proteins with different terminal context: the index keeps one raw-peptide row per sequence but each protein occurrence carries `PROT_NTERM_HERE`/`PROT_CTERM_HERE` context bits, so a `^` variant is emitted only if some occurrence is protein-N-terminal and the reported protein list is filtered to the supporting occurrences. FI_DB and PI_DB must match the plain-FASTA attribution exactly. |
| **T47** (`t47_terminal_mod_xml_annotations`) | pepXML `<terminal_modification protein_terminus>` and mzIdentML `<SearchModification>` specificity CV terms for the four terminal codes `n`, `^`, `c`, `$`; residue declarations appear once, a slot holding both codes for one terminus is declared once as peptide-scoped, `^` resolves to its UNIMOD entry; FI_DB/PI_DB internal-decoy searches produce resolvable `PeptideEvidence`. |
| **T48** (`t48_v5_missing_context_bytes_rejected`) | A v5 `.idx` whose protein list lacks the per-occurrence context bytes fails cleanly at load. |
| **T49** (`t49_bridge_edge_cases`) | Legacy mixed slots keep v2026.02.2's meaning on plain FASTA, FI_DB and PI_DB alike: `nK 0 3 0 0` is `^` plus K restricted to protein position 0 (no fixture protein starts with K, so it equals `^` and differs from unrestricted `^K`), `cM 0 3 0 1` likewise equals `$`; `n` and `^` in different slots coexist without cross-talk. |
| **T50** (`t50_idx_protein_context_bytes`) | The v5 protein-list context bytes on disk equal the FASTA-derived per-protein context for every peptide (repeated-in-one-protein, shared-with-different-context, plain internal), checked on a fresh build of `t50_context.fasta` and on the committed t2/t3/t6 fixtures. |
| **T51** (`t51_ascorepro_with_protein_term_mods`) | `print_ascorepro_score=1` with `^`/`$` protein-terminal variable mods configured: a pure `^`/`$` mod (no residues) is not registered with AScorePro, every path runs with AScorePro on and off with the same (peptide, protein) result set, and the `.txt` carries a numeric `ascorepro` value on the terminally-modified hit. |
| **T52** (`t52_peff_annotation_ids`) | PEFF annotation identifiers (`# HasAnnotationIdentifiers=true`, PEFF 1.0 section 3.4.2; issue #132): the same three entries written with and without `<label>:` prefixes (`\ModResPsi=(1:8|MOD:00046|...)`, UniPEFF-style half-cystine + `\DisulfideBond` annotations, a labeled `?` position, `\VariantSimple=(1:7|W)`, `\VariantComplex=(1:9|10|WW)`) must give identical rank-1 PSMs -- the phospho on S8 (formerly placed on residue `<label>`) and both variant peptides (formerly dropped). PEFFs, a two-term OBO and theoretical spectra are generated into a temp dir. |
| **T53** (`t53_peff_nested_parens`) | PEFF entries whose names contain parentheses (UniPEFF PSI-MOD names such as `N6-(4-amino-2-hydroxybutyl)-L-lysine`): attribute values are split on top-level parentheses only. `\ModResPsi=(8|MOD:00046|O-phospho-(L)-serine)(20|MOD:00798|half (cystine))` must apply both mods -- formerly a name ending in `)` stopped parsing and silently dropped that mod and every later entry on the protein -- and the ignored `(?|...)` mod, two-residue `\VariantSimple=(3|WW)` and empty-residue `\VariantSimple=(6|)` entries must each give exactly one clean `peff_verbose_output` warning quoting the entry as written, with no name-fragment warnings. `(6|)` follows a valid `(9|W)` and must not inherit its `W` (a failed `>> strVariant` leaves the string unchanged; the formerly shared variable turned it into a bogus L6W variant, matched by scan 3). A second entry has a stray `(` in a name (`\ModResPsi=(2|MOD:00046|bad (name)(6|MOD:00798|half cystine)`): unbalanced text falls back to splitting on every `)`, so the half cystine after it must still be applied (scan 4). Labeled entries with no usable position (`(1:|MOD:00046|x)`, `\VariantSimple=(2:?|W)`) must be ignored with warnings that quote the `<label>:` as written. Generated into a temp dir, reusing T52's OBO and spectrum writer. |
| **T54** (`t54_pyroglu_all_paths`) | Peptide-N-terminal pyroglutamate (`-17.026549 Q 0 1 0 2`, `-18.010565 E 0 1 0 2`) on plain FASTA, FI_DB and PI_DB: `Q[-17]TAGSPELK` and `E[-18]GTWLDNAPK` are found, the internal Q of `AGQEAPLSVR` is never modified (an unrestricted `Q` control does modify it on every path), the `.idx` `VariableMod:` slots persist the `:0:2` restriction, and a protein-terminus distance > 0 (`M 0 3 2 0`) warns on the index paths only. An n-term mod under a peptide-C-terminus rule (`42.010565 n 0 1 8 3`: acetyl only on peptides of length <= 9) is placed on `QTAGSPELK` (9) and not on `AGQEAPLSVR` (10) on all three paths. FASTA, spectra and params are generated into a temp dir. |
| **T55** (`t55_ascorepro_position_filter`) | AScorePro respects position restrictions: the spectrum carries pyroglutamate on the internal Q5 of `QTAGQPELK`, which `Q 0 1 0 2` forbids, so the search reports `Q[-17]TAGQPELK`; with `print_ascorepro_score = 1` on all three paths the peptide is not relocalized to Q5 and the site scores never name position 5 (Comet passes AScorePro a peptidoform filter; without it AScorePro relocalizes to `QTAGQ[-17]PELK`). |
| **T56** (`t56_idx_build_determinism`) | Default-suite counterpart of T18: a generated 400-protein FASTA where every protein repeats a 10-mer (short path) and a 14-mer (long path) at its N-terminus and internally with a different next residue each time, so one protein feeds the dedup merge several tuples of the same sequence. No-enzyme builds (length 8-15) with `num_threads = 1` and `16` must be byte-identical, with `equal_I_and_L` 0 and 1. Fails on the pre-fix build (the dedup sort tied such copies and the stored representative followed thread scheduling). |
| **T57** (`t57_position_rule_edge_cases`) | Plain-FASTA position-rule edge cases from the terminalmods code review: invalid fifth/sixth-field values (`M 0 3 0 4`, `M 0 3 -3 0`) are rejected with an error; binary mods honor a peptide-C-terminus rule (`8.014199 K 1 3 1 3` -> `AGSPELK[+8.01]`) and a protein-C-terminus rule (`79.966331 S 1 3 3 1` -> `AGPEMNVS[+80]S[+80]R`, whose start is farther than 3 from the protein end); a c-term mod with a protein-N-terminus rule (`14.01565 c 0 1 11 0`) reaches `QTAGSPELK`, whose missed-cleavage extensions end past 11; a binary group of an n-term acetyl under `n 1 3 8 3` plus a plain n-term mate counts its one site once (acetyl on the 9-mer, not the 10-mer; the 10-mer takes the plain mate as a positive control); a binary mod under `-2` (`8.014199 K 1 3 -2 0`) counts only the non-terminal site of `QTAGSPELKAGSPELK`, so `QTAGSPELK[+8.01]AGSPELK` is generated (the cumulative residue count used to include the terminal K and the all-or-nothing check rejected the only valid form); binary-group members are judged by their own rule, not the first slot's: with `K 1 3 -1 0` + `S 1 3 0 0` `AGSPELK[+8.01]` is generated (its S, counted under the first slot's rule, used to make it a 2-site peptide that rejected the 1-mod form), and with `M 1 3 0 0` + `K 1 3 -1 0` `AGSPELK[+8.01]` is generated (K, counted under the first slot's protein-N rule, was never a site). The invalid-value, binary rule, binary-group acetyl, binary `-2`, mixed-rule group and c-term checks fail on v2026.02.2 (FI/PI do not implement binary mods, so FASTA only). |
| **T58** (`t58_protein_term_rule_attribution`) | Protein attribution of a residue mod with a protein-terminus position rule: `MAGSPELK` is protein-N-terminal in `t58_a` and internal in `t58_b`; with `15.9949 M 0 3 0 0` the oxidized PSM must list only `t58_a` on plain FASTA, FI_DB and PI_DB (the unmodified control lists both). Before the fix FI/PI listed both: the shared index row admits the mod from its OR'd flanks, so attribution now goes through the protein-occurrence context bits, like `^`/`$`. |
| **T59** (`t59_ascorepro_fasta_protein_offset`) | AScorePro's peptidoform filter on the plain-FASTA path evaluates a protein-terminus rule on the true protein offset (`ProteinEntryStruct` `iStartResidue`/`iProteinLength`), not on the flanks: `AMMGSPELK` starts at protein offset 1 after an N-terminal K, and with `15.9949 M 0 3 3 0` both Ms (protein positions 2 and 3) are legal, so the alternative placement must be scored and the site score must not be the 5000.0 "only possible site" value. Fails on the build before the fix (flank test rejected the alternative). |
| **T60** (`t60_index_search_type_scope`) | `index_search_type` only selects the index type to auto-build when `database_name` names a missing `.idx` (`0` builds a peptide index, `1`/absent a fragment ion index, checked in the `IndexSearchType:` header). `comet -p` does not mention the parameter and `comet -q` writes `index_search_type = -1` (-1 = not set, the default: fragment ion index for an auto-build, never a warning), so a 0/1 in a params file is intent: otherwise any explicit 0/1 is ignored with a warning -- a FASTA database ("not an .idx file", results identical to the run without the parameter) or an existing `.idx` of the other type (names the file's type and the `-i`/`-j` to rebuild; a matching value is quiet); a value other than -1/0/1 warns and uses the default; an explicit `-i`/`-j` build that agrees with the value is quiet, one that disagrees warns that the flag overrides it. Prompted by an issue-132 comment. |
| **T61** (`t61_readprecursors_header_only`) | `fragindex_skipreadprecursors = 0` (the precursor pre-read, which Crux sets by default): `CometPreprocess::ReadPrecursors` reads mzML/mzXML header-only (`MSReader::setHeaderOnly`: the parser stops before the peak data) and runs one thread-pool task per input file. Four generated indexed files (mzXML and mzML, each with and without precursor charges; three 2+ MS2 scans plus an MS1) are searched in one `num_threads = 16` run against a fragment index; with skip = 0 every scan must get the same rank-1 peptide as with skip = 1 (no pre-read), and the `read precursors` step must run only for skip = 0. The charge-less files are the point: Comet guesses 2+/3+ from the peaks, so a header-only read that dropped them would assume 1+ and prune the 2+/3+ mass bins, losing those PSMs. Verified to fail when the peak re-read for charge-less spectra is disabled. |
| **T62** (`t62_precursor_mask_isotope_edges`) | The precursor-mass mask built by the `fragindex_skipreadprecursors = 0` pre-read must cover exactly the windows the search accepts (`mass + x * C13_DIFF` inside the window). The mask used to shift each isotope window by `x * C13_DIFF * PROTON_MASS` (1.01064 Da per step instead of 1.00335), dropping peptides within ~0.0073 * x Da of a window edge (167 e-value differences and 6 missing candidate rows between skip = 0 and skip = 1 on a 40k-spectrum file). With a 0.01 Da fragment bin, a `QTAGSPELK` spectrum whose isotope-shifted mass sits at 99% of the way to the upper edge of the 20 ppm +2-isotope window (`isotope_error = 2`) and to the lower edge of the -3-isotope window (`isotope_error = 6`), and at the upper/lower edge of the +-2-step windows of `isotope_error = 7` (steps of 4.0070995 Da, `ISOTOPE_ERROR_7_STEP`, not `C13_DIFF`; the mask was off by 0.0126 Da there), and a -1-isotope peptide under `isotope_error = 4` (the mask only marked -1 for modes 5 and 6), must be found with skip = 0 exactly as with skip = 1. Verified to fail (both skip = 0 checks) with the old shift restored. |
| **T63** (`t63_fi_equal_mass_order_invariant`) | Fragment-index entries of identical mass are ordered by (mass, raw peptide, mod number, decoy flag), not by `std::sort`'s arbitrary order for equal keys. The index search scores only the first `FRAGINDEX_MAX_NUMSCORED` (100) candidates ranked by matched ions then index position, so an order that changed with the set being sorted (for example what the `fragindex_skipreadprecursors = 0` mask pruned) changed which tied candidates were scored, the e-value histogram built from them, and which of several equal scores was rank 1 (23 differing rows on a 40k-spectrum file after the mask fix; 38 of 100 on the fixture here). 160 permutations of one residue set (`AG` + permutation of `STVNDE` + `K`: identical mass, each matching at least the same three fragment ions, so more than 100 tie at the cap) plus 400 filler peptides that only the skip = 1 index contains are searched with `num_output_lines = 100`; skip = 0 and skip = 1 must report the same 100 rows (peptide, protein, xcorr, e-value). Verified to fail on the `mass`-only sort. |
| **T64** (`t64_precursor_mask_mass_offsets`) | `mass_offsets` and the `fragindex_skipreadprecursors = 0` precursor mask. The search accepts a peptide when `mass + offset + isotope step` lies in the precursor window for one of the listed offsets (only listed offsets are tried, so 0 only if listed), but the mask was built from the unshifted window only, so with skip = 0 every peptide matched through an offset was missing from the index while skip = 1 found it (Crux exposes `mass_offsets` and defaults to skip = 0). Cases: a single offset, an offset list matched through the first and through the second entry, offset + `isotope_error` 1 (+1 step), offset + `isotope_error` 5 (-1 step), offset + `isotope_error` 7 (+-2 steps), and a control (the unshifted peptide is not a match when offsets are listed, in either mode); skip = 0 must give the same rank-1 peptide as skip = 1. The mask is now built from the same shift list as `CheckMassMatch()` (`PrecursorWindowShifts()` in `CometPreprocess.cpp`). Verified to fail on the offset cases with the old mask. |
| **T65** (`t65_required_varmod_index`) | The seventh field of `variable_modNN` (required) on the FI_DB and PI_DB indexes. Both builders only dropped the unmodified variant, so a variant carrying another mod but not the required one was indexed and reportable. Fixture `MAGGK...R` with M oxidation and a required K mod: the indexed variant count must be exactly 2 (K, M+K) rather than 3, and lower than without the flag. Verified to fail (3 variants) with `PassesRequiredVarMods()` disabled. |
| **T66** (`t66_ascore_protein_term_static`) | AScorePro and protein-terminus static mods. `add_Nterm_protein`/`add_Cterm_protein` apply only to a peptide with a `-` flank, so `CalculateAScorePro()` adds them per PSM to a private copy of the AScorePro options (peptide-terminus ones are in the shared options). Synthetic spectrum of `SSDTAGELK` (a whole one-peptide protein, phospho on S2) with N-term +100 and C-term +50 in the b/y ions: the rank-1 site is S2 with AScorePro score >= 13, on FASTA_DB, PI_DB and FI_DB, each with and without a position-restricted variable-mod slot (which routes through the thread-local options copy). Verified to fail (AScorePro score 0.0 in all six cases) with the per-PSM adjustment disabled. |
| **T67** (`t67_chimeric`) | RTS chimeric-spectrum search, `DoSingleSpectrumSearchChimeric()` (`docs/20261105_RTSchimeric.md`). `tests/unit/data/t67_make_chimeric.py` writes (deterministically, into a temp dir) a FASTA with P1 `LVNELTEFAK`, P2 `LLEALEDNHHSYFR` (chosen so its 3+ m/z is 0.37 from P1's 2+ m/z -- inside a +/- 2 m/z window, outside 20 ppm), P3 `FVNELTELAK` (a shuffle of P1: same mass, mostly different ions) and 300 filler peptides, plus six synthetic b/y spectra as `rts_repro` fixtures: P1 alone; P1 (x3) + P2 (x1) at P1's m/z; P1 (x3) + P3; P1 with 2+ and 3+ candidates at its m/z; noise only; P1 with a water-loss peak under each ion. Searched through `rts_repro`'s chimeric modes (1 = charge-range form, charges 2-4, +/- 2 m/z; 2 = explicit candidate list from the fixture's blocks, configured tolerance) on FI_DB and PI_DB with `dMaxExpect = 0.01`: scans must report exactly {P1/2+}, {P2/3+, P1/2+} (P2 first: lowest E-value, 52 ions at 1+/2+ vs 18, despite a third of the intensity), {P1/2+, P3/2+} (same charge twice), {P1/2+} (the 3+ candidate explains nothing), nothing, and {P1/2+}, with iterations 0..n-1 and every row under the threshold; `dMaxExpect = 1e-30` reports no rows anywhere; the `RTS_CHIMERIC_DEBUG` trace shows scan 1's iteration 0 removed all 18 of P1's peaks; on scan 6 the removal takes 18 peaks with `use_NL_ions = 0` on both index types and with `use_NL_ions = 1` on FI_DB (which never scores the neutral-loss matrix), but 36 -- the ions and their water-loss peaks -- on PI_DB, mirroring exactly when `XcorrScoreI()` credits those peaks; 1 vs 4 worker threads are byte-identical. Linux only (skips on a Windows `--comet`). |
| **T68** (`t68_histogram_fill`) | `fragindex_histogram_fill` (`docs/20261106_FI_evalue_histogram_fill.md`): FI_DB queries score few real candidates (median 1 on the HeLa benchmark) and the E-value regression fits a synthetic-decoy tail steeper than the real chance-sharing one; the fill keeps scoring the ranked sub-floor (2-hit, 1-hit) candidates into the histogram without changing who can be stored. `tests/unit/data/t68_make_histfill.py` writes a FASTA with the target `LVNELTEFAK`, 20 composition-identical permutations sharing exactly 2 index ions (K-terminal b9 = M - K, plus y9 from a leading L) and 30 sharing exactly 1 (b9 only) -- same precursor mass, so all are candidates; a shared prefix/suffix residue multiset shares two ions since b(n-k) = M - y(k), which the generator's model counts exactly -- plus 150 0-hit fillers, and a one-scan fixture of the target's 1+ b/y ions. Through `rts_repro` mode 0 with `COMET_EVALUE_TRACE=1`: fill 0 leaves the target alone in the histogram (`hist=1`); fill 100 -- the default, so fill not sent is byte-identical to it and differs from 0 -- gives `hist=51`, fill 25 exactly 25; in every run the rank-1 is the target with identical xcorr, exactly one candidate is stored, and the fill-100 E-value is not smaller. Linux only. |
| **T69** (`t69_histogram_fill_batch`) | Fill candidates are histogram-only in a batch FI_DB search (`docs/20261106_FI_evalue_histogram_fill.md` section 3.1). T68's generator output, its one-scan fixture rewritten as an `.ms2`, searched by `comet.exe` against a fragment ion index with pepXML output and `num_output_lines = 20`, at `fragindex_min_ions_score` / `fragindex_min_ions_report` 3 / 3 and 3 / 2, each with `fragindex_histogram_fill` 0 and 100. In all four runs the only stored PSM is the target and `num_matched_peptides` is 1; the report-below-score warning is printed exactly for 3 / 2; and fill 100 gives a larger E-value than fill 0, so the fill really ran. Before the fix, 3 / 2 at fill 100 reported the 20 2-hit fillers and both fill-100 runs said `num_matched_peptides="51"`. No Linux-only skip: it drives `comet.exe` directly, not `rts_repro`. |
| **T70** (`t70_fi_fragment_charge`) | Fragment-index lookup of multiply-charged fragment peaks (`docs/20261108_FI_fragment_charge_lookup.md`): `SearchFragmentIndex()` converts a peak seen at charge z to the indexed 1+ mass as mz * z - (z - 1) * proton; it used to subtract (z - 1) * 1.0, a 0.0073 Da error per extra charge. `tests/unit/data/t70_make_highcharge.py` writes a FASTA with a 23-residue tryptic target and 30 fillers, and a 3+ `.ms2` scan holding only those of the target's 2+ b/y ions that the old conversion looked up in the wrong bin, at least 0.1 bin from any edge under both conversions so float peak storage cannot flip them (12 peaks). Searched with `comet.exe` against a fragment ion index and a peptide index (`digest_mass_range` widened to 600-5000): both must return the target at rank 1. Before the fix the FI search counted 0 index hits and returned nothing while PI found it. No Linux-only skip. |
| **T71** (`t71_rts_z1_ions`) | z+1 ions in the RTS result table that `SearchSingleSpectrumCore()` uses for the reported matched fragments and the chimeric removal bins. The scorer places z+1 (z', the z-dot ion plus one H) at y - NH2 + H; the table used y - NH3, 2.016 Da lower, so with `use_Z1_ions` the chimeric search never removed the winner's z+1 peaks and RTS listed z+1 matches at the wrong m/z. The table now takes its offsets from the scorer's own `massUtility` constants. One scan of T67's P1 `LVNELTEFAK` at 2+ with its 1+ b, y and z+1 ions (27 peaks), against T67's FASTA, through `rts_repro` with `RTS_REPRO_PARAMS="i:use_Z1_ions=1"`, on FI_DB and PI_DB: the chimeric charge-range search reports the peptide once and its `RTS_CHIMERIC_DEBUG` trace shows iteration 0 removing all 27 peaks (18 before the fix); a plain search with `RTS_REPRO_FRAGMENTS=1` lists all 9 z+1 ions at the scorer's mass (none before) and all 18 b/y ions. Linux only. |
| **T72** (`t72_chimeric_budget`) | `max_index_runtime` ends `DoSingleSpectrumSearchChimeric()` (`docs/20261105_RTSchimeric.md` 4.8). An expired core search returns 0 rows, and the loop's only budget check used to sit after its 0-row skip, so an exhausted budget never ended the call. One spectrum, `LVNELTEFAK`'s 1+ b/y ions plus 15 weak noise peaks, with 400 explicit-list 2+ candidates, its own m/z first, against T67's FASTA (FI_DB), through `rts_repro` mode 2 with `RTS_CHIMERIC_DEBUG=1` and the budget set by `RTS_REPRO_PARAMS="i:max_index_runtime=N"`. No budget: iteration 0 searches all 400 candidates and the call goes on to iteration 1. 1 ms: iteration 0 stops before the last candidate (after the first, which carries one-time setup), its trace line ends in `budget-expired`, and no iteration 1 runs; before the fix it searched all 400. 40 ms: when the trace shows the budget ran out after the target was found (~160 candidates here), the target is reported as iteration 0 and nothing else; on a machine where that timing does not occur the test prints a NOTE instead of checking it. Both budget runs set `bIncomplete`, so their output ends with `INCOMPLETE budget`; the unlimited run has no marker. Linux only. |
| **T73** (`t73_rts_invalid_inputs`) | Non-finite and invalid RTS inputs never crash a search (`docs/20261105_RTSchimeric.md` 4.8). A NaN fails every comparison, so "reject if too high" gates let a NaN precursor m/z reach preprocessing, which sized the query's arrays from it and crashed; a NaN `dMaxExpect` passed every E-value; a negative or `-inf` peak m/z was binned below the array start (crash) and a `+inf` intensity ruined the spectrum. `LVNELTEFAK`'s 1+ b/y ions plus 15 noise peaks against T67's FASTA, through `rts_repro` (which reads `nan`/`inf` with `strtod`), on FI_DB and PI_DB. Plain search (mode 0): the clean spectrum and the same spectrum with six invalid peaks (NaN, -50, -inf and +inf m/z; NaN and +inf intensity) give the same peptide and xcorr; NaN, +inf, -inf and negative precursor m/z return no result. Explicit list (mode 2): NaN, inf and negative candidates beside the valid one are dropped and the target is found once; invalid peaks are ignored; a scan with only invalid candidates returns false (ERROR row). Charge range (mode 1): invalid peaks ignored; a NaN m/z returns false. `dMaxExpect` NaN returns false; `+inf` is accepted as no cutoff. Before the fix plain, list and range all segfaulted and the NaN cutoff reported a row. Linux only. |
| **T74** (`t74_minimum_peaks_counting`) | `minimum_peaks` counts only the peaks preprocessing scores (`docs/20261108_minimum_peaks_counting.md`): m/z positive and finite, intensity finite, > 0 and at or above the `minimum_intensity` / `percentage_base_peak` cutoff; it used to count every peak except `clear_mz_range` ones. Six 2+ `LVNELTEFAK` spectra against T67's FASTA (FI_DB): 8 or 12 of its b/y ions, each padded with 5 extra peaks. The 8-ion spectra are padded with peaks that do not count, in three ways: zero intensity (defaults), intensity 2 under `minimum_intensity` 10, and intensity 1 under `percentage_base_peak` 0.05 (base 100). In each setting the padded 8-ion spectrum is not searched and the 12-ion one is searched and finds the peptide, in batch (`comet.exe`, .txt rows) and RTS (`rts_repro` mode 0, settings through `RTS_REPRO_PARAMS`). Before the change, every padded 8-ion spectrum was searched and identified. The RTS half needs Linux; the batch half also runs with a Windows build. |
| **T75** (`t75_chimeric_exact_duplicates`) | Chimeric explicit-list candidates: only exact duplicates are dropped (`docs/20261105_RTSchimeric.md` 4.8). The old rule dropped a candidate when an earlier same-charge one lay within the incoming candidate's tolerance, so the result depended on list order. `LVNELTEFAK` (2+, m/z 582.319) b/y ions plus 15 noise peaks against T67's FASTA (FI_DB), through `rts_repro` mode 2 with `RTS_CHIMERIC_DEBUG=1`, candidates carrying their own tolerance in the fixture's optional sixth SPECTRUM field. 581.75 and 582.20 at +/- 0.5 m/z: the peptide is found at iteration 0 and both candidates are searched in either order (before: 581.75 first searched only itself and found nothing). Two exact duplicates plus a candidate 0.17 ppm away: 2 searches. The same m/z at tolerances 0.5 and 1.0: 2 searches (before: 1). The same m/z at 0.5 twice: 1 search. Linux only. |
| **T76** (`t76_rts_shared_bins`) | The RTS result extraction (matched fragments, chimeric removal bins) is built from the shared bin builder, so it covers exactly the bins `XcorrScoreI()` credits (`docs/20261108_shared_bin_builder.md` step 2). Its old hand-maintained loop subtracted one fragment neutral loss even when `scale_fragmentNL` scores k losses, and never touched `precursor_NL_ions` peaks. `LVS[80]ELT[80]EFAK` (phospho on S3 and T6, loss 97.976896; `max_variable_mods_in_peptide` 3) at 2+, its own FASTA with 40 fillers, through `rts_repro` with `RTS_CHIMERIC_DEBUG=1` and `RTS_REPRO_PARAMS`. FI_DB, `scale_fragmentNL = 1`: a spectrum of the 18 b/y ions plus each phosphorylated ion at ion - k x loss (30 peaks) -- iteration 0 removes all 30 (24 before: the six 2x-loss peaks stayed). Adding `precursor_NL_ions = 97.976896` and its 2+ and 1+ precursor-loss peaks: 32 of 32 (24 before). PI_DB with the same scaled spectrum: 30 of 30 (24 under PI's pre-2026-11-08 rules, which never scaled). PI/FI parity (`docs/20261108_shared_bin_builder.md` section 5): plain searches give the same xcorr on FI_DB and PI_DB for the doubly-phosphorylated peptide with `scale_fragmentNL` (6.795; old PI 5.59) and for `LVS[80]EGLEM[16]AK` with a phospho S3 and an oxidized M8 carrying a 63.998285 loss (7.984; old PI 7.144, which credited only one mod type per ion series). Plain search with `RTS_REPRO_FRAGMENTS=1` on FI_DB: the six doubly-phosphorylated ions are listed at ion - 2 x loss and the six singly-phosphorylated ones at ion - loss (no 2x-loss fragments before). Linux only. |
| **T77** (`t77_rts_charge_limits`) | Precursor charge limits in RTS (`docs/20261105_RTSchimeric.md` 4.8). The chimeric charge-range overload used to take the requested range as given, and neither RTS entry point honored `min_precursor_charge` / `max_precursor_charge`. `LVNELTEFAK` 2+ (b/y ions) against T67's FASTA (FI_DB), through `rts_repro`. Chimeric charge range: 10..12 returns false (ERROR row; before, true with nothing searched); 2..12 and 2..2147483647 (with `max_precursor_charge` 9) find the peptide with at most 8 candidate searches (before, 2..INT_MAX aborted with `std::bad_alloc`); 2..4 under `min_precursor_charge` 3 skips the 2+ candidate and finds nothing (before, it found the peptide). Plain search: found with the defaults; not searched under `min_precursor_charge` 3 or `max_precursor_charge` 1 (before, found in both). Linux only. |
| **T78** (`t78_wrappercheck`) | CometWrapper input validation (`docs/RealTimeSearch.md`, C# / wrapper integration). Runs `RealtimeSearch.exe wrappercheck` (`RealtimeSearch/WrapperCheck.cs`) from the Windows build against a fragment ion index of T67's FASTA and a `LVNELTEFAK` 2+ fixture. Each of the four RTS calls -- `DoSingleSpectrumSearchMultiResults`, `DoSingleSpectrumSearchChimeric` in list and charge-range form, `DoMS1SearchMultiResults` -- is given six kinds of bad peak arrays: null mass, null intensity, empty with `iNumPeaks` 0, empty with 5, `iNumPeaks` one larger than the arrays, and an intensity array 5 shorter. Each must return false with empty, non-null outputs, no exception, and a `GetLastWrapperError()` reason. Before the fix, `&pdMass[0]` on a null or empty array throws (`NullReferenceException`, `IndexOutOfRangeException`) by C++/CLI semantics, and a count larger than the arrays reads past them. That is from the code: the check cannot run against the old wrapper, which lacks `GetLastWrapperError()`. A list-form call with 2 m/z values and 1 charge is rejected. The three MS2 calls then find the peptide with valid arrays and with buffers 20 longer than `iNumPeaks`: 31 checks in all. Skips when `RealtimeSearch/bin/x64/Release/RealtimeSearch.exe` is not built. Native-exception containment is not exercised: the only known trigger was an unclamped `chimeric_remove_isotopes`, which loops rather than throws on Windows, and it has been clamped since T81. |
| **T79** (`t79_runner_accounting`) | The runner's own accounting (code review item 12; see "Skips are counted as skips" above). Each check runs `run_tests.py` code in a fresh Python process. (a) A probe test that raises `TestSkipped` is reported by `main()` as `0 passed, 0 failed, 1 skipped`, with its reason listed. (b) With `RTS_REPRO_CPP` pointed at a copy of `rts_repro.cpp` with a syntax error, T67 returns a "failed to build" failure carrying the compiler error instead of skipping. (c) A copy of the `rts_repro` binary backdated an hour, with an extra fake library newer than it added to `RTS_REPRO_LIBS`, is rebuilt. (b) and (c) need `g++`; without it the test skips after (a). |
| **T80** (`t80_rts_harness_drift`) | A drift report, not a parity gate. It compares the literal search parameters `RealtimeSearch/SearchMS1MS2.cs` sets in `ConfigureInputSettings()` before its database branch with `rts_repro.cpp`'s defaults, using `tests/rts_repro/harness_param_drift.py`, which can also be run by hand. It prints a NOTE for each value that differs and each parameter only the harness sets. It never fails on drift, since the harness is an example program whose settings change and the RTS tests pin their own parameters. It fails only when either file yields fewer than 15 parameters, meaning the parser has gone stale. On 2026-11-08 it reports no differing values, plus `min_precursor_charge` 1 and `minimum_intensity` 0, which only the harness sets, to Comet's defaults. On the previous `rts_repro.cpp` it reported `isotope_error` 0 vs 2 and `max_index_runtime` 100 vs 500. |
| **T81** (`t81_chimeric_knob_clamps`) | Chimeric removal knobs (code review item 14; `docs/20261105_RTSchimeric.md` 4.5). T67's chimeric fixture runs on FI_DB through `rts_repro`'s charge-range mode with `RTS_CHIMERIC_DEBUG=2`. `chimeric_remove_isotopes` outside 0..3 is clamped with a warning: `INT_MAX` gives byte-identical output to 3 and finishes, where before it ran past the 300 s test timeout; -5 gives the output of 0. The removal flank follows `theoretical_fragment_ions` and has no knob. Setting `chimeric_remove_flank_bins` to 0 or 1000 through `RTS_REPRO_PARAMS`, or passing 0 or 1000 as `rts_repro`'s ignored `flankBins` argument, gives byte-identical output and exactly the same removed bins (the `RTS_CHIMERIC_DETAIL` lists) as the defaults. The argument also prints a note. The removed-bin lists discriminate: when the knob still existed, a flank of 0 removed 55 bins per kept PSM where the default removed 163. Before, a flank of 1000 lost the second peptides of scans 2 and 3. The defaults print no warning. Linux only. |
| **T82** (`t82_chimeric_cancel_keeps_rows`) | A cancelled chimeric call keeps its complete iterations (code review item 15; `docs/RealTimeSearch.md`, `bIncomplete`). `DoSingleSpectrumSearchChimeric()` returns false with `bIncomplete` when cancelled, keeping the rows of the iterations completed before. It used to return false with no way to tell those rows were valid, and every caller dropped them. 600 calls, T67's scans 1, 2, 3 and 6 copied 150 times with unique scan numbers, run on FI_DB through `rts_repro`'s charge-range mode with 4 workers. A reference run has no race; a second run has `RTS_REPRO_CANCEL_RACE=300`, which cancels and resets the search at random points. Invariants: no `ERROR` line, meaning no false without `bIncomplete`; every `INCOMPLETE stopped` call's rows are a prefix of the same scan's reference rows; every call without a marker matches the reference exactly; every call produced output; and both cancelled and completed calls occurred. A typical run has about 440 cancelled calls, over 100 of which returned rows. The race is timing-dependent, but the invariants are not. Linux only. |
| **T83** (`t83_rts_trace_range_topn`) | Three small RTS contracts (review follow-ups, 2026-11-09). (a) `COMET_EVALUE_TRACE`'s `tope` is the rank-1 E-value Comet reports: P1's spectrum plus 28 random-noise spectra at its m/z, searched through `rts_repro` mode 0 with a +/- 300 Da window and `fragindex_min_ions_score/report` 1, so the noise reports weak hits, 13 of them at E = 999; every reported row is paired with its trace line by xcorr (all RTS queries trace as `scan=0`). The trace used to print the raw fit (1490-1749 for a reported 999). (b) A chimeric candidate whose MH+ (1162.59) lies just below the index's `MassRange` (1163-2000) with a +/- 1.0 m/z window finds P1 (MH+ 1163.63) in both the explicit-list and charge-range forms; the chimeric gate used to reject anything below `dPeptideMassLow`, which `DoSingleSpectrumSearchMultiResults()` never did. (c) `RTS_REPRO_TOPN=-1` and `0` give NO_MATCH (the call returns false) and `1` finds P1; `-1` used to abort `rts_repro` with `std::length_error`. All three parts fail on the code before the fix. |
| **T84** (`t84_searchchimeric_errors`) | `RealtimeSearch.exe chimeric` (`SearchChimeric.cs`) reports bad input instead of crashing (review follow-up, 2026-11-09; Windows build, skipped when `RealtimeSearch.exe` is absent). On T67's `.ms2` and an FI index it exits 0 and writes LVNELTEFAK rows. A corrupted first peak line is reported as `<file> line <n>: cannot parse "<line>"`, an `--out` path in a missing directory as `cannot write`, both before the index loads, and bad usage prints the usage; each exits 1 with no unhandled exception or stack trace. A parse error used to be an unhandled `FormatException`, and the output file was opened only after the whole search. Not covered: the per-scan catch in the worker, which no input reaches now that the wrapper contains native exceptions (T78). |
| **T85** (`t85_evalue_outputs`) | Which outputs make a batch search compute E-values (`Options::OutputsNeedEValues()`, 2026-11-09). On the T68 fixture (one target, 50 sub-floor fillers) searched by `comet.exe` against an FI index: an mzIdentML-only search with `print_expect_score = 0` writes the same E-value as with 1 (3.86E-09), not 9.99E+02 -- mzIdentML was missing from the four hand-written copies of the test, so before the fix every PSM got 999 (the pre-fix binary fails exactly those two checks) -- and the histogram fill still runs for it (fill 100 gives a larger E-value than fill 0). An SQT-only search with `print_expect_score = 0`, which needs no E-values and so now runs with the fill off, writes the same SQT with the fill at 100 and 0; with `print_expect_score = 1` the fill still runs and moves the SQT's E-value. The skip itself is a speed change and is not visible in the output. |
| **T86** (`t86_preprocess_verify`) | The `COMET_PREPROCESS` verify harness for the sparse-preprocessing work (`docs/20261109_sparse_preprocessing.md` section 5, phase 0; Linux only, since a Windows binary does not see the environment variable from WSL). `COMET_PREPROCESS=verify` rebuilds every preprocessed spectrum into a heap-allocated shadow `Query` and compares the fast-xcorr, NL and Sp sparse matrices, their sizes and `iMinXcorrHisto` bit for bit. On T67's fixture, through `rts_repro` (plain search and the chimeric list form; high resolution, low resolution 1.0005 / 0.4 / `theoretical_fragment_ions` 1, and `use_NL_ions` 1) and batch `comet.exe` (FASTA and FI_DB, the fused path): every spectrum is compared with zero mismatches, and the results equal a run without the variable (RTS lines compared sorted, since two worker threads reorder them). `COMET_PREPROCESS=verify-selftest` perturbs one stored shadow value, and every comparison must then report a mismatch -- the check that the comparator can fail. With the variable unset nothing is printed. Phase 1 (2026-11-09): on the RTS path the primary is now the sparse build (`BuildSparseMatricesFromPeaks()`) and the shadow the dense one, so every RTS comparison is sparse vs dense; batch is still dense vs dense until phase 2. Five synthetic edge-case spectra -- a peak in the precursor-mass cutoff gap, peaks below bin 75, isolated bands and one overlapping cluster, peaks below the 5% normalization cut, two peaks in one bin, peaks at the precursor and neutral-loss m/z values, a precursor near the top of the mass range, a 3+ precursor -- must match under ten settings: high resolution, `theoretical_fragment_ions` 1, NL ions with and without flanking peaks, low resolution with and without flanking peaks (sparse forced), and `remove_precursor_peak` 1-4. The verify summary also counts sparse builds the gap-residue guard sent to the dense build. Phase 2: batch `comet.exe` (FASTA and FI_DB) now builds sparse too, so its comparisons are sparse vs dense as well; verify mode also poisons the scratch buffers when they are allocated, so a build that relies on memory it never cleared fails here instead of passing on freshly zeroed pages (with the phase-2 first-use-clear bug reintroduced, 26 checks fail). Phase 3: the default (`auto`) now takes the sparse build wherever the section 4.6 bound picks it, and T86 also checks that runs without the variable give the same results as `COMET_PREPROCESS=dense`, RTS and batch. A 4-thread FASTA batch case (the fixture's spectra repeated 80 times with fresh scan numbers) compares the default build (twice) and verify mode against dense: the batch pool hands one raw buffer to different threads, and a per-thread record of the bins it set once left stale values behind (2026-11-10). **Windows `--comet`** (2026-11-10): the batch FASTA, FI_DB and 4-thread parts run, with the variable passed through `WSLENV` -- so the sparse build is checked against dense on the MSVC `/fp:fast` build too (zero mismatches; 10 spectra per run, 800 in the threaded case); the real Windows RTS path runs through `RealtimeSearch.exe chimeric` (`CometWrapper.dll`, both chimeric forms, `elapsed_ms` ignored in the comparison; 36 and 19 spectra, zero mismatches), but its command line sets no low-resolution or NL parameters, so those settings and the edge-case spectra stay `rts_repro` (Linux build) only. A low-resolution FI_DB batch case (Linux) runs on a heap pre-filled with `MALLOC_PERTURB_` with dense scans first and then 24 scans of a long peptide with 12 peaks (four above the dense scans' cleared range), and the default build must equal dense: the RTS/fused scratch is allocated uninitialized and a buffer first used by a dense spectrum was once cleared only over that range, so a later sparse spectrum read leftovers (2026-11-10; fails on the pre-fix binary). |
| **T87** (`t87_decoygen_verify`) | The two decoy-score paths of `GenerateXcorrDecoys()` (`docs/20261109_decoy_gather.md`). The scatter walks the spectrum's nonzero bins through the inverted decoy index; the gather sums each decoy's own ions from a dense copy of the spectrum, eight decoys at a time, in each decoy's scatter order, so the scores are bit-identical. `COMET_DECOYGEN=verify` runs both on every decoy set, compares them bit for bit, and prints a summary that also counts which path `auto` would take and how many sets had each fragment-charge limit (1-5). **Linux build:** T67's fixtures plus a 4+ and a 5+ scan of the first scan's peptide (so charge-limit tables 3 and 4 are read; the T67 scans reach 1 and 2) through `rts_repro` -- the plain search and both chimeric forms (charge range 2+..5+, explicit list) -- at high resolution, low resolution (1.0005 / 0.4 / `theoretical_fragment_ions` 1) and with a/b/y ions and `max_fragment_charge` 4 -- and batch `comet.exe` on the FASTA, an FI_DB index and a PI_DB index. Every run must show zero mismatches; every charge-limit table the setting should reach is read; at low resolution auto takes the gather for most sets (an inverted auto rule fails); `verify-selftest` (the last score of the discarded scatter copy perturbed by 1000) flags every comparison, and each printed mismatch line must name the query (precursor MH+, `iArraySize`, `iLoopMax`, charge limit), since every RTS query has scan number 0; `scatter`, `gather`, the default, `verify` and `verify-selftest` all give identical results (the self-test first perturbed the array the histogram bins, so a self-test run changed the output; reverting that fails four checks); nothing is printed with the variable unset or forcing a path. **Windows `--comet`:** the batch FASTA, FI_DB and PI_DB parts run (the variable reaches the binary through `WSLENV`, paths through `_to_win`, and `_rts_build_index()` builds the index with the Windows binary); the Windows RTS path runs through `RealtimeSearch.exe chimeric` (`CometWrapper.dll`, both chimeric forms with the 4+/5+ scans, default settings only; the charge-limit tables 1-3 are read), while the low-resolution and a/b/y settings need the Linux `rts_repro`. The Windows Comet.exe built 2026-11-10 gives the same decoy-set counts and choices as Linux. |
| **T88** (`t88_reinit_decoy_tables`) | Same-process re-initialization of the E-value decoy tables, built once per process (`docs/20261109_decoy_gather.md` section 5). `tests/unit/reinit_driver.cpp` runs several batch searches in one process the way an embedding host re-initializes; the test builds it from the sources (`Comet.cpp` with `main` renamed, plus `CometPostAnalysis.cpp` compiled with AddressSanitizer when the toolchain has it, so an out-of-bounds write in the decoy code aborts the run) into `tests/unit/reinit_driver` (gitignored), independent of `--comet`. The same parameters twice give identical results. A second session with a changed ion series (b+y -> c+z, same count), fragment mass type, `max_fragment_charge` or `fragment_bin_tol` is refused with the stale-table error: the first session's output is written, the second's is not, the driver exits 1. A second session with `digest_mass_range` 2000 -> 5000, searching a 30-residue peptide (MH+ 3377, above the 3000 the first session's arrays are sized for) with the gather forced, completes under AddressSanitizer and equals a fresh process (scatter). A low-resolution single session searching a 3377 Da peptide under a 3400 Da `digest_mass_range` ceiling -- the query's array ends 23 bins below the global one, closer than `xcorr_processing_offset` -- must stay in bounds and find the peptide: the dense fast-xcorr loop's max once ran 75 bins past the query's array, which padding on the scratch arrays covered; the driver compiles `CometPreprocess.cpp` with AddressSanitizer too, so a read past an array aborts the run. Mutations that fail it: the ion-series list not compared (2 checks), the fragment mass constants not compared (2), the table bound / dense copy sized from `iArraySizeGlobal` as before the 2026-11-10 fix (2, an AddressSanitizer abort), and the max loops unguarded with the padding removed (2, an AddressSanitizer abort). |

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
used by `CometUnitTests` below. `fixture_peptides.py` holds the one copy of the residue,
proton and water masses and the b/y ladder and `SPECTRUM`-block helpers that the RTS tests
and the fixture generators in `data/` (`t67_make_chimeric.py`, `t68_make_histfill.py`,
`t70_make_highcharge.py`) build synthetic spectra from (2026-11-09; before, nine tests and
three generators each carried their own, and `run_tests.py`'s proton and water masses
differed from the generators' in the seventh decimal).

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
| `TestModificationsPermuter.cpp` | `PermuterTest` | 18 | `ModificationsPermuter` (P1-P18): sentinel terminal-slot positions in the mod sequence, residue-only sequences without sentinels, mod-char translation, terminal + residue entries, terminal mods counting toward the per-peptide and per-mod caps, protein-N-term-only and both-termini cases, zero-combination/overflow guards, determinism, upstream overlapping-K mods, and the `PROT_*_HERE` context-flag rule that a both-termini variant needs one occurrence carrying both bits; P14-P17: variable_mod fifth/sixth-field position rules (peptide-N, `-2`, peptide-C and protein-N rules; position classes in the dedup key; `getPositionClass()` terminal-site rules, incl. an n-term site under a which_term 3 rule admitted when L-1 <= d; P18: positions a restricted mod cannot take do not split the dedup key). |
| `TestMSReaderHeaderOnly.cpp` | `MSReaderHeaderOnlyTest` | 5 | `MSReader`'s header-only mode (`setHeaderOnly()` / `lastReadHeaderOnly()`), the reader contract the `fragindex_skipreadprecursors = 0` pre-read relies on, on an mzXML (and an ms2) the tests write to the temp directory: a header-only read returns scan, precursor m/z and charge without peaks; header-only -> full read of the same scan -> header-only on one reader has no stale state either way; a read that returns peaks (full read, another format, ms2) never leaves the flag set; 200 header-only reads in a row stay consistent. Verified to fail (`FlagDoesNotLeakIntoOtherFormats`) with the flag resets in `readFile`/`readMSTFile` removed. |

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
| `internaldecoy1` | 1 (internal decoy, concatenated) | `data/comet_phospho_internaldecoy1.params` | fasta, fi, pi |
| `internaldecoy2` | 2 (internal decoy, separate) | `data/comet_phospho_internaldecoy2.params` | fasta, fi, pi |

Every variant runs in every mode. FI_DB internal decoys (`docs/20260914_FI_internal_decoys.md`,
also covered by T34/T40/T24b in `run_tests.py`) exist since 2026.03.0, so against an older
baseline the `fi` + internal-decoy combinations compare a current build that searches decoys
with a baseline that searches its FI index without them. Those rows are run "target-side only":
the current binary builds and searches with `decoy_search = 2`, so its decoys go to a separate
`.decoy.txt` and the compared file holds every spectrum's best target regardless of how many
decoys outscore it (a concatenated run could push it below `num_output_lines`); PSMs whose
protein starts with the params' `decoy_prefix` are dropped from both compared files as a guard.
The decoy-side
numbers (and the `decoy_search=2` `.decoy.txt` comparison) are not a comparison until the
baseline is 2026.03.0 or later.

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
# build: single g++ line in rts_repro/README.md (no Makefile); run_tests.py builds it itself (_rts_repro_status():
#   rebuilt when older than rts_repro.cpp or any linked library; a failed build fails the RTS tests)
./rts_repro <database.idx> <fixture_file> <num_threads> <output_file> [ascorepro:0|1] [index_search_type:0=PI_DB|1=FI_DB]
# chimeric search (T67; docs/20261105_RTSchimeric.md): trailing [chimeric:0|1|2] [zmin] [zmax] [tolMZ] [maxExpect] [maxPeptides]
#   [flankBins] [isotopes] [isotopeError] [globalMinIonsScore] [histogramFill] [useNLIons]
#   1 = charge-range form per SPECTRUM block; 2 = every block sharing a scan number is one (m/z, charge) candidate;
#   flankBins is ignored (the removal flank follows theoretical_fragment_ions); isotopes is chimeric_remove_isotopes (2, 0..3);
#   isotopeError sets isotope_error in every mode (2);
#   globalMinIonsScore > 0 sets fragindex_min_ions_score, histogramFill >= 0 sets fragindex_histogram_fill,
#   useNLIons >= 0 sets use_NL_ions, all in every mode (not sent by default); maxExpect defaults to 0.1
# fixture numbers are read with strtod, so nan / inf / -inf load as those values (T73)
# fixture SPECTRUM lines take an optional 6th field, a candidate's own tolerance in m/z for chimeric mode 2 (T75)
# environment: RTS_REPRO_PARAMS="<i|d|s|v>:<name>=<value>;..." (v = comma-separated doubles) sets any further parameter, applied after the
#   built-in ones (e.g. "i:use_Z1_ions=1"; the type must be the one Comet reads it as -- a mismatch crashes);
#   RTS_REPRO_FRAGMENTS=1 adds a FRAG line per matched fragment (type = ION_SERIES_*, mass = 1+ mass);
#   RTS_CHIMERIC_DEBUG=1|2 traces the chimeric iterations; RTS_REPRO_CANCEL_RACE=<us> cancels/resets the search at random points (T82);
#   RTS_REPRO_TOPN=<n> sets mode 0's topN (default 1; T83)
# chimeric output: a call that stopped early keeps its rows and adds 'INCOMPLETE budget' (true) or 'INCOMPLETE stopped' (false); ERROR = invalid arguments
python3 tests/rts_repro/ms2_to_fixture.py in.ms2 > fixture.txt
python3 tests/rts_repro/mzxml_to_fixture.py in.mzXML [--max N] > fixture.txt   # MS2 scans of an mzXML (real data on Linux)
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
