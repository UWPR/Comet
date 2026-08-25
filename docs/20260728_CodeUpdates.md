# Follow-ups from the DataStructures/GlobalVariables/RealTimeSearch doc audit

Status: COMPLETE (7/8 DONE, 1/8 DEFERRED -- see item 8; no items remain pending)

## Background

`docs/DataStructures.md`, `docs/GlobalVariables.md`, and `docs/RealTimeSearch.md` were audited
against current code on 2026-07-28 (see the commits syncing those three docs). The audit was a
documentation-accuracy exercise, not a profiling run, but it surfaced several genuine code issues
along the way -- dead code, a latent reliability gap in the RTS timeout contract, and a couple of
places worth deeper investigation given this repo's history of RTS-vs-batch and fused-vs-legacy
scoring divergence bugs. This document tracks those items so they can be picked off one at a time.

Status values used below: `NOT STARTED`, `IN PROGRESS`, `DONE`, `DEFERRED`, `WON'T FIX`.

## Summary

| # | Item | Category | Status |
|---|------|----------|--------|
| 1 | RTS `g_sCometVersion` missing git SHA | Quick fix | DONE |
| 2 | Dead field: `StaticParams::tRealTimeStart` | Quick fix | DONE |
| 3 | Dead globals: `g_dbIndexMutex` / `g_vSpecLibMutex` | Quick fix | DONE |
| 4 | `g_bPlainPeptideIndexRead` / `g_bSpecLibRead` should be atomic | Quick fix | DONE |
| 5 | `CalculateAScorePro` not timeout-gated in RTS per-call flow | Reliability risk | DONE |
| 6 | `MassRange` renarrowing asymmetry (fused vs. legacy/RTS) | Investigate | DONE (confirmed benign) |
| 7 | RTS E-value nondeterminism -- already resolved pre-session, re-confirmed | Investigate | DONE |
| 8 | `SearchMemoryPool` is a single process-wide instance | Known limitation | DEFERRED |

---

## 1. RTS `g_sCometVersion` missing git SHA

Status: DONE

**Fix applied:** Factored the three near-identical assembly blocks into one shared inline
helper, `BuildCometVersionString()` (`CometSearch/Common.h`, next to the `comet_version`
macro and `g_sCometVersion` extern it already declared). `Comet.cpp`'s `main()`,
`CometSearchManager::DoSearch()`, and `CometSearchManager::InitializeSingleSpectrumSearch()`
now all call it instead of each re-deriving the string (the RTS site previously skipped the
`GITHUBSHA` append entirely). Verified: full build succeeds, all 19 unit tests pass, and
`./comet.exe` prints the same version string as before (`GITHUBSHA` is empty in a local
build, so behavior for the two sites that already appended it is unchanged; the RTS site now
matches them instead of omitting the hash).

**Where:** `CometSearch/CometSearchManager.cpp:2217`, inside `InitializeSingleSpectrumSearch()`.

**Issue:** `g_sCometVersion` is assembled at three independent sites: `main()` in `Comet.cpp:44-52`
(batch via CLI) and `DoSearch()` in `CometSearchManager.cpp:1985-1993` (batch via DLL) both append
the `GITHUBSHA` macro to `comet_version` when non-empty, producing e.g.
`"2026.02 rev. 1 (a1b2c3d)"`. The RTS site just does `g_sCometVersion = comet_version;` with no SHA
appended. RTS-produced results can't be traced back to the exact commit that produced them.

**Suggested fix:** Append `GITHUBSHA` in `InitializeSingleSpectrumSearch()` the same way the other
two sites do -- ideally factor the three near-identical blocks into one shared helper instead of a
third copy-paste.

---

## 2. Dead field: `StaticParams::tRealTimeStart`

Status: DONE

**Where:** `CometSearch/core/Params.h:244`.

**Issue:** Declared but never read or assigned anywhere in current code (confirmed by repo-wide
grep). Appears to be a leftover from a design that was refactored away -- both FI_DB and PI_DB RTS
paths now use the per-`Query` `pQuery->tSearchStart` for timeout enforcement instead.

