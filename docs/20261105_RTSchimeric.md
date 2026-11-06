# RTSchimeric: chimeric-spectrum identification on the RTS path (2026-11-05)

Project plan for a new real-time-search (RTS) entry point that can report more than one
peptide from a single MS/MS spectrum. Written after the design discussion on 2026-11-05;
the decisions in section 2 are settled, everything after it is the implementation plan.

## 1. Goal

`DoSingleSpectrumSearchMultiResults()` (`CometSearch/CometSearchManager.cpp:2749`) answers
"what is the best peptide for this spectrum at this precursor m/z and charge?". The new
function `DoSingleSpectrumSearchChimeric()` answers "which peptides are in this spectrum?":

1. Search the spectrum once per **precursor candidate** (an (m/z, charge) pair -- either an
   explicit list, or one m/z expanded across a charge range).
2. Take the best PSM across all candidates (lowest E-value, ties by xcorr).
3. If its E-value is <= the caller's `dMaxExpect`, report it, **remove the fragment peaks
   it explains from the spectrum**, and go back to step 1 on the reduced spectrum.
4. Stop when the best PSM fails the E-value test, the per-spectrum peptide cap is reached,
   or the spectrum has too few peaks left to search.

Each reported row is the winning PSM of its iteration, with that iteration's own
delta-Cn, plus the charge state and iteration number it was found at.

## 2. Decisions from the design discussion

