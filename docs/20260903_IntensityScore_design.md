# Intensity score design: a Carafe predicted-intensity PSM score alongside XCorr (2026-09-03)

Companion to `docs/20260903_IntensityScore.md` (datasets, repo state). This document proposes
how a predicted-intensity score gets into Comet as a *separate* score function, selectable as
the primary search score via a user parameter, with XCorr untouched as the default.

Branch state: `IntensityScore` fast-forwarded to carafe head `ddcd4280` on 2026-09-03 (build +
57 fast tests pass). No scoring code written yet.

## 1. Findings that shape the design

Everything below was verified against the current tree; file:line refs are to `ddcd4280`.

**1.1 Two near-identical scorers, both bin-based.** `CometSearch::XcorrScore()`
(`CometSearch.cpp:4825`, FASTA path) and `CometSearch::XcorrScoreI()` (`CometSearch.cpp:8427`,
FI_DB + PI_DB). Both receive the peptide's fragment ladder *already binned*
(`uiBinnedIonMasses[charge][series][ladderPos][nlSlot]`) and read one float per bin from
`Query::ppfSparseFastXcorrData`. Fragment m/z values are gone by score time, but the ladder
index `ladderPos` is preserved positionally, which is exactly the coordinate the Carafe
predictions use (b(j+1) / y(nAA-1-j) at row j).

**1.2 Observed intensities already survive preprocessing.** `Query::ppfSparseSpScoreData`
holds binned `sqrt(intensity)`, max-per-bin, normalized to 100 (`CometPreprocess.cpp:1577`).
Today only `CometPostAnalysis::CalculateSP()` reads it (`FindSpScore()`,
`CometPostAnalysis.cpp:1447`). It is the natural observed vector for an intensity score:
same `BIN()` coordinate as XCorr, zero extra memory, and it is *not* background-subtracted
the way the XCorr array is.

**1.3 The candidate identity is the same 4-tuple the mask uses.** At score time both
indexed paths hold `(iWhichPeptide, modNumIdx, cNtermMod, cCtermMod)` (`CometSearch.cpp:
1783-1787` FI_DB; `:2298/:2319` PI_DB), plus a dense variant index (`uiWhichVariant` into
`g_fragmentPeptides`, or position `i` into `g_dbIndexVariants`). `CometPredictedMask`
already proves the loading pattern: key-sorted table guarded by `.idx` CRC-32 fingerprint +
`VarModConfig` string, resolved once into a positional cache by variant index, then the
key table is freed (`CometFragmentIndex.cpp:384-417`). The FASTA path has no variant
identity, so the intensity score is an **indexed-search feature only** (FI_DB, PI_DB, RTS).

**1.4 XCorr is load-bearing far beyond the score itself.** A primary-score switch must
re-point all of these, or candidates the new score likes get silently pruned/hidden:

| Coupling | Where |
|---|---|
| Early-reject gate `dXcorr + 0.00005 >= pQuery->dLowestXcorrScore` | `CometSearch.cpp:5000`, `:8600-8632` |
| Worst-slot eviction + `dLowestXcorrScore` re-scan (tie-break: score, peptide, mod sites) | `StorePeptide()` `:5024`, `StorePeptideI()` `:8739`, re-scan `:9182` |
| `SortFnXcorr` at the 3 sort sites; `usiRankXcorr` adjacency grouping via `isEqual` | `CometPostAnalysis.cpp:399, :476`, `CometSearchManager.cpp:2837` |
| deltaCn `1 - score_j/score_i` | `CometPostAnalysis.cpp:345` |
| E-value: histogram filled inside the scorers, synthetic-decoy fill assumes a linear bin sum over the XCorr array, log-linear regression in 0.1-score bins | `CometSearch.cpp:4975`, `GenerateXcorrDecoys()` `CometPostAnalysis.cpp:1341`, `LinearRegression()` `:1154` |
| Writer report gates `fXcorr <= dMinimumXcorr -> skip row` (24 sites) and `ResetOneResult()` seeding `fXcorr = dMinimumXcorr` | `CometWrite*.cpp`, `core/Types.h:168` |

**1.5 The `.cps` store is not directly searchable.** `tools/carafe_cps.py`: dense
`(nAA-1) x 4 x u16` per row, channels `(b_z1, y_z1, b_modloss_z1, y_modloss_z1)`, keyed by
Carafe `row_index`, no `.idx` binding. The z2 fragment channels were **dropped** (only their
max survives in `base8`). ~75-80% of stored slots are zero. Joining to Comet identity needs
the variant map, whose order is *not* key order. So a search-time consumer needs an offline
join into a new `.idx`-bound file, exactly as `carafe_cps_to_fi_mask.py` does for masks.

**1.6 FI_DB pre-selects candidates by matched-ion count**, caps scoring at 100 per
spectrum (`FRAGINDEX_MAX_NUMSCORED`). An intensity score re-ranks within that 100; it does
not change recall of the candidate stage. PI_DB scores every mass-window candidate.

**1.7 PI_DB internal decoys have no predictions.** `decoy_search=1/2` on PI_DB reverses
sequences on the fly (`CometSearch.cpp:2697-2998`); no Carafe record exists for them. With
intensity as primary, internal decoys would all score 0 and FDR estimation would be
meaningless. Both existing datasets were predicted with `--include-decoys` against a
target-decoy FASTA, which is the supported configuration.

## 2. Proposed design

### 2.1 The score

Start with the **spectral-similarity family on sqrt intensities**, computed over the
peptide's full predicted ladder (all `nAA-1` positions x predicted channels):