**Suggested fix:** Remove the field. Low risk -- it's read by nothing, so removal can't change
behavior. Worth a final grep immediately before deleting in case something outside `CometSearch/`
(e.g. a generated/vendored copy) references it.

**Fix applied:** Re-grepped the whole repo (excluding `_site/`) immediately before deleting;
confirmed zero references anywhere, including `RestoreDefaults()`. Removed the field from
`core/Params.h:244` and the now-unused `#include <chrono>` at the top of the same file (also
re-grepped to confirm no other `chrono`/`time_point` usage remained in that file). Verified:
full build succeeds, all 19 unit tests pass.

---

## 3. Dead globals: `g_dbIndexMutex` / `g_vSpecLibMutex`

Status: DONE

**Where:** `CometSearch/core/Types.h:984-985` (`extern Mutex g_dbIndexMutex;` / `extern Mutex g_vSpecLibMutex;`).

**Issue:** Both are declared `extern` but never defined anywhere in the repo, and therefore never
locked. They're easy to confuse with the real, in-use `g_pvDBIndexMutex` and `g_pvQueryMutex` --
`g_vSpecLibMutex` in particular reads as if it's what protects `g_vSpecLib`, when that's actually
`g_pvQueryMutex` (a repurposed, confusingly-named holdover per `docs/GlobalVariables.md`). This is
a landmine for a future contributor who greps for "what protects `g_vSpecLib`" and finds this name.

**Suggested fix:** Remove both declarations, unless someone can confirm they're a half-finished
refactor in progress and intended to be wired up (in which case, wire them up or leave a comment
explaining the plan).

**Fix applied:** Re-grepped the whole repo (excluding `_site/`) immediately before deleting;
confirmed both are declared and nowhere else referenced (no definition, no lock/unlock call
sites). Removed both `extern Mutex` declarations from `core/Types.h:984-985`. Verified: full
build succeeds, all 19 unit tests pass.

---

## 4. `g_bPlainPeptideIndexRead` / `g_bSpecLibRead` should be atomic

Status: DONE

**Where:** `CometSearchManager.cpp:91,93` (declarations); written at `CometFragmentIndex.cpp:1604`,
`CometSearchManager.cpp:2291` and `CometSpecLib.cpp:93,687`; read from RTS search threads at
`CometSearch.cpp:175,240` and `CometSpecLib.cpp:42`.

**Issue:** Both are plain `bool`, unlike their structurally identical sibling `g_bPeptideIndexRead`,
which is `std::atomic<bool>` with explicit acquire/release ordering. The plain-`bool` pair is
currently safe only because their writes happen to be confined inside the RTS init path, which is
itself guarded by a *different* atomic (`singleSearchInitializationComplete`)'s happens-before edge.
That's an implicit invariant rather than a self-evidently correct one -- easy for a future change to
break without realizing it depends on this.

**Suggested fix:** Change both to `std::atomic<bool>` with the same acquire/release pattern as
`g_bPeptideIndexRead`, for consistency and defense in depth. Cheap, low-risk change.

**Fix applied:** Changed the type of both to `std::atomic<bool>` in `core/Types.h` (declarations)
and `CometSearchManager.cpp` (definitions), plus one redundant local `extern bool` re-declaration
in `search/FiStrategy.cpp:27` that also needed updating to match. It turned out `g_bPeptideIndexRead`
itself doesn't use explicit `.load(acquire)`/`.store(release)` anywhere -- every read/write site
uses plain `if (!x)`/`x = true` via `std::atomic<bool>`'s implicit conversion/assignment operators,
which default to the (stronger) `memory_order_seq_cst`. So "the same pattern" meant just the type
change -- every existing `g_bPlainPeptideIndexRead`/`g_bSpecLibRead` read/write site already used
exactly that plain-operator style and needed no further changes. `<atomic>` was already available
everywhere it was needed (transitively via `core/Types.h`, which already used it for the sibling
flag). Verified: full build succeeds, all 19 unit tests pass.

