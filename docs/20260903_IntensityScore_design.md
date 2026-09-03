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

**Formula decision for Phase 2.** Keep `intensity_score` = cosine on sqrt intensities as
the reported column (decision 1), but make the *primary* score for `primary_score = 1` a
combined score rather than raw cosine: the working proposal is `xcorr * intensity_score`
(no tuned constants, both factors already computed per candidate, +9.6% here); the additive
`xcorr + 2*cos` is within noise of it and needs a constant. E-value-based combinations are
not usable as a primary score because the E-value is only known after all candidates are
scored. This changes Section 2.5's premise (the eviction gate, sort, deltaCn and writer
gates would key on the combined score) and needs sign-off before Phase 2 starts.

**Phase 2: primary-score switch.** Section 2.5 in full, RTS plumbing, init validation.
T41: same fixture searched with `primary_score=0/1` changes rank order as predicted;
T22-style 1-vs-8-thread RTS determinism with `primary_score=1`. Full-scale: PSMs at 1% FDR
for `primary_score=1` vs 0 on both datasets, target to beat: masking-only +3.7-6.2%.

**Phase 3: follow-ups gated on Phase 1/2 results.**
- Intensity-score E-value: candidates are (i) score the same 3000 precomputed decoys with a
  flat predicted pattern (cheap, distribution mismatch), (ii) shifted-ladder self-decoys
  (score each candidate's prediction against the spectrum with randomly shifted bins),
  (iii) target-decoy empirical calibration only. Decide from Phase 2 histograms.
- z2 channels: `.cps` v2 storing 8 channels (the parquet trees still exist: 1.5 GB / 21 GB;
  regenerating the store is minutes), then `Channels` bit in the intensity file.
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