```
dot   = sum_f  p_f * o_f          p_f = sqrt(predicted relative intensity), o_f = observed bin value
cos   = dot / (|p| * |o|)         |p| precomputed at file-build time; |o| over the same positions
score = cos                       (spectral contrast angle 1 - 2*acos(cos)/pi as a variant)
```

Both vectors range over the same fragment set, so unexplained *observed* peaks outside the
ladder do not penalize (same as XCorr), while observed peaks at predicted-zero positions do
(unlike XCorr). Score is in [0,1], rounded to 4 decimals before gating/sorting (XCorr uses 3).
Cosine on sqrt intensities is what Prosit/MS2PIP-style rescoring uses; whether it is also the
best *primary* score is an empirical question that Phase 1 below answers before we commit to
the expensive plumbing. The implementation keeps the formula behind one function
(`CometIntensityScore::Score(...)`) so `cos`, spectral angle, and a dot product weighted by
explained-intensity fraction can be compared without touching callers.

Fragment set used: b/y at z1 (+ the two modloss channels when a neutral-loss mod is active and
the variant is modified). No a/c/x/z series, no z2 (not in the `.cps`, see 3.4), no precursor
NL peaks. For precursor charge >= 3 the charge-2 prediction is used as-is (documented
approximation; see 3.4 for the path to z2).

Observed lookup: `ppfSparseSpScoreData` at `BIN(m/z)`, same bin as XCorr. Flanking bins are
not used initially (`theoretical_fragment_ions` semantics are XCorr's; revisit if evaluation
says so).

### 2.2 Search-time data: a new `.idx`-bound intensity file

New artifact `<flavor>.carafe_inten`, built offline by `tools/carafe_cps_to_inten.py`
(stdlib-only; reuses `carafe_cps_to_fi_mask.py`'s variant-map byte-range streaming,
worker-side sort + parent k-way merge, and post-write verification; `carafe.py inten`;
`prerun --inten` runs it as stage s7 per flavor). **Implemented 2026-09-03 (Phase 0).**
Format mirrors `.fi_mask` v3:

```
magic    "Comet Carafe intensity v1\n"
header   SourceIdxFingerprint, SourceIdxNumRawPeptides, SourceIdxPath, SourceCpsPath,
         VarModConfig, Mode (general|phospho),
         Channels (names in channel-code order: b_z1,y_z1[,b_modloss_z1,y_modloss_z1]),
         Transform (sqrt), Quant (u8), MinRelativeIntensity, MaxPeaks
u64      entry count
entries  variable length, sorted strictly increasing by (iWhichPeptide, modNumIdx, cNtermMod, cCtermMod):
           u32 iWhichPeptide, i32 modNumIdx, i8 cNtermMod, i8 cCtermMod   (10 B key)
           u8  nPeaks                                                      (1 B)
           nPeaks x { u16 code, u8 q }   code = (channel << 8) | ladderPos; q = round(255*sqrt(rel))
         peaks within an entry sorted by code
```

Ladder position = Comet's per-peptide loop index i (b-ion length i+1 and y-ion length i+1
both at i), the same coordinate as the mask bit before its "-2" shift; b maps from AlphaBase
row r directly (ladderPos r), y is mirrored (ladderPos nAA-2-r). Unlike the mask there is
no "i > 1" gate: b1/y1/b2/y2 are scoreable and kept when above threshold. The per-entry
norm |p| is not stored: it is a pure function of the stored bytes and the C++ decoder
recomputes it (storing it would only invite inconsistency).

Sparse on purpose: keep peaks with relative intensity >= `MinRelativeIntensity` (default
0.01) up to `MaxPeaks` (default 32), highest first, ties broken by code; peaks quantizing
to 0 are dropped; an entry with zero peaks is still written so coverage is exact. Zero-
prediction positions are implicit, so `|o|` still ranges over the full ladder at score
time (the scorer iterates all positions anyway).

The `Channels` header field is what lets a later `.cps` v2 (or a parquet-direct builder) add
z2 without a format break; the C++ loader dispatches on it and ignores channels it does
not score. Measured sizes are recorded in Section 3 (Phase 0).

### 2.3 C++ module `CometIntensityStore` (new files `CometSearch/CometIntensityStore.{h,cpp}`)

Modeled on `CometPredictedMask`:

- `Load(path)`: magic/header, hard-fail on `VarModConfig` or `.idx` fingerprint mismatch
  (reuse `CometPredictedMask::ComputeVarModConfigString()` / `ComputeIdxFingerprint()`,
  factor them into a shared helper), `fread` entries into a key-sorted table.
- `BuildPositional(variantArray)`: after `g_fragmentPeptides` (FI_DB) or `g_dbIndexVariants`
  (PI_DB) is final, resolve each variant's record once by 4-tuple lookup into
  `vector<uint32_t> offsets` (by variant index; sentinel = no record) + one packed blob, then
  free the key table. Search-time lookup is O(1) by `uiWhichVariant` / `i`. Memory:
  4 B/variant + blob (~1.2-2 GB phospho, ~150 MB oxmet). Coverage (variants with a record /
  total) is logged; a coverage below 100% is a warning, below a threshold an error.
- `Score(variantIdx, ladder-binned masses, Query*)`: decodes the record into a small dense
  stack array `pred[channel][ladderPos]`, then one pass over the ladder accumulating `dot`
  and `|o|^2`. Runs fused with the XCorr loop in `XcorrScoreI()` (same iteration, second
  array read), so both scores exist for every candidate in either primary mode.
