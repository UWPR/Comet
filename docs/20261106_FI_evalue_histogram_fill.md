# Fragment-index E-value histogram fill (2026-11-06)

Project plan for calibrating E-values on fragment-index (FI_DB) queries by filling the
xcorr histogram with the next-weakest *real* candidates before `GenerateXcorrDecoys()`
pads it with synthetic ones. Spun out of the chimeric-search review
(`docs/20261105_RTSchimeric.md` section 4.6), where this mechanism was measured through a
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
file against a human target-decoy phospho index (section 9, Phase 0):

- The 3,000-entry padded histogram of a **full** spectrum is 93% synthetic entries at xcorr
  0.0-0.3 (per query, mean 778 / 322 / 248 / 1507 -- the 0.3 pile is `iMinXcorrHisto`'s
  lumping of half the zero scores) with a tail of 83 / 38 / 14 / 5.5 / 2.2 / 1.1 at 0.4-0.9.
  (Section 9 has the raw-bin tables.) This is not an artifact of chimeric subtraction, which
  only thins the tail further; it is what the synthetic decoys are.
- The FASTA/PI null is different: real peptides at the right mass, most scoring ~0 too, but
  with a tail of peptides that share one to three ions by chance of composition and tryptic
  structure. On the same spectra PI scores a median of ~5,000 real window peptides (three
  isotope windows at `isotope_error` 2) -- above `EXPECT_DECOY_SIZE`, so PI is not padded at
  all -- of which ~88 / 44 / 15 / 6 sit at xcorr 0.4 / 0.5 / 0.6 / 0.7. FI drops exactly those
  peptides at the 3-ion candidate floor: its histogram holds a median of **7** real entries
  (often only the winner). The regression's cumulative fit starts at the "second-best" bin
  (`iNextCorr`, xcorr 0.9-1.0) and runs downward, so those tail bins *are* the fit.
- The chimeric loop's re-search passes were the first symptom: a stripped spectrum leaves a
  handful of real candidates above the floor, and a knob that let 2-ion candidates be scored
  into the histogram (`chimeric_fragindex_min_ions_score`, since removed) made the weak
  re-search winners' E-values honest. That is a property of every fragment-index query.

