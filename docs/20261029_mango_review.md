# Mango search path review (2026-10-29)

Review of how Comet currently handles the `mango_search = 1` option: how the paired
precursors from a Mango-generated `.ms2` file become separate searches, how the results
are labeled on output, and what the code does *not* do today. Written before any Mango
extension work on the `Mango` branch so the starting point is on record.

## 1. Purpose of the option

A cleavable cross-linker joins two peptides. After cleavage both peptides are released into
the same MS/MS scan, each carrying a stump modification (+197.032422 on an internal lysine).
An external program writes a `.ms2` file in which every scan has an even number of `Z`
lines: consecutive pairs of `Z` lines are the two released peptides' precursors (charge and
MH+). Comet searches each `Z` line as its own query against the one shared peak list and
labels the output so the two halves of a pair can be matched back together downstream.

Comet itself knows nothing about the cross-linker chemistry or the stump mass. The +197.032422
modification has to be supplied by the user as an ordinary `variable_mod` on K. Nothing in
the source references that mass (`grep 197.03` only hits decoy tables in `CometDecoys.h`).

## 2. Parameter plumbing

| Step | Location |
|------|----------|
| Parameter registered as an int | `Comet.cpp:368` (`"mango_search"` in the parse table) |
| Copied into globals | `CometSearchManager.cpp:774-780` sets `g_staticParams.options.bMango` |
| Field definition / default | `core/Params.h:62` (`bool bMango`), default `false` at `core/Params.h:381` |

`bMango` is read-only after init like the rest of `g_staticParams`.

## 3. How the `Z` lines become separate queries

### 3a. MSToolkit parsing (`MSToolkit/src/MSToolkit/MSReader.cpp`)

- `S` line (`:986`): scan number, and the m/z in the fourth column becomes `spec.getMZ(0)`.
  An `.ms2` scan therefore always has exactly one precursor m/z entry (`sizeMZ() == 1`).
- `Z` line (`:1027`): each line is parsed into a `ZState { z, mh }` and appended with
  `addZState()`. N `Z` lines give `sizeZ() == N`.

### 3b. Charge/precursor expansion (`CometSearch/CometPreprocess.cpp`)

The same block is duplicated in three functions. All three build a
`vector<pair<int,double>> vChargeStates` (charge, m/z):

| Function | Lines | Used by |
|----------|-------|---------|
| `ReadPrecursors()` | `:543` (loop at `:641-782`) | FI_DB pre-pass that marks precursor mass bins (`FiStrategy.cpp:77`) |
| `PreprocessSpectrum()` | `:2511` (loop at `:2527-2671`) | Legacy batch path (FASTA_DB; and FI_DB / PI_DB when Mango or speclib is on) |
| `FusedSearchSpectrum()` | `:3428` (loop at `:3444-3556`) | Fused FI_DB / PI_DB path -- never taken for Mango, see section 5 |

Flow for a Mango scan (`sizeMZ() == 1`, `sizeZ() == N > 1`), in the default
`override_charge = 0` case:

1. The outer `for (i < sizeMZ())` loop runs once (`i == 0`).
2. `sizeMZ() == 1 && sizeMZ() < sizeZ()` branch: `iSpectrumCharge = atZ(0).z`
   (`:2535-2540`).
3. `dMZ = getMonoMZ(0)`, falling back to `getMZ(0)` (the `S`-line m/z), and only if that is
   also 0 to `atZ(0).mh / iSpectrumCharge` (`:2559-2573`).
4. Entry 0 pushed as `(atZ(0).z, dMZ)` (`:2634`).
5. Entries 1..N-1 pushed as `(atZ(ii).z, (atZ(ii).mh + PROTON*(z-1)) / z)` (`:2637-2643`),
   i.e. each extra `Z` line's own MH+ converted back to m/z.
6. The precursor loop (`:2671`) creates one `Query` per entry, computes
   `dExpPepMass = mz*z - PROTON*(z-1)`, applies `peptide_mass_low/high` and
   `min/max precursor charge` filters, and stamps
   `pScoring->dMangoIndex = iScanNumber + 0.0001 * distance(vChargeStates.begin(), iter)`
   (`:2684`).

Every one of those `Query` objects then shares the same binned peak data from the same scan;
only `dExpPepMass`, charge and the derived fragment-charge limit differ. Nothing downstream
treats them as linked -- scoring, E-value, delta-Cn and output are all per-query.