---

## 5. `CalculateAScorePro` not timeout-gated in RTS per-call flow

Status: DONE

**Where:** `CometSearchManager.cpp:2582` (SP pre-check), `2599` (E-value pre-check), `2613`
(DeltaCn pre-check), `2640` (`CalculateAScorePro` call, no pre-check), `2658` (next check, which
only gates the subsequent protein-name-resolution step -- runs *after* AScorePro already executed).

**Issue:** `g_staticParams.options.iMaxIndexRunTime` exists specifically to bound per-call RTS
latency, and `DoSingleSpectrumSearchMultiResults` re-checks it against `pQuery->tSearchStart`
before each of `CalculateSP`/`CalculateEValue`/`CalculateDeltaCn`, skipping the step past the
deadline. `CalculateAScorePro` is the one exception -- it always runs once `CalculateDeltaCn`
completes, with no pre-check. AScorePro localization can be one of the more expensive
post-analysis steps for peptides with many candidate mod sites, so a single call could blow past
the caller's configured latency budget without being caught until after the fact.

**Suggested fix:** Add the same `iMaxIndexRunTime`/`tSearchStart` pre-check before the
`CalculateAScorePro` call that the three steps before it already have.

**Fix applied:** Inserted the identical timeout-check block (same `iMaxIndexRunTime`/
`tSearchStart` pattern, same `goto cleanup_results` on expiry) immediately before the
`CometPostAnalysis::CalculateAScorePro(pQuery, g_AScoreInterface)` call, inside the
`if (!bHasTerminalVariableMod)` block so it only fires on the path that would actually run
AScorePro. Verified: full build succeeds, all 19 unit tests pass, including the two
AScorePro-specific regression tests (t19, t20) -- `iMaxIndexRunTime` is disabled (`0`) in
those test configs, same as it was for the three pre-existing checks, so this is a true no-op
in the untimed case and only changes behavior when a caller actually sets a timeout.

---

## 6. `MassRange` renarrowing asymmetry (fused vs. legacy/RTS)

Status: DONE (investigated; confirmed benign, no code change needed beyond a clarifying comment)

**Where:** `search/SearchUtils.cpp:236-238`, inside `RunSearchAndPostAnalysis()` (shared by the
FASTA path, and the PI_DB/FI_DB *legacy* fallback paths).

**Issue:** On the legacy three-sweep batch paths, `g_massRange.dMinMass`/`dMaxMass` get
re-narrowed every batch from that batch's actual `SearchSession.queries` (sorted by peptide mass).
The fused batch path (`FusedLoadAndSearchSpectra`) and the RTS path do not do this
per-batch/per-call re-narrowing -- they rely on the wider bounds set once at init. It's unclear
from the code alone whether this is intentional (the fused/RTS paths don't need it because of how
they process spectra) or a latent correctness gap. Given this repo's recent history of exactly
this class of bug -- `0c064a2e` (iHighestIon RTS/batch divergence), `c32816c5` (fragindex bounds),
`a5032359` (minimum_peaks/clear_mz_range divergence), all fused-vs-legacy or RTS-vs-batch scoring
mismatches -- this asymmetry is a plausible next place for a divergence bug to hide.

**Suggested approach:** Not a fix yet -- first determine whether the fused/RTS paths' early-exit
decisions in `SearchForPeptides` are ever affected by the wider (un-narrowed) mass range in a way
that would change results vs. the legacy paths. A targeted batch-vs-RTS (and fused-vs-legacy)
comparison run on spectra near the edges of the configured mass range would confirm whether this
is benign or a real divergence source -- use real `.raw` data, not the small crafted test
fixtures, since small fixtures have previously missed real RTS/batch divergences in this repo (see
`docs/ReadingRawFilesOnLinux.md` for reading `.raw` files on Linux for this purpose).