| # | Topic | Decision |
|---|-------|----------|
| 1 | Precursor mass window | The charge-range use case runs with a wide, fixed +/- Da window (m/z units, derived from the isolation window), not the configured `peptide_mass_tolerance`. Implemented as a per-call override applied to the per-Query window only (no `g_staticParams` writes). Explicit-list callers pass 0 to use the configured tolerance. |
| 2 | "Best" across candidates | Lowest `dExpect`; ties broken by higher `xCorr`. |
| 3 | Fragment removal | Bin-exact removal of the matched fragments the scorer credited, driven by the `vector<Fragment>` the existing extraction already produces. A `iRemoveFlankBins` knob (default 0) and an isotope-removal option exist so flanking-bin / isotope removal can be measured on real high-resolution data before deciding the defaults. Precursor-derived peaks stay with `remove_precursor_peak` (evaluated per pass with that pass's charge). |
| 4 | Decoy winners | Treated exactly like targets: reported, fragments removed, iteration continues. Needed for honest FDR on this path. |
| 5 | Termination guards | `iMaxPeptidesPerSpectrum` cap passed by the caller; `max_index_runtime` is a budget for the whole call; preprocessing returning null (too few peaks) ends the loop. |
| 6 | Rows per iteration | Only the winning PSM. `topN` is still accepted (passed through to the per-candidate core) but the chimeric loop reports one row per iteration. Multiple IDs at the same charge from different iterations are legitimate and expected. |
| 7 | Where charge lives | New fields `iCharge` and `iIteration` on `CometScores` / `ScoreWrapper`; no parallel out-vectors. The existing function fills `iCharge` too. |
| 8 | Charge/mass out of range | A candidate whose neutral mass falls outside `[dPeptideMassLow, dPeptideMassHigh]` is skipped silently; it never fails the call. |
| 9 | Cost | Measured with the benchmark harness; the charge/candidate loop stays sequential within a call (the C# layer already parallelises across spectra). |
| 10 | C# harness | New `RealtimeSearch/SearchChimeric.cs`. Explicit-list mode takes its candidates from multi-`Z`-line `.ms2` files (the Mango format); extend later if candidates come from elsewhere. |
| 11 | Tests | Synthetic two-peptide fixture + `tests/rts_repro` style native driver + a `run_tests.py` test. |
| 12 | Naming | `DoSingleSpectrumSearchChimeric`, `iMinPrecursorCharge`, `iMaxPrecursorCharge`, `dMaxExpect`, `iMaxPeptidesPerSpectrum`. |
| 13 | One function or two | One core taking a `vector<PrecursorCandidate>`; the charge-range form is a thin overload that builds the list. |
| 14 | Candidates that already produced an ID | Stay in the list for later iterations (symmetry with the charge-range mode; the E-value threshold decides). Revisit if measurements show wasted passes. |
| 15 | Per-candidate tolerance | `PrecursorCandidate::dToleranceMZ` (0 = inherit the per-call value) is designed in now so a mixed-confidence list (precise deconvolved precursors + a wide catch-all at the isolation-window centre) is a one-line change later. All current callers leave it 0. |

### What the Mango branch contributes

`origin/Mango` (one commit, `78bb7505`, plus `docs/20261029_mango_review.md`) handles
cleavable cross-linker data: each `Z` line of a multi-`Z`-line `.ms2` scan becomes an
independent batch `Query` against the full, unreduced peak list, labelled `000_A/000_B/...`
for downstream pairing. It does no fragment removal and no iteration, and runs only on the
batch path. Nothing of its scoring is reused here, but its input format (one peak list, N
`(MH+, z)` precursors) is exactly the explicit-list input, so the harness reads that format
and `tests/unit/data/t67_mango.ms2` on that branch is a usable second fixture.

## 3. Starting point in the code

Everything the loop needs already exists and is thread-local:

| Piece | Where | Reused how |
|-------|-------|------------|
| Charge-driven preprocessing into a caller-owned `Query*` | `CometPreprocess::PreprocessSingleSpectrumThreadLocal()` -> `PreprocessSingleSpectrumCore()` (`CometPreprocess.cpp:2357`, `:1689-1718`) | Called once per candidate per iteration; `dExpPepMass`, `usiMaxFragCharge`, precursor-peak removal and the mass window all follow from `iPrecursorCharge`. |
| Per-Query mass window | `CometPreprocess::AdjustMassTol()` (`CometPreprocess.cpp:2747`) | Gains the tolerance override; the isotope-error branch below it is untouched. |
| Index search + post-analysis on a `Query*` | `CometSearch::RunSearch(Query*)`, `CalculateSP/CalculateEValue/CalculateDeltaCn/CalculateAScorePro(Query*)` | Unchanged. |
| Result extraction, including the matched-fragment list | `DoSingleSpectrumSearchMultiResults()` Step 6 (`CometSearchManager.cpp:~3040-3480`) | Moved into a shared core; the `Fragment{mass, charge, type, number, neutralLoss, neutralLossMass}` list it builds is the removal set. |
| Thread-local binned spectrum | `g_rtsScratch.pdTmpRawData` via `GetRtsRawDataBuffer()` | One per thread; overwritten by every preprocess call, which is why extraction happens per candidate (section 4.4). |
| Natural stop | `PreprocessSingleSpectrumCore()` returns null below `minimum_peaks` (`CometPreprocess.cpp:1721`); `iMatchPeptideCount == 0` when nothing scores | Both end the loop without a sentinel. |
| Native RTS driver | `tests/rts_repro/rts_repro.cpp` | Extended with a chimeric mode. |

## 4. Design

### 4.1 Types (`CometSearch/CometData.h`)

```cpp
struct PrecursorCandidate
{
   double dMZ;
   int    iCharge;
   double dToleranceMZ;   // +/- window in m/z units for this candidate;
                          // 0 = inherit the per-call value (which may be 0 = configured params)

   PrecursorCandidate() : dMZ(0.0), iCharge(0), dToleranceMZ(0.0) {}
   PrecursorCandidate(double mz, int z, double tol = 0.0) : dMZ(mz), iCharge(z), dToleranceMZ(tol) {}
};
```

`CometScores` gains two fields, both zero-initialised in every constructor and copied in
the copy constructor / `operator=`:

```cpp
int iCharge;      // precursor charge the PSM was searched at
int iIteration;   // 0-based chimeric iteration; 0 for DoSingleSpectrumSearchMultiResults
```

`ScoreWrapper` (`CometWrapper/CometDataWrapper.h:331`) exposes both as read-only
properties.

### 4.2 Public API

`CometSearch/CometInterfaces.h` (`ICometSearchManager`), `CometSearch/CometSearchManager.h`,
and `CometSearchManager.cpp`:

```cpp
// Core form: explicit precursor candidates (e.g. deconvolved from the MS1 isolation window,
// or the Z lines of a multi-Z .ms2 scan).
virtual bool DoSingleSpectrumSearchChimeric(const int topN,
                                            const std::vector<PrecursorCandidate>& vPrecursors,
                                            const double dPrecursorToleranceMZ,   // per-call override; 0 = configured tolerance
                                            const double dMaxExpect,
                                            const int iMaxPeptidesPerSpectrum,
                                            double* pdMass,
                                            double* pdInten,
                                            const int iNumPeaks,
                                            vector<string>& strReturnPeptide,
                                            vector<string>& strReturnProtein,
                                            vector<vector<Fragment>>& matchedFragments,
                                            vector<CometScores>& scores) = 0;

// Convenience form: one m/z expanded across [iMinPrecursorCharge, iMaxPrecursorCharge].
virtual bool DoSingleSpectrumSearchChimeric(const int topN,
                                            const double dMZ,
                                            const int iMinPrecursorCharge,
                                            const int iMaxPrecursorCharge,
                                            const double dPrecursorToleranceMZ,
                                            const double dMaxExpect,
                                            const int iMaxPeptidesPerSpectrum,
                                            double* pdMass,
                                            double* pdInten,
                                            const int iNumPeaks,
                                            vector<string>& strReturnPeptide,
                                            vector<string>& strReturnProtein,
                                            vector<vector<Fragment>>& matchedFragments,
                                            vector<CometScores>& scores) = 0;
```

Return value: `true` if the call ran to a normal termination (including "zero peptides
passed the threshold"); `false` only on an error (init failure, cancel, `iNumPeaks == 0`,
empty candidate list, min > max charge). The outputs are appended to, like the existing
function.

The existing `DoSingleSpectrumSearchMultiResults()` signature does not change; it starts
filling `CometScores::iCharge` (`iIteration = 0`).

### 4.3 Internal refactor: one shared per-candidate core

`DoSingleSpectrumSearchMultiResults()` is ~750 lines doing preprocess -> search -> sort ->
SP/E-value/dCn/AScore -> extraction. That body becomes a private member:

```cpp
// One (spectrum, precursor) search. Appends up to takeN rows to the four output vectors.
// Returns the number of rows appended (0 = nothing scored / filtered / timed out), or -1 on
// error (bSucceeded == false in today's code). Never touches SearchSession.
int CometSearchManager::SearchSingleSpectrumCore(const int takeN,
                                                 const int iPrecursorCharge,
                                                 const double dMZ,
                                                 const double dToleranceMZ,       // 0 = configured
                                                 const double* pdMass,
                                                 const double* pdInten,
                                                 const int iNumPeaks,
                                                 const std::chrono::high_resolution_clock::time_point tStart,  // max_index_runtime budget origin
                                                 vector<string>& strReturnPeptide,
                                                 vector<string>& strReturnProtein,
                                                 vector<vector<Fragment>>& matchedFragments,
                                                 vector<CometScores>& scores);
```

`DoSingleSpectrumSearchMultiResults()` becomes: the two early-return checks, `Initialize`,
then one call with `tStart = now()` and `dToleranceMZ = 0`. The refactor is a pure move
and is landed first, on its own, so the existing RTS tests (`tests/rts_repro`, T-tests that
go through the wrapper) pin its behaviour before any new code arrives. `RTS_TIMING` output
moves with it and gains the charge/iteration columns.

Two small changes inside the moved body:

- `pQuery->tSearchStart = tStart` instead of `now()`, so `max_index_runtime` is a budget
  for the whole chimeric call (decision 5). The existing function passes `now()`, so its
  behaviour is unchanged.
- The tolerance override is threaded through
  `PreprocessSingleSpectrumThreadLocal()` -> `PreprocessSingleSpectrumCore()` ->
  `AdjustMassTol()` as a new trailing `double dToleranceMZ = 0.0` parameter. When > 0,
  `AdjustMassTol()` sets `dPeptideMassToleranceLow/High = -/+ dToleranceMZ * usiChargeState`
  (m/z-unit semantics, same scaling as `peptide_mass_tolerance` with `precursor_tolerance_type
  = 1`, `CometPreprocess.cpp:2755-2759`) and skips the units branch; the `isotope_error`
  branch that follows runs unchanged on top of it. Batch callers never pass it.

### 4.4 The chimeric loop

```
DoSingleSpectrumSearchChimeric(list form):
   validate: iNumPeaks > 0, !vPrecursors.empty(), iMaxPeptidesPerSpectrum > 0, Initialize ok
   tStart = now()
   copy pdMass/pdInten into thread-local vectors vMass/vInten   -- never mutate the caller's
                                                                  arrays (C# pins them)
   for iIteration in 0 .. iMaxPeptidesPerSpectrum-1:
      best = none
      for each candidate c in vPrecursors:
         neutral = c.dMZ * c.iCharge - (c.iCharge - 1) * PROTON_MASS
         if neutral outside [dPeptideMassLow, dPeptideMassHigh]: continue      -- decision 8
         tol = c.dToleranceMZ > 0 ? c.dToleranceMZ : dPrecursorToleranceMZ
         tmpPep/tmpProt/tmpFrag/tmpScore cleared
         n = SearchSingleSpectrumCore(takeN = 1, c.iCharge, c.dMZ, tol, vMass, vInten, size, tStart, tmp...)
         if n < 0: return false                                                -- hard error
         if n == 0: continue                                                   -- nothing scored
         if best is none or tmpScore[0] beats best (lower dExpect, tie: higher xCorr): best = tmp (move)
         if budget exceeded or g_cometStatus.IsCancel(): stop outer loop
      if best is none or best.dExpect > dMaxExpect: break
      best.score.iCharge = c.iCharge; best.score.iIteration = iIteration
      append best's row to the four outputs
      RemoveMatchedFragments(vMass, vInten, best.fragments)                     -- 4.5
      if vMass.size() < minimum_peaks: break                                   -- cheap pre-check; preprocess would also return null
   return true
```

Why extraction runs per candidate rather than once for the winner: the binned spectrum
that the matched-fragment extraction reads (`pdTmpSpectrum`, the thread-local
`g_rtsScratch.pdTmpRawData`) is overwritten by every preprocess call. Extracting the
rank-1 row (peptide string, protein string, matched fragments) is microseconds against a
multi-millisecond index search, so doing it for every candidate and keeping the best is
simpler than keeping a per-candidate copy of the 40 KB buffer.

`topN` is accepted for interface symmetry and is honoured by the convenience overload
exactly as by the list form: the core is always asked for 1 row per candidate. If a
future caller wants the losing candidates or the runner-up rows, `topN > 1` is the natural
place to hang that; the plan does not implement it now.

### 4.5 Fragment removal

```cpp
// Drops from (vMass, vInten) every peak whose BIN() lies within +/- iRemoveFlankBins of the
// bin of any matched fragment (and, if bRemoveIsotopes, of its +1 and +2 13C isotopes).
static void RemoveMatchedFragments(std::vector<double>& vMass,
                                   std::vector<double>& vInten,
                                   const std::vector<Fragment>& vFrags,
                                   int iRemoveFlankBins,
                                   bool bRemoveIsotopes);
```

- The fragment m/z is rebuilt exactly as the extraction computed it: `mz = (mass +
  (charge - 1) * PROTON_MASS) / charge`, minus `neutralLossMass / charge` when
  `neutralLoss` is set (`CometSearchManager.cpp:~3375` and `~3425`). `BIN()` of that m/z is
  the bin the scorer credited.
- Removed bins are collected into a sorted `vector<int>` (a few dozen entries); the peak
  arrays are then compacted in one pass. Peaks are **dropped, not zeroed**, so the
  `minimum_peaks` check (`CometPreprocess.cpp:1721`, which counts array entries) and
  AScorePro's `vRawFragmentPeakMassIntensity` both see the true remaining spectrum.
- Isotopes: bins of `mz + k * C13_DIFF / charge`, k = 1, 2.
- Knobs: two new `g_staticParams.options` entries, `chimeric_remove_flank_bins` (int,
  default 0) and `chimeric_remove_isotopes` (0/1, default 0), set through the usual
  `SetParam` plumbing so the C# harness can sweep them without a rebuild. They stay
  undocumented in `comet -p` output until the measurements in section 7.3 pick the defaults.

### 4.6 Threading

Nothing new is shared. Per call: the copied peak vectors (thread-local, reused across
spectra like `tl_vProteinTargets`), the per-candidate temporaries, and the thread's
`g_rtsScratch` pool. `g_staticParams` is read only; the tolerance override lives on the
`Query`. Concurrent calls from C# `Task`s remain safe for the same reason the existing
function is.

### 4.7 Edge cases

| Case | Behaviour |
|------|-----------|
| Candidate mass outside the index's `MassRange` | Skipped silently (decision 8). If every candidate is skipped the call returns `true` with no rows. |
| `iMinPrecursorCharge > iMaxPrecursorCharge`, or charge < 1 / > `MAX_FRAGMENT_CHARGE` bound used by preprocessing (`CometPreprocess.cpp:1689-1692`) | Overload returns `false`; list form skips the offending candidate (the preprocess core already rejects out-of-range charges). |
| Best PSM is a decoy | Reported and removed like a target (decision 4). |
| Same peptide wins twice | Possible only if removal left enough of its ions to re-score under `dMaxExpect`; reported both times. The diagnostics in 7.3 count this, since it is the clearest symptom of insufficient removal. |
| Spectrum drops below `minimum_peaks` | Loop ends; rows found so far are returned. |
| `max_index_runtime` exceeded mid-iteration | The current candidate's result is discarded, the loop ends, rows found so far are returned, `true`. |
| `g_cometStatus.IsCancel()` | `false`, as today. |

## 5. Files to touch

| Layer | File | Change |
|-------|------|--------|
| Native types | `CometSearch/CometData.h` | `PrecursorCandidate`; `CometScores::iCharge/iIteration` + ctors/copy/assign. |
| Native params | `CometSearch/core/Params.h`, `CometSearchManager.cpp` (`InitializeStaticParams` param copy block, ~`:774` region) | `iChimericRemoveFlankBins`, `bChimericRemoveIsotopes`. |
| Preprocess | `CometSearch/CometPreprocess.h/.cpp` | `dToleranceMZ` parameter on `PreprocessSingleSpectrumThreadLocal()`, `PreprocessSingleSpectrumCore()`, `AdjustMassTol()`. |
| Manager | `CometSearch/CometInterfaces.h`, `CometSearchManager.h/.cpp` | `SearchSingleSpectrumCore()` (moved body), two `DoSingleSpectrumSearchChimeric()` overloads, `RemoveMatchedFragments()`. |
| Wrapper | `CometWrapper/CometWrapper.h/.cpp`, `CometDataWrapper.h` | Two managed overloads (list form takes `List<PrecursorCandidateWrapper^>^` or parallel `double[]`/`int[]` arrays -- parallel arrays, to avoid another ref class and a per-candidate allocation); `ScoreWrapper::iCharge/iIteration`. |
| C# | `RealtimeSearch/SearchChimeric.cs`, `RealtimeSearch.csproj`, `Search.cs` (dispatch by a new CLI mode) | Section 6. |
| Native driver | `tests/rts_repro/rts_repro.cpp`, `ms2_to_fixture.py`, `README.md` | `--chimeric zmin zmax tolMZ maxExpect maxPep` mode; fixture format gains optional extra `Z` entries per scan for the list mode. |
| Tests | `tests/unit/run_tests.py`, `tests/unit/data/`, `tests/tests.md` | Section 7. |
| Docs | `docs/RealTimeSearch.md`, `docs/DataStructures.md`, `CLAUDE.md` threading table | Document the new entry point next to the existing RTS ones. |

## 6. C# harness: `RealtimeSearch/SearchChimeric.cs`

A third mode alongside `Search.cs` / `SearchMS1MS2.cs`, selected by a CLI argument. Inputs:
the `.ms2` file, the `.idx`, and the chimeric parameters.

- **Charge-range mode** (`--zrange zmin zmax --tol mzDa`): for every MS2 scan, call the
  overload with the scan's precursor m/z (the `S` line's m/z; for Mango-style files where
  it is 0.0, the first `Z` line's MH+ converted at its charge, as the Mango branch does at
  `CometPreprocess.cpp:505-508`).
- **Explicit-list mode** (`--zlines --tol mzDa`): one candidate per `Z` line of the scan
  (`(MH+ + (z-1) * PROTON) / z`, charge z). Multi-`Z`-line `.ms2` is the only source for
  now (decision 10).
- Common: `--max-expect E --max-peptides N --topn 1 --threads T`, same `ConcurrentQueue`
  work-stealing structure as `SearchMS1MS2.cs`.
- Output: one tab-delimited row per reported PSM -- `scan, iteration, charge, mz, neutral
  mass, peptide, protein, xcorr, dCn, expect, matched/total ions, elapsed ms (whole call)`
  -- plus a per-run summary (spectra searched, spectra with 0/1/2/3+ IDs, mean candidates
  searched per spectrum, mean wall-clock per spectrum). The row format is deliberately
  the same as `tests/rts_repro`'s so the two drivers can be diffed.

The `.ms2` reading stays in C# (MSToolkit is not exposed through the wrapper); the
existing `Search.cs` reader is reused and extended to keep every `Z` line of a scan.

## 7. Testing

### 7.1 Synthetic chimeric fixture (new unit test, next free ID -- T67 is taken on the Mango branch, so T68)

`tests/unit/data/t68_chimeric.fasta`: a small protein set containing two tryptic peptides
P1 and P2 of distinct masses, plus filler peptides. `tests/unit/data/t68_chimeric.ms2`
(generated by a committed script, `tests/unit/data/t68_make_chimeric.py`, so the fixture
is reproducible):

| Scan | Content | Expected |
|------|---------|----------|
| 1 | P1 at 2+ only | exactly one row, P1, charge 2 |
| 2 | P1 (2+) + P2 (3+) b/y ions merged, P1 ions 3x the intensity of P2's, precursor m/z set to P1's | two rows: iteration 0 = P1/2+, iteration 1 = P2/3+ (found through the wide window in range mode, through its own `Z` line in list mode) |
| 3 | P1 (2+) + P2 (2+), near-isobaric by construction (same nominal mass) | two rows, both charge 2 -- exercises decision 6's "same charge twice" |
| 4 | P1 at 2+ with `Z` lines claiming 2+ and 3+ | one row at 2+; the 3+ candidate must not win (its E-value is worse) |
| 5 | noise only | zero rows, call returns `true` |

Driven through `tests/rts_repro/rts_repro.cpp --chimeric ...` (built by `run_tests.py`
like the existing rts_repro case, see `tests/tests.md` "rts_repro" section) and asserted
on the tab-delimited output. Also asserted: after scan 2's first iteration, none of the
bins listed in P1's matched fragments survive in the peak list the second iteration
searched (exposed by an `RTS_CHIMERIC_DEBUG` env-var dump of the per-iteration peak count
and removed-bin list, the same opt-in pattern as `COMET_PRELOAD_BENCHMARK`).

### 7.2 Regression on the refactor

Before any new code: land `SearchSingleSpectrumCore()` and confirm `tests/rts_repro`'s
fixture output and every `run_tests.py` test that drives the wrapper/RTS path are
byte-identical to the pre-refactor build. This is the checkpoint that makes the rest safe.

### 7.3 Removal validation on real data (the measurement decision 3 asks for)

Run `SearchChimeric.cs` in charge-range mode over a real high-resolution file (the
`20260420-human-phosho/` set used in `docs/20260825_RTS.md`) with `fragment_bin_tol =
0.02` and the three settings `flank = 0`, `flank = 1`, `flank = 1 + isotopes`. Report, per
setting:

- IDs per spectrum histogram and total PSMs at iteration >= 1;
- fraction of iteration-1 winners whose matched ions fall within +/-2 bins of an
  iteration-0 removed bin (a "ghost" re-hit) -- the direct measure of leftover signal;
- count of same-peptide repeats (section 4.7);
- q-value at 1% on the iteration-0 rows vs the existing function's rows via
  `tools/qvalue.py` (must be unchanged: iteration 0 with `tol = 0` **is** the existing
  search), and separately on iteration >= 1 rows (the decoy winners from decision 4 make
  this computable).

The defaults for `chimeric_remove_flank_bins` / `chimeric_remove_isotopes` are set from
this table and the numbers go into a follow-up section of this document.

### 7.4 Benchmark

`comet-benchmark` skill, RTS mode, same file/index as 7.3: existing function vs. chimeric
with `zrange 2 4` at `max-peptides 1` (isolates the candidate-loop cost) and `max-peptides
5` (full cost). Expectation from the design: roughly `(zmax - zmin + 1) x mean iterations`
times the single-search cost; the point of measuring is to see whether preprocessing or
extraction becomes visible and whether the per-candidate `Query` allocation should go
through the `RtsScratch` pool.

## 8. Implementation phases

Each phase builds on Linux (`make`) and Windows (`MSBuild`, see `comet-build` skill) and
passes `run_tests.py` before the next starts. No commits unless asked.

| Phase | Work | Checkpoint |
|-------|------|------------|
| 0 | `CometScores::iCharge/iIteration` + `ScoreWrapper` properties; existing function fills `iCharge`. | Builds; wrapper consumers unaffected. |
| 1 | Refactor: `SearchSingleSpectrumCore()`; `tStart` and `dToleranceMZ` parameters threaded through preprocess (`= 0` / `now()` from the existing caller). | 7.2 byte-identical regression. |
| 2 | `PrecursorCandidate`, `RemoveMatchedFragments()`, both `DoSingleSpectrumSearchChimeric()` overloads, params for flank/isotopes, `RTS_CHIMERIC_DEBUG` dump. | `rts_repro --chimeric` mode + T68 fixture pass on Linux and Windows. |
| 3 | Wrapper overloads; `SearchChimeric.cs`; `.ms2` multi-`Z` reading. | Harness reproduces T68 results through C#; MSBuild + wrapper DLL copy step. |
| 4 | Removal validation (7.3) and benchmark (7.4) on real data; pick knob defaults. | Results appended to this doc. |
| 5 | Docs: `RealTimeSearch.md`, `DataStructures.md`, `tests/tests.md`, `CLAUDE.md` threading table. | -- |

## 9. Out of scope / later

- Candidate retirement after an ID (decision 14) -- revisit with 7.3/7.4 numbers.
- Per-candidate tolerance callers (decision 15) -- the field exists; nothing sets it yet.
- Precursor candidates from an in-process MS1 scan (RawFileReader) rather than `.ms2` `Z`
  lines (decision 10).
- Returning runner-up rows per iteration (`topN > 1`), and any combined/paired scoring
  of the reported peptides (the Mango-style questions in its review doc, section 6).
- Batch-path (`comet.exe`) exposure of the chimeric loop. The design is RTS-only; a batch
  version would want the fused path's arenas rather than per-call `Query` allocation.