Note on step 3/4: the first precursor uses the `S`-line m/z rather than `Z[0].mh`, while
all later precursors use their `Z`-line MH+. This is only equivalent if the generating
program writes the `S`-line m/z to match the first `Z` line. Worth confirming against a
real Mango `.ms2` before extending the path; the repo has no fixture with multiple `Z` lines
per scan (checked every `.ms2` under `tests/`).

Also note `override_charge` / `precursor_charge` interactions: if `override_charge = 1` the
`Z` lines are ignored entirely and `dMZ` (the `S`-line m/z) is expanded over the charge
range (`:2580-2585`); only the plain `override_charge = 0` branch walks the extra `Z` lines.

### 3c. `dMangoIndex` (`core/Types.h:1139`)

`double dMangoIndex` on `Query` encodes `scan + 0.0001 * precursorOrdinal`, e.g. `2401.0000`,
`2401.0001`, `2401.0002`, `2401.0003` for a four-`Z`-line scan 2401. It exists solely to
recover original file order after the query vector has been sorted by mass. Resolution is
0.0001, so up to 9999 precursors per scan before ordinals collide.

## 4. Pair labeling on output

### 4a. Label assignment (`CometSearch/search/SearchUtils.cpp:229-250`)

`RunSearchAndPostAnalysis()` runs once per batch of queries. When `bMango` is set, before
the mass sort it:

1. Sorts `session.queries` by `dMangoIndex` (`compareByMangoIndex`, `SearchUtils.h:52`).
2. Walks the sorted vector, resetting a counter `iMangoIndex` to 0 whenever the scan number
   changes and incrementing it otherwise.
3. Writes `szMango` (`char[32]` in `SpectrumInfoInternal`, `core/Types.h:111`) as
   `"%03d_%c"` with `iMangoIndex / 2` and `'A'` for even / `'B'` for odd.

So consecutive `Z` lines are paired by position: ordinals 0,1 -> `000_A`, `000_B`;
ordinals 2,3 -> `001_A`, `001_B`; and so on. The pairing is purely positional in the input
file; Comet never checks that the two masses sum to anything or that both halves were
actually searched.

