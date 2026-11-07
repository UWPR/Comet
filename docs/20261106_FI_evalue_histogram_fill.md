# Fragment-index E-value histogram fill (2026-11-06)

Project plan for calibrating E-values on fragment-index (FI_DB) queries by filling the
xcorr histogram with the next-weakest *real* candidates before `GenerateXcorrDecoys()`
pads it with synthetic ones. Spun out of the chimeric-search review
(`docs/20261105_RTSchimeric.md` section 10.7), where this mechanism was measured through a
knob (`chimeric_fragindex_min_ions_score`) that this project is meant to retire. Plan
only; nothing implemented yet.

## 1. Problem

Comet's E-value for a PSM comes from a linear regression on the cumulative xcorr
distribution of the candidates scored for that spectrum (`CometPostAnalysis::
LinearRegression()`, `CalculateEValue()`). The null that distribution represents depends
on the search path:

| path | what gets scored (and lands in the histogram) | real entries per query |
|---|---|---|
| FASTA batch | every peptide in the precursor mass window | thousands |
| PI_DB (`AnalyzePeptideIndex()`) | every indexed peptide in the mass window | thousands |
| FI_DB (`SearchFragmentIndex()`, batch fused path and RTS alike) | only peptides with >= `fragindex_min_ions_score` (3) index hits among the top `fragindex_num_spectrumpeaks` peaks, ranked by hit count, at most `FRAGINDEX_MAX_NUMSCORED` (100) | median **5-7**; 54% of queries fewer than 10; 20% at the cap |

Whenever the histogram holds fewer than `EXPECT_DECOY_SIZE` (3,000) entries --
always, on FI -- `GenerateXcorrDecoys()` tops it up by scoring 3,000 - N fixed, precomputed
~30-residue random sequences (`CometDecoys.h`, precomputed b/y bins, scattered per fragment
charge, no mass or length constraint) against the spectrum. Measured on the HeLa benchmark
file against a human target-decoy phospho index (`docs/20261105_RTSchimeric.md` 10.7):

- The 3,000-entry padded histogram of a **full** spectrum is 93% synthetic entries at xcorr
  0.0-0.3 (per query, mean 753 / 302 / 248 / 1478 -- the 0.3 pile is `iMinXcorrHisto`'s
  lumping of half the zero scores) with a tail of 100 / 57 / 28 / 16 / 8 / 5 at 0.4-0.9.
  (Section 9 has the raw-bin tables; an earlier version of these numbers was read off the
  histogram *after* `LinearRegression()` had overwritten it with the cumulative
  distribution, and is corrected there.) This is not an artifact of chimeric subtraction,
  which only thins the tail further and lowers the winner's xcorr from 0.93 to 0.79; it is
  what the synthetic decoys are.
- The FASTA/PI null is different: real peptides at the right mass, most scoring ~0 too, but
  with a tail of peptides that share one to three ions by chance of composition and tryptic
  structure. On the spectra where FI identifies something, PI scores ~1,600 real window
  peptides of which ~68 sit at xcorr >= 0.4 -- a real tail that extends further and flatter
  than the synthetic one (63 entries >= 0.4 but steeper). FI drops exactly those peptides at
  the 3-ion candidate floor: its histogram holds a median of **one** real entry, the winner.
  The regression's cumulative fit starts at the "second-best" bin (`iNextCorr`, xcorr 0.8-0.9)
  and runs downward, so those tail bins *are* the fit; the intercepts of the FI and PI fits
  are identical and the whole FI-vs-PI E-value gap (x1.6 on the same PSMs) is the slope.
- Letting 2-ion candidates be scored on chimeric re-search passes (report floor unchanged,
  so none can be reported) raises the real entries per query from a median of 4 to 100, adds
  +22-50% to the 0.4-0.9 bins, flattens the slope (-0.457 -> -0.430) and raises weak
  winners' E-values x1.41 -- making them honest: the decoy rate among reported rows at
  E <= 0.05 halves (4.4% -> 1.9%) and confident IDs rise 43% (2,181 -> 3,116). On the plain
  RTS search the same change is +1.7% confident IDs and a cleaner E <= 0.05 set (0.7% vs
  1.2% decoy).

So FI E-values are optimistic on every FI query, batch or RTS -- x1.6-2 against PI on the
same PSMs across all xcorr deciles (section 9) -- and the weaker the spectrum the worse. The
chimeric knob papers over it for one caller.

## 2. Goal

Make the FI histogram's real-candidate content approximate the FASTA/PI null -- the ranked
weaker candidates the index already knows about -- for every FI query, so that:

