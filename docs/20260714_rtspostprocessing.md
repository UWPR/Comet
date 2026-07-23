# RTS PI_DB search: post-determination pipeline review

Status: Findings 1, 2, 4, 5, 6 IMPLEMENTED and validated (see their sections below); findings 3, 7, 8
remain REVIEW ONLY, no implementation change made for those. Scope: everything that happens after
`CometSearch::RunSearch(Query*)` has identified the candidate peptide matches for a single RTS
spectrum, through to `DoSingleSpectrumSearchMultiResults()` returning results to the C# caller.
Entry point: `CometSearchManager::DoSingleSpectrumSearchMultiResults()`
(`CometSearchManager.cpp:2399-3041`). Complements `docs/20260713_PIopt.md`, which covered the
search/scoring side of the same RTS path.

## Pipeline as it stands today

1. Sort all stored candidates by XCorr (`CometSearchManager.cpp:2515-2518`).
2. `CometPostAnalysis::CalculateSP()` -- SP (preliminary) score for the top `takeSearchResultsN`
   (`:2541`).
3. `CometPostAnalysis::CalculateEValue()` -- E-value via decoy-histogram linear regression
   (`:2555`).
4. `CometPostAnalysis::CalculateDeltaCn()` -- delta-Cn and rank assignment (`:2569`).
5. `CometPostAnalysis::CalculateAScorePro()` -- conditional, rank-1 only, mod-localization via the
   external AScorePro library (`:2588`).
6. Protein name resolution, modified-peptide string construction, and b/y (and optionally a/c/x/z,
   and neutral-loss) fragment-ion matching against the binned spectrum, per returned result
   (`:2634-3015`).
7. Cleanup: close any open file handle, `delete pQuery` (`:3034-3037`).

The code already has `RTS_TIMING` instrumentation bucketing each of these steps individually
(`llCalcSP`, `llCalcEValue`, `llCalcDeltaCn`, `llCalcAScore`, `llResults`, plus preprocess/search/sort)
-- this review is code-level (no live RTS_TIMING run was performed), so treat the ranking below as
reasoned from complexity/allocation analysis, not measured wall-clock proportions. Enabling
`RTS_TIMING` for a real RTS run (see the `comet-build` skill) would validate/reorder this ranking
before investing in any of the fixes below.

## Findings, ranked by expected impact

### 1. `_pResults`/`_pDecoys` are heap-allocated and reset from scratch on every spectrum -- IMPLEMENTED

`CometPreprocess::PreprocessSingleSpectrumCore()` -- called by `PreprocessSingleSpectrumThreadLocal()`,
the RTS entry point -- does, **on every single spectrum**:

```cpp
pScoring->_pResults = new Results[g_staticParams.options.iNumStored];   // CometPreprocess.cpp:1872
...
for (int j = 0; j < g_staticParams.options.iNumStored; ++j)             // :1877-1896
{
   pScoring->_pResults[j].dPepMass = 0.0;
   ...  // ~10 fields reset per element, plus 3 std::string/vector .clear() calls
}
```

`iNumStored` defaults to 100 (`core/Params.h:354`) and is never smaller than
`iNumPeptideOutputLines + 1` (`CometSearchManager.cpp:1393-1395`). `Results`
(`core/Types.h:44-74`) is a large struct:

| Member | Size |
|---|---|
| `piVarModSites[MAX_PEPTIDE_LEN_P2]` (`int[53]`) | 212 B |
| `pdVarModSites[MAX_PEPTIDE_LEN_P2]` (`double[53]`) | 424 B |
| `pszMod[MAX_PEPTIDE_LEN][MAX_PEFFMOD_LEN]` (`char[51][16]`) | 816 B |
| `szPeptide[MAX_PEPTIDE_LEN]` | 51 B |
| scalars (masses, scores, ranks, flags) | ~50 B |
| 2x `std::string`, 2x `std::vector<ProteinEntryStruct>` | ~112 B (empty/SSO) |
| **Total** | **~1.6 KB** |

So every RTS spectrum pays for a **~160 KB heap allocation**, 100x default-construction of a struct
with 4 non-trivial members, a hand-rolled 100-iteration reset loop touching ~10 fields each, and --
at the end of `DoSingleSpectrumSearchMultiResults()` -- a full `delete[] _pResults` (`Query`
destructor, `core/Types.h:792-799`) that runs 100 destructors. This is true even when the actual
search only ever populates a handful of candidates (`iMatchPeptideCount` can be far smaller than
100) and the caller only asked for `topN` (often 1-10) results back.