So FI E-values are optimistic on every FI query, batch or RTS -- about x2 against PI on the
same PSMs through the first eight xcorr deciles (section 9) -- and the weaker the spectrum the
worse. The chimeric knob papered over it for one caller.

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
floor therefore already means "histogram entry, never reported" -- but only while the
reporting floor is at least the scoring floor. With `fragindex_min_ions_report` below
`fragindex_min_ions_score` (say 2 and 3), a 2-hit fill candidate clears the reporting
floor and was stored and reported, which no candidate could be before the fill existed.
So `XcorrScoreI()` treats an FI_DB candidate whose hit count is below the scoring floor --
exactly the fill candidates -- as histogram-only: it adds the histogram entry, is never
stored, and is left out of the matched-peptide counts (pepXML `num_matched_peptides`, the
SQT match count, Percolator's `lnNumSP`). A reporting floor below the scoring floor
therefore has no effect, as before the fill, and `InitializeStaticParams()` warns about it
(2026-11-08, section 9).

The change: when fewer than `fragindex_histogram_fill` candidates were scored, continue
down the ranked list through the candidates *below* `fragindex_min_ions_score` (2-hit, then
1-hit), in the same count-descending, index-ascending order, until that many have been
scored or the list ends. They never become results and never count as matched peptides.

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

A batch search none of whose outputs reports E-values (`Options::OutputsNeedEValues()`
false: `print_expect_score = 0` and no pepXML, `.txt`, Percolator or mzIdentML output, i.e.
SQT-only with the Sp column) never collects the histogram, so `DoSearch()` runs it with the
fill at 0 (2026-11-09): the fill would be pure cost there (+11-17% per FI query single-
threaded) and the output is unchanged. The RTS entry points always compute E-values and keep
the configured fill: `InitializeSingleSpectrumSearch()` forces `bPrintExpectScore` on so the
predicate is true there whatever the output flags say (2026-11-10; before, an RTS session with
every file output off got decoy-only E-values ~100x too optimistic). The same helper replaced four hand-written copies of the "does an output
need E-values?" test, which had all omitted mzIdentML: an mzIdentML-only search with
`print_expect_score = 0` wrote E = 9.99E+02 for every PSM (pre-existing on master; test T85).

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
| `CometSearch/core/Types.h`, `CometSearchManager.cpp/.h` | remove `iFragIndexMinIonsScoreOverride` and the chimeric `iMinIonsScore` plumbing; `chimeric_fragindex_min_ions_score` param removed; `SearchChimericMS2.cs` and `rts_repro` lose their `--min-ions-score` / `minIonsScore` arguments |
| `CometSearch/CometSearchManager.cpp` (`RTS_CHIMERIC_EVAL/_HIST` trace) | generalise the per-query histogram/fit dump to an env var usable from the plain RTS path (`COMET_EVALUE_TRACE`), so section 5 can see batch and RTS FI queries, not only chimeric ones |
| `tests/unit/run_tests.py`, `tests/tests.md` | T68 (section 5.1); T67 updated for the removed argument |
| `docs/RealTimeSearch.md`, `docs/20261105_RTSchimeric.md` 4.6 / 10.6 | point here; knob removed |

## 5. Validation and measurement

### 5.1 Unit test (T68)

A synthetic sparse spectrum and a small FASTA built so that exactly one peptide clears the
3-ion floor while several share 1-2 ions (the T67 generator's approach: theoretical b/y
ions written to a fixture, a FASTA with the target and designed near-miss peptides):

- `fragindex_histogram_fill = 0`: the histogram holds the target alone (the pre-change
  behaviour; the determinism and T63 guarantees hold);
- `fragindex_histogram_fill = N`: the same rank-1 peptide, xcorr identical, E-value larger,
  `uiHistogramCount` (via the trace) equal to the number of designed sub-floor candidates;
  the stored count unchanged; not sent = the default (100), byte-identical to 100;
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
chimeric knob must not lose confident later-iteration IDs against the knob.

### 5.4 Cost

`comet-benchmark` skill for batch Hz and the RTS ms/spectrum, fill 0 vs the chosen default,
single-threaded over the full file as in section 10.7 (index load subtracted).

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
| Default fill | 100 (= `FRAGINDEX_MAX_NUMSCORED`): "always score the 100 best-supported candidates". Kept an integer so 25 / 50 can be measured. |
| 1-hit candidates | Included: they fill the ~12% of queries where 2-hit candidates run out -- the sparse spectra where calibration matters most -- and a single shared ion at 0.02 Da is chance-level, the null wanted. `mPeptides` is a `std::unordered_map` keyed by peptide index: one pass splits it into the qualified candidates and the sub-floor ones, only the prefix of the sub-floor bucket that can still be scored is ordered (`std::partial_sort` by hits, then peptide index, so the tie order matches T63), and the large 1-hit bucket is never fully sorted. |
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
prints were removed in its favour. Since 2026-11-09 the `tope` field follows the reporting
rules (999 when the slope is >= 0, capped at 999), so it is the rank-1 E-value Comet reports;
before, it printed the raw fit, e.g. 1754 for a reported 999 (test T83).

**Setup.** As `docs/20261105_RTSchimeric.md` section 10: the 42,030-scan HeLa fixture;
FI (`-i`) and PI (`-j`) indexes of `human.canonical.target-decoy.fasta` with
`comet_phospho.params`; `rts_repro` mode 0 (the file's charge, 20 ppm, `isotope_error` 2),
8 threads. Histogram traces: first 1,000 scans, single thread. Decoy = `DECOY_`;
q-values by E-value order, FDR = decoys / targets. Every number in this section was
re-measured on 2026-11-17 on master `c990672c` at the current defaults; the 2026-11-06 runs
were discarded.

**A. Plain RTS search, FI (fill 0) vs PI (full file).**

| | FI | PI |
|---|---|---|
| rank-1 PSMs (decoy %) | 31,438 (18.0%) | 39,511 (24.1%) |
| PSMs at E <= 0.05 (decoy %) | 18,141 (0.8%) | 17,438 (0.4%) |
| targets at q <= 0.01 | 18,216 | 18,332 |
| decoy winners with E <= 0.05 / E <= 0.01 | 2.6% / 0.7% | 0.7% / 0.1% |

**B. Same rank-1 peptide on both (24,446 spectra; xcorr identical on 100%).** Median
E(FI) / E(PI) by rank-1 xcorr decile, lowest to highest:
0.66, 0.46, 0.49, 0.53, 0.52, 0.51, 0.58, 0.64, 0.87, 1.69 -- FI below PI on 65-83% of
PSMs in the first eight deciles, and *above* PI in the top decile (the candidate-cap bias,
section 10.5 of the chimeric doc; see Phase 2).

**C. Paired fits, same spectrum and peptide (24,455 queries, full file), medians.**

| | FI fill 0 | PI |
|---|---|---|
| real entries in the histogram | **7** | 5,071 (unpadded) |
| `iNextCorr` (xcorr) | 0.9 | 1.0 |
| slope (per 0.1 xcorr bin) | -0.389 | -0.385 |
| intercept (log10 count) | 3.91 | 4.16 |
| median E | 0.0005 | 0.00102 |

E(FI) / E(PI): median 0.60, FI below PI on 70%. At `isotope_error` 2 the two fits differ
mainly in the **intercept** (0.25 in log10, x1.8): PI's histogram holds ~5,000 real entries
and is never padded, FI's is padded to exactly 3,000, so the two cumulative distributions are
normalised to different populations. The slopes are close in the median, but not per decile:
FI's is steeper on weak spectra (few real tail entries) and flatter on the strongest ones
(the 100 scored candidates are the best-supported of a wide three-window set).

**D. Histogram shapes (first 1,000 scans; per query, mean entries per 0.1-xcorr bin;
"padded" = what the regression fits).**

| xcorr | 0.0 | 0.1 | 0.2 | 0.3 | 0.4 | 0.5 | 0.6 | 0.7 | 0.8 | 0.9 | 1.0 |
|---|---|---|---|---|---|---|---|---|---|---|---|
| FI fill 0, real (171 queries; median 2 real entries) | 0 | 0 | 0.1 | 0.6 | 0.8 | 0.9 | 0.9 | 0.8 | 0.5 | 0.6 | 0.3 |
| FI fill 0, padded | 778 | 322 | 248 | 1507 | 83 | 38 | 14 | 5.5 | 2.2 | 1.1 | 0.4 |
| FI fill 100, real (171; median 100) | 0.8 | 8.1 | 16.9 | 28.5 | 13.1 | 10.9 | 5.8 | 3.7 | 1.7 | 1.1 | 0.4 |
| FI fill 100, padded | 757 | 321 | 258 | 1492 | 93 | 47 | 18 | 8.3 | 3.3 | 1.6 | 0.6 |
| PI, real (665 queries; median 4,979) | 2467 | 421 | 375 | 1791 | 88 | 44 | 15 | 6.2 | 2.3 | 0.8 | 0.3 |
| PI, padded (= real: above `EXPECT_DECOY_SIZE`) | 2657 | 451 | 409 | 2054 | 95 | 47 | 16 | 6.5 | 2.4 | 0.9 | 0.3 |

The 0.3 pile in every padded row is `iMinXcorrHisto`'s lumping of half the zero scores
(real and synthetic alike). The fill supplies the real chance-sharing tail FI lacks (0.4-0.7:
13 / 11 / 6 / 4 real entries per query against 0.8 / 0.9 / 0.9 / 0.8), and the padded FI tail
then matches PI's bin for bin (93 / 47 / 18 / 8 vs 88 / 44 / 15 / 6). What it cannot supply is
PI's population size: PI's histogram is ~5,000 entries, FI's 3,000.


### Phase 2 (2026-11-06; re-measured 2026-11-17): measurements at fill 25 / 50 / 100

Same file, indexes and `rts_repro` settings as Phase 0 (plain RTS: file charge, 20 ppm,
`isotope_error` 2, 8 threads); PI as the comparator.

**5.2 Calibration -- median E(FI) / E(PI) on the 24,446 spectra with the same rank-1
peptide, by rank-1 xcorr decile (lowest to highest); [% of PSMs with FI below PI]**

| fill | deciles 1-10 |
|---|---|
| 0 | 0.66 [78], 0.46 [83], 0.49 [82], 0.53 [78], 0.52 [75], 0.51 [73], 0.58 [68], 0.64 [65], 0.87 [54], 1.69 [41] |
| 25 | 0.81 [68], 0.61 [75], 0.68 [73], 0.76 [66], 0.78 [64], 0.80 [62], 0.85 [57], 0.87 [56], 1.07 [48], 1.88 [38] |
| 50 | 0.89 [62], 0.72 [68], 0.79 [67], 0.89 [59], 0.92 [56], 0.98 [52], 1.03 [48], 1.13 [45], 1.37 [41], 2.15 [35] |
| **100** | **0.97 [53], 0.84 [62], 0.90 [59], 0.99 [51], 1.03 [47], 1.10 [43], 1.26 [39], 1.46 [35], 1.77 [33], 2.80 [30]** |

Paired fits over the full file (24,455 spectra with the same rank-1 peptide in all three):

| | real entries | `iNextCorr` | slope | intercept | median E |
|---|---|---|---|---|---|
| FI fill 0 | 7 | 0.9 | -0.389 | 3.91 | 0.0005 |
| FI fill 100 | 100 | 1.0 | **-0.369** | 3.90 | **0.00107** |
| PI | 5,071 | 1.0 | -0.385 | 4.16 | 0.00102 |

E(FI fill 100) / E(PI): median 1.06, FI below PI on 45%. The fill brings the median E-value
onto PI's and calibrates the weak and middle deciles (0.84-1.10), but not by reproducing PI's
fit: the FI intercept stays 0.25 below PI's (the 3,000-entry padding against PI's unpadded
~5,000) and the FI slope goes flatter than PI's (-0.369 vs -0.385), which compensate near the
median and diverge at the ends -- FI slightly optimistic in the weakest deciles, and 1.3-2.8x
pessimistic in the strongest three. The flatter slope is the candidate cap: the 100 scored
candidates are the best-supported of the whole three-window set, a biased sample whose
histogram sits high (`docs/20261105_RTSchimeric.md` 10.5, where the same effect inside a
+/- 1 m/z window is 10-1000x). Both the normalisation and the cap bias are open items
(section 10 of the chimeric doc, section 11).

**5.3 / 5.5 FDR, yield and decoy-winner calibration (plain RTS, full file)**

| | rank-1 | at E <= 0.05 (decoy %) | targets q <= 0.01 | q <= 0.05 | decoy winners E <= 0.05 / 0.01 |
|---|---|---|---|---|---|
| PI | 39,511 | 17,438 (0.4%) | 18,332 | 20,520 | 0.7% / 0.1% |
| FI fill 0 | 31,438 | 18,141 (0.8%) | 18,216 | 20,470 | 2.6% / 0.7% |
| FI fill 25 | 31,438 | 17,833 (0.5%) | 18,454 | 20,542 | 1.6% / 0.4% |
| FI fill 50 | 31,430 | 17,677 (0.4%) | 18,448 | 20,528 | 1.3% / 0.2% |
| **FI fill 100** | 31,438 | 17,503 (0.3%) | **18,513** | 20,556 | **1.1% / 0.2%** |

The rank-1 set is identical across fills (xcorr and ranking never change); only the
E-values move. Fill 100 yields +1.6% confident targets over fill 0 and passes PI; the
decoy-winner leakage at E <= 0.05 halves. (Fill 50's rank-1 count differs by 8: a fill
candidate that clears the store gate on 8 spectra; closed on 2026-11-08, see the progress
table.)

**Batch FI** (`comet.exe`, fused path, 20 threads, this template's `isotope_error = 2`,
`tools/qvalue.py`): targets at 1% FDR by E-value 18,168 -> **18,466 (+1.6%)**, by xcorr
16,189 in both; 28,639 of 31,371 rows differ only in the E-value column. The batch path
behaves exactly like RTS.

**Chimeric search** (charges 2-4, +/- 1 m/z, <= 5 peptides; `docs/20261105_RTSchimeric.md`
10.4 has the full table):

| | iteration 0 rows (decoy %, q01) | later rows (decoy %, q01) | all q <= 0.01 |
|---|---|---|---|
| fill 0, dMaxExpect 0.1 | 21,398 (1.9%, 19,781) | 5,640 (7.4%, 2,019) | 22,258 |
| fill 100, dMaxExpect 0.05 | 18,763 (0.4%, 18,691) | 3,575 (2.2%, 3,000) | 22,188 |
| **fill 100, dMaxExpect 0.1** | 20,256 (0.9%, 20,071) | 4,686 (4.4%, 3,121) | **23,545** |
| fill 100, dMaxExpect 0.2 | 21,943 (2.0%, 20,165) | 6,583 (9.0%, 3,070) | 23,617 |

With the fill, `chimeric_fragindex_min_ions_score` no longer changes anything (the 2-hit
candidates are scored regardless; byte-identical with and without it): it was retired. The
hard `dMaxExpect` gate was tuned under optimistic E-values; now that they are honest, 0.05
loses ~1,400 iteration-0 rows and 0.1 is the better default for the harnesses, while 0.2 adds
0.3% in total at twice the later-row decoy rate.

**5.4 Cost** (single thread, full file, 87 s load subtracted): fill 0 **13.5 s**
(0.32 ms/spectrum), fill 100 **17.1 s** (0.41 ms/spectrum): +27% of a plain search whose
other stages got cheaper with sparse preprocessing (`docs/20261109_sparse_preprocessing.md`);
on the chimeric loop the fill is a few percent (`docs/20261105_RTSchimeric.md` 10.8).


**Decision**: `fragindex_histogram_fill` default **100**, as proposed in section 8;
carried out in Phase 3 together with the knob retirement and the chimeric gate change.

| Date | Phase | Result |
|---|---|---|
| 2026-11-06 | 0 | `COMET_EVALUE_TRACE` in `CalculateEValue()`; FI-vs-PI baseline tables A-D above (re-measured 2026-11-17 at `isotope_error` 2: the gap is the missing real chance-sharing tail plus, at this setting, the padding's normalisation against an unpadded PI). |
| 2026-11-06 | 1 | `fragindex_histogram_fill` (`Params.h` / `Constants.h` default 0, `Comet.cpp` parse table and `comet.params` text, `InitializeStaticParams()` with clamping to 0..`FRAGINDEX_MAX_NUMSCORED`); the fill in `SearchFragmentIndex()`: sub-floor candidates collected in the same map walk, the needed prefix `partial_sort`ed by (hits desc, index asc) and appended after the floor-clearing list, the one scoring loop taking them while `uiNumScored < fill` under the unchanged cap. `rts_repro` `histogramFill` argument. Byte-identical at 0: the full-surface RTS dump of the 197-spectrum fixture matches the original Phase-1 baseline on FI and PI with AScorePro off and on. T68 (`t68_make_histfill.py`: one target, 20 exact-2-hit and 30 exact-1-hit composition-identical fillers): histogram 1 / 51 / 25 entries at fill 0 / 100 / 25, one stored candidate throughout, identical xcorr, E-value 3.1e-11 -> 3.9e-9. Note for the semantics: `XcorrScoreI()`'s reporting floor is on the full-spectrum matched-ion count, so a fill candidate *can* be stored if it clears the same floor and xcorr threshold as any candidate; in every measurement so far the stored set was unchanged. |
| 2026-11-06 | 4 | Docs and regression expectations. T23/T24 count PSMs at 1% FDR by **xcorr**, which the fill never changes, so their checks and the pinned-baseline comparison are unaffected (noted in `tests/tests.md`); E-value-ranked FI counts move by about +1.6%. `comet.params` carries the new key; `docs/RealTimeSearch.md`, `GlobalVariables.md` and the `comet-search-run` skill describe it. No release-notes file exists in the repo, so the note for the next release is kept here: *"New parameter `fragindex_histogram_fill` (default 100). Fragment-ion-index searches now also score their 100 best-supported candidates below `fragindex_min_ions_score` into the E-value histogram, which brings FI E-values to peptide-index E-values for the same weak and mid-strength peptide-spectrum matches (they were optimistic by about 2x); xcorr, ranking, reported peptides and matched-peptide counts do not change, E-values of weak hits increase, and E-value-based filtering at a fixed cutoff admits fewer decoys. Set `fragindex_histogram_fill = 0` to reproduce previous E-values. Real-time search: `isotope_error = 2` in the example harness; `DoSingleSpectrumSearchChimeric()` for chimeric spectra."* |
| 2026-11-06 | 3 | Default set to 100 (`Constants.h`, `comet.params` text). `chimeric_fragindex_min_ions_score`, `Query::iFragIndexMinIonsScoreOverride` and the core's `iMinIonsScore` argument removed; `SearchFragmentIndex()` reads the configured floor again. `rts_repro` and `SearchChimericMS2.cs` lose their min-ions arguments and default `dMaxExpect` to 0.1. T68 updated (not sent = 100). Verified: T67/T68, the full suite and T22 at the new defaults; the full-file chimeric run at the defaults byte-identical to Phase 2's fill-100 / gate-0.1 run; Windows solution builds clean. Docs: `RealTimeSearch.md`, `GlobalVariables.md`, `DataStructures.md`, `tests.md`, the `rts_repro` README, the chimeric doc (4.6, 10.6, progress). |
| 2026-11-08 | review | Fill candidates made histogram-only (code review of the branch). Two leaks found: with `fragindex_min_ions_report` below `fragindex_min_ions_score` a fill candidate cleared the store gate and was reported (T68 fixture, score 3 / report 2: 1 PSM became 20, the 2-hit fillers), and every fill candidate counted toward `_uliNumMatchedPeptides` (pepXML `num_matched_peptides` 1 -> 51 at the defaults, moving SQT and Percolator `lnNumSP` too). `XcorrScoreI()` now recognizes a fill candidate by its hit count (FI_DB, below the scoring floor), keeps its histogram entry and skips the store gate and both counters; `InitializeStaticParams()` warns when the report floor is below the score floor and leaves the value as set. T69 (batch, pepXML) covers score 3 / report 3 and 3 / 2 with fill 0 and 100: one stored PSM and `num_matched_peptides` 1 in all four, the warning exactly when report < score, and the fill still moving the E-value. |
| 2026-11-06 | 2 | Full-file measurements at fill 25 / 50 / 100 (tables above, re-measured 2026-11-17 at `isotope_error` 2): at 100 the FI E-values match PI's on the same PSMs through the weak and middle xcorr deciles (0.84-1.10) and the median lands on PI's (1.06); the strongest deciles sit above PI (the candidate-cap bias, open). Decoy-winner leakage at E <= 0.05 halves (2.6% -> 1.1%), confident targets +1.6% on RTS and on batch (fused path), ahead of PI on yield; cost +27% single-threaded on the plain search. The chimeric knob becomes a no-op (byte-identical with and without it) and the chimeric `dMaxExpect` gate should move 0.05 -> 0.1 (+6.1% total confident PSMs). Default 100 confirmed. |

## 10. Open questions (resolved in section 8)

1. Default fill: 100 (= the cap, simplest: "always score the 100 best-supported
   candidates") or lower to bound the cost on busy indexes? Section 5.4 decides; 100 is the
   proposal.
2. Should 1-hit candidates count, or only 2-hit? On a 0.02 Da bin a single shared index ion
   is close to a chance event -- which is exactly the null we want -- but their number can
   be large; the ranked order means they are only reached when nothing better exists.
3. Batch FI output changes for weak PSMs. Acceptable for the next release, with a note in
   the release notes and the T23/T24 baseline expectation updated?

**2026-11-10 note.** A query with no candidate at or above `fragindex_min_ions_score` stores no PSM and never
computes an E-value, so the fill is skipped for it (`SearchFragmentIndex()`); the output is byte-identical and the
single-threaded cost drops below the figure quoted above (the review measured 11-15% of FI search time saved).