1. FI E-values on weak hits agree with PI/FASTA E-values for the same PSMs;
2. `chimeric_fragindex_min_ions_score` and `Query::iFragIndexMinIonsScoreOverride` can be
   removed;
3. nothing changes for a search that opts out (byte-identical output with the feature off).

## 3. Design

### 3.1 Mechanism

`SearchFragmentIndex()` already builds `mPeptides` (peptide -> index-hit count among the
top peaks), keeps the entries with count >= `fragindex_min_ions_score`, sorts them by count
(descending, tie-broken by peptide index for determinism -- T63), and calls `XcorrScoreI()`
on the first `FRAGINDEX_MAX_NUMSCORED`. `XcorrScoreI()` always adds the histogram entry
and only then applies the reporting floor (`fragindex_min_ions_report`) and the
lowest-stored-xcorr test before `StorePeptideI()`. Scoring a candidate below the reporting
floor therefore already means "histogram entry, never reported" -- no new store/no-store
mode is needed.

The change: when fewer than `fragindex_histogram_fill` candidates were scored, continue
down the ranked list through the candidates *below* `fragindex_min_ions_score` (2-hit, then
1-hit), in the same count-descending, index-ascending order, until that many have been
scored or the list ends. The reporting floor is untouched, so these never become results.

```
collect mPeptides (as today)
vScore  = entries with count >= fragindex_min_ions_score, sorted (count desc, index asc)
vFill   = entries with 1 <= count < fragindex_min_ions_score, sorted the same way   [new]
score vScore up to FRAGINDEX_MAX_NUMSCORED                                           (as today)
if (scored < fragindex_histogram_fill)                                               [new]
   score vFill until scored == fragindex_histogram_fill or vFill is exhausted
```

Both lists come from the one map walk; the only new work is sorting the sub-floor entries
(bounded by the map size, which is bounded by the posting lists already walked) and the
extra `XcorrScoreI()` calls.

### 3.2 Parameter