This is the exact class of problem `docs/20260713_PIopt.md` already fixed for the *spectrum-side*
scratch buffers via `RtsScratch` (`CometPreprocess.cpp:44-54` and surrounding, a `thread_local`
pool with `EnsureInitialized()`/reset-in-place semantics, "eliminates per-spectrum heap traffic").
`_pResults`/`_pDecoys` were not included in that pool and remain on the old per-spectrum
new/reset/delete pattern.

**Implemented**: extended `RtsScratch` (`CometPreprocess.cpp`) with `pResults`/`pDecoys` pool
fields plus `EnsureResultsInitialized()` (allocates `Results[iNumStored]` once per thread, and
`pDecoys` only when `iDecoySearch == 2`, re-checked the same way `EnsureInitialized()` re-checks
`iArraySizeGlobal`) and `ResetResultsForNewSpectrum()` (resets the same field list the batch path's
inline loop already reset, factored into a shared `ResetOneResult()` helper to avoid the two loops
drifting apart). `PreprocessSingleSpectrumCore()` now branches on `bUseThreadLocalPool`: the RTS
path pulls `_pResults`/`_pDecoys` from the pool instead of `new`-ing and manually resetting them;
the batch path (`bUseThreadLocalPool=false`) is untouched, byte-for-byte the same code as before.
Added `Query::bResultsFromPool` (`core/Types.h`), mirroring the existing `bSparseFromPool` flag
exactly, so the destructor skips `delete[] _pResults`/`delete[] _pDecoys` when the memory is
pool-owned -- the pool frees it when the thread exits, not when any individual `Query` is destroyed.

**Validation**:
- Full Linux unit suite (19 tests) passes unchanged -- confirms the untouched batch path still
  works (this suite doesn't exercise the RTS single-spectrum path at all, since that's reached only
  through the C++/CLI wrapper from C#, so this alone would not have caught a bug in the pooling
  logic itself).
- Built `CometWrapper.dll`/`RealtimeSearch.exe` via MSBuild and ran a real RTS search: 8 threads,
  49,844 MS2 scans from `20250520_Hela_60min_06.raw` against a freshly built 5,445,388-peptide PI_DB
  index (`human.small.fasta`, trypsin, M-oxidation + STY-phospho). Correct peptide/mod output
  throughout (e.g. `K.KSALDEIM[15.9949]EIEEEK.K`, `R.KT[79.9663]IGY[79.9663]KVPR.N`), no crashes, no
  errors -- confirms the thread-local pool behaves correctly under real 8-way concurrency (each
  thread gets its own `g_rtsScratch` instance automatically via `thread_local`, so there's no
  cross-thread sharing to get wrong, but this exercises the actual allocate/reset/reuse cycle
  thousands of times per thread against real data, not just a synthetic test).
