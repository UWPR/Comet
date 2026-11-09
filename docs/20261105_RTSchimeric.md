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
| 6 | Rows per iteration | Only the winning PSM; the chimeric loop reports one row per iteration. Neither overload takes a `topN` (removed 2026-11-09: it was accepted and ignored, an API trap; a runner-up option would be a new parameter, section 11). Multiple IDs at the same charge from different iterations are legitimate and expected. |
| 7 | Where charge lives | New fields `iCharge` and `iIteration` on `CometScores` / `ScoreWrapper`; no parallel out-vectors. The existing function fills `iCharge` too. |
| 8 | Charge/mass out of range | A candidate whose MH+ lies above `dPeptideMassHigh` is skipped silently; it never fails the call. There is no lower bound, the same gate `DoSingleSpectrumSearchMultiResults()` applies: with an isotope error or a wide m/z tolerance a precursor just below `dPeptideMassLow` can still match a peptide inside the range (changed 2026-11-09; before, candidates below the range were skipped too). |
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
  and mass within the tolerance are one precursor. First adopted for the explicit-list form
  as candidate de-duplication with the candidate's search tolerance as the window; replaced
  on 2026-11-08 by dropping exact duplicates only (section 4.8). A fixed ppm assumes one
  instrument's mass accuracy, and a window tied to the search tolerance made the result
  depend on list order, so precursor merging is left to whatever builds the list.
- **Counters** worth reproducing in the harness/diagnostics: "winner by E-value disagrees
  with xcorr", per pass "same peptide as before / different peptide / no hit / hit where
  there was none", "same peptide as partner", "two distinct peptides at E <= 0.01".
- **Known gap it documents**: with `use_NL_ions = 1` the water/ammonia-loss peaks are
  scored by xcorr but not removed. Same gap here: the extraction loop only enumerates the
  variable-mod neutral losses. Noted in section 11.
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
virtual bool DoSingleSpectrumSearchChimeric(const std::vector<PrecursorCandidate>& vPrecursors,
                                            const double dPrecursorToleranceMZ,   // per-call override; 0 = configured tolerance
                                            const double dMaxExpect,
                                            const int iMaxPeptidesPerSpectrum,
                                            double* pdMass,
                                            double* pdInten,
                                            const int iNumPeaks,
                                            vector<string>& strReturnPeptide,
                                            vector<string>& strReturnProtein,
                                            vector<vector<Fragment>>& matchedFragments,
                                            vector<CometScores>& scores,
                                            bool& bIncomplete) = 0;

// Convenience form: one m/z expanded across [iMinPrecursorCharge, iMaxPrecursorCharge].
virtual bool DoSingleSpectrumSearchChimeric(const double dMZ,
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
                                            vector<CometScores>& scores,
                                            bool& bIncomplete) = 0;
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
         if MH+ above dPeptideMassHigh: continue                              -- decision 8
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

The core is always asked for 1 row per candidate. Neither overload takes a `topN`: one was
accepted "for interface symmetry" and ignored until 2026-11-09, when it was removed, since a
caller passing `topN > 1` silently got one row per iteration. If a future caller wants the
losing candidates or the runner-up rows, that is a new parameter (section 11).

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
| ~~`chimeric_remove_flank_bins`~~ | -- | Removed 2026-11-08: the flank always follows `theoretical_fragment_ions` (+/- 1 bin iff it is 0), the bins the scorer credits. 10.2 measured flanks alone barely differ from bin-exact removal, so the knob had nothing to tune; forcing 1 under `theoretical_fragment_ions = 1` removed uncredited peaks, forcing 0 under 0 left half-credited ones. |
| `chimeric_remove_isotopes` | 2 | +1..+N 13C peaks of fragments that have a peak; 0 = off. Clamped to 0..3 at init with a warning (2026-11-08). |

The defaults are the wrapper's validated ones; section 7.3 confirms them on RTS data.

### 4.6 Candidate floor on re-search passes

As designed, this followed the wrapper's F10 observation (section 2, "Prior art") that
second peptides vanish because they fall below `fragindex_min_ions_score` after removal,
and it had two params (`_score` and `_report`). The measurements (sections 10.2, 10.4,
10.7) showed the vanishing does not reproduce here and that the benefit is a different
mechanism entirely, so the design is now:

- For a while one param remained, `chimeric_fragindex_min_ions_score` (re-search passes
  only, carried on the `Query`), whose real effect was to let 2-ion candidates be *scored
  into the E-value histogram* without becoming reportable: on a stripped spectrum only a
  handful of real candidates clear 3 ions and the synthetic decoy padding's tail is steeper
  than the real one, so weak re-search winners got optimistic E-values (10.7).
- That is a property of every fragment-index query, not of chimeric passes, and
  `fragindex_histogram_fill` (`docs/20261106_FI_evalue_histogram_fill.md`, default 100) now
  fixes it for all of them; with it on, the chimeric floor was byte-for-byte a no-op and was
  removed. The reporting floor (`fragindex_min_ions_report`) is never lowered: the few PSMs
  that a 2-ion report floor lets win are junk (10.7). `dMaxExpect` remains the guard on what
  is reported -- 0.1 by default in the harnesses now that the E-values are calibrated.

### 4.7 Threading

Nothing new is shared. Per call: the copied peak vectors and the `bin -> m/z` map
(thread-local, reused across spectra like `tl_vProteinTargets`), the per-candidate
temporaries, and the thread's `g_rtsScratch` pool. `g_staticParams` is read only; the
tolerance and min-ions overrides live on the `Query`. Concurrent calls from C# `Task`s
remain safe for the same reason the existing function is.