`fragindex_histogram_fill` (int, default to be set by section 5's measurements; 0 = off,
today's behaviour): the minimum number of real candidates to score into the histogram. A
value above `FRAGINDEX_MAX_NUMSCORED` is clamped to it. Registered in `Comet.cpp`'s parse
table and `comet.params` output next to `fragindex_min_ions_score`, copied in
`InitializeStaticParams()`, read-only after init like the rest of `g_staticParams`;
settable through `SetParam` for RTS.

Why a count and not a lower `fragindex_min_ions_score`: the floor also decides which
candidates compete for the result slots in `_pResults` -- with the 1-hit candidates
admitted the sort cap would fill with junk before the real 3-hit candidates on busy
spectra, and the measured "47 junk winners" effect would return through the stored list
even with the report floor at 3 (a 2-hit candidate can enter `_pResults` when nothing
better exists). A fill count scores extra candidates only when the real list is short, and
never changes which candidates are scored first.

### 3.3 What stays the same

- `FRAGINDEX_MAX_NUMSCORED` (100) remains the hard cap on `XcorrScoreI()` calls per query.
- `GenerateXcorrDecoys()` still pads to 3,000 afterwards; the synthetic bulk at 0.0-0.3 is
  harmless once the 0.3-0.9 region holds real entries.
- The `iMinXcorrHisto` lumping of zero-score entries in `XcorrScoreI()` (proteins A-H)
  is existing behaviour and is kept.
- PI_DB and FASTA paths are untouched.
- Candidate order and the stored-result logic are untouched, so with
  `fragindex_histogram_fill = 0` the output is byte-identical.

### 3.4 Effects to expect

- Batch FI E-values change for weak PSMs (larger), so T23/T24-style FI-vs-PI PSM counts at
  1% FDR should move *toward* PI, not away; the regression baselines that compare against a
  pinned previous release will differ in E-values for weak hits by design.
- RTS: the plain search gains a little and gets a cleaner E-threshold set; the chimeric
  search loses its knob with no loss of its measured gain.
- Cost: up to `fragindex_histogram_fill` extra `XcorrScoreI()` calls on sparse queries only.
  Scoring at the 100 cap on every re-search query made the chimeric run no slower in
  wall-clock (295 s vs 305 s for 42,030 spectra single-threaded, the difference being fewer
  iterations), so the expected cost is small; section 5 measures it.

## 4. Files to touch

| File | Change |
|---|---|
| `CometSearch/CometSearch.cpp` (`SearchFragmentIndex()`, ~`:1930-2010`) | second ranked list of sub-floor candidates; fill loop after the main scoring loop; remove the `Query::iFragIndexMinIonsScoreOverride` consult once the chimeric knob is retired |
| `CometSearch/core/Params.h`, `CometSearchManager.cpp` (`InitializeStaticParams`), `Comet.cpp` (parse table, `comet.params` text) | `fragindex_histogram_fill` |
| `CometSearch/core/Types.h`, `CometSearchManager.cpp/.h` | remove `iFragIndexMinIonsScoreOverride` and the chimeric `iMinIonsScore` plumbing; `chimeric_fragindex_min_ions_score` param removed; `SearchChimeric.cs` and `rts_repro` lose their `--min-ions-score` / `minIonsScore` arguments |
| `CometSearch/CometSearchManager.cpp` (`RTS_CHIMERIC_EVAL/_HIST` trace) | generalise the per-query histogram/fit dump to an env var usable from the plain RTS path (`COMET_EVALUE_TRACE`), so section 5 can see batch and RTS FI queries, not only chimeric ones |
| `tests/unit/run_tests.py`, `tests/tests.md` | T68 (section 5.1); T67 updated for the removed argument |
| `docs/RealTimeSearch.md`, `docs/20261105_RTSchimeric.md` 4.6 / 10.6 / 10.7 | point here; knob removed |

## 5. Validation and measurement

### 5.1 Unit test (T68)

A synthetic sparse spectrum and a small FASTA built so that exactly one peptide clears the
3-ion floor while several share 1-2 ions (the T67 generator's approach: theoretical b/y
ions written to a fixture, a FASTA with the target and designed near-miss peptides):

- `fragindex_histogram_fill = 0`: output byte-identical to the pre-change build (the
  determinism and T63 guarantees hold);
- `fragindex_histogram_fill = N`: the same rank-1 peptide, xcorr identical, E-value larger,
  `uiHistogramCount` (via the trace) equal to the number of designed sub-floor candidates;
  no sub-floor candidate is reported at any `num_output_lines`;
- 1 vs 8 threads byte-identical on the T22 fixture with the fill on.

### 5.2 Calibration against the PI null (the decisive measurement)

Same HeLa file and FASTA as `docs/20261105_RTSchimeric.md` section 10, indexes built both
ways (`-i` and `-j`). For every spectrum where FI and PI report the same rank-1 peptide,
compare E-values: median FI/PI ratio and its distribution, split by rank-1 xcorr decile.
Today FI should be optimistic (ratio < 1) for low-xcorr PSMs; with the fill the ratio should
approach 1 across deciles. This is the number that says the null is right, independent of
any FDR effect.

### 5.3 FDR and yield

`rts_repro` mode 0 (plain RTS, FI) and `comet.exe` batch FI on the full file, with fill 0 /
25 / 50 / 100: targets at q <= 0.01 and q <= 0.05, decoy rate at E <= 0.05, and the
T24-style FI-vs-PI count agreement. Chimeric mode 1 with the fill and *without* the
chimeric knob must reproduce section 10's 3,116 confident later IDs (or better).

### 5.4 Cost

`comet-benchmark` skill for batch Hz and the RTS ms/spectrum, fill 0 vs the chosen default,
single-threaded over the full file as in section 10.5 (index load subtracted).

### 5.5 Decoy-winner calibration

For spectra whose rank-1 PSM is a decoy, the E-value should behave like a p-value times the
candidate count: the fraction of decoy winners with E <= 0.05 at fill 0 vs the chosen
fill, on FI vs PI. A calibrated FI should match PI.

## 6. Phases

| Phase | Work | Checkpoint |
|---|---|---|
| 0 | Generalise the histogram/fit trace to an env var on the plain FI path; capture the 5.2 FI-vs-PI baseline on today's code | Baseline tables in this doc |
| 1 | `fragindex_histogram_fill` param and the fill loop, default 0 | Byte-identical output at 0 (full `run_tests.py`, rts_repro fixture, T22); T68 |
| 2 | Measurements 5.2-5.5 at 25 / 50 / 100; pick the default | Tables in this doc |
| 3 | Default set; retire `chimeric_fragindex_min_ions_score` and its plumbing; T67 updated; chimeric section-10 numbers reproduced without the knob | `run_tests.py`, Windows build |
| 4 | Docs (`RealTimeSearch.md`, `comet.params` text, the chimeric doc's cross-references), regression baseline note for T23/T24 | -- |

## 7. Alternatives considered

- **Mass- and length-matched synthetic decoys** (e.g. shuffled index peptides at the
  precursor mass): closer to the FASTA null than the fixed 30-mers, but it approximates
  with extra scoring what the index already holds for free, and keeps a second scoring
  pathway to maintain. Not pursued unless 5.2 shows the real sub-floor candidates are not
  enough.
- **Lowering `fragindex_min_ions_score` globally**: measured (+1.7% confident IDs on the plain
  search) but it changes which candidates compete for the result slots and admits junk
  winners on empty spectra (section 3.2). The fill count has the calibration benefit
  without that.
- **Changing `LinearRegression()`'s window selection** so it ignores a thin tail: treats the
  symptom; the tail is thin because the null is wrong, not because the fit is.

## 8. Decisions (2026-11-06)

| Question | Decision |
|---|---|
| Default fill | 100 (= `FRAGINDEX_MAX_NUMSCORED`): "always score the 100 best-supported candidates". 2-hit candidates alone reach the cap on 59% of re-search queries; the plain search with a global 2-hit floor cost nothing measurable (43.2 vs 42.9 s at 8 threads). Kept an integer so 25 / 50 can be measured. |
| 1-hit candidates | Included: they fill the ~12% of queries where 2-hit candidates run out -- the sparse spectra where calibration matters most -- and a single shared ion at 0.02 Da is chance-level, the null wanted. `mPeptides` is a `std::map` keyed by peptide index, so one walk into count buckets (>= 3, 2, 1) gives each bucket in index order: no sort of the large 1-hit bucket, and the tie order matches T63. |
| Batch FI E-values change | Acceptable: only weak-hit E-values move, upward, toward PI/FASTA; xcorr, ranking and peptides do not. `fragindex_histogram_fill = 0` reproduces today's values; release note and T23/T24 expectations updated. |
| Unchanged | `fragindex_min_ions_score` 3, `fragindex_min_ions_report` 3 (who may win / be reported), `FRAGINDEX_MAX_NUMSCORED` 100, `EXPECT_DECOY_SIZE` 3,000, the `iMinXcorrHisto` lumping (a cheap pseudo-random spread of half the zero scores into one low bin; it applies to fill candidates like everything else). |
| Scope | Applied uniformly: plain RTS, chimeric iteration 0, batch FI. |

## 9. Progress

### Phase 0 (2026-11-06): trace and baseline

**Trace.** `COMET_EVALUE_TRACE=1` prints one `COMET_EVALUE` line per query from
`CometPostAnalysis::CalculateEValue()` -- every search path -- with the stored and real
scored counts, `iMaxCorr` / `iNextCorr`, slope, intercept and the rank-1 xcorr / E-value;
`=2` adds `COMET_EVALUE_HIST_REAL` (the scored candidates, before `GenerateXcorrDecoys()`)
and `COMET_EVALUE_HIST` (after padding, the counts the regression fits), both as raw
per-bin `bin:count` lists taken *before* `LinearRegression()` overwrites bins
0..`iNextCorr` with the cumulative distribution. The chimeric path's own fit/histogram
prints were removed in its favour.

**Setup.** As `docs/20261105_RTSchimeric.md` section 10: the 42,030-scan HeLa fixture;
FI (`-i`) and PI (`-j`) indexes of `human.canonical.target-decoy.fasta` with
`comet_phospho.params`; `rts_repro` mode 0 (the file's charge, 20 ppm, `isotope_error` 0),
8 threads. Histogram traces: first 1,000 scans, single thread. Decoy = `DECOY_`;
q-values by E-value order, FDR = decoys / targets.

**A. Plain RTS search, FI vs PI (full file).**

| | FI | PI |
|---|---|---|
| rank-1 PSMs (decoy %) | 27,655 (24.2%) | 39,461 (31.6%) |
| PSMs at E <= 0.05 (decoy %) | 13,174 (1.2%) | 12,775 (0.7%) |
| targets at q <= 0.01 | 12,808 | 12,973 |
| decoy winners with E <= 0.05 / E <= 0.01 | 2.4% / 0.7% | 0.7% / 0.1% |

**B. Same rank-1 peptide on both (19,598 spectra; xcorr identical on 100%).** Median
E(FI) / E(PI) by rank-1 xcorr decile, lowest to highest:
0.87, 0.69, 0.66, 0.62, 0.60, 0.53, 0.49, 0.50, 0.51, 0.57 -- FI below PI on 66-79% of
PSMs in every decile. FI is optimistic for strong hits as much as for weak ones.

**C. Paired fits, same spectrum and peptide (107 queries, first 1,000 scans), medians.**

| | FI | PI |
|---|---|---|
| real entries in the histogram | **1** | 1,644 |
| real entries at xcorr >= 0.4 | 1 | 68 |
| padded entries at xcorr >= 0.4 | 63 | 91 |
| `iNextCorr` (xcorr) | 0.9 | 0.9 |
| slope (per 0.1 xcorr bin) | -0.474 | -0.443 |
| intercept (log10 count) | 3.92 | 3.88 |
| rank-1 xcorr / E(FI)/E(PI) | 1.30 / 0.63 | |

Decomposing log10(E_PI / E_FI) per query: slope term 0.17, intercept term -0.02 (median).
**The entire gap is the slope; normalisation is not the issue** (so the padding count
does not need to track the window size, an idea floated during the review and dropped
here).

**D. Histogram shapes (per query, mean entries per 0.1-xcorr bin; "padded" = what the
regression fits).**

| xcorr | 0.0 | 0.1 | 0.2 | 0.3 | 0.4 | 0.5 | 0.6 | 0.7 | 0.8 | 0.9 | 1.0 |
|---|---|---|---|---|---|---|---|---|---|---|---|
| FI plain, real (128 queries; median 1 real entry) | 0 | 0 | 0.1 | 0.2 | 0.3 | 0.3 | 0.4 | 0.4 | 0.4 | 0.3 | 0.2 |
| FI plain, padded | 763 | 326 | 245 | 1507 | 93 | 41 | 15 | 5.5 | 2.2 | 0.9 | 0.4 |
| PI plain, real (661 queries; ~1,400 real entries) | 832 | 144 | 127 | 604 | 32 | 17 | 5.8 | 2.3 | 0.9 | 0.4 | 0.1 |
| chimeric iteration >= 1, floor 3, real (107) | 0 | 0.3 | 1.1 | 3.8 | 4.0 | 4.9 | 3.5 | 2.5 | 1.6 | 0.6 | 0.4 |
| chimeric iteration >= 1, floor 3, padded | 913 | 304 | 173 | 1510 | 53 | 26 | 11 | 5.4 | 2.4 | 0.9 | 0.5 |
| chimeric iteration >= 1, floor 2, real (104; median 100) | 0.1 | 2.8 | 6.7 | 19.8 | 15.8 | 14.8 | 8.5 | 4.8 | 2.8 | 0.9 | 0.6 |
| chimeric iteration >= 1, floor 2, padded | 894 | 301 | 178 | 1498 | 65 | 36 | 16 | 7.8 | 3.6 | 1.2 | 0.7 |

The 0.3 pile in every padded row is `iMinXcorrHisto`'s lumping of half the zero scores
(real and synthetic alike). Floor 2 on the paired iteration-1 queries: real entries 4 -> 100
(median), real entries >= 0.4 17 -> 48 (mean), slope -0.457 -> -0.430, E x1.41.

**What this fixes in the premise.** The synthetic tail is not "too low" in count -- on the
spectra FI identifies it holds about as many entries above 0.4 as PI's real tail (63 vs
68) -- it is *steeper*: real window peptides that share ions by chance extend further than
random 30-mers do, and that difference in slope is the whole E-value gap. The fill supplies
exactly those real entries (what the 2-hit floor did on re-search passes: +31 real entries
>= 0.4 per query), so the expectation for Phase 2 is that FI's slope moves to PI's and the
decile ratios in B move to ~1. Caveat: the histogram traces are 1,000-scan samples; Phase 2
repeats C and D on the full file. Note also that PI pads too (its ~1,600 real entries are
below `EXPECT_DECOY_SIZE`), so PI is the batch-equivalent null, not an absolute truth.

| Date | Phase | Result |
|---|---|---|
| 2026-11-06 | 0 | `COMET_EVALUE_TRACE` in `CalculateEValue()`; FI-vs-PI baseline tables A-D above; the premise sharpened to "the synthetic tail is steeper than the real one, and the gap is slope-only". |

## 10. Open questions (resolved in section 8)

1. Default fill: 100 (= the cap, simplest: "always score the 100 best-supported
   candidates") or lower to bound the cost on busy indexes? Section 5.4 decides; 100 is the
   proposal.
2. Should 1-hit candidates count, or only 2-hit? On a 0.02 Da bin a single shared index ion
   is close to a chance event -- which is exactly the null we want -- but their number can
   be large; the ranked order means they are only reached when nothing better exists.
3. Batch FI output changes for weak PSMs. Acceptable for the next release, with a note in
   the release notes and the T23/T24 baseline expectation updated?