- Used the existing `RTS_TIMING` instrumentation (temporarily enabled in
  `CometSearch.vcxproj`'s Release config, reverted after measuring -- see the "resultsalloc" column
  documented in `CometPreprocess.cpp`'s `PREPROCESS` printf) to directly measure the specific step
  this change targets, comparing the same search before (`git stash`, i.e. the pre-pooling code)
  and after (pooled) on identical input:

  | | mean | median | total (49.8K spectra) |
  |---|---|---|---|
  | Before (heap alloc + full reset) | 2.41 us | 2.0 us | 0.120 s |
  | After (pooled, reset in place) | 0.90 us | 1.0 us | 0.045 s |
  | **Change** | **-62.6%** | | **-0.075 s** |

  This is a precise, isolated measurement of the exact code path changed, not an end-to-end
  estimate, and confirms the fix works as designed.

**Honest caveat -- end-to-end throughput did not move measurably.** Across three repeated full
searches (49,844 spectra, 8 threads, `RTS_TIMING` off for these runs), MS2 search throughput was
5119-5492 Hz both before and after, with the pre/post pairs falling well within that same
run-to-run spread -- i.e. no end-to-end win outside of noise, despite the real, measured 62.6%
reduction in the targeted step. Reconciling the two: the `resultsalloc` step was already tiny
relative to total per-spectrum cost (a 49.8K-spectrum, 8.4s search averages ~169 us/spectrum;
`resultsalloc` was ~2.4 us of that even *before* this change, roughly 1.4%), so the OS/CRT heap
allocator was evidently already handling this specific repeated-same-size allocation pattern
efficiently (likely via a per-thread free list, not going back to the OS on every call) -- the fix
removes real, measured work, but that work was never the bottleneck in this particular workload.
Other steps (`CalculateAScorePro` most plausibly, per finding 6) dominate total per-spectrum time
and were untouched by this change. The fix is still worth keeping: it's architecturally correct,
matches an established pattern in the same codebase, removes genuine (if small) per-spectrum
allocator pressure that could matter more under different allocator behavior or workloads with many
more spectra per second, and cost nothing in maintainability (the batch path is provably unchanged).
It should not be read as a demonstrated end-to-end speedup, though.

### 2. Reset loop and sort both touch all `iNumStored` (100) entries when only a handful are ever read -- PARTIALLY IMPLEMENTED

Two separate but related over-scoping issues on the same array; only the second was implemented --
the first was investigated and found unsafe to change.

**Reset loop: investigated, NOT changed.** The hypothesis was that resetting slots beyond
`iMatchPeptideCount` might be unnecessary. Verified this is false: `RunSearch()`'s candidate
insertion logic (`CometSearch.cpp:4753-4757`) scans **all** `iNumStored` slots on every candidate
considered, to find the current lowest-scoring slot to replace --
`for (short siA = 1; siA < g_staticParams.options.iNumStored; ++siA) { if
(pQuery->_pResults[siLowestXcorrScoreIndex].fXcorr > pQuery->_pResults[siA].fXcorr) ... }`. This
means every slot's `fXcorr` (and other compared fields) must already hold a valid sentinel value
before the search begins, for *every* slot, regardless of how many real candidates end up matching
this spectrum -- the search can't know in advance which slots it will end up using. Skipping the
reset for any subset of slots chosen ahead of time would let stale data from a previous pooled
spectrum corrupt this scan. Left as-is; this is a case where the original caution ("not
independently verified... treat as a hypothesis to confirm... since an incomplete reset could
introduce a stale-data read bug") turned out to be warranted.

**Sort: IMPLEMENTED.** `std::sort(pQuery->_pResults, pQuery->_pResults + iSize, SortFnXcorr)`
(`CometSearchManager.cpp`) replaced with `std::partial_sort` bounded to
`iSortBound = min(iSize, max(takeSearchResultsN, iNumPeptideOutputLines + 1))`. The
`iNumPeptideOutputLines + 1` term (not just `takeSearchResultsN`) is required because
`CalculateDeltaCn()` -> `CalculateDeltaCnsAndRank()` walks up to that many entries and, unlike
`CalculateEValue()`, *does* depend on adjacency-based sort order to compute delta-Cn/rank correctly
for the entries actually returned -- using only `takeSearchResultsN` as the bound would have been
wrong whenever a caller requests fewer results (`topN`) than the server-side `num_output_lines`
config. `takeSearchResultsN`'s computation was moved earlier in the function (it only ever depended
on the `topN` parameter and `iSize`, both already known before the old sort call) so it's available
to compute this bound.

**Validation.** Given this touches only the RTS single-spectrum path (unreachable from Linux batch
tests), the same rigor as finding 1 was applied, plus a full-output correctness diff (finding 1 only
checked "doesn't crash" via a search summary, not an exhaustive per-PSM comparison):

- Full Linux unit suite (19 tests) passes -- confirms no build/logic regression reachable from that
  suite (which doesn't exercise this code path either, same caveat as finding 1).
- Built `CometWrapper.dll`/`RealtimeSearch.exe` and ran the same 8-thread, 49,844-spectrum RTS
  search as finding 1, capturing the **full per-PSM `rts.out` file** (not just a slowest-N summary)
  for an exact diff between the pre-change and post-change binaries -- 45,076 returned PSM lines.
  Every peptide/xcorr/charge/mass field matched exactly except for a small number of entries where
  a *different* peptide won a near-tied score, and a somewhat larger number where only the E-value
  differed for the *same* winning peptide.
- **These residual differences were investigated and are pre-existing RTS nondeterminism, not
  something this change introduced.** Reran the *unmodified* (pre-change) binary a second time and
  diffed it against its own first run: the same classes and magnitude of difference appeared (9
  peptide-flip "core mismatches" and 280 E-value-only mismatches out of 44,759 compared entries,
  same-code-vs-same-code, versus 10 and 269 respectively for before-vs-after) -- including one
  identical peptide flip (scan 25030, `R.SVSSSSYRR.M` <-> `R.AYAETSKMK.V`) appearing in *both*
  comparisons. This matches this repository's own commit history (`git log` shows "Fix
  non-deterministic FASTA_DB search results under concurrent scoring ties"), i.e. RTS's
  multi-threaded near-tied-score handling has known, pre-existing run-to-run nondeterminism
  unrelated to this change. With that noise source accounted for, the diff shows no evidence this
  change altered correctness.
- `RTS_TIMING`-instrumented before/after comparison of the `llSort` column specifically:

  | | mean | median | total (49.8K spectra) |
  |---|---|---|---|
  | Before (`std::sort`, full range) | 64.22 us | 68.0 us | 3.198 s |
  | After (`std::partial_sort`, bounded) | 10.80 us | 11.0 us | 0.538 s |
  | **Change** | **-83.2%** | | **-2.660 s** |

  A sanity-check column (`llRunSearch`, the XCorr scoring step, untouched by this change) was also
  compared and came back essentially identical before/after (637.668 us vs 637.860 us mean),
  confirming the measurement methodology isn't picking up unrelated noise.
- **Unlike finding 1, this shows a real end-to-end win, not just an isolated one**: `llTotal` (sum
  of per-spectrum wall time, the same metric the printed Hz figure derives from) dropped from
  1316.6 us to 1265.1 us mean, a 3.9% reduction, and the printed `MS2 search elapsed time`/Hz moved
  consistently in the same direction across the compared runs (7.96s/5425 Hz before vs 7.68s/5586 Hz
  after under identical `RTS_TIMING`-instrumented conditions). The reasoned prediction in the
  original review (`O(n log k)` vs `O(n log n)`, plus far fewer ~1.6 KB full-struct swaps) held up
  better here than it did for finding 1, likely because `iSortBound` in this workload (M-oxidation +
  STY-phospho params, `num_output_lines` and `topN` both small) was a much smaller fraction of
  `iSize` (up to 100) than finding 1's per-spectrum allocation was of total per-spectrum time.

### 3. `Results`' size amplifies both of the above

At ~1.6 KB per element, `Results` is heavy for something sorted and bulk-reset routinely. The two
largest members -- `pdVarModSites[53]` (424 B) and `pszMod[51][16]` (816 B, used only for PEFF mod
strings) -- dominate. A lower-effort, lower-risk version of findings 1/2 (pool + partial_sort)
captures most of the win without touching the struct layout; shrinking `Results` itself (e.g.
storing PEFF mod strings only when actually a PEFF search) is a real but more invasive follow-on,
not suggested as a first step.

### 4. Output-loop vectors have no `reserve()` -- IMPLEMENTED

In the per-result extraction loop (`CometSearchManager.cpp`):

- The four outer result vectors (`strReturnPeptide`, `strReturnProtein`, `matchedFragments`,
  `scores`) are sized to exactly `takeSearchResultsN` iterations but never `reserve(takeSearchResultsN)`
  before the loop -- straightforward, safe fix.
- `eachMatchedFragments` (`vector<Fragment>`, declared fresh per result) is filled inside
  a `position x charge x ionSeries x [neutral-loss: VMODS x 2]` nested loop via repeated
  `push_back()` with no `reserve()`. A rough upper bound
  (`usiLenPeptide * usiMaxFragCharge * iNumIonSeriesUsed`, ignoring NL) avoids most of the
  reallocation churn for longer peptides / higher charge states / multiple ion series enabled.

**Implemented**: both `reserve()` calls added exactly as scoped above. Low-risk, mechanical change
-- Linux unit suite (19 tests) passes. Not separately performance-measured (per the original review,
this was flagged as low-effort/low-risk, not expected to be individually significant relative to
findings 1/2); bundled with the E-value investigation below rather than measured in isolation.

### 5. Protein name lookup allocates fresh vectors per result -- IMPLEMENTED

`vProteinTargets`/`vProteinDecoys` (`vector<string>`) were declared fresh inside the per-result
loop. For PI_DB/FI_DB (the indexed-DB path relevant here), protein names already come from the
in-memory `g_pvProteinNameCache` -- no file I/O, which is good -- but the vectors themselves were
still heap-churn per result.

**Implemented**: two `thread_local std::vector<string>` (`tl_vProteinTargets`/`tl_vProteinDecoys`)
declared once per thread, `.clear()`-ed (not reallocated) at the top of each result instead of
being freshly constructed; the rest of the loop body binds local references to them so the
surrounding logic is unchanged. `DoSingleSpectrumSearchMultiResults()` is the RTS single-spectrum
entry point only (never called from the batch multi-threaded path -- confirmed by grep, only one
C++ implementation and one caller, the C++/CLI wrapper), so `thread_local` is safe: each RTS worker
thread gets its own persistent pair of vectors, reused both across results within one spectrum and
across spectra over that thread's lifetime.

**Validation**: same before/after `git stash` methodology as findings 1/2/4 -- built via MSBuild,
ran the same 8-thread, ~49,844-scan RTS search against `human.small.fasta`'s PI_DB index twice
(before/after), diffed the full `rts.out`. Of 281 differing lines, 280 had byte-identical peptide
and protein fields (only E-value differed -- the same pre-existing near-tied-score nondeterminism
documented under the E-value investigation above); the 1 remaining difference was a genuinely
different winning peptide with its own (correct) protein string, i.e. also the pre-existing
near-tie-flip class, not a protein-string bug. No case had a *mismatched* protein for the *same*
winning peptide, which is what this change could plausibly have broken. Linux unit suite (19 tests)
passes.

**Honest performance result**: `RTS_TIMING`'s `llResults` bucket (the step this touches) showed no
measurable change (4.382us -> 4.379us mean, -0.1%, effectively noise) -- consistent with the
original review's own "low priority... small win" framing, and with finding 1's similar outcome:
the targeted step was already a small fraction of per-spectrum time (`llResults` is ~0.3% of
`llTotal` per the finding-6 measurement table), so removing its allocator churn doesn't move the
end-to-end number. Kept anyway: it's architecturally correct, matches the established
thread-local-pool pattern used elsewhere in this file, and removes genuine (if small) per-result
heap traffic.

### 6. `CalculateAScorePro` cost -- MEASURED; premise was wrong, and a real correctness bug was found and fixed

The original text of this finding speculated AScorePro was "plausibly the single largest cost among
the post-determination steps" and recommended measuring `llCalcAScore` via `RTS_TIMING` before
investing further. Doing that measurement surfaced something more important than a timing number.

**AScorePro was never running in RTS PI_DB search at all**, regardless of `print_ascorepro_score`.
The first `RTS_TIMING` run showed `llCalcAScore == 0` for **100% of ~49,800 spectra**, and every
`AScore` field in `rts.out` was `0.00`. Root-caused via two layers:

1. `RealtimeSearch/SearchMS1MS2.cs` never reads `comet.params` -- every search parameter is set via
   hardcoded `SetParam()` calls in `SearchSettings.ConfigureInputSettings()`, and
   `print_ascorepro_score` was hardcoded to `0` (off) there, silently ignoring whatever the params
   file said. (This also means most other `comet.params` edits made during this investigation, and
   presumably in prior sessions, had no effect on RTS behavior -- only the params this method
   explicitly forwards, plus mod info read from the `.idx` header, actually apply.)
2. Even after wiring that up (see fix below), AScorePro *still* didn't run: `g_AScoreInterface`
   stayed `nullptr` for the whole session. `CometSearchManager::InitializeSingleSpectrumSearch()`'s
   FI_DB branch calls `SetAScoreOptions()` + `CreateAScoreDllInterface()` when loading its index, but
   the **PI_DB branch right below it never did** -- it only calls `ReadPeptideIndex()` and sets
   `g_bPeptideIndexRead = true`. `CometSearch::EnsurePeptideIndexLoaded()` has its own copy of the
   same interface-creation code, but it's gated on `g_bPeptideIndexRead` being false -- which by the
   time `RunSearch(Query*)` first calls it, is already true (set by
   `InitializeSingleSpectrumSearch()` above), so that code path is dead for RTS PI_DB. Confirmed via
   a temporary debug print: `cHasVariableMod` correctly evaluated to the AScorePro-triggering value
   for ~45% of matched spectra (phospho search), but `g_AScoreInterface` was `0x0000000000000000`
   every time.

**Fixed, two changes:**
- `RealtimeSearch/SearchMS1MS2.cs`: added an optional 5th CLI argument (`ascorepro`: 0=off default,
  1=localize all mods), threaded through `ConfigureInputSettings()`, replacing the hardcoded `0`.
  Defaults to the prior (off) behavior so existing invocations of `RealtimeSearch.exe` are unaffected.
  (Superseded later in the same branch: the default was subsequently changed to `1` (on), so
  invocations without the 5th arg now get AScorePro localization enabled by default.)
- `CometSearchManager.cpp`: added the same `SetAScoreOptions()` + `CreateAScoreDllInterface()` block
  to the PI_DB branch of `InitializeSingleSpectrumSearch()` that the FI_DB branch already had, so
  `g_AScoreInterface` actually gets created for RTS PI_DB search.
- `CometPostAnalysis.cpp`: added a defensive `if (ascoreInterface == nullptr) return;` at the top of
  `CalculateAScorePro()` as a safety net, since a null interface reaching this function (from either
  this bug recurring elsewhere, or a future code path) should silently no-op rather than risk
  dereferencing a null pointer.

**Real `RTS_TIMING` measurement, with AScorePro now genuinely running** (same 8-thread,
~49,800-scan PI_DB RTS run, `print_ascorepro_score=-1` via the new `ascorepro=1` flag):

| Step | mean | % of `llTotal` |
|---|---|---|
| `llPreprocess` | 426.51 us | 32.20% |
| `llRunSearch` | 644.21 us | 48.63% |
| `llSort` | 10.84 us | 0.82% |
| `llCalcSP` | 1.10 us | 0.08% |
| `llCalcEValue` | 191.45 us | 14.45% |
| `llCalcDeltaCn` | 0.06 us | 0.00% |
| **`llCalcAScore`** | **43.18 us** | **3.26%** |
| `llResults` | 3.80 us | 0.29% |

`llCalcAScore` ran on 45.2% of spectra (matched a variable mod); when it ran, mean 95.44 us, median
53 us, p90 188 us, max 5278 us (occasional slow outlier worth noting for tail latency, not
investigated further here).

**Conclusion: the original finding's premise was wrong.** Now that it's measured rather than
guessed, `CalculateAScorePro` is a small fraction of total per-spectrum time (3.26%), smaller than
`CalculateEValue` (14.45%, and already documented as optimized -- see finding 7) and far smaller
than `llPreprocess`/`llRunSearch` (80.8% combined, out of scope for this "post-determination" review).
There is no performance case for a dedicated `AScorePro/`-internals investigation right now. The
actual valuable outcome here was the correctness fix: RTS PI_DB searches requesting AScorePro
localization were silently getting empty results with no error, for every RTS PI_DB deployment using
this C# harness, regardless of `comet.params` -- that's now fixed and validated (Linux unit suite,
including T19/T20's own AScorePro assertions, plus a live RTS run showing 22,047/49,844 spectra with
real non-zero AScore/site-score output).

### 7. Not a finding -- confirmed already well-optimized

`CalculateEValue()` -> `GenerateXcorrDecoys()` (`CometPostAnalysis.cpp:1262-1342`) uses a documented,
already-implemented inverted-index scatter approach (`std::call_once`-guarded global init,
thread-local score accumulator sized to fit L1 cache) explicitly commented as a "10-30x reduction"
over the naive per-decoy-per-ion lookup it replaced. `LinearRegression()`
(`CometPostAnalysis.cpp:1083-1249`) operates on a fixed `HISTO_SIZE`=152 array regardless of
`iNumStored`/candidate count -- bounded, cheap, no action suggested. `CalculateSP()`'s per-position
fragment lookup (`FindSpScore`, `CometPostAnalysis.cpp:1345-1357`) is an O(1) sparse-array index,
not a search. `CalculateDeltaCnsAndRank()` (`CometPostAnalysis.cpp:269-363`) is bounded to
`iNumPeptideOutputLines + 1` internally regardless of how many candidates were matched -- worst-case
O(n^2 x peptideLen) but n is tiny (typically single digits), negligible in practice. None of these
four are suggested as optimization targets.

### 8. Minor, low-confidence notes (not prioritized)

- Five separate `high_resolution_clock::now()` + `duration_cast` timeout checks are scattered
  through the function for the same `iMaxIndexRunTime` budget (`CometSearchManager.cpp:2504,2530,
  2547,2561,2606`). Each is cheap individually; consolidating wouldn't meaningfully change wall
  time, just code shape. Not worth doing on its own.
- Peptide string construction via repeated `std::string operator+=` (`:2653-2682`) for a
  <=50-character sequence plus a few mod annotations is unlikely to matter given
  geometric-growth `std::string` implementations and the small final size -- flagged for
  completeness, not recommended as a priority.
- The FASTA_DB (non-indexed) protein-lookup branch (`:2716-2765`) does per-protein `fseek`/`fgets`
  file I/O per result -- real cost, but out of scope: PI_DB (the case this review is about) already
  avoids it via the in-memory `g_pvProteinNameCache` path (`:2692-2713`). Noted as a confirmation
  that PI_DB doesn't have this problem, not as a new finding.

## RTS E-value nondeterminism under concurrency -- RESOLVED

Separate from the 8 findings above: replicate 8-thread RTS runs against identical input (same
`.raw` file, same PI_DB index) produce a small number of PSMs (roughly 200-300 out of ~44,700
scored spectra, ~0.5-0.7%) whose E-value differs between runs, and occasionally (single digits)
a different peptide wins a near-tied XCorr. This was investigated via clean bisection rather than
code reading alone, since the affected code (RTS single-spectrum path) is unreachable from the
Linux batch unit suite.

**Bisection methodology.** Built a PI_DB index from `human.small.fasta` and ran the same 8-thread,
~49,844-scan RTS search against `20250520_Hela_60min_06.raw` twice per configuration, diffing the
full `rts.out` (normalizing away the per-spectrum timing column, which is expected to vary).

| Configuration | Differing PSM lines |
|---|---|
| `num_threads=1` | **0** -- fully deterministic |
| `num_threads=8`, baseline (finding 1+2 code) | ~292-300 |
| `num_threads=8`, `print_ascorepro_score=0` (AScorePro disabled) | 281 -- rules out AScorePro |
| `num_threads=8`, entire per-spectrum C++ pipeline (preprocess through `CalculateAScorePro`) serialized behind one mutex | 71-86 -- large reduction, not zero |
| `num_threads=8`, C# `rawFile` reads serialized (fix below), C++ unserialized | ~297 -- no measurable change alone |
| `num_threads=8`, both of the above combined | 86 -- consistent with the C++-only-serialized row |

The single-thread row proves the underlying search/scoring/E-value code is deterministic given
identical input -- this is a concurrency effect, not floating-point or algorithmic nondeterminism.
Ruled out along the way: `AcquirePoolSlot()`/`SearchMemoryPool` (mutex+condition-variable protected,
20 logical cores on the test machine vs. 8 RTS threads, no starvation), the `RtsScratch` sparse
child-block pool (proved safe by induction: each `ResetForNewSpectrum()` fully re-zeros exactly what
the immediately preceding spectrum on that thread wrote, so no stale-data path exists regardless of
how usage varies spectrum to spectrum), `GenerateXcorrDecoys()`/`LinearRegression()` (both pure
functions of the query's own sparse spectrum data plus read-only-after-`call_once` global tables,
confirmed via code trace), and `Query::iXcorrHistogram` zero-initialization (confirmed in the
constructor).

**Found and fixed: unsynchronized concurrent access to the shared Thermo `IRawDataPlus` reader.**
`RealtimeSearch/SearchMS1MS2.cs`'s `ProcessScans()` worker (the function actually compiled into
`RealtimeSearch.exe` -- a second, unused `Search.cs` in the same project has a different, single
raw-file-access-per-scan structure but isn't part of the `.csproj`'s `Compile` items) has 8 `Task`
threads pulling from a `ConcurrentQueue<int>` of scan numbers and calling `rawFile.Get...()` methods
(`GetScanStatsForScanNumber`, `GetCentroidStream`, `GetSegmentedScanFromScanNumber`,
`GetScanEventForScanNumber`, `GetTrailerExtraInformation`) directly on one shared `rawFile` instance,
under a comment claiming "SHARED raw file reader (thread-safe)". That claim was never verified and
turned out to parallel the AScorePro interface's similar unverified "assumed thread-safe" comment in
`CometSearchManager.cpp` (see `g_AScoreInterface`) -- Thermo's RawFileReader evidently serializes
concurrent calls internally enough not to crash, but does not guarantee the returned scan content is
correct/consistent when multiple threads call it concurrently.