### 4.8 Edge cases

| Case | Behaviour |
|------|-----------|
| Candidate MH+ above the index's `MassRange` | Skipped silently (decision 8); a candidate below the range is searched, as in `DoSingleSpectrumSearchMultiResults()`. If every candidate is skipped the call returns `true` with no rows. |
| Duplicate candidates in the explicit list | Only exact duplicates -- same charge, same m/z and same effective tolerance, hence the same search -- are dropped at call entry, keeping the first; every other candidate is searched in the caller's order, however close (2026-11-08). Precursor merging is the caller's job: it knows the instrument's mass accuracy, and a near-duplicate only costs one extra search, never a duplicate row (one row per iteration). Until then same-charge candidates within the incoming candidate's tolerance were merged, which depended on list order. The charge-range overload never produces duplicates. |
| Charge range: `iMinPrecursorCharge < 1` or `> iMaxPrecursorCharge`; charges above `MAX_PRECURSOR_CHARGE` (9) | The overload returns `false` for a minimum below 1 or above the maximum. Otherwise it intersects the range with 1..`MAX_PRECURSOR_CHARGE` before allocating anything, and returns `false` if nothing is left (2026-11-08; before, 10..12 returned `true` with nothing searched and a huge maximum aborted with `std::bad_alloc`). The list form drops a candidate with a charge below 1, and preprocessing rejects one above 9. |
| Candidate charge outside `min_precursor_charge` / `max_precursor_charge` | Skipped by preprocessing, like a mass outside the index range; both RTS entry points honor these as batch does (2026-11-08). |
| Best PSM is a decoy | Reported and removed like a target (decision 4). |
| Same peptide wins twice | Possible only if removal left enough of its ions to re-score under `dMaxExpect`; reported both times. The diagnostics in 7.3 count this, since it is the clearest symptom of insufficient removal. |
| Spectrum drops below `minimum_peaks` | Loop ends; rows found so far are returned. |
| `max_index_runtime` exceeded mid-iteration | The current candidate's result is discarded (an expired core search returns no rows), no further candidate is searched, and the call ends with `true`. The best PSM among the candidates searched so far in that iteration is reported if it passes `dMaxExpect` -- it was fully scored, but it may not be the best over every candidate -- after the rows of earlier iterations. The loop checks the budget itself, before each candidate and after each core call (2026-11-08). `bIncomplete` is set (2026-11-08). |
| `g_cometStatus.IsCancel()`, or an index search fails (core returns -2) | `false` with `bIncomplete` set. The rows of the iterations completed before it are kept: they are fully scored and valid, and the call just stopped early (2026-11-08). Before, the call returned `false` with those rows and nothing saying they were usable, so every caller dropped them. |
| Candidate m/z NaN, infinite, zero or negative | Dropped at call entry like a charge < 1; the rest of the list is searched. If no valid candidate remains, `false`. The mass gate and the preprocess core are NaN-safe as well (2026-11-08). |
| `dMaxExpect` NaN | `false` (a NaN cutoff would pass every E-value). `+inf` is accepted and means no cutoff. |
| Peak m/z not a positive finite number, or intensity not finite | The peak is ignored: left out of the working copy and treated as absent by preprocessing, like a `clear_mz_range` peak (it does not count toward `minimum_peaks`). |

## 5. Files to touch