- Missing record: score 0.0, per-run counter reported at end of search. Never silently
  substitutes XCorr (different scale).

Hooks: FI_DB after the mass sort in `GenerateFragmentIndex()` (`CometFragmentIndex.cpp:384`,
same spot as the mask cache, before `g_iFragmentIndex` is allocated); PI_DB after
`GenerateVariantArray()`. Freed in `FiStrategy::finalize()` / the PI equivalent.

### 2.4 Parameters

```
predicted_intensity_file =              # path to <flavor>.carafe_inten; empty = disabled
primary_score = 0                       # 0 = xcorr (default), 1 = intensity score
```

Plumbing follows the `fragment_index_predicted_mask_file` / `index_search_type` precedents:
`Comet.cpp` parse map (trim-whitespace string; `parse_int`), `Options` fields
`sPredictedIntensityFile` / `iPrimaryScore` (`core/Params.h`), defaults in
`RestoreDefaults()`, `GetParamValue` + range clamp in `InitializeStaticParams()`,
`-p` template lines in the advanced block, RTS CLI args + unconditional `SetParam` in
`RealtimeSearch/SearchMS1MS2.cs`.

Init-time validation when `primary_score = 1`: `predicted_intensity_file` must be set; the
search must be FI_DB or PI_DB (FASTA -> error, not silent fallback); `decoy_search` must be 0
(internal decoys have no predictions, 1.7). When `primary_score = 0` and a file is given, the
intensity score is computed and reported as a secondary column only.

### 2.5 Making the primary score switchable

Introduce the notion of a *primary score* once, mechanically, rather than special-casing:

- `Results` gains `float fIntensityScore` (reset to 0 in `ResetOneResult()` and the two
  `SearchUtils.cpp` re-floors). Add `inline float PrimaryScore(const Results&)` returning
  `fXcorr` or `fIntensityScore` per `g_staticParams.options.iPrimaryScore`.
- `Query::dLowestXcorrScore` / `siLowestXcorrScoreIndex` (and decoy twins) are renamed
  `dLowestPrimaryScore` / `siLowestPrimaryScoreIndex`; the scorers' early-reject gates, the
  eviction scan, and the re-scan compare `dPrimary`. Tie-break structure (score, `strcmp`
  peptide, `piVarModSites`) is unchanged.
- `SortFnXcorr` becomes `SortFnPrimary` (same `isEqual`-first structure; `isEqual` is
  relative-`FLT_EPSILON`, fine for a [0,1] score at 4 decimals); the 3 sort sites and
  `usiRankXcorr` grouping use it. `usiRankXcorr` keeps its name but means "primary rank"
  (documented in the writers).
- deltaCn is `1 - primary_j / primary_i` (semantics preserved for a ratio score).
- Writer gates `fXcorr <= dMinimumXcorr` become `PrimaryScore(r) <= minimumPrimary()`
  (`dMinimumXcorr` for XCorr, 0.0 for intensity). A stored candidate with intensity primary
  may legitimately have XCorr <= 0; today's gates would drop that row.
- E-value: **stays XCorr-derived in Phase 2** (XCorr is still computed for every candidate,
  so `iXcorrHistogram` is filled exactly as today). Reported `expect` is then not monotone in
  the primary score; `qvalue.py`'s side-by-side xcorr/e-value output will show that
  explicitly. An intensity-score null model is Phase 3 (see 3.3).

### 2.6 Output

- txt: new column `intensity_score` inserted **right after `delta_cn`** (i.e. next to
  `xcorr`), present whenever an intensity file is loaded. This shifts every later column,
  so `tools/qvalue.py` was reworked (2026-09-03) to resolve columns by header name instead
  of fixed positions (this also fixes its pre-existing breakage on PEFF output, which
  inserts `peff_modified_peptide` before `protein`). A `--score-col NAME` option to rank by
  an arbitrary named column follows in Phase 1.
- pepXML: `<search_score name="intensity_score" .../>`; pin: feature `IntensityScore` before
  `Peptide`/`Proteins`; mzIdentML: `<userParam>` (no CV term) at the three positional edit
  points; sqt: no slot, omitted.
- RTS: `CometScores::dIntensityScore` (4 edit sites), `score.dIntensityScore = ...` in
  `DoSingleSpectrumSearchMultiResults()`, `ScoreWrapper::dIntensityScore` property.

## 3. Phasing

**Phase 0: offline artifact (Python only). DONE 2026-09-03.** `carafe_cps_to_inten.py`,
`carafe.py inten`, `prerun --inten` (stage s7), `test_carafe_inten.py` in T38's suite list
(9 tests). Both files built from the existing stores, no inference; each validated three
ways: header fingerprint / raw-peptide count / VarModConfig identical to the flavor's
existing `.fi_mask`, the post-write strictly-increasing verifier, and hundreds of random
variants re-derived independently from the `.cps` matching the file byte for byte.

| | OxMet | Phospho-large |
|---|---|---|
| Entries | 3,760,672 | 46,588,597 |
| Peaks (mean/entry) | 63.4M (16.9) | 1,001.9M (21.5) |
| Entries at the 32-peak cap | 14,891 (0.4%) | 4,722,457 (10.1%) |
| Zero-peak entries | 271 | 423 |
| Entries with modloss peaks | 0 (general mode) | 40,228,543 |
| File size | 231 MB | 3.52 GB |
| Build wall / peak RSS (16 workers) | 32 s / 0.38 GB | 8m02s / 3.9 GB |

Phospho came in above the 2.5 GB estimate because the variant-weighted mean peptide is
long (23.8 residues, up to 4 channels) and 10% of entries hit the cap; the cap is a build
flag (`--max-peaks`) if Phase 1 shows the tail matters either way.