**Resolved via static call-graph tracing -- no empirical test needed.** This turned out to be a
question code reading can answer definitively (which functions read which fields), not a
data-dependent question an empirical run could only ever partially confirm. Traced every
`g_massRange.dMinMass`/`dMaxMass` read site in the repo (`CometSearch.cpp`,
`CometFragmentIndex.cpp`, `CometPeptideIndex.cpp`):

- `dMinMass`/`dMaxMass` are read for candidate filtering by exactly four functions --
  `SearchForPeptides()`, `WithinMassTolerance()`, `MergeVarMods()`, `CompoundModSearch()` -- and
  every call site among those four is internal to that same cluster.
- That whole cluster is reachable from exactly one entry point:
  `CometSearch::DoSearch(sDBEntry&, bool*, const vector<Query*>&)` -- the FASTA-digestion search
  engine. It's invoked in exactly two contexts: real `FASTA_DB` search (`FastaStrategy`, which has
  no fused/legacy split -- always three-sweep, always re-narrows), and one-time index building
  (`CreateFragmentIndex()`/peptide-index build), which runs once at init against the full
  configured range, not per-batch.
- Every FI_DB/PI_DB search call path -- fused (`CometSearch::RunSearch(Query*, int iSlot)`), legacy
  batch (`RunSearch(int,int,ThreadPool*,vector<Query*>&)`'s FI_DB/PI_DB branches), and RTS
  (`CometSearch::RunSearch(Query*)`) -- dispatches *only* to `SearchFragmentIndex()` or
  `SearchPeptideIndex()`. Read both functions' full bodies: neither ever reads
  `g_massRange.dMinMass`/`dMaxMass`. `SearchFragmentIndex()` only reads the unrelated
  `g_massRange.uiMaxFragmentArrayIndex` (a CSR array bound, set once at init, untouched by the
  re-narrowing code). `SearchPeptideIndex()` does its own per-query binary search directly against
  `pQuery->_pepMassInfo.dPeptideMassToleranceMinus/Plus`, never touching the global at all.

**Conclusion: not a bug.** Whether or not a code path re-narrows `g_massRange.dMinMass`/`dMaxMass`
per batch is irrelevant to FI_DB/PI_DB search correctness, because no FI_DB/PI_DB search function
(fused, legacy, or RTS) ever reads those two fields -- there is no shared consumer of the
un-narrowed vs. re-narrowed value across those paths. Even the legacy FI_DB/PI_DB fallback, which
does execute the re-narrowing lines (it shares `RunSearchAndPostAnalysis()` with `FastaStrategy`),
computes a value nothing in its own search path reads. Only `FastaStrategy` genuinely depends on
the re-narrowed value, and it always re-narrows -- no asymmetry exists there either. This also
means item 6 was never actually a plausible instance of the RTS/batch divergence bug class its
description cited (`0c064a2e`/`c32816c5`/`a5032359` all involved fields that genuinely are read by
both compared paths) -- worth remembering that superficial pattern-matching on "looks like the
same bug class" doesn't substitute for tracing the actual reads.

**Applied:** added a code comment at the `g_massRange.dMinMass`/`dMaxMass` assignment site in
`search/SearchUtils.cpp` explaining this, so a future reader doesn't re-flag the same asymmetry.
Also expanded `docs/DataStructures.md`'s `MassRange` section with the same trace. Verified:
`CometSearch` still builds clean (comment-only change).

---

## 7. RTS E-value nondeterminism -- TSan the non-atomic index-read flags

Status: DONE (already resolved pre-session; re-confirmed 2026-07-28; MS1 path unverified)

**Correction to this item's original framing:** this item was written from a project memory
(`project_rts_evalue_nondeterminism`) that turned out to be stale. The RTS E-value jitter this item
describes was already root-caused and fixed **before this session**, documented in full in
`docs/20260714_EvalueJitter.md` (a 5-phase investigation: reproducer built, root cause localized to
an under-bounded buffer-clear in the RTS preprocessing path, fixed, and validated byte-identical --
TSan was explicitly *not* needed because Phase 3 proved it was an intra-thread stale-data bug, not
a data race). That original fix (clamping `iHighestIon`) was itself superseded 2026-07-27 (commit
`0c064a2e`) by a different mechanism after it was found to cause a *separate* deterministic
RTS-vs-batch divergence bug -- see the "Update (2026-07-27)" note in that doc. The memory this item
was based on had not been updated to reflect either of those facts; it's now corrected.

**Where:** Ties together item 4 above with the pre-existing `docs/20260714_EvalueJitter.md`
investigation and its 2026-07-27 follow-up fix.

**TSan is no longer the recommended next step** (superseded by the correction above) --
`docs/20260714_EvalueJitter.md` Phase 3 already proved the original jitter was an intra-thread
stale-buffer bug, not a data race, by reproducing and eliminating it via buffer-clearing changes
alone, with no synchronization involved. A TSan run is not ruled out as generally useful, but it's
no longer *this item's* blocking next step.

**2026-07-28: independent re-confirmation against current code.** Rebuilt the full Windows
solution (Release/x64, Clean then Build, per the documented `zconf.h` gotcha since Linux `make`
had just run) to pick up today's items 1-5 plus the 2026-07-27 `iZeroBound` fix, then ran 4
replicate RTS searches from `20260728-rts-evalue-determinism/`:

```
RealtimeSearch.exe 20170103_Hela_01.raw 20170103_Hela_01.raw human.canonical.target-decoy.fasta.idx 20
```

against `20260420-human-phosho/20170103_Hela_01.raw` (56,152 total scans, 40,302 MS2) and
`20260420-human-phosho/human.canonical.target-decoy.fasta.idx` (FI_DB, phospho search params:
STY 79.966331 / M 15.9949 variable mods), 20 threads. This is new coverage relative to
`docs/20260714_EvalueJitter.md`'s own Phase 0-5 validation, which only tested PI_DB at 8 threads
on a different dataset -- this run adds FI_DB, 20 threads, and a different dataset as a fourth
independent data point.

**Result: 0 differences in every scientific field across all 4 replicates.** All four runs
produced the identical set of 27,862 scored PSMs (scan numbers matched exactly); for every one of
those 27,862 scans, peptide, xcorr, e-value, charge, exp/calc mass, AScore, Sites string, and
protein assignment were byte-identical across all 4 output files. The only differences between the
raw `.out` files were wall-clock-timing artifacts expected to vary run to run: the per-spectrum
`<N> ms` field, the timing histogram section, the "5 Slowest MS2 Runs" list, and the `<=5ms/>5ms/
>10ms` summary line -- none of which are scored/scientific values. Full writeup moved into
`docs/20260714_EvalueJitter.md`'s "Independent re-validation (2026-07-28)" section, since that's
the canonical document for this issue now.

**Remaining caveat, not yet closed:** `MS1 search elapsed time: 0.00 s` in all 4 logs confirms this
run (and the original Phase 0-5 investigation) never exercised the MS1/spectral-library RTS path
(`DoMS1SearchMultiResults`, `g_bSpecLibRead`, `pMS1Aligner`/`g_ms1AlignerMutex`) -- only MS2
(FI_DB here, PI_DB in the original investigation). If MS1 RTS jitter is ever reported separately,
treat it as unverified by this item and check `PreprocessMS1SingleSpectrumThreadLocal()` for an
analogous stale-buffer issue rather than assuming this fix covers it.

---

## 8. `SearchMemoryPool` is a single process-wide instance

Status: DEFERRED

**Where:** `CometSearch.cpp`'s file-static `s_pool`; see the `TODO` comment at the top of that file
and `docs/20260615_multiple_rts_instances.md`.

**Issue:** `s_pool` cannot support multiple concurrent `ICometSearchManager` instances performing
RTS searches against different fragment indexes in the same process. This is an existing,
documented, intentional limitation -- not a newly discovered bug -- carried here for visibility
since it came up again during the audit. No action needed unless multi-instance concurrent RTS
becomes an actual requirement.