| Layer | File | Change |
|-------|------|--------|
| Native types | `CometSearch/CometData.h` | `PrecursorCandidate`; `CometScores::iCharge/iIteration` + ctors/copy/assign. |
| Native params | `CometSearch/core/Params.h`, `CometSearchManager.cpp` (`InitializeStaticParams` param copy block, ~`:774` region) | `iChimericRemoveIsotopes` (as built; `iChimericRemoveFlankBins` and `iChimericFragIndexMinIonsScore` were later removed -- the flank knob on 2026-11-08, the re-search floor in Phase 3; a `_report` twin existed from Phase 2 to Phase 4). |
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
| 2026-11-06 | 3 | `CometWrapper`: both `DoSingleSpectrumSearchChimeric()` overloads (the list form takes parallel `double[] m/z`, `int[] charge`, `double[] tolerance` arrays -- no managed candidate type, nothing allocated per candidate) with the output conversion factored into one helper shared with `DoSingleSpectrumSearchMultiResults()`. `RealtimeSearch/SearchChimeric.cs`: `RealtimeSearch.exe chimeric <spectra.ms2> <database.idx> [--zrange zmin zmax \| --zlines] [--tol mz] [--max-expect E] [--max-peptides N] [--threads T] [--ascorepro 0\|1] [--flank -1\|0\|1] [--isotopes N] [--min-ions-score N] [--max-runtime ms] [--out file]` (a `--min-ions-report` option existed until Phase 4b), selected by `chimeric` as the first argument of the existing executable; its own `.ms2` reader (every `Z` line kept), the MS1/MS2 harness's `ConfigureInputSettings()` block reused (`SearchSettings` made `internal`), the same `ConcurrentQueue` worker pattern; one tab-delimited row per reported peptide (scan, iteration, charge, precursor m/z, candidate count, calculated neutral mass, peptide, protein, xcorr, delta-Cn, E-value, ions, elapsed ms) and a run summary (spectra with 0/1/2/3+ peptides, rows at iteration >= 1, same-peptide repeats, errors). `t67_make_chimeric.py` also writes the scans as a multi-`Z`-line `.ms2`. Verified: on the T67 `.ms2` against a Windows-built FI index, both forms reproduce `rts_repro`'s rows exactly (peptides, charges, xcorr, E-values, ion counts); Windows solution builds clean. |
| 2026-11-06 | 4 | Removal validation and cost on `20170103_HelaQC_01` (42,030 HCD scans) against a current-format human canonical target-decoy FI index with the phospho configuration (section 10). Outcome: the wrapper's removal model confirmed (isotope removal is what matters); `chimeric_fragindex_min_ions_score/_report` default changed 0 -> 2 (+34% confident later IDs, decoy rate halved); the +/- 1 m/z window, not the charge range, is what finds chimeras (2,914 vs 445 confident later IDs); `isotope_error = 2` is worth +42% on the plain RTS search and is recommended for `SearchMS1MS2.cs` independently; cost 7.0 ms/spectrum/thread vs 0.63 for the plain search. Tooling: `tests/rts_repro/mzxml_to_fixture.py`, `rts_repro` arguments for the `chimeric_*` knobs and `isotope_error`, `RTS_CHIMERIC_DEBUG=2` bin-level trace, `docs/20261105_rtschimeric_validate.py`. |
| 2026-11-06 | 4b | Review of the re-search floor (section 10.7): `chimeric_fragindex_min_ions_report` and `Query::iFragIndexMinIonsReportOverride` removed (the reporting floor is never lowered); `chimeric_fragindex_min_ions_score` kept at 2 and re-documented as an E-value histogram-population control with the measured mechanism; `RTS_CHIMERIC_DEBUG=2` now also prints each candidate search's real-candidate count (`RTS_CHIMERIC_CAND`); the E-value fit and raw histogram dump became `COMET_EVALUE_TRACE` in `CalculateEValue()` for every search path (the histogram-fill project's Phase 0); `rts_repro` gained a `globalMinIonsScore` argument. Open item recorded: a better `GenerateXcorrDecoys()` for sparse fragment-index queries. |
| 2026-11-06 | 6 | `chimeric_fragindex_min_ions_score` and `Query::iFragIndexMinIonsScoreOverride` removed: `fragindex_histogram_fill` (default 100, the histogram-fill project's Phase 3) makes FI E-values honest on every query and the chimeric floor a byte-for-byte no-op. Harness `dMaxExpect` defaults 0.05 -> 0.1; `rts_repro` and `SearchChimeric.cs` lose their min-ions arguments. At the new defaults (fill 100, no floor, gate 0.1) the full-file chimeric run reports 22,930 confident PSMs vs 22,453 for the best Phase 4 configuration. |
| 2026-11-09 | review | Lower-severity review items. The candidate mass gate is now the one `DoSingleSpectrumSearchMultiResults()` uses, only above `dPeptideMassHigh` (decision 8): a precursor just below the range whose window or isotope offset reaches an in-range peptide used to be skipped. Outside the chimeric loop: `DoSingleSpectrumSearchMultiResults()` returns false for `topN < 1`, `COMET_EVALUE_TRACE`'s `tope` follows the 999 reporting rules, `SearchChimeric.cs` reports bad input with exit code 1 instead of an unhandled exception, and the top-level Makefile relinks `comet.exe` after an MSToolkit-only change. Tests T83, T84. Cleanup: `topN` removed from both chimeric overloads (native and wrapper; it was accepted and ignored), the wrapper overloads share one search-and-convert helper, and the synthetic-spectrum helpers moved into `tests/unit/fixture_peptides.py`. |
| 2026-11-08 | review | Calls that stop early keep their rows, flagged (review item 15). Both `DoSingleSpectrumSearchChimeric()` overloads, native and wrapper, gain a `bIncomplete` output. With the budget out, the call returns `true` with `bIncomplete` and the rows found so far. When cancelled or when a search fails (core -2), it returns `false` with `bIncomplete` and keeps the completed iterations' rows. Invalid arguments give `false` with no rows. A separate output, not a per-row flag, so a call that stops before keeping any PSM is still marked. The wrapper also keeps the rows on a caught native exception when the four vectors are aligned. `SearchChimeric.cs` and `rts_repro` write the rows and then an `INCOMPLETE budget|stopped` line; `RealtimeSearch.exe chimeric --max-runtime 1` showed it end to end. T82 is a cancel race through `rts_repro`'s `RTS_REPRO_CANCEL_RACE`: 600 calls, about 440 cancelled, over 100 of those returning rows. Every cancelled call kept a prefix of its uncancelled result, every completed call matched it exactly, and no call returned `false` without the flag. T72 checks the budget marker. |
| 2026-11-08 | review | Removal knobs (review item 14). `chimeric_remove_flank_bins` and `chimeric_remove_isotopes` were loop bounds with no range check. A flank of 1000 stripped nearly every peak, so T67's second peptides were lost; 100,000 threw `std::bad_alloc`. An isotope count of `INT_MAX` looped about 2 billion times per observed peak: hours on Windows, `bad_alloc` or a test timeout on Linux, and an int overflow in the isotope m/z. `chimeric_remove_isotopes` is now clamped at init to 0..3, with a warning when a value changes. `chimeric_remove_flank_bins` is removed: the flank always follows `theoretical_fragment_ions` (+/- 1 bin iff it is 0, the bins the scorer credits; 10.2 found flanks alone change little). `rts_repro` keeps, and ignores, its `flankBins` argument, and `RealtimeSearch.exe chimeric` drops `--flank`. Defaults are unchanged: -1 already followed `theoretical_fragment_ions`. This removes the last known way for native code to throw through the wrapper (item 11). T81. |
| 2026-11-08 | review | Precursor charge limits (review item 10). The charge-range overload now intersects the requested range with 1..`MAX_PRECURSOR_CHARGE` before allocating, returning `false` when nothing is left: 10..12 used to return `true` with nothing searched, and 2..`INT_MAX` reserved about 2 billion candidates and aborted with `std::bad_alloc` through the wrapper. `PreprocessSingleSpectrumCore()` now rejects a charge outside `min_precursor_charge` / `max_precursor_charge`, so both RTS entry points honor them as batch does. They were ignored before, although the RealtimeSearch harnesses set `max_precursor_charge = 6`. HeLa has no 7+ spectra: the full-file plain RTS search (42,030 spectra, FI) is byte-identical. T77. |
| 2026-11-08 | review | Removal and matched fragments from the shared bin builder (review item 9; `docs/20261108_shared_bin_builder.md`). The extraction loop's own b/y table and neutral-loss loop had drifted from `XcorrScoreI()`: with `scale_fragmentNL` the k x loss peaks of a fragment with k phospho sites were never removed (24 of 30 peaks on a doubly-phosphorylated winner), and `precursor_NL_ions` peaks were never removed. Step 1 moved FI, PI-target and PI-decoy scoring onto one `BuildIonLadder()` / `BuildBinnedIons()` (byte-identical on HeLa batch and RTS); step 2 rebuilds each result's credited bins with it and derives the matched fragments and the removal bins from them, so they can no longer drift. Flanks, water/ammonia losses and isotopes stay as explicit removal rules (not stored bins). HeLa at the phospho defaults unchanged (3,091 chimeric rows byte-identical); T76. This closes the item the z+1 row below left open. |
| 2026-11-08 | review | Explicit-list candidates: only exact duplicates dropped (code review of the branch). The old rule dropped a candidate when an earlier same-charge one lay within the incoming candidate's tolerance: order-dependent (with 581.75 and 582.20 at +/- 0.5 m/z, 581.75 first kept a window that missed the precursor at 582.32 -- no match; the reverse order found it) and it merged genuinely different precursors under a wide tolerance. Neither the search tolerance nor the wrapper's fixed `same_precursor_ppm` (an instrument assumption) is a sound merge window, so now only candidates with the same charge, m/z and effective tolerance -- the same search -- are dropped, found by sorting (O(n log n)). `rts_repro` fixtures take an optional sixth SPECTRUM field, a candidate's own tolerance in m/z. T75. |
| 2026-11-08 | review | Non-finite and invalid inputs (code review of the branch). A NaN fails every comparison, so the chimeric mass gate and `DoSingleSpectrumSearchMultiResults()`'s gate (the latter pre-existing) let a NaN precursor m/z through; preprocessing sized the query's arrays from it, `(int)NaN` went negative and reached `memset`: a segfault in both RTS entry points. A NaN `dMaxExpect` passed every E-value. Checking the peak arrays as well: a negative or `-inf` peak m/z with a real intensity got a negative bin and was written below the array start (segfault), a `+inf` intensity made the spectrum infinite (no match); NaN peak values were skipped, but only because the comparisons failed. Now `PreprocessSingleSpectrumCore()` rejects a non-finite or non-positive precursor mass and treats invalid peaks as absent; the chimeric call drops invalid candidates and invalid peaks on entry, rejects a NaN `dMaxExpect`, and its mass gate and E-value stop test are written so a NaN fails them; `MultiResults`' gate likewise. Edge-case table above extended. `rts_repro` reads fixture numbers with `strtod` so `nan`/`inf` load. T73. The batch peak loops in `CometPreprocess.cpp` have the same lower-bound gap for a negative peak m/z read from a file; not changed here. |
| 2026-11-08 | review | `max_index_runtime` now ends the chimeric call (code review of the branch). An expired core search returns 0 rows, and the loop's only budget check sat after its 0-row `continue`, so once the budget ran out every remaining candidate was still preprocessed and searched and a whole extra iteration ran (reviewer: a 10 ms budget took as long as an unlimited one). The loop now checks the budget before each candidate and after each core call, from the same `tStart` and clock the core uses; the incomplete iteration's best PSM is still reported (decision kept, edge-case table in 4.8 made explicit). The `RTS_CHIMERIC_DEBUG` iteration line ends in `budget-expired` when that happened. T72: 400 explicit-list candidates; a 1 ms budget stops iteration 0 after 1 candidate (all 400 before the fix), a 40 ms budget after ~160 with the target reported and no iteration 1, no budget searches all 400 and continues. |
| 2026-11-08 | review | z+1 ions in the result table (code review of the branch): the table that converts b/y masses to a/c/x/z/z+1 in `SearchSingleSpectrumCore()`'s result extraction -- the source of both the reported matched fragments and the chimeric removal bins -- put z+1 at y - NH3, 2.016 Da below the scorer's y - NH2 + H (the z' ion; z is the z-dot ion, y - NH2, and was right). With `use_Z1_ions` the winner's z+1 peaks were never removed (18 of 27 peaks on a b/y/z+1 test scan) and RTS listed z+1 matches at the wrong m/z. The z+1 entry predates this branch: it was already present when the sources moved to this repository in 2021. The table now uses the scorer's `g_staticParams.massUtility` constants, as `CometMassSpecUtils::GetFragmentIonMass()` does, so it also follows `mass_type_fragment` (it was hardcoded monoisotopic). `rts_repro` gained `RTS_REPRO_PARAMS` (typed `key=value` parameters, no new positional slot) and `RTS_REPRO_FRAGMENTS` (matched-fragment lines); T71. Deriving the removal set from the scorer's own binned ions -- review item 9, which would also cover the multi-loss neutral-loss gap -- remains open. |
| 2026-11-06 | 7 | Section 11 review: the `use_NL_ions` removal gap closed (water/ammonia-loss bins removed exactly when `XcorrScoreI()` credits them: `use_NL_ions` on, PI_DB, 1+ a/b/y), `rts_repro` `useNLIons` argument, T67 scan 6. Per-candidate tolerance callers deferred with the MS1-scan candidate source; batch exposure deferred as a possible separate project. |
| 2026-11-06 | 5 | Documentation pass: `CLAUDE.md` threading model (chimeric MS2 RTS entry), `docs/DataStructures.md` (`CometScores::iCharge/iIteration`, `PrecursorCandidate`, `Query::iFragIndexMinIonsScoreOverride`), `docs/GlobalVariables.md` (the `chimeric_*` options), `tests/rts_repro/README.md` (chimeric arguments, `RTS_CHIMERIC_DEBUG`, `mzxml_to_fixture.py`), the `comet-codebase` / `comet-search-run` / `comet-benchmark` skills (call path, harness command, measured cost). `docs/RealTimeSearch.md` and `tests/tests.md` were updated in Phases 3-4b. Follow-on project planned separately: `docs/20261106_FI_evalue_histogram_fill.md`. |
| 2026-11-06 | -- | Build-system side work prompted by Phase 1's baseline capture: root `-j` race (MSToolkit entered twice concurrently) fixed with an order-only `mstoolkit` target; automatic `-MMD -MP` header dependencies in all four Makefiles (previously only four objects wrote `.d` files and none were included); MSToolkit objects now wait for the expat/zlib header install. |

## 10. Phase 4 results: removal validation and cost on real data (2026-11-06)

### 10.1 Setup

- **Spectra**: `20170103_HelaQC_01.mzXML` (the benchmark file; 42,030 HCD/FTMS MS2 scans,
  precursor charges z2 24,845 / z3 15,048 / z4 1,974 / z5 163), converted with
  `tests/rts_repro/mzxml_to_fixture.py` (3 s).
- **Index**: `comet.exe -i` on `human.canonical.target-decoy.fasta` (40,908 entries, `DECOY_`
  prefix) with `data/comet_phospho.params` (trypsin, M oxidation and STY phospho up to 2 each,
  8-50 residues, 800-5000 MH+): 4.4 M unmodified / 79 M modified peptides, 2.2 B fragment-index
  entries; 43 s and 862 MB to build; 22 s to load at 8 threads, 79 s at 1 (the per-session
  fragment-index regeneration parallelises with `num_threads`).
- **Searches**: `tests/rts_repro/rts_repro` with its RealtimeSearch-mirroring parameters
  (20 ppm, 0.02 Da bins with flanking peaks, `max_fragment_charge` 3, `minimum_peaks` 10, B/Y,
  `isotope_error` 0 unless stated). Chimeric runs: charge-range form, charges 2-4, +/- 1.0 m/z,
  `dMaxExpect` 0.05, up to 5 peptides, `max_index_runtime` 0. Removal / threshold settings are
  the `chimeric_*` params. 8 worker threads unless stated.
- **FDR**: decoy = protein prefix `DECOY_`; q-values from E-value order (ties xcorr) within the
  set named -- iteration-0 rows, iteration >= 1 rows, or all rows -- with FDR = decoys / targets
  (`tools/qvalue.py`'s convention; `docs/20261105_rtschimeric_validate.py` is the script).
- **Baseline** (`DoSingleSpectrumSearchMultiResults`, the file's charge, 20 ppm): 27,655 rank-1
  PSMs (6,704 decoys), **12,808 targets at q <= 0.01**; at E <= 0.05: 13,174 PSMs, 1.2% decoy.

### 10.2 Removal model and re-search threshold (section 7.3)

| setting | removal | re-search min ions | spectra with 0/1/2/3+ peptides | rows | rows at iter >= 1 | decoy % iter 0 / iter >= 1 | targets q <= 0.01: iter 0 / iter >= 1 / all |
|---|---|---|---|---|---|---|---|
| S_exact | bin only | unchanged (3) | 22,279 / 16,235 / 3,160 / 356 | 23,650 | 3,899 | 1.1 / 5.7 | 19,275 / 1,672 / 21,306 |
| S_flank | bin +/- 1 | unchanged | 22,279 / 16,149 / 3,226 / 376 | 23,754 | 4,003 | 1.1 / 5.9 | 19,275 / 1,733 / 21,227 |
| **S_default** | bin +/- 1, +1/+2 isotopes | unchanged | 22,279 / 15,914 / 3,388 / 449 | 24,076 | 4,325 | 1.1 / 4.4 | 19,275 / 2,181 / 21,966 |
| **S_min2** | bin +/- 1, +1/+2 isotopes | 2 / 2 | 22,279 / 16,456 / 2,962 / 333 | 23,399 | 3,648 | 1.1 / 2.1 | 19,275 / **2,914** / **22,336** |

Per-iteration decoy rates (rows at E <= 0.05): S_default 1.1% / 4.1% / 6.9% / 10.5% for
iterations 0-3 (19,751 / 3,837 / 449 / 38 rows); S_exact 1.1 / 5.2 / 9.6 / 20.0%; S_min2
1.1 / 1.9 / 3.9 / 10.0%.

- **The wrapper's removal model is confirmed as the default.** Iteration 0 is identical across
  settings (removal only affects later passes). Adding the isotope peaks to flank removal both
  finds more later peptides and makes them cleaner (4.4% vs 5.9% decoy; 2,181 vs 1,733 at
  q <= 0.01); flanks alone barely differ from bin-exact. `chimeric_remove_flank_bins = -1`,
  `chimeric_remove_isotopes = 2` stay.
- **Bin-adjacent ghost re-hits are negligible in every model.** From the `RTS_CHIMERIC_DEBUG=2`
  traces (first 5,000 scans, single thread; 269 later-iteration PSMs with 2,066 matched ions):
  98.5% of a later PSM's matched ions lie >= 10 bins (>= 0.2 Da) from any bin removed earlier,
  1.2% within +/- 2 bins -- 1.3% for bin-exact removal. The isotope benefit above is therefore
  not visible to an adjacency metric: the +1/+2 isotope peaks sit ~50 bins away, and are what
  seeds the extra junk when left in place. A kept PSM removes on average 13.9 of 80 peaks (208
  bins).
- **Lowering the re-search candidate floor to 2 ions is a clear win here** -- the opposite sign
  of the wrapper's batch observation. Rows at iteration >= 1 fall (3,648 vs 4,325) but their
  decoy rate halves (2.1% vs 4.4%) and confident later IDs rise by a third (2,914 vs 2,181):
  with more candidates scored, the E-value histogram is better populated, the E-values become
  more conservative, and `dMaxExpect` keeps the junk out. Pending the 20 ppm confirmation in
  10.4, `chimeric_fragindex_min_ions_score` should default to 2 (section 10.7 then showed the
  mechanism and that the `_report` floor must stay at 3).
- No spectrum reported the same peptide twice in any setting.

### 10.3 What the charge range and the wide window do to iteration 0

Iteration 0 agrees with the baseline rank-1 peptide on 67.0% of the spectra where both have
a PSM, and picks the file's charge on 92.5%. The disagreement is dominated by a mass effect,
not by chimeras: iteration-0 winners sit at an offset of 0 Da from the precursor neutral mass
in 12,198 rows, **-1 Da in 4,143 and -2 Da in 880** (0.5-0.7% decoy -- real identifications
whose precursor was isolated on the M+1 / M+2 isotope), +1..+3 Da in ~200 (8-37% decoy),
and at a non-integer offset in 2,230 (5.0% decoy). The +/- 1 m/z window acts as an
`isotope_error` the RealtimeSearch configuration (`isotope_error = 0` in
`SearchMS1MS2.cs`, mirrored by `rts_repro`) does not have; that, not the second peptide,
is why iteration 0 reaches 19,275 targets at q <= 0.01 against the baseline's 12,808 (13,174
PSMs at the same E <= 0.05 cut). Of the 5,850 spectra where iteration 0 differs from the
baseline, the baseline PSMs replaced were 42.8% decoy and only 567 were at q <= 0.01; the
replacements are 1.5% decoy. Of those 567 confident baseline IDs, **471 are reported at a
later iteration** (the spectrum held both; the loop ranked the other first on evidence,
345/567 with the better E-value), so ~96 confident baseline IDs are absent from the chimeric
output. Section 10.4 separates the charge-range, window and isotope-error contributions.

### 10.4 Charge range vs window vs isotope error

Same file and index; the chimeric runs use the default removal model. `base_iso2` is the plain
search with `isotope_error = 2`.

| run | charges | window | isotope_error | re-search min ions | dMaxExpect | spectra 0/1/2/3+ | targets q <= 0.01: iter 0 / iter >= 1 / all | iter 0 = baseline |
|---|---|---|---|---|---|---|---|---|
| baseline | file's | 20 ppm | 0 | -- | -- | -- | 12,808 / -- / 12,808 | -- |
| base_iso2 | file's | 20 ppm | 2 | -- | -- | -- | 18,184 / -- / 18,184 | -- |
| S_tol0 | 2-4 | 20 ppm | 0 | 3 | 0.05 | 28,536 / 13,219 / 271 / 4 | 12,964 / 148 / 13,063 | 98.2% |
| S_tol0_iso2 | 2-4 | 20 ppm | 2 | 3 | 0.05 | 23,450 / 17,765 / 793 / 22 | 18,304 / 445 / 18,800 | 97.7% of base_iso2 |
| S_tol0_iso2_min2 | 2-4 | 20 ppm | 2 | 2 | 0.05 | 23,450 / 17,877 / 687 / 16 | 18,304 / 439 / 18,784 | 97.7% of base_iso2 |
| S_default | 2-4 | +/- 1 m/z | 0 | 3 | 0.05 | 22,279 / 15,914 / 3,388 / 449 | 19,275 / 2,181 / 21,966 | 67.0% |
| **S_min2** | 2-4 | +/- 1 m/z | 0 | 2 | 0.05 | 22,279 / 16,456 / 2,962 / 333 | 19,275 / **2,914** / **22,336** | 85.6% of base_iso2 |
| S_wide_min2_e01 | 2-4 | +/- 1 m/z | 0 | 2 | 0.01 | 25,342 / 14,803 / 1,765 / 120 | 16,638 / 2,005 / 18,643 | 68.8% |

Iteration >= 1 decoy rates: S_tol0 10.4%, S_tol0_iso2 6.9%, S_tol0_iso2_min2 5.8%, S_default
4.4%, S_min2 2.1%, S_wide_min2_e01 0.3% (and 0.3% at iteration 0).

- **Charge ambiguity alone is worth little on this file.** At 20 ppm, searching charges 2-4
  instead of the file's charge changes 1.8% of iteration-0 PSMs and adds ~150 confident
  identifications in either iteration; the instrument's charge assignment is nearly always
  right, and a second peptide at the *same* m/z (an isobaric co-isolation) is rare -- 271
  spectra with two peptides, 148 confident later IDs.
- **`isotope_error` is the single biggest lever and is orthogonal to this project.** The
  plain RTS search gains 42% (12,808 -> 18,184 targets at q <= 0.01) from `isotope_error = 2`.
  `RealtimeSearch/SearchMS1MS2.cs` sets 0 (`rts_repro` mirrors it); the batch
  `comet_phospho.params` also runs 0. That setting, not anything chimeric, is what the +/- 1 m/z
  runs were recovering at iteration 0 (section 10.3). Recommendation: set `isotope_error = 2`
  in the RTS harness regardless of the chimeric work.
- **The +/- 1 m/z window is what finds chimeras.** With isotope error already accounted for
  (compare against `base_iso2`), the window adds ~1,100 confident PSMs at iteration 0 and
  raises confident later-iteration IDs from 445 to 2,181 (default threshold) or 2,914
  (min ions 2): co-isolated precursors at a different m/z inside the isolation window, which
  no charge or isotope expansion of one m/z can reach. Total at q <= 0.01: 22,336 vs 18,184,
  +23% over the best plain search.
- **Re-search min ions 2 is neutral at 20 ppm (439 vs 445) and +34% with the window**, with
  the decoy rate of later rows halved; it is now the default (section 10.6).
- **A tighter `dMaxExpect` (0.01) buys purity, not yield**: 0.3% decoy at every iteration, but
  2,005 confident later IDs instead of 2,914 and 16,638 instead of 19,275 at iteration 0 --
  the q-value already controls FDR, so 0.05 is the better default for callers that filter
  downstream; 0.01 suits a caller that wants to use every reported row as is.

### 10.6 Defaults set from these measurements

| param | before | after | why |
|---|---|---|---|
| `chimeric_remove_flank_bins` | -1 | -1 (confirmed); the knob was removed on 2026-11-08 and the flank always follows `theoretical_fragment_ions` | 10.2 |
| `chimeric_remove_isotopes` | 2 | 2 (confirmed) | 10.2: 4.4% vs 5.9% decoy, +26% confident later IDs |
| `chimeric_fragindex_min_ions_score` | 0 (unchanged) | 2, then removed | 10.2 / 10.4 / 10.7: +43% confident later IDs with the window (3,116 vs 2,181 with the report floor kept at 3), neutral without; superseded by `fragindex_histogram_fill` (byte-identical results with the floor on or off once the fill is on) |
| `chimeric_fragindex_min_ions_report` | 0 (unchanged) | removed | 10.7: the 47 PSMs that only a 2-ion report floor lets win are 31.9% decoy |
| harness `dMaxExpect` default | 0.05 | 0.1 | with calibrated E-values 0.05 drops ~1,100 iteration-0 rows; 0.1 gives 22,930 confident PSMs in total vs 22,453 for the best fill-0 configuration (`docs/20261106_FI_evalue_histogram_fill.md` Phase 2) |
| `rts_repro` / `SearchChimeric.cs` `--min-ions-*` defaults | 0 | 2 | follow the params |
| recommended for callers: `isotope_error` | 0 in `SearchMS1MS2.cs` | 2 | 10.4: +42% on the plain search; keeps the chimeric window honest about what it adds |

### 10.5 Cost (section 7.4)

Single thread, first 5,000 scans, index load (79 s) subtracted:

| run | search time | per spectrum | vs plain search |
|---|---|---|---|
| plain search (`DoSingleSpectrumSearchMultiResults`, file charge, 20 ppm) | 1.8 s | 0.36 ms | 1x |
| chimeric, charge 2 only, +/- 1 m/z, <= 5 peptides | 3.6 s | 0.7 ms | 2x |
| chimeric, charges 2-4, +/- 1 m/z, 1 peptide (one iteration) | 13.8 s | 2.8 ms | ~8x |
| chimeric, charges 2-4, +/- 1 m/z, <= 5 peptides | 20.5 s | 4.1 ms | ~11x |

Full file (42,030 scans), single thread, load subtracted -- the numbers to quote:

| run | search time | per spectrum | vs plain search |
|---|---|---|---|
| plain search, file charge, 20 ppm | 26.3 s | 0.63 ms | 1x |
| chimeric, charges 2-4, +/- 1 m/z, 1 peptide | 198.7 s | 4.7 ms | 7.6x |
| chimeric, charges 2-4, +/- 1 m/z, <= 5 peptides, min ions 3 | 304.7 s | 7.3 ms | 11.6x |
| chimeric, charges 2-4, +/- 1 m/z, <= 5 peptides, min ions 2 (default) | 295.7 s | 7.0 ms | 11.2x |

At 8 threads over the full file: 21 s (plain) vs 37 s (charges 2-4 at 20 ppm) vs 63 s
(charges 2-4 at +/- 1 m/z), after the 22 s load. The cost is not the candidate count but the
window: the 3+ and 4+ candidates account for ~17 of the 20.5 s on the 5,000-scan subset,
because +/- 1 m/z is +/- 3 / +/- 4 Da of peptide mass there and those windows are dense; the
extra iterations add only ~50% on top of the first (most spectra stop after one or two). At
7 ms/spectrum/thread, a 20-thread RTS host sustains ~2,800 spectra/s on this index, well
inside real-time budgets; the window width (and the charge ceiling) is the lever if a
denser index or a smaller machine needs one.

### 10.7 Why the re-search candidate floor works: the E-value histogram on a stripped spectrum

Question raised in review: a peptide that cannot match 3 fragment-index entries is not a
good identification, so what is a 2-ion floor for -- and with `GenerateXcorrDecoys()`
padding every histogram to `EXPECT_DECOY_SIZE` (3,000) synthetic scores, how could a few
real entries move the fit? Measured with `RTS_CHIMERIC_DEBUG=2` and `COMET_EVALUE_TRACE=2` (per-query histogram
size, `iNextCorr`, slope, intercept, rank-1 xcorr/E-value, and the non-zero histogram
bins), on the same iteration-1 queries under both floors (identical spectrum state and
candidate; 5,219 queries for the counts, 102 with full histograms):

- **It is not rescue.** Lowering both floors to 2 let exactly 47 later-iteration PSMs win
  (of 3,648) that a 3-ion report floor would have blocked, and they were 31.9% decoy. With
  the report floor kept at 3 the 3,601 shared PSMs have identical xcorr; only their E-values
  change (larger under floor 2 in 2,574, median x1.31-1.41). Score floor 2 with report
  floor 3 is the best configuration: 3,116 confident later IDs at 1.9% decoy, against 2,914
  with both floors at 2 and 2,181 with both at 3.
- **It is not one entry among thousands: it is ~40 per query.** At floor 3 a re-search
  query scores a median of **7** real candidates into the histogram (54% of queries fewer
  than 10): the first peptide's peaks are gone, so few peptides reach 3 index hits among the
  top peaks. At floor 2 the median is **100** (the `FRAGINDEX_MAX_NUMSCORED` cap, mean 72;
  +36 median per query), and only 12% of queries stay below 10. Stored results are identical
  (median 7) because the report floor stays at 3.
- **The synthetic padding's tail is thin where the fit happens.** The padded histogram
  holds 3,000 entries, 93% of them synthetic scores at xcorr 0.0-0.3 (per query, mean
  913 / 304 / 173 / 1510 -- the 0.3 pile is `iMinXcorrHisto`'s lumping of half the zero
  scores). Per re-search query the entries above that are 53 / 26 / 11 / 5.4 / 2.4 / 0.9 at
  xcorr 0.4 / 0.5 / 0.6 / 0.7 / 0.8 / 0.9. `LinearRegression()` builds its cumulative
  distribution downward from the "second-best" bin (`iNextCorr`, median 0.8), so those thin
  bins are the fit. Floor 2 raises the real entries from a median of 4 to 100 and adds
  12 / 10 / 5 / 2.4 / 1.2 / 0.3 entries in exactly those bins (+22-50%); the median slope
  goes -0.457 -> -0.430 and the rank-1 E-value rises x1.41 (in 81% of queries). The
  re-search winners themselves are weak (median xcorr 0.79, at the edge of that tail), which
  is why their E-values depend on its shape; iteration-0 winners sit far above it and barely
  move (the plain search gains only +1.7% from a global score floor of 2). (These are raw
  per-bin counts from `COMET_EVALUE_TRACE=2`; the first version of this bullet, taken from a
  dump made after `LinearRegression()` had overwritten the histogram with its cumulative
  distribution, quoted cumulative values and an "~8,500 entries" total that was their sum.)
- **Open item (not this project):** whether the synthetic decoys are poor or the stripped
  spectrum is simply sparse. Random peptides scored against a spectrum with its dominant
  peptide removed may genuinely score 0.0-0.3, in which case the padding is a correct null
  and the 2-ion candidates -- real peptides *selected* for sharing >= 2 ions -- are a biased,
  more conservative one. Target-decoy FDR sides with the conservative fit on these spectra
  (decoy rate among later rows at E <= 0.05: 1.9% vs 4.4%), but a better
  `CometPostAnalysis::GenerateXcorrDecoys()` -- synthetic scores that reach the xcorr range
  where weak hits live, or scoring the next-weakest real candidates into the histogram
  without storing them, for every sparse fragment-index query -- would remove the need for
  this knob. Follow-up measured on full spectra: the synthetic entries are already 93% at
  0.0-0.3 before any subtraction (subtraction only thins the tail further and lowers the
  winner's xcorr 0.93 -> 0.79), and against PI on the same PSMs the FI E-values are x1.6-2
  optimistic in every xcorr decile, entirely through the fitted slope -- so the null is
  wrong for every FI query, not just stripped ones. Its own project, with the Phase 0
  baseline tables: `docs/20261106_FI_evalue_histogram_fill.md`.

## 11. Out of scope / later

- Candidate retirement after an ID (decision 14) -- revisit with 7.3/7.4 numbers.
- Per-candidate tolerance callers (decision 15) -- the field exists; nothing sets it yet.
- Precursor candidates from an in-process MS1 scan (RawFileReader) rather than `.ms2` `Z`
  lines (decision 10).
- Returning runner-up rows per iteration (a new parameter; the ignored `topN` was removed
  2026-11-09), and any combined/paired scoring
  of the reported peptides.
- ~~`use_NL_ions = 1` water/ammonia-loss peaks: scored by xcorr but not in the removal
  set~~ -- closed 2026-11-06: `AddChimericRemovalBins()` now also clears the ion - H2O and
  ion - NH3 bins (flanks and observed-peak isotopes alike) exactly when `XcorrScoreI()`
  credits them -- `use_NL_ions` on, PI_DB, 1+ a/b/y ions; FI_DB never scores the NL matrix.
  T67's scan 6 (a water-loss peak under each ion) checks 18 vs 36 peaks removed. No RTS
  configuration here enables `use_NL_ions`, so this is correctness for callers that do,
  not a measured gain.
- Batch-path (`comet.exe`) exposure of the chimeric loop. The design is RTS-only; a batch
  version would want the fused path's arenas rather than per-call `Query` allocation.