**Phase 1: secondary score + evaluation. DONE 2026-09-03 (formula decision below).**
Implemented: `CometSearch/CometIntensityStore.{h,cpp}` (load + guards + variant binding +
`Score()`), fused into `XcorrScoreI()` via a new `uiVariant` argument (FI_DB passes
`uiWhichVariant`, PI_DB passes its `g_dbIndexVariants` position, PI on-the-fly decoys pass
`NO_VARIANT`), `Results::fIntensityScore`, params `predicted_intensity_file` /
`primary_score` (value 1 accepted with a warning, not yet honored), output in txt (after
`delta_cn`), pepXML (`intensity_score`), pin (`IntensityScore` after `Sp`), mzIdentML
(`userParam Comet:intensity_score`), `CometScores::dIntensityScore` + `ScoreWrapper`
property for RTS, `qvalue.py --score-col`. Tests T39 (plumbing + guards) and T40 (exact
cosine vs a first-principles oracle, with and without modloss channels) pass; the full
fast suite is 59/59 on both the Linux build and the MSVC `x64/Release/Comet.exe` (full
Windows solution incl. `CometWrapper.dll` / `RealtimeSearch.exe` builds clean). Overhead on the OxMet FI_DB search of `20170103_HelaQC_01.mzXML`:
+1 s load, +0.25 GB RSS, 100.00% of the 3.75M FI variants bound.

Evaluation (OxMet, all 24,460 spectra with results, `num_output_lines = 5`, rank-1 unless
"rerank", target-decoy FDR via `qvalue.py`-equivalent counting):

| Ranking score | PSMs @1% FDR | @5% | vs xcorr @1% |
|---|---|---|---|
| xcorr | 12,992 | 15,169 | -- |
| -log10 e-value | 13,677 | 15,564 | +5.3% |
| cosine alone (rank-1 by xcorr) | 10,564 | 15,325 | -18.7% |
| cosine alone (rerank top 5 by cosine) | 9,703 | 14,681 | -25.3% |
| xcorr * cosine (rerank top 5) | 14,238 | 15,765 | +9.6% |
| xcorr + 2*cosine (rerank top 5) | 14,275 | 15,788 | +9.9% |
| -log10 e-value + 3*cosine (rerank top 5) | 14,358 | 15,854 | +10.5% |

Split by precursor charge (the file has no 1+ precursors), the all-charge deficit of the
pure cosine turns out to come entirely from 3+ and higher -- the population where a z1-only
prediction of a 2+ precursor is compared against spectra dominated by z2 fragments:

| Ranking score | 2+ only (14,762 spectra) @1% | vs xcorr | 3+ and up (9,698) @1% | vs xcorr |
|---|---|---|---|---|
| xcorr | 9,402 | -- | 3,832 | -- |
| -log10 e-value | 9,528 | +1.3% | 4,043 | +5.5% |
| cosine alone (rank-1 by xcorr) | 9,733 | +3.5% | 2,827 | -26.2% |
| cosine alone (rerank top 5) | 9,261 | -1.5% | 1,194 | -68.8% |
| xcorr * cosine (rerank top 5) | 10,263 | +9.2% | 3,968 | +3.5% |
| xcorr + 2*cosine (rerank top 5) | 10,330 | +9.9% | 3,987 | +4.0% |

Rank-1 median cosine for 2+: targets 0.931, decoys 0.414; for 3+ and up: 0.572 / 0.267. On
the precursors the predictions actually model, the cosine alone already beats xcorr and
e-value at 1% FDR. Conclusion: the z2 fragment channels (dropped by the .cps v1 store and
not read by the scorer) and eventually 3+ inference are prerequisites for judging the
formula, not Phase 3 polish -- do the z2 extension before freezing the Phase 2 primary score.

Rank-1 median cosine over all charges: targets 0.825, decoys 0.320 -- the score separates well, but as a
pure PSM ranking over all charges it is length-blind (a 7-residue peptide matching 5 of 12 positions can
out-cosine a 25-residue peptide matching 30 of 48), which is why it loses to xcorr alone at
1% FDR while every xcorr-times/plus-cosine combination gains ~10%. Percolator was not
available on this machine, so the "as a rescoring feature" measurement is the top-5
re-rank above rather than a full semi-supervised model.

**Formula decision for Phase 2 (updated after Phases 1b-1d).** Keep `intensity_score` =
cosine on sqrt intensities as the reported column (decision 1). For `primary_score = 1` the
candidates are (a) raw cosine -- viable only with charge-matched predictions, where it beats
xcorr by +7.5% (OxMet) as a rank-1 score but loses ~6% of that when allowed to re-pick
candidates (N=5), or (b) a combined `xcorr + 2*intensity_score`, which is +12% (OxMet,
per-charge) / +20% (phospho, 2+-only preds) over xcorr, robust to the cosine weight (2-3),
equivalent to `xcorr * (0.25 + cos)`, and insensitive to N. E-value combinations are a
post-search ranking only (the E-value is not known when a candidate is scored). Working
proposal: (b). This changes Section 2.5's premise (the eviction gate, sort, deltaCn and
writer gates would key on the combined score) and needs sign-off before Phase 2 starts.

