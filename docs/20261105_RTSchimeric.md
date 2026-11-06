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
| 3 | Fragment removal | Remove every bin a theoretical fragment of the winning PSM can be credited in: all enabled ion series, fragment charges 1..`usiMaxFragCharge`, variable-mod neutral-loss variants, plus the flanking bins when `theoretical_fragment_ions = 0` (xcorr credits those at half weight), plus the +1..+N 13C isotope peaks of fragments that actually have a peak (N = `chimeric_remove_isotopes`, default 2). This is the model `mango_search_wrapper.py` validated (see "Prior art" below); the knobs stay so the defaults can be confirmed on real high-resolution data. Precursor-derived peaks stay with `remove_precursor_peak` (evaluated per pass with that pass's charge). |
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

### Prior art: `mango_search_wrapper.py`

`brucelab/mango_internal`, branch `mango-search-wrapper`, `tools/mango_search_wrapper.py`
(+ `tools/README.md`, version 2026.10.30) is an out-of-process prototype of this loop for
cleavable cross-linker data: it runs batch Comet on a multi-`Z`-line `.ms2`, picks the
winner of each precursor pair by E-value (ties by xcorr -- decision 2 independently),
rebuilds the winner's fragment bins from the pepXML with a re-implementation of Comet's
mass tables, writes a reduced `.ms2` (scan `scan*1000+k`, losers as `Z` lines) and
re-searches it, then merges pepXML. Comet is never modified. Everything below is taken
from its README and code and shapes this plan:

- **Removal model (adopted, decision 3).** It removes every *theoretical* fragment bin
  (not only bins with a peak): all enabled series x charges 1..`max_frag_charge(z)`, the
  neutral-loss ion of every fragment carrying a variable mod with a fragment NL
  (`variable_modNN` 8th field, exactly as `CalculateSP` credits it), the two flanking bins
  when `theoretical_fragment_ions = 0`, and the +1..+N 13C peaks (`remove_isotope_peaks`,
  default 2) computed from the *observed* peak m/z of ions that actually have a peak, so
  unmatched theoretical ions never strip `theoretical + 1.003`. Removal is always applied
  to the original spectrum as a union of sets, never to an already-edited copy. Its
  `--validate` mode reproduced every rank-1 `calc_neutral_pep_mass` /
  `num_matched_ions` / `tot_num_ions` on the 2026-10-30 test files (20 ppm, 0.02 Da
  bins), so the model is known to match what Comet scores. In-process we get this for
  free -- the ion loop in Step 6 of the existing function *is* Comet's -- so the plan
  collects the bins there instead of re-deriving them.
- **The fragment-index candidate-step pitfall (new to this plan, section 4.6).** Comet
  only scores a peptide when >= `fragindex_min_ions_score` (3) of its fragments coincide
  with the `fragindex_num_spectrumpeaks` (150) most intense peaks. After the winner's
  peaks are removed a second peptide can lose one of the peaks that qualified it and is
  then never scored at all, rather than scored lower: on file F10, 66 of 586 re-searched
  losers vanished although on average they kept 3.5 of their 5.9 matched ions. Re-search
  with `fragindex_min_ions_score = fragindex_min_ions_report = 2` cut that to 3 of 586, at
  the price of more junk hits with large E-values (400 vs 37) that the E-value threshold /
  PeptideProphet discount. End to end (XLinkProphet, P >= 0.9): pass 1 alone 253
  cross-links; two-pass with identical params **248** (a net loss); two-pass with the
  lowered threshold 275. So removal without the candidate-step fix can lose ground.
- **Hits where there were none** after removal are expected: removal changes which peaks
  reach the candidate list, so weaker candidates become reachable with large E-values.
  `dMaxExpect` is what keeps them out.
- **Same-precursor skip** (`same_precursor_ppm`, 20): two candidates with the same charge
  and mass within the tolerance are one precursor. Adopted for the explicit-list form as
  candidate de-duplication (section 4.8).
- **Counters** worth reproducing in the harness/diagnostics: "winner by E-value disagrees
  with xcorr", per pass "same peptide as before / different peptide / no hit / hit where
  there was none", "same peptide as partner", "two distinct peptides at E <= 0.01".
- **Known gap it documents**: with `use_NL_ions = 1` the water/ammonia-loss peaks are
  scored by xcorr but not removed. Same gap here: the extraction loop only enumerates the
  variable-mod neutral losses. Noted in section 10.
- Cross-link-specific pieces (`keep_stump_terminal_ions`, pepXML merging, pair roles) do
  not carry over.

Its re-search `.ms2` (one `S` block, several `Z` lines, reduced peak list) is also the
input format the C# harness reads for explicit-list mode (section 6).

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
         minIons = iIteration >= 1 ? chimeric_fragindex_min_ions_* : 0         -- 4.6; 0 = params unchanged
         n = SearchSingleSpectrumCore(takeN = 1, c.iCharge, c.dMZ, tol, minIons, vMass, vInten, size, tStart, tmp...,
                                      &tmpRemovalBins)                        -- 4.5
         if n < 0: return false                                                -- hard error
         if n == 0: continue                                                   -- nothing scored
         if best is none or tmpScore[0] beats best (lower dExpect, tie: higher xCorr): best = tmp (move)
         if budget exceeded or g_cometStatus.IsCancel(): stop outer loop
      if best is none or best.dExpect > dMaxExpect: break
      best.score.iCharge = c.iCharge; best.score.iIteration = iIteration
      append best's row to the four outputs
      vRemovedBins |= best.removalBins (collected by the core, 4.5)
      RemovePeaksInBins(vMass, vInten, vRemovedBins)                            -- 4.5
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

The removal set is collected inside the Step 6 ion loop of the shared core while it
extracts the rank-1 row: that loop already enumerates every theoretical ion (all enabled
series, charges 1..`usiMaxFragCharge`, variable-mod neutral-loss variants) and tests
`pdTmpSpectrum[BIN(mz)] > 0` (`CometSearchManager.cpp:~3375-3430`). The core gains an
optional `std::vector<int>* pvRemovalBins` output: when non-null, **every** theoretical
ion's bin goes in (not only the matched ones -- an unmatched bin is a no-op to remove, and
the wrapper's measurements show the complete set is what works), and for ions that have a
peak the isotope bins are added from the observed peak's m/z. The chimeric loop then
calls:

```cpp
// Drops from (vMass, vInten) every peak whose BIN() is in vBins (sorted, unique).
static void RemovePeaksInBins(std::vector<double>& vMass,
                              std::vector<double>& vInten,
                              const std::vector<int>& vBins);
```

Bin set for one theoretical ion of m/z `mz` at fragment charge `z`:

- `BIN(mz)`, and when `theoretical_fragment_ions = 0` also `BIN(mz) +/- 1` -- xcorr credits
  flanking bins at half weight, so leaving them is leaving signal that scored.
- If a peak exists in any of those bins, for `k = 1..chimeric_remove_isotopes`:
  `BIN(mz_obs + k * C13_DIFF / z)` with the same flanks, where `mz_obs` is the most
  intense raw peak in the matched bin (the preprocessed buffer only keeps the max per bin,
  so the raw-peak lookup is a per-call `bin -> m/z` map built once per iteration).
- Neutral-loss variants: `mz - dNLmass / z`, exactly as the loop computes `dNLfragMz`.

Peaks are **dropped, not zeroed**, by compacting the two arrays in one pass, so the
`minimum_peaks` check (`CometPreprocess.cpp:1721`, which counts array entries) and
AScorePro's `vRawFragmentPeakMassIntensity` see the true remaining spectrum. Removal is
cumulative on the original spectrum: iteration n removes the union of iterations 0..n-1's
sets.

Knobs (new `g_staticParams.options` entries, set through `SetParam` so the C# harness can
sweep them without a rebuild):

| Param | Default | Meaning |
|-------|---------|---------|
| `chimeric_remove_flank_bins` | -1 | -1 = follow `theoretical_fragment_ions` (flanks iff 0); 0/1 force off/on. |
| `chimeric_remove_isotopes` | 2 | +1..+N 13C peaks of fragments that have a peak; 0 = off. |

The defaults are the wrapper's validated ones; section 7.3 confirms them on RTS data.

### 4.6 Candidate-step thresholds on re-search passes

The wrapper's F10 numbers (section 2, "Prior art") show that the fragment-index candidate
filter, not the scoring, is what loses second peptides: after removal a peptide can fall
below `fragindex_min_ions_score` among the top `fragindex_num_spectrumpeaks` peaks and is
never scored. Two new params, `chimeric_fragindex_min_ions_score` and
`chimeric_fragindex_min_ions_report` (default 0 = no change), apply to iterations >= 1
only. They are carried on the `Query` (`Query::iFragIndexMinIonsScoreOverride`, set by the
core from a new optional argument) and consulted at the three comparison sites
(`CometSearch.cpp:1936`, `:1982`, `:8796`) in place of `g_staticParams` when non-zero.
`g_staticParams` itself is never written, so the batch path and iteration 0 are untouched.
Section 7.3 measures 3 vs 2 the way the wrapper did; the wrapper's default is "unchanged",
and that stays the default here unless the RTS numbers say otherwise.

Lowering the threshold admits more weak candidates on re-search passes; `dMaxExpect` is
the guard, and the harness reports "hits where there were none" separately so the effect
is visible.

### 4.7 Threading

Nothing new is shared. Per call: the copied peak vectors and the `bin -> m/z` map
(thread-local, reused across spectra like `tl_vProteinTargets`), the per-candidate
temporaries, and the thread's `g_rtsScratch` pool. `g_staticParams` is read only; the
tolerance and min-ions overrides live on the `Query`. Concurrent calls from C# `Task`s
remain safe for the same reason the existing function is.

### 4.8 Edge cases

| Case | Behaviour |
|------|-----------|
| Candidate mass outside the index's `MassRange` | Skipped silently (decision 8). If every candidate is skipped the call returns `true` with no rows. |
| Duplicate candidates in the explicit list (same charge, m/z within the candidate's effective tolerance) | De-duplicated once at call entry, keeping the first; mirrors the wrapper's `same_precursor_ppm` skip. The charge-range overload never produces duplicates. |
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
| Native params | `CometSearch/core/Params.h`, `CometSearchManager.cpp` (`InitializeStaticParams` param copy block, ~`:774` region) | `iChimericRemoveFlankBins`, `iChimericRemoveIsotopes`, `iChimericFragIndexMinIonsScore`, `iChimericFragIndexMinIonsReport`. |
| Preprocess | `CometSearch/CometPreprocess.h/.cpp` | `dToleranceMZ` parameter on `PreprocessSingleSpectrumThreadLocal()`, `PreprocessSingleSpectrumCore()`, `AdjustMassTol()`. |
| Search | `CometSearch/core/Types.h` (`Query`), `CometSearch/CometSearch.cpp:1936,1982,8796` | Per-Query min-ions override fields consulted in place of `g_staticParams` when non-zero. |
| Manager | `CometSearch/CometInterfaces.h`, `CometSearchManager.h/.cpp` | `SearchSingleSpectrumCore()` (moved body, optional removal-bin output), two `DoSingleSpectrumSearchChimeric()` overloads, `RemovePeaksInBins()`. |
| Wrapper | `CometWrapper/CometWrapper.h/.cpp`, `CometDataWrapper.h` | Two managed overloads (list form takes `List<PrecursorCandidateWrapper^>^` or parallel `double[]`/`int[]` arrays -- parallel arrays, to avoid another ref class and a per-candidate allocation); `ScoreWrapper::iCharge/iIteration`. |
| C# | `RealtimeSearch/SearchChimeric.cs`, `RealtimeSearch.csproj`, `Search.cs` (dispatch by a new CLI mode) | Section 6. |
| Native driver | `tests/rts_repro/rts_repro.cpp`, `ms2_to_fixture.py`, `README.md` | `--chimeric zmin zmax tolMZ maxExpect maxPep` mode; fixture format gains optional extra `Z` entries per scan for the list mode. |
| Tests | `tests/unit/run_tests.py`, `tests/unit/data/`, `tests/tests.md` | Section 7. |
| Docs | `docs/RealTimeSearch.md`, `docs/DataStructures.md`, `CLAUDE.md` threading table | Document the new entry point next to the existing RTS ones. |

## 6. C# harness: `RealtimeSearch/SearchChimeric.cs`

A third mode alongside `Search.cs` / `SearchMS1MS2.cs`, selected by a CLI argument. Inputs:
the `.ms2` file, the `.idx`, and the chimeric parameters.

- **Charge-range mode** (`--zrange zmin zmax --tol mzDa`): for every MS2 scan, call the
  overload with the scan's precursor m/z (the `S` line's m/z; when it is 0.0, as the
  wrapper's re-search files write it, the first `Z` line's MH+ converted at its charge:
  `(MH+ + (z-1) * PROTON) / z`).
- **Explicit-list mode** (`--zlines --tol mzDa`): one candidate per `Z` line of the scan
  (`(MH+ + (z-1) * PROTON) / z`, charge z). Multi-`Z`-line `.ms2` -- the format
  `mango_search_wrapper.py` consumes and writes -- is the only source for now
  (decision 10).
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

### 7.1 Synthetic chimeric fixture (new unit test, next free ID at implementation time -- T67 on master today)

`tests/unit/data/tNN_chimeric.fasta`: a small protein set containing two tryptic peptides
P1 and P2 of distinct masses, plus filler peptides. `tests/unit/data/tNN_chimeric.ms2`
(generated by a committed script, `tests/unit/data/tNN_make_chimeric.py`, so the fixture
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
0.02`, sweeping removal (`flank = 0`, `flank = 1`, `flank = 1 + isotopes = 2`) and the
re-search candidate threshold (`chimeric_fragindex_min_ions_score/report` = unchanged vs
2). Report, per setting:

- IDs per spectrum histogram and total PSMs at iteration >= 1;
- fraction of iteration-1 winners whose matched ions fall within +/-2 bins of an
  iteration-0 removed bin (a "ghost" re-hit) -- the direct measure of leftover signal;
- count of same-peptide repeats (section 4.8);
- the wrapper's counters: "winner by E-value disagrees with xcorr"; per iteration "hit
  where there was none" (candidates reachable only because removal changed the top-150
  peak list) and "no hit" (the candidate-step loss the min-ions override is for);
- q-value at 1% on the iteration-0 rows vs the existing function's rows via
  `tools/qvalue.py` (must be unchanged: iteration 0 with `tol = 0` **is** the existing
  search), and separately on iteration >= 1 rows (the decoy winners from decision 4 make
  this computable) -- the analogue of the wrapper's "two distinct peptides at E <= 0.01"
  gain/loss table, which is the number that decides whether the feature nets positive.

The defaults for the four `chimeric_*` params are confirmed or changed from this table
and the numbers go into a follow-up section of this document.

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
| 2 | `PrecursorCandidate`, removal-bin collection in the core, `RemovePeaksInBins()`, both `DoSingleSpectrumSearchChimeric()` overloads, the four `chimeric_*` params and the per-Query min-ions override, `RTS_CHIMERIC_DEBUG` dump. | `rts_repro --chimeric` mode + the 7.1 fixture pass on Linux and Windows. |
| 3 | Wrapper overloads; `SearchChimeric.cs`; `.ms2` multi-`Z` reading. | Harness reproduces the 7.1 results through C#; MSBuild + wrapper DLL copy step. |
| 4 | Removal validation (7.3) and benchmark (7.4) on real data; pick knob defaults. | Results appended to this doc. |
| 5 | Docs: `RealTimeSearch.md`, `DataStructures.md`, `tests/tests.md`, `CLAUDE.md` threading table. | -- |

## 9. Progress

| Date | Phase | Result |
|------|-------|--------|
| 2026-11-06 | 0 | `CometScores::iCharge` / `iIteration` (`CometData.h`), `ScoreWrapper::Charge` / `Iteration` (`CometDataWrapper.h`); `DoSingleSpectrumSearchMultiResults()` fills `iCharge`. |
| 2026-11-06 | 1 | `SearchSingleSpectrumCore()` holds the moved body; `dToleranceMZ` threaded through `PreprocessSingleSpectrumThreadLocal()` -> `PreprocessSingleSpectrumCore()` -> `AdjustMassTol()` (override branch in place, default 0 = unchanged); `tStart` with the default-constructed sentinel keeping the old "after preprocessing" clock. Regression (7.2): a full-surface dump driver (top-5, every `CometScores` field, every matched fragment; a scratch copy of `tests/rts_repro/rts_repro.cpp`) over the 197-spectrum fixture is byte-identical before and after on FI and PI, each with AScorePro off and on (1220 / 1212 / 4244 / 4231 lines), as is the stock `rts_repro` output. `run_tests.py`: 85/85 unit, T22 RTS FI/PI (+ protein-terminal mod variants) 4/4. Windows: full solution (Release x64) builds clean, `CometWrapper.dll` included. |
| 2026-11-06 | 2 | `PrecursorCandidate` (`CometData.h`); the four `chimeric_*` params (`core/Params.h`, `SetParam` only); `Query::iFragIndexMinIonsScoreOverride` / `ReportOverride` consulted at the three `CometSearch.cpp` sites; removal-bin collection in the core's Step 6 ion loop (`AddChimericRemovalBins()`: every theoretical ion, flanks per `theoretical_fragment_ions`, observed-peak isotopes), `RemovePeaksInBins()`, both `DoSingleSpectrumSearchChimeric()` overloads, `RTS_CHIMERIC_DEBUG` trace. `rts_repro` gained chimeric modes 1 (charge range) and 2 (candidate list = blocks sharing a scan). Test T67 (`tests/unit/data/t67_make_chimeric.py` + `run_tests.py`): all five fixture scans behave as section 7.1 specifies on FI_DB and PI_DB in both forms, the threshold ends the loop, the trace shows all 18 of P1's peaks removed, 1 vs 4 threads byte-identical. One behaviour the fixture made explicit: in scan 2 the 3+ 14-mer wins iteration 0 on E-value (52 ions vs 18) although it has a third of the intensity -- the loop orders by evidence, not abundance. The pre-existing path stayed byte-identical (same full-surface dump as Phase 1). |
| 2026-11-06 | 3 | `CometWrapper`: both `DoSingleSpectrumSearchChimeric()` overloads (the list form takes parallel `double[] m/z`, `int[] charge`, `double[] tolerance` arrays -- no managed candidate type, nothing allocated per candidate) with the output conversion factored into one helper shared with `DoSingleSpectrumSearchMultiResults()`. `RealtimeSearch/SearchChimeric.cs`: `RealtimeSearch.exe chimeric <spectra.ms2> <database.idx> [--zrange zmin zmax \| --zlines] [--tol mz] [--max-expect E] [--max-peptides N] [--threads T] [--ascorepro 0\|1] [--flank -1\|0\|1] [--isotopes N] [--min-ions-score N] [--min-ions-report N] [--max-runtime ms] [--out file]`, selected by `chimeric` as the first argument of the existing executable; its own `.ms2` reader (every `Z` line kept), the MS1/MS2 harness's `ConfigureInputSettings()` block reused (`SearchSettings` made `internal`), the same `ConcurrentQueue` worker pattern; one tab-delimited row per reported peptide (scan, iteration, charge, precursor m/z, candidate count, calculated neutral mass, peptide, protein, xcorr, delta-Cn, E-value, ions, elapsed ms) and a run summary (spectra with 0/1/2/3+ peptides, rows at iteration >= 1, same-peptide repeats, errors). `t67_make_chimeric.py` also writes the scans as a multi-`Z`-line `.ms2`. Verified: on the T67 `.ms2` against a Windows-built FI index, both forms reproduce `rts_repro`'s rows exactly (peptides, charges, xcorr, E-values, ion counts); Windows solution builds clean. |
| 2026-11-06 | -- | Build-system side work prompted by Phase 1's baseline capture: root `-j` race (MSToolkit entered twice concurrently) fixed with an order-only `mstoolkit` target; automatic `-MMD -MP` header dependencies in all four Makefiles (previously only four objects wrote `.d` files and none were included); MSToolkit objects now wait for the expat/zlib header install. |

## 10. Out of scope / later

- Candidate retirement after an ID (decision 14) -- revisit with 7.3/7.4 numbers.
- Per-candidate tolerance callers (decision 15) -- the field exists; nothing sets it yet.
- Precursor candidates from an in-process MS1 scan (RawFileReader) rather than `.ms2` `Z`
  lines (decision 10).
- Returning runner-up rows per iteration (`topN > 1`), and any combined/paired scoring
  of the reported peptides.
- `use_NL_ions = 1` water/ammonia-loss peaks: scored by xcorr but not in the removal set
  (same gap as the wrapper documents). Add to the Step 6 enumeration if 7.3's ghost-hit
  rate points at them.
- Batch-path (`comet.exe`) exposure of the chimeric loop. The design is RTS-only; a batch
  version would want the fused path's arenas rather than per-call `Query` allocation.