Caveat: the label is derived from the ordinal of the *surviving* queries, not the `Z`-line
ordinal. If one `Z` line in a pair is dropped by the mass or charge filters in
`PreprocessSpectrum()` (or by `AdjustMassTol()`), the remaining entries shift and later
pairs get mislabeled (e.g. the second pair's first member would be tagged `000_B`). The
0.0001 fraction in `dMangoIndex` holds the true ordinal but is not used for the label.

Second caveat: the label depends on both members of a pair, and in fact all queries of a
scan, being in the same batch. `PreprocessSpectrum()` emits all of a scan's queries
together, so a scan is never split across batches today, but any future change to batching
would have to preserve that.

### 4b. Output writers

| Writer | Mango handling |
|--------|----------------|
| `CometWritePepXML.cpp:191` | `search_engine_version` is prefixed with `"Mango "`. |
| `CometWritePepXML.cpp:447-454` | `spectrum_query spectrum=` attribute becomes `<base>_<szMango>.<scan>.<scan>.<z>` instead of `<base>.<scan>.<scan>.<z>`. |
| `CometWriteTxt.cpp:276-296` | An extra tab-separated column with the same `<base>_<szMango>.<scan>.<scan>.<z>` string is inserted right after the `scan` column in every result row. |
| `CometWriteTxt.cpp:86-110` | The header line is **not** adjusted; a Mango `.txt` file has one more data column than header column. |
| SQT, Percolator, mzIdentML writers | No Mango awareness at all. |

Example of the pepXML spectrum attribute for scan 2401, second pair, second member, 3+:
`myfile_001_B.02401.02401.3`.

## 5. Which search path runs

Both index strategies gate the fused per-spectrum path on `!bMango`:

- `search/FiStrategy.cpp:165-168`
- `search/PiStrategy.cpp:84-88`

With Mango on, FI_DB and PI_DB fall back to the legacy three-sweep batch path
(`executeBatchLegacy`: load a batch of spectra -> search -> post-analysis), the same path
FASTA_DB always uses. The reason is the label assignment in section 4a, which needs the
whole batch in one vector so it can be sorted by `dMangoIndex` before the mass sort. The
fused path searches each spectrum as it is read and never has the batch vector available
in that form. Real-time search (`DoSingleSpectrumSearchMultiResults`) has no Mango code.

Cost: Mango runs lose the fused path's speed and the FI batch optimization described in
`docs/20260611_batchFIoptimization.md`.

## 6. What Comet does not do today

- No notion of a cross-linked pair at search or scoring time. Each half is an independent
  PSM with its own E-value and delta-Cn computed against the full peak list, including the
  partner peptide's fragment ions.
- No check that both halves of a pair identified anything, or that the stump mod is present
  in the identified peptide.
- No mass-sum or complementary-mass validation between the two `Z` lines of a pair.
- No shared or combined score for the pair; pairing is left to downstream tooling via the
  `szMango` label.
- No Mango-specific test in `tests/unit/` and no multi-`Z`-line `.ms2` fixture.

## 7. Relevant history

| Commit | Change |
|--------|--------|
| `6ab38b72` | Add support for Mango cross-link pipeline (`bMango`, `dMangoIndex`, `szMango`, pepXML labeling). |
| `f52cfe8e` | Add the Mango-specific column to text output. |
| `5eddf930` | Further Mango support alongside unrelated fixes. |
| `f27ce821` | Cleanup; fixed the charge variable used for the Mango index. |
| `e40e50f5` | Precursor handling rework: `vChargeStates` became (charge, m/z) pairs so each extra `Z` line carries its own MH+ instead of reusing the `S`-line m/z. |
| `c0e01fd5` | Batch FI optimization; introduced the `!bMango` gate that keeps Mango on the legacy path. |

## 8. Files to touch for any Mango extension

- `CometSearch/CometPreprocess.cpp` -- three copies of the precursor expansion (`:641`,
  `:2527`, `:3444`); any change must be made in all three or consolidated first.
- `CometSearch/search/SearchUtils.cpp:229` -- label assignment.
- `CometSearch/core/Types.h:111,1139` -- `szMango`, `dMangoIndex`.
- `CometSearch/CometWriteTxt.cpp`, `CometSearch/CometWritePepXML.cpp` -- output.
- `CometSearch/search/FiStrategy.cpp:165`, `PiStrategy.cpp:84` -- path gating.

## 9. Measured impact on the TAC/SHAM iqPIR search (2026-10-29)

Checked the caveats in section 4a against the real run in
`/data/users/engj/20261016-idxtest/index/` (135 Mango `.ms2` inputs, each with a Comet
`.pep.xml.gz`, `comet.params` with `mango_search = 1`, `digest_mass_range = 600.0 5000.0`,
`min/max_precursor_charge = 1/6`, `override_charge = 0`). Script:
`docs/20261029_mango_shift_check.py`; per-file table: `docs/20261029_mango_shift_perfile.tsv`.

Method: parse every `S`/`Z` line, apply Comet's precursor filter to each `Z` line, relabel
the survivors the way `SearchUtils.cpp:229` does, and compare the predicted multiset of
(label, neutral mass, charge) with the `spectrum_query` elements in the pepXML. pepXML writes
a `spectrum_query` for every query, hits or not (`CometWritePepXML.cpp:47-57`), so the
comparison is exact. Predicted and observed agree on all 21,566,253 queries in all 135 files.

| Metric | Count |
|--------|------:|
| Scans | 1,277,072 |
| `Z` lines (one query each) | 21,596,140 |
| Pairs | 10,798,070 |
| `Z` lines dropped by the precursor filter | 29,887 |
| ... dropped because MH+ > 5000 (`digest_mass_range`) | 29,887 |
| ... dropped for MH+ < 600 or charge out of range | 0 |
| Scans with at least one dropped `Z` line | 4,792 |
| Scans whose A/B labels shifted | 4,321 |
| Queries carrying a wrong pair id and/or wrong A/B side | 77,156 |
| Pairs with at least one mislabeled member | 51,079 |
| Files with at least one shifted scan | 73 of 135 |

Notes:

- Every dropped `Z` line is the B (second, heavier) member of its pair, exceeding 5000 Da.
  The remaining 471 scans with drops but no shift lost only their final `Z` line, leaving an
  orphan A with no shift of earlier labels.
- The worst file (`092120_3707_TAC_iqPIR_F11_14_mango_4h_2`) has 505 shifted scans and
  6,913 mislabeled pairs; the F11_14 fractions dominate because their scans carry the most
  `Z` lines.
- Relative to the whole run this is 0.5% of pairs, but within an affected scan every pair
  after the first drop is wrong, so the downstream pairing of those PSMs is silently
  incorrect rather than merely missing.

### 9a. Second finding: first `Z` line mass is wrong for charge >= 2

The validation only matched once the model reproduced a mass error Comet makes on the first
`Z` line of every scan. The Mango `.ms2` writes the `S` line m/z as `0.0`, so
`PreprocessSpectrum()` reaches the fallback at `CometPreprocess.cpp:2574`:

```
if (dMZ == 0.0 && iSpectrumCharge != 0)
   dMZ = spec.atZ(i).mh / iSpectrumCharge;
```

`mh` is MH+, so the correct m/z is `(mh + PROTON*(z-1)) / z` (the formula used for `Z` lines
2..N at `:2641`). Dividing MH+ by z drops `(z-1)` protons: the query's `dExpPepMass` is low by
1.00728 Da at 2+ and 2.01455 Da at 3+. Only the first `Z` line of a scan is affected, and
only when its charge is 2 or more. In this run:

| First-`Z`-line charge | Queries affected |
|---|---:|
| 2+ | 353,898 |
| 3+ | 2,899 |
| 4+ | 11 |

Example from `020420_3702_TAC_iqPIR_F6_7_mango_2h_1`, scan 6305: input `Z 2 853.388576`;
pepXML reports `precursor_neutral_mass="851.374"` (should be 852.381).

With `isotope_error = 4` (offsets -1..3) and 20 ppm tolerance the 2+ cases are mostly
rescued: the -1.00728 Da error is 0.0039 Da from the -1 C13 offset, inside tolerance, so the
right peptide can still match. The 3+ and 4+ cases (2,910 queries) are off by two or more
protons and cannot be rescued by the isotope offsets. In all cases the reported
`precursor_neutral_mass` and the `massdiff` values in the pepXML are wrong for those queries.
The same fallback exists in `ReadPrecursors()` (`:685`) and `FusedSearchSpectrum()` (`:3474`).

## 10. Fixes applied (Mango branch, 2026-10-29)

Both section 9 bugs are fixed on the `Mango` branch; `tests/unit/run_tests.py` gained
`t67_mango` (fixture `tests/unit/data/t67_mango.{fasta,ms2}`) covering both.

**First `Z` line mass** -- the three `dMZ == 0.0` fallbacks in `CometPreprocess.cpp`
(`ReadPrecursors()`, `PreprocessSpectrum()`, `FusedSearchSpectrum()`) now convert MH+ to m/z
the same way the later `Z` lines already did:

```
dMZ = (spec.atZ(i).mh + PROTON_MASS * (iSpectrumCharge - 1)) / iSpectrumCharge;
```

**A/B label shift** -- `szMango` is now written when the `Query` is created in
`PreprocessSpectrum()` / `FusedSearchSpectrum()`, from the `Z`-line ordinal
(`distance(vChargeStates.begin(), iter)`), i.e. `ordinal/2` and `A`/`B` by parity. A `Z` line
rejected by the mass/charge filter never creates a `Query`, so it can no longer shift the
labels of the precursors after it. The old post-hoc relabeling block in
`SearchUtils.cpp:RunSearchAndPostAnalysis()` (sort by `dMangoIndex`, walk per scan, count
survivors) is removed, and with it `Query::dMangoIndex` and `compareByMangoIndex()`, whose only
purpose was that sort. `szMango` is zero-initialised in the `Query` constructor.

Not changed: Mango still forces the legacy (non-fused) batch path in `FiStrategy` /
`PiStrategy`; the gate is unrelated to labeling and was left for a separate decision. The
`.txt` writer still emits the Mango column without a header entry.

### 10a. Verification

- `t67_mango` passes on the fixed build (both peptides found, labels `000_A`, `001_A`, `001_B`
  with the rejected 6000 Da `Z` line leaving no `000_B` and shifting nothing).
- Re-searched `092120_3752_TAC_iqPIR_F6_7_mango_4h_1_low.ms2` (3,250 scans, 20,492 `Z` lines,
  23 rejected, 579 first-`Z`-line queries at 2+ or higher) with the fixed build and the run's
  `comet.params`: all 20,469 `spectrum_query` elements carry exactly the label derived from
  their `Z`-line ordinal and the correct MH+-derived neutral mass; zero mismatches.

The 135-file production run in `/data/users/engj/20261016-idxtest/index/` was searched with
the old code and still has the section 9 labels and masses; it needs to be re-searched to pick
up the fixes.