**Phase 1b: doubly-charged fragments (z2). DONE 2026-09-03.** The `.cps` v1 store had
dropped the b_z2/y_z2 (and modloss z2) predictions and the scorer read only charge-1 bins.
Now: `.cps` v2 (`carafe_pred_to_cps.py --channels 8`, the default; `CpsReader.read_row8()`,
with `read_row()` still serving the v1 4-channel view so mask tooling is untouched);
`.carafe_inten` v2 with explicit `code=name` channel pairs (codes 4-7 = z2) and the C++
scorer reading fragment charges 1..min(2, usiMaxFragCharge), excluding higher-charge
channels from both the dot product and |p| -- so for 2+ precursors (usiMaxFragCharge 1) z2
predictions are ignored entirely, exactly as XCorr never scores z2 fragments there, and
the 2+ numbers are unchanged by construction (T40 asserts this on the fixture). Rebuilt
stores: oxmet.v2.cps 1.05 GB (21 s), phospholarge.v2.cps 17.8 GB (5.5 min); oxmet.v2
.carafe_inten 267 MB, 20.0 peaks/entry (13% of peaks are z2); phospholarge.v2.carafe_inten
3.80 GB, 23.5 peaks/entry (13 min, 4.2 GB RSS).

| Ranking score | 3+ and up, z1 only | 3+ and up, z1+z2 | vs xcorr (3,832) |
|---|---|---|---|
| cosine alone (rank-1 by xcorr) | 2,827 | 3,327 | -26.2% -> -13.2% |
| cosine alone (rerank top 5) | 1,194 | 2,419 | -68.8% -> -36.9% |
| xcorr * cosine (rerank top 5) | 3,968 | 4,019 | +3.5% -> +4.9% |
| xcorr + 2*cosine (rerank top 5) | 3,987 | 4,050 | +4.0% -> +5.7% |

Rank-1 median cosine for 3+ and up: targets 0.572 -> 0.520, decoys 0.267 -> 0.215 (|o| now
spans the z2 ladder too; separation improves). All-charge totals: cosine alone 10,216,
xcorr * cosine 14,128 (+8.7%), xcorr + 2*cosine 14,212 (+9.4%). So z2 recovers about half
of the pure-cosine deficit on 3+ precursors; the rest is the prediction itself, which
Carafe made for 2+ precursors only (`--charges 2`). Closing it needs inference at 3+ (the
per-charge rows the store and builder already merge across), not more scorer work.

**Phase 1c: combination grid on both datasets (2026-09-03).** Unmasked FI_DB searches,
`num_output_lines = 5`, z2 intensity files; scores combined from xcorr, cosine and
E = -log10 e-value; PSMs at 1% FDR ranking spectra by the combined score (N=1 keeps
xcorr's peptide, N=5 lets the score re-pick among the stored 5 -- the two agree to ~1%,
i.e. the gain is cross-spectrum ordering, not peptide choice). MM2_R1 phospho: 34,445
spectra (13,005 at 2+, 21,440 at 3+ and up), 2.0e9 FI entries, 58 s, 14.4 GB RSS, 100% of
46.58M variants bound.

| Score | OxMet HeLa (xcorr 12,992) | MM2_R1 phospho (xcorr 14,831) |
|---|---|---|
| E alone | 13,677 (+5.3%) | 16,616 (+12.0%) |
| cosine alone (rank-1) | 10,216 (-21%) | 10,167 (-31%) |
| cosine alone, 2+ only | 9,733 vs xcorr 9,402 (+3.5%) | 8,128 vs xcorr 6,446 (+26%) |
| xcorr * cos | 14,158 (+9.0%) | 17,789 (+19.9%) |
| xcorr + 2*cos | 14,302 (+10.1%) | 17,850 (+20.4%) |
| E + 3*cos | 14,303 (+10.2%) | 17,776 (+19.9%) |
| E + xcorr + 3*cos | 14,403 (+10.9%) | 18,046 (+21.7%) |
| E + 2*xcorr + 3*cos | 14,403 (+10.9%) | 18,086 (+21.9%) |

Same plateau shape on both: every base score plus ~2-4 units of cosine gains the same
amount within ~1%, the cosine weight 2-3 transfers, products and sums are equivalent, and
three-term scores add <1% over the best two-term one. The absolute gain is twice as large
on phospho (+20% vs +10%), where the mod-site ambiguity gives the intensity pattern more
to disambiguate. Rank-1 median cosine on MM2_R1: targets 0.636, decoys 0.155.

**Phase 1d: per-charge records and 3+ predictions (2026-09-03, in progress).** Carafe was
run for 2+ precursors only on both datasets, so every 3+ spectrum was scored against the 2+
fragmentation pattern -- the remaining half of the pure-cosine 3+ deficit after z2. Format
v3 of `.carafe_inten` adds a per-entry precursor-charge byte (0 = the max-merge over all
predicted charges, the previous behaviour) and a `PerCharge` header; `carafe_cps_to_inten.py
--per-charge` (charges read from the peptides TSV by row_index, needs `--out-tsv`) writes one
record per (variant, predicted charge), `--only-charges` restricts which store rows are used
(for a same-.idx 2+-only baseline). Entries of one variant are consecutive, so the loader
still keeps one offset per variant; `Score()` walks them and takes the exact precursor-charge
match, else a charge-0 record, else the nearest lower predicted charge, else the lowest
higher (T40 covers all four cases). OxMet predictions were regenerated with `--charges 2,3`
in `carafe_oxmet_z23_20260903/` (7.52M rows, 150 chunks, 90 min inference, `primary.cps` v2
2.09 GB); three intensity files on that one .idx: `--only-charges 2` (the 2+-only baseline,
75,103,786 peaks -- identical to the old oxmet.v2 file, so inference is reproducible),
`--per-charge` (7.52M records, 543 MB), and the default max-merge over both charges.
xcorr and E-value are identical across the three runs (same index, same candidates); only
the cosine differs. HeLa, PSMs at 1% FDR, N=1 / N=5:

| Score | 2+-only preds | per-charge preds | merged max(2+,3+) |
|---|---|---|---|
| **All charges** (xcorr 12,992; E 13,677) | | | |
| cosine alone | 10,216 / 9,436 | **13,969 / 13,120** | 12,251 / 10,451 |
| xcorr * cos | 14,158 / 14,128 | 14,505 / 14,468 | 14,276 / 14,334 |
| xcorr + 2*cos | 14,302 / 14,212 | **14,570 / 14,521** | 14,303 / 14,346 |
| E + 3*cos | 14,303 / 14,317 | 14,525 / 14,541 | 14,422 / 14,454 |
| **3+ and up** (xcorr 3,832; E 4,043) | | | |
| cosine alone | 3,327 / 2,419 | **4,143 / 4,032** | 4,145 / 4,053 |
| xcorr + 2*cos | 4,048 / 4,050 | 4,190 / 4,203 | 4,163 / 4,163 |
| E + 3*cos | 4,194 / 4,200 | 4,241 / 4,247 | 4,239 / 4,244 |
| rank-1 median cosine, 3+ (targets/decoys) | 0.520 / 0.215 | 0.801 / 0.231 | 0.683 / 0.239 |

(2+ numbers are identical for the 2+-only and per-charge files by construction.) With
charge-matched predictions the pure cosine beats xcorr on every charge class and on all
charges combined (+7.5%, above the E-value's +5.3%), the 3+ target median cosine rises from
0.52 to 0.80, and xcorr + 2*cos reaches +12.1% over xcorr (+9.3% on 3+). Max-merging the
two charges' predictions is clearly worse than selecting by charge (it dilutes both
patterns), which settles the record design. Charges 4+ still fall back to the 3+ record
(1,034 spectra here); predicting 4+ as well is cheap at OxMet scale.