**Fix**: wrapped every `rawFile.*` call inside `ProcessScans()` in `lock (rawFileLock)` (two lock
scopes: one for scan stats/RT/filter/peak-data retrieval, one for the MS2 precursor
m/z-and-charge-from-trailer-data lookup), leaving the CPU-bound `DoSingleSpectrumSearchMultiResults`/
`DoMS1SearchMultiResults` calls unlocked so the actual scoring work stays parallel across threads --
only the I/O calls into the shared reader are serialized. This is a real, independently-justified
thread-safety fix (unsynchronized concurrent calls into a shared native/COM object is unsound
regardless of its measured impact on this specific symptom) and has been kept.

**Update: the residual left by the C# fix was tracked down and fixed in a dedicated follow-up
investigation -- see `docs/20260714_EvalueJitter.md` for the full plan, bisection, and root-cause
localization.** Summary of what that investigation found beyond this section: the C# `rawFile` fix
above was real but not the dominant cause (confirmed here already, "no measurable difference...
when the C++ side runs normally"). The actual root cause was in
`CometPreprocess.cpp::PreprocessSingleSpectrumCore()`'s RTS thread-local-pool path:
`pPre.iHighestIon` (updated from every peak's bin, including peaks whose bin lands at or beyond
`iArraySize`) was passed uncapped into `MakeCorrData()`, which reads/writes
`pdTmpRawData`/`pdTmpCorrelationData` up to `iHighestIon` -- but the RTS path's buffer-clearing
optimization only pre-zeroes those buffers up to `iArraySize + iXcorrProcessingOffset`, not the full
`iArraySizeGlobal` the batch path always clears. A spectrum with any peak beyond that range let
`MakeCorrData` read stale data left behind by a *different, larger* spectrum previously processed on
the same thread, corrupting the windowed-normalization result (and therefore XCorr) for candidates
whose fragment bins shared a window with the contaminated region -- explaining both the "same
peptide, different E-value" cases (a non-winning candidate's score shifts a histogram bin) and the
rarer "different peptide wins" cases. Confirmed via a temporary full-clear experiment (jitter
dropped to zero across 4 independent run pairs; reverting brought it back), then fixed narrowly by
clamping `iHighestIon` to `iArraySize - 1` before the `MakeCorrData()` call -- no memset widening
needed, since `pdTmpRawData` is provably zero beyond `iArraySize` once cleared. This is why
`CalculateSP`/`CalculateEValue`/`CalculateDeltaCn` themselves checked out clean in the paragraph
above: the corruption happened upstream, in preprocessing, before any of those functions ever see
the data.

**Validation of both fixes (final)**: replicate real-Windows-RTS-harness runs (same 8-thread,
49,844-scan methodology as the bisection table above) are now **byte-identical** -- 0 differing PSM
lines in the full `rts.out` diff, matching the determinism bar batch search already meets. Also
validated via `tests/rts_repro/` (a fast, Linux-buildable, Thermo-independent reproducer built
during the follow-up investigation): 5 independent 8-thread runs, all pairwise diffs zero. Linux
unit suite (19 tests) passes; an `RTS_TIMING` before/after comparison showed no measurable
performance change (+0.36% mean `llPreprocess`, within run-to-run noise), since the fix is a single
integer clamp, not a widened memset.

## Suggested next step, if you want to act on this

Findings 1, 2, 4, 5, and 6 are done (see above). Finding 1's own `RTS_TIMING` measurement showed the
step it targeted was already a small fraction of per-spectrum time (no measurable end-to-end win,
despite a real 62.6% reduction in the targeted step); finding 2's did show a real end-to-end win
(3.9% total per-spectrum time, from an 83.2% reduction in the sort step specifically); finding 5
landed like finding 1 (real, correct, no measurable end-to-end win -- `llResults` was already ~0.3%
of total time); finding 6 turned out not to be a performance question at all -- AScorePro was
completely non-functional in RTS PI_DB search (a real correctness bug, now fixed), and once measured
for real it's only 3.26% of total per-spectrum time, not the dominant cost the original speculative
ranking assumed. Measuring before optimizing paid off repeatedly here: once by redirecting effort in
finding 2 (bounding beat pooling), once by setting realistic expectations for finding 5, and once by
revealing finding 6's target wasn't even running.

- `CalculateEValue` (14.45% of total per-spectrum time, per the finding-6 table) is now the largest
  measured cost among the post-determination steps -- but finding 7 already documents it as using a
  10-30x-optimized inverted-index scatter approach, so this isn't a fresh, unexamined target; treat
  it as "already addressed" unless a further measurement shows otherwise.
- The reset-loop half of finding 2 was investigated and found unsafe to change as originally framed
  (see above) -- not worth revisiting unless the underlying insertion algorithm in `RunSearch()`
  changes to not require all `iNumStored` slots to hold a valid sentinel throughout the search.
- The E-value nondeterminism residual (above) is the most promising remaining correctness lead if
  you want to pursue it further: the bisection table shows the source is real and C++-side, roughly
  3-4x larger than the (now-fixed) C# raw-file race, and not yet localized to a specific function.
  A profiler/ThreadSanitizer run against the RTS path (not attempted this session -- this repo's
  Linux build doesn't currently have a TSan target) would likely localize it faster than further
  manual bisection.
