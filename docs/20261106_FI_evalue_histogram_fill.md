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

- The synthetic entries are 97% at xcorr 0.0-0.3 on a **full** spectrum (per query, median
  2999 / 2313 / 1797 / 1602 at 0.0-0.3; 86 / 37 / 12 / 5 / 1 at 0.4-0.8). Against a 0.02 Da
  bin width a random ion lands in a credited bin with probability of order 0.3%, so random
  peptides score ~0. This is not an artifact of chimeric subtraction (which thins the tail a
  further ~3x and lowers the winner's xcorr from 0.93 to 0.79); it is what the synthetic
  decoys are.
- The FASTA/PI null is different: real peptides at the right mass, most scoring ~0 too, but
  with a tail of peptides that share one to three ions by chance of composition and tryptic
  structure. FI drops exactly those peptides at the 3-ion candidate floor and replaces them
  with decoys that have no chance-sharing structure. The regression's cumulative fit starts
  at the "second-best" bin (`iNextCorr`, median xcorr 0.8-0.9) and runs downward, so the
  thin 0.4-0.9 bins *are* the fit.
- Letting 2-ion candidates be scored (report floor unchanged, so none can be reported)
  adds a median of 36 real entries per query, +30-50% in the 0.4-0.8 bins, flattens the
  slope (-0.457 -> -0.430) and raises weak winners' E-values x1.41 -- making them honest:
  on chimeric re-search passes the decoy rate among reported rows at E <= 0.05 halves
  (4.4% -> 1.9%) and confident IDs rise 43% (2,181 -> 3,116). On the plain RTS search, whose
  winners sit well above the tail, the same change is +1.7% confident IDs and a cleaner
  E <= 0.05 set (0.7% vs 1.2% decoy).

So FI E-values are optimistic for weak hits on every FI query, batch or RTS, and the
weaker the spectrum the worse. The chimeric knob papers over it for one caller.

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

## 8. Open questions for the discussion

1. Default fill: 100 (= the cap, simplest: "always score the 100 best-supported
   candidates") or lower to bound the cost on busy indexes? Section 5.4 decides; 100 is the
   proposal.
2. Should 1-hit candidates count, or only 2-hit? On a 0.02 Da bin a single shared index ion
   is close to a chance event -- which is exactly the null we want -- but their number can
   be large; the ranked order means they are only reached when nothing better exists.
3. Batch FI output changes for weak PSMs. Acceptable for the next release, with a note in
   the release notes and the T23/T24 baseline expectation updated?