Phospho at 2+/3+ (2026-09-04): charge-3 predictions only were generated in
`carafe_phospholarge_z3_20260903/` (46.6M rows, 19.2 h inference, `primary.cps` 17.8 GB),
verified to share variant numbering with the existing 2+ workdir (all 46,588,597 tuples
identical; only the .idx fingerprint differs, because it encodes absolute file positions
shifted by the header's path string), so both per-charge files were built against the
existing `phospholarge.fasta.idx` and merged with `carafe.py inten-merge` into
`phospholarge.z23.percharge.carafe_inten` (93.18M records, 2.31e9 peaks, 8.06 GB; the MM2_R1 search then peaks at 18.6 GB RSS).
MM2_R1, same index, xcorr/E identical across runs, PSMs at 1% FDR, N=1 / N=5:

| Score | 2+-only preds | per-charge 2+/3+ preds |
|---|---|---|
| **All charges** (xcorr 14,831; E 16,616) | | |
| cosine alone | 10,167 / 8,539 | **18,039 / 17,312** |
| xcorr * cos | 17,789 / 17,631 | 18,575 / 18,585 |
| xcorr + 2*cos | 17,850 / 17,796 | 18,524 / 18,549 |
| xcorr + 3*cos | 17,683 / 17,634 | **18,650 / 18,705** |
| E + xcorr + 3*cos | 18,012 / 18,046 | 18,301 / 18,333 |
| **3+ and up** (xcorr 8,874; E 9,648) | | |
| cosine alone | 8,256 / 6,356 | **10,615 / 10,228** |
| xcorr + 3*cos | 10,311 / 10,162 | 10,674 / 10,668 |
| rank-1 median cosine, 3+ (targets/decoys) | 0.497 / 0.117 | 0.798 / 0.134 |

Same picture as OxMet, larger: with charge-matched predictions the pure cosine beats xcorr
by +21.6% and the E-value by +8.6% over all charges (3+: +19.6% over xcorr), the 3+ target
median cosine rises from 0.50 to 0.80, and xcorr + 3*cos reaches +26% over xcorr. Charges 4+
(4,903 spectra) still use the 3+ record.

**Phase 1e: XCorr-style background subtraction of the cosine (2026-09-04).** `intensity_score_bg`
= cosine minus the mean, over the 2*`xcorr_processing_offset` (150) nonzero bin shifts, of the
shifted *normalized* dot product (the normalized spectrum is shifted; the denominator stays
the unshifted |p||o|, so the mean is linear and reduces to one 151-bin window sum per ladder
position minus the centre; bins past the array edges count as 0, as in XCorr). Always
reported next to `intensity_score` (txt/pepXML/pin/mzIdentML/RTS); may be negative. T40
checks it equals the cosine on the plain fixture (no two peaks within 75 bins) and drops by
exactly p_b5*sqrt(49)/150/(|p||o|) when a noise peak is placed 0.5 Da above b5.

Effect: none to speak of. Rank-1 median cos - bg is 0.003 for targets and decoys alike on
both datasets; every ranking in the grid moves by < 0.2% (HeLa per-charge: cos 13,969 vs bg
13,953; xcorr + 2*cos 14,570 vs 14,553. MM2_R1: 10,167 vs 10,106; 17,850 vs 17,847). The
reason is the bin width: at `fragment_bin_tol` 0.02 the +/-75-bin window is +/-1.5 Da, so
apart from a fragment's own isotope peak the 150 shifted bins are almost all empty and the
subtracted mean is ~0. XCorr's construction was calibrated for 1 Da bins (+/-75 Da). A
mass-based window (e.g. +/-75 Da) would be the faithful analogue at high resolution, at
7,500 bins per ladder position -- only worth doing with the precomputed-array approach.

Low-resolution check (`fragment_bin_tol` 1.0005, `fragment_bin_offset` 0.4; the only local
human low-res data is the 2009 LTQ Orbitrap pair `20130226-comet-tests/sh_1617_JX_070209p_
KO410_run{1,2}.mzXML`, ion-trap CID, 1,252 MS2 scans with results pooled; OxMet per-charge
predictions, which are HCD-trained -- so a fragmentation-type mismatch is folded in). Here
the subtraction bites: rank-1 median cos - bg is 0.09 for targets vs 0.12 for decoys, so
the medians move from 0.512/0.376 (cos) to 0.386/0.220 (bg) and the rank-1 target-vs-decoy
AUC goes xcorr 0.648, E 0.652, cos 0.669, bg 0.677. PSM counts are too small for 1% FDR to
be meaningful (xcorr 102; a single decoy moves things by ~1%); at 5-10% FDR bg-based
combinations edge out cos-based ones by a few PSMs, consistent with the AUC. Also notable:
at 0.02 bins the ion-trap data yields results for only 86 of ~660 scans (the FI candidate
stage needs 0.02-Da matches), so the low-res settings are mandatory for such data.

Full-scale low-res check: `C:\Work\data\20231228_Lu_100ng_Hela_ITMS2_01.raw` (Lumos, ion-trap
MS2, 132,125 scans; searched with the Windows build, which reads .raw directly), same OxMet
per-charge predictions, 1.0005/0.4 bins, all-charge FDR, PSMs at 1%, N=1 / N=5:

| Score | ITMS2 HeLa |
|---|---|
| xcorr | 4,006 |
| -log10 e-value | 9,776 |
| cosine alone | 13,853 / 7,446 |
| cosine_bg alone | 5,861 / 2,791 |
| xcorr + 2*cos | 14,166 / 14,604 |
| xcorr + 2*bg | 14,580 / 14,948 |
| xcorr + 3*cos | **17,837 / 18,227** |
| xcorr + 3*bg | 16,736 / 16,961 |
| xcorr * cos | 16,961 / 17,182 |
| xcorr * max(bg, 0) | 17,149 / 17,217 |
| E + 3*cos | 15,468 / 15,857 |

Cross-check against a plain FASTA search with the user's standard low-res params
(`C:\Work\Data\comet.params`: same FASTA, Cys +57, 20 ppm, `isotope_error 0`, but
`theoretical_fragment_ions = 1`): xcorr 5,332 / E 15,107 at 1% FDR. With
`theoretical_fragment_ions = 0` (what the FI run above used) the FASTA search gives 3,834 /
9,704 -- i.e. the FI_DB run reproduces plain FASTA to within 5%, and the 1.5x E-value gap
was entirely the flanking-peak setting. **Rule (2026-09-04): low-res MS/MS searches with
`fragment_bin_tol = 1.0005` must use `theoretical_fragment_ions = 1` (M peak only); flanking
peaks (`theoretical_fragment_ions = 0`) are for high-res settings only.** The earlier ITMS2
table above (flanking peaks) is kept for the record; the M-peak-only table below is the one
to quote.
Re-running the FI + intensity search with `theoretical_fragment_ions = 1` (cosine is
unaffected; xcorr/E improve):

| Score | ITMS2 HeLa, M-peak-only (N=1 / N=5) |
|---|---|
| xcorr | 5,442 |
| -log10 e-value | 15,273 |
| cosine alone | 15,117 / 7,947 |
| cosine_bg alone | 6,207 / 2,539 |
| xcorr + 2*cos | 22,315 / 22,763 |
| xcorr + 3*cos | **24,863 / 25,126** |
| xcorr + 3*bg | 20,821 / 20,762 |
| xcorr * cos | 23,020 / 23,261 |
| E + 3*cos | 20,549 / 20,718 |

So on ion-trap data with the proper xcorr settings: cosine alone matches the E-value
(15,117 vs 15,273; 2.8x xcorr), and xcorr + 3*cos is 4.6x xcorr and 1.6x the E-value.

Rank-1 medians targets/decoys (flanking run): cos 0.677/0.532, bg 0.366/0.226 (the
subtraction removes 0.27 from targets and 0.29 from decoys -- at 1 Da bins the 150-shift
window really is noise). Two conclusions: (1) on ion-trap data the cosine is far stronger than XCorr as a
score (3.5x the PSMs alone; xcorr + 3*cos is 4.5x xcorr and 1.8x the E-value), so the
intensity score matters MORE at low resolution, where XCorr's 1 Da bins discard most of the
discriminating information; (2) the background subtraction still does not pay: bg alone is
much worse than cos (it removes a constant-ish offset whose size tracks |o|, i.e. spectrum
density, which the cosine's own normalisation already handles), and in combinations it is
a wash (better at weight 2, worse at weight 3). Caveats: all-charge FDR with 1 Da bins mixes
charge classes with different score distributions (xcorr's all-charge count is below its
2+-only count for that reason -- per-charge FDR or Percolator would be the fair accounting),
and the HCD-trained predictions are being applied to CID spectra.

**Phase 2 (2026-09-04): primary_score switch implemented and evaluated for raw cosine and
cosine_bg.** `primary_score` = 0 xcorr / 1 intensity_score / 2 intensity_score_bg selects the
score that gates retention (XcorrScoreI's early reject, StorePeptideI's eviction and the
lowest-stored-score threshold -- `PrimaryScore()`, `PrimaryScoreOf()`, `ResultIsReportable()`,
`LowestPrimaryScoreInit()` in core/Types.h), orders results (SortFnXcorr), and defines
usiRankXcorr, deltaCn and the writers' report gates; empty slots read as "no floor" so a
negative intensity_score_bg can be stored; the E-value stays xcorr-derived; minimum_xcorr
applies to xcorr mode only. Modes 1/2 require predicted_intensity_file, an indexed search
and decoy_search 0 (refused loudly; for an indexed search decoy_search comes from the .idx
header). T41 covers the three modes on the fixture and the refusals; fast suite 60/60.
Combined scoring as a primary is deferred (user decision).

Results, rank-1 PSMs at 1% FDR; rows = which score chose each spectrum's peptide (the run's
primary), columns = which column ranked spectra for the FDR:

| Run | ranked by xcorr | by cosine | by E | by cosine_bg |
|---|---|---|---|---|
| **HeLa OxMet, high-res** (24,460 spectra) | | | | |
| primary = xcorr | 12,992 | 13,973 | 13,691 | 13,954 |
| primary = cosine | 13,822 | **13,100** | 13,934 | 13,067 |
| primary = cosine_bg | 13,790 | 13,067 | 13,917 | **13,059** |
| **MM2_R1 phospho, high-res** (34,445) | | | | |
| primary = xcorr | 14,831 | 17,927 | 16,611 | 17,937 |
| primary = cosine | 17,508 | **16,562** | 17,373 | 16,487 |
| primary = cosine_bg | 17,514 | 16,562 | 17,421 | **16,488** |
| **ITMS2 HeLa OxMet, low-res** (127,665) | | | | |
| primary = xcorr | 5,442 | 15,116 | 15,261 | 6,206 |
| primary = cosine | 21,138 | **3,976** | 23,515 | 755 |
| primary = cosine_bg | 15,829 | 5,552 | 20,685 | **1,126** |

The bold diagonal is "the score as a true primary" (it chose the peptide and ranks the
spectra). Cosine as primary: HeLa 13,100 (+0.8% over xcorr as primary, but 6% below
using the cosine to rescore xcorr's picks), MM2_R1 16,562 (+11.7% / 7.6% below rescoring),
ITMS2 3,976 (-27% / far below rescoring's 15,116). cosine_bg as primary is indistinguishable
from cosine at high resolution and worse at low. "E-value as primary" is not a separate
search-time option -- within a spectrum the E-value is a monotone transform of xcorr, so it
picks the same peptide as xcorr in 100% of spectra -- but ranking xcorr's picks by E is the
best true primary among the existing scores on every dataset: HeLa 13,677, MM2_R1 16,616,
ITMS2 15,273. The cosine as primary is 4% below that on HeLa, effectively tied on MM2_R1
(16,562 vs 16,616) and far below at low resolution. The peptide choice differs from xcorr's
in 17% (HeLa), 29% (MM2_R1) and 74% (ITMS2) of spectra, with symmetric target/decoy label
flips -- the primary-cosine losses come from the inflated decoy tail when the max over all
candidates is taken, as the top-5 analysis predicted, and they grow with candidate count
(1 Da bins).

Unexpected and useful: the cosine-primary run *rescored by the E-value* is the best single-
column result on the low-res data (23,515; xcorr-mode E 15,261; the offline xcorr + 3*cos
rerank gave 24,863), and second-best at high resolution (HeLa 13,934 vs 13,973; MM2_R1
17,373 vs 17,927). Letting the cosine choose the peptide and xcorr/E rank the spectra is an
implicit two-factor score -- a decoy that wins on cosine rarely also has a high xcorr -- and
it is available today without any combined-score code. It is the natural bridge to the
deferred combined-score work.

**Phase 2 (original plan): primary-score switch.** Section 2.5 in full, RTS plumbing, init validation.
T41: same fixture searched with `primary_score=0/1` changes rank order as predicted;
T22-style 1-vs-8-thread RTS determinism with `primary_score=1`. Full-scale: PSMs at 1% FDR
for `primary_score=1` vs 0 on both datasets, target to beat: masking-only +3.7-6.2%.

**Phase 3: follow-ups gated on Phase 1/2 results.**
- Intensity-score E-value: candidates are (i) score the same 3000 precomputed decoys with a
  flat predicted pattern (cheap, distribution mismatch), (ii) shifted-ladder self-decoys
  (score each candidate's prediction against the spectrum with randomly shifted bins),
  (iii) target-decoy empirical calibration only. Decide from Phase 2 histograms.
- z2 channels: DONE (Phase 1b above).
- Precursor z >= 3: new inference with `--charges 2,3` if the z2-channel approximation
  underperforms on high-charge spectra.
- Intensity-aware FI candidate counting (weighting posting hits by predicted intensity) if
  the 100-candidate cap is shown to limit recall.

## 4. Decisions (signed off 2026-09-03)

1. Score family: **cosine on sqrt intensities**. Decided.
2. Indexed-only scope: intensity score is unavailable for plain-FASTA searches and for
   PI_DB internal decoys; `primary_score=1` errors in those configurations rather than
   falling back. Decided.
3. E-value remains XCorr-based through Phase 2. Decided. A cosine-score-based E-value is
   wanted eventually and stays a Phase 3 item to evaluate and develop, not a rejected idea.
4. Parameter names `predicted_intensity_file` / `primary_score`. Decided. The txt
   `intensity_score` column is inserted right after `delta_cn`, and `tools/qvalue.py`
   locates columns by header name (done). Decided.
