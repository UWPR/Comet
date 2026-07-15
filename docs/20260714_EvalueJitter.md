# Plan: identify and resolve RTS E-value jitter

Status: Phases 0-1 DONE. Phase 0: batch determinism premise confirmed and strengthened. Phase 1:
arrival-order/tie-break hypothesis ruled out for RTS (confirmed via zero mutex contention); the
underlying batch-search correctness bug was fixed anyway as a separate item, validated not to
change RTS's jitter rate. Phases 2-5 not yet started.

## Problem statement

Replicate RTS searches (`RealtimeSearch.exe`, multi-threaded) against identical input (same
`.raw` file, same PI_DB index, same params) produce a small number of PSMs whose E-value differs
between runs, and occasionally (single digits out of tens of thousands) a different peptide wins a
near-tied XCorr. `num_threads=1` RTS runs are always byte-identical across reruns -- this is a
concurrency effect, not a floating-point/algorithm bug that would also show up single-threaded.

Per the user: an earlier analysis (document since deleted, not recovered in this repo's git
history) established that **Comet's batch search shows no equivalent E-value jitter** under the
same kind of replicate-run test. That comparison point is the sharpest tool available here -- batch
search is also multi-threaded and shares a large fraction of its code with RTS (same
`XcorrScoreI`/`StorePeptideI` scoring/storage functions, same `CalculateEValue`/
`GenerateXcorrDecoys`/`LinearRegression` post-analysis, same `SearchMemoryPool`). If batch is clean
and RTS is not, the bug is most likely in code that is genuinely RTS-exclusive, or in a code path
that behaves differently under RTS's specific concurrency pattern than under batch's -- not in
logic errors within the shared scoring math itself (which batch would also expose).

This document is a plan, not an implementation. No code changes are proposed to be made from this
document directly; each phase below produces evidence that the next phase (or a final fix) depends
on.

## What's already known (from `docs/20260714_rtspostprocessing.md`)

A full investigation was already run this session via replicate 8-thread RTS runs on
`human.small.fasta`'s PI_DB index against `20250520_Hela_60min_06.raw` (~49,844 scans), diffing the
full `rts.out`. Summary of the bisection table (full detail in that doc's "RTS E-value
nondeterminism under concurrency" section):

| Configuration | Differing PSM lines (of ~44,700) |
|---|---|
| `num_threads=1` | 0 -- fully deterministic |
| `num_threads=8`, baseline | ~292-300 |
| `num_threads=8`, AScorePro disabled | 281 -- rules out AScorePro |
| `num_threads=8`, entire C++ per-spectrum pipeline serialized behind one mutex | 71-86 -- large reduction, not zero |
| `num_threads=8`, C# `rawFile` reads serialized (real bug, fixed and shipped) | ~297 -- no measurable change alone |
| `num_threads=8`, both combined | 86 |

Two real, independently-justified bugs were found and fixed along the way (both already committed):
unsynchronized concurrent access to the shared Thermo `IRawDataPlus` reader in
`RealtimeSearch/SearchMS1MS2.cs`, and AScorePro never running at all in RTS PI_DB search (missing
`CreateAScoreDllInterface()` call in the PI_DB init branch). Neither fix eliminated the jitter.

Ruled out via code trace, not just testing: `SearchMemoryPool`/`AcquirePoolSlot()` (mutex+CV
protected, no starvation at 8 threads / 20 cores), the `RtsScratch` sparse child-block pool (proved
safe by induction over `ResetForNewSpectrum()`'s reset-what-was-just-used invariant),
`GenerateXcorrDecoys()`/`LinearRegression()` (pure functions of the query's own data plus read-only
global tables), `Query::iXcorrHistogram` zero-init, and `CalculateSP()` (only stack locals and the
per-query `Results*` array).

**Confirmed via `DBGHIST`-style instrumentation** (temporary, already removed): when the same
peptide wins between two runs, its XCorr, `iMatchPeptideCount` (`iSize`), and
`pQuery->uiHistogramCount` are all bit-identical -- only the E-value differs. Since
`GenerateXcorrDecoys()`/`LinearRegression()` are proven pure functions of
`(pQuery->iXcorrHistogram[], iLoopMax = EXPECT_DECOY_SIZE - uiHistogramCount, read-only globals)`,
and `iLoopMax` is identical (same `uiHistogramCount`), the only way the *E-value* can differ while
the *count* doesn't is if the **distribution of scores within the histogram differs** -- i.e. some
*non-winning* candidate's own computed XCorr crossed a histogram bin boundary between runs, even
though the winner's XCorr did not move. This is the single most important unexplained fact and
should anchor Phase 2 below.

## New findings from this planning pass (not yet tested against RTS)

While researching this plan, two structural facts were confirmed by code reading that materially
change where to look next:

**1. RTS's per-query candidate scan is single-threaded, end to end.** Both
`SearchFragmentIndex(Query*, bool*)` (`CometSearch.cpp:1436`) and
`SearchPeptideIndex(Query*, bool*, int)` (`CometSearch.cpp:1887`) are plain sequential loops over
the fragment-index posting list / peptide-index mass range for a *single* `Query*` -- there is no
internal thread dispatch. Combined with the fact that one RTS worker thread owns one `Query*` for
its entire lifetime (`PreprocessSingleSpectrumThreadLocal()` -> `RunSearch(Query*)` ->
`DoSingleSpectrumSearchMultiResults()`, no other thread ever touches that `Query*`), this means: for
RTS, the *order* in which candidates are discovered and scored for a given spectrum is 100%
deterministic across runs (same `g_pvDBIndex`/`g_iFragmentIndex`, same fixed scan order, same
thread doing all the work). This **rules out** arrival-order/tie-break races as an explanation for
RTS's jitter specifically, even though such races are a real, distinct concern for *batch* search
(see finding 2 below) -- because RTS never has two threads racing to store into the same `Query`.

**2. `XcorrScoreI()`/`StorePeptideI()` (the RTS/thread-local-safe scoring+storage pair, used by both
RTS and batch FI_DB/PI_DB) still guard `Query::accessMutex` (`CometSearch.cpp:7932/7997`) and use a
epsilon-based fuzzy comparison for the storage gate (`dXcorr + 0.00005 >= dLowestXcorrScore`,
`CometSearch.cpp:7986`), not the `isEqual()`-based tie-break pattern that
`SortFnXcorr`/`SortFnSp`/`StorePeptide` (the *older*, FASTA_DB-only, `int iWhichQuery`-based
sibling) were fixed to use in commit `7c04814d` ("Fix non-deterministic FASTA_DB search results
under concurrent scoring ties"). That commit's own description explains the exact failure mode it
fixed: "the survivor of a multi-way tie depend[s] on arrival order among the DB-scanning worker
threads" -- which is precisely what happens when *multiple threads scan different parts of the same
query's candidate space concurrently* (batch search's standard parallelization strategy: split the
database across threads, not the spectrum list). `accessMutex`'s presence in `XcorrScoreI` is
existence-proof that some caller *does* exercise that concurrency for FI_DB/PI_DB, even though (per
point 1) RTS itself never does.

**Implication:** this is very likely a real, latent bug in **batch** FI_DB/PI_DB search
specifically (the commit fixed FASTA_DB's `XcorrScore`/`StorePeptide`, but there is no evidence it
also touched `XcorrScoreI`/`StorePeptideI`), but it does *not* directly explain **RTS's** jitter,
because RTS's single-threaded-per-query scan means arrival order can't vary run to run. It is
included here because (a) it's a legitimate, separately worth-fixing correctness bug the user should
know about regardless of the RTS investigation's outcome, and (b) confirming it does *not* affect
RTS (Phase 1 below) is a cheap, high-value way to close off a plausible-looking dead end before
spending more time on it.

## Hypotheses, ranked

Given points 1-2 above, arrival-order/tie-break races are demoted for RTS specifically. The
leading candidates for *why a non-winning candidate's own XCorr value differs between two RTS runs
of the identical spectrum on the identical single thread* are:

| # | Hypothesis | Why plausible | Why uncertain |
|---|---|---|---|
| A | Stale/incompletely-cleared `RtsScratch` dense preprocessing buffers (`pdTmpRawData`, `pdTmpFastXcorrData`, `pdTmpCorrelationData`, `pfFastXcorrData`, `pfFastXcorrDataNL`, `pfSpScoreData`) leak a previous, differently-sized spectrum's leftover values into an intermediate computation for the current spectrum, on the same thread. Matches project memory's own note that this buffer's ring-buffer/full-clear was "deliberately deferred" as an unverified performance tradeoff. | Only the *final sparse-matrix copy* was rigorously bounds-checked this session (loop bound `i < iArraySize`, matches memset bound); the *intermediate* sliding-window/lookback computation (`CometPreprocess.cpp:1745-1794`) was spot-checked but not exhaustively verified across all parameter branches (e.g. `theoretical_fragment_ions=1`, PEFF variants, compound mods) | A first-pass audit of the water/ammonia-loss lookback branch (lines 1786-1792) did not find an obvious out-of-bounds read; the buffers' clear bound (`iArraySize + iXcorrProcessingOffset`) matches the sliding-window loop's own read bound. Not conclusively safe, but the "obvious" version of this bug wasn't found on inspection. |
| B | Genuine floating-point non-associativity: `/fp:fast` + compiler auto-vectorization of the dot-product/binning math produces ULP-level differences depending on data alignment, which is influenced by which heap addresses `RtsScratch`'s buffers happen to occupy -- itself a function of that thread's allocation history, which varies run to run under real concurrent scheduling even though the *computation* on a given thread is otherwise deterministic. | Explains why serializing the pipeline reduces but doesn't eliminate jitter (heap layout still varies thread to thread even serialized) and why it's concurrency-linked without being a classic data race | Would require a much more invasive proof (disassembly, `/fp:precise` A/B test) to confirm; low precedent for this in scalar (non-SIMD-batched) per-candidate scoring code |
| C | A genuine, not-yet-localized read of RTS-exclusive shared/thread-adjacent state, outside the range covered by the "full pipeline serialized" experiment (which spanned preprocess through `CalculateAScorePro`, but not the code immediately before/after -- e.g. `GetRtsRawDataBuffer()`, `EnsurePeptideIndexLoaded()`'s one-time init, or the C++/CLI marshaling boundary itself) | The "full serialize" test cut jitter by ~3-4x but not to zero, so *something* outside that serialized window (or inside it but missed) is still contributing | Broadest, least specific hypothesis -- last resort if A and B don't pan out |
| D | `StorePeptideI`'s post-insertion "find new lowest" recompute (`CometSearch.cpp:8225-8232`, raw `<` comparison, no `isEqual()`) behaves differently under some edge case even for a single-threaded scan | Uses the same un-fixed comparison pattern as point 2 above | Per point 1, RTS's scan order is fixed, so this recompute should be as deterministic as everything else in a single-threaded context -- likely a red herring for RTS specifically, but cheap to double check |

## Phased plan

### Phase 0 -- re-establish ground truth (small, do first) -- DONE, premise confirmed

The "no jitter in batch" claim was from a deleted document; re-verified directly against current
code rather than trusting it as-is.

**Method.** Built both a PI_DB index (`comet.exe -D human.small.fasta -j`) and a fresh FI_DB index
(`comet.exe -D human.small.fasta -i`) from `human.small.fasta`, ran batch `comet.exe` twice against
`20250520_Hela_60min_06.raw` for each (8 threads, same `comet_phospho.params` as the RTS
investigation), and diffed the resulting `.txt` output files (excluding the header line, which
embeds the `-N` output basename and differs trivially by design).

| Search mode | PSM rows | Diff vs. replicate run |
|---|---|---|
| PI_DB batch (classic multi-query dispatch, `SearchPeptideIndex(tp, queries)`) | 44,758 | **0** -- MD5-identical |
| FI_DB batch, Fused path (`FusedLoadAndSearchSpectra`, the *default* FI_DB batch path per `search/FiStrategy.h`, not a special opt-in -- architecturally closer to RTS's per-spectrum-synchronous model than classic batch) | 16,566 | **0** -- MD5-identical |

Both modes reproduced their own output byte-for-byte (`md5sum` of the PSM data, header excluded,
matched exactly) across independent runs, including AScorePro site-localization output (non-empty,
real values -- ruling out a trivial "nothing ran" explanation). This is a stronger confirmation than
a line-count diff: full content identity, not just "same number of differences as noise."

**Conclusion.** The premise holds and is now *stronger* than originally stated: it's not just
"classic batch search" that's clean -- the **Fused FI_DB batch path**, which shares the
per-spectrum-synchronous processing model with RTS (unlike classic batch's multi-query-per-thread
dispatch), is *also* completely deterministic. This rules out "the fused/per-spectrum-synchronous
architecture is inherently racy" as an explanation (a live possibility this plan flagged going in) --
whatever causes RTS's jitter is not simply "process one spectrum at a time, synchronously, per
thread." It must be something narrower and genuinely RTS-exclusive: the C# harness / C++/CLI
marshaling boundary, the `RtsScratch` thread-local pooling (used by RTS's
`PreprocessSingleSpectrumCore(bUseThreadLocalPool=true)` but *not* by either batch path tested
here), the fresh-`Query`-per-spectrum lifecycle, or the `DoSingleSpectrumSearchMultiResults()`
orchestration function itself -- none of which the two batch modes exercise. This sharpens Phase 2's
reproducer requirement: it must specifically exercise `RtsScratch`/`PreprocessSingleSpectrumCore`
and `DoSingleSpectrumSearchMultiResults()`, not just "per-spectrum processing" in the abstract, since
Fused batch already proves the latter alone is clean.

### Phase 1 -- close off the tie-break/arrival-order dead end (small, do second) -- DONE

**Step 1 (contention check).** Added temporary instrumentation at the `Query::accessMutex` lock site
in `XcorrScoreI()` (`try_lock()` first; on failure, log and fall back to a blocking `lock()`) and ran
the same 8-thread, ~49,844-scan RTS PI_DB search used throughout this investigation.
**Result: zero contentions.** Confirms point 1 above empirically, not just by code reading -- RTS's
single-threaded-per-query scan means this mutex is never actually raced for RTS. Instrumentation
removed after confirming (diff against HEAD showed a clean revert).

**Step 3 (expected outcome -- apply the fix as a separate item).** Applied the `isEqual()`/tie-break
fix from commit `7c04814d` to both branches (decoy and target) of `StorePeptideI()`
(`CometSearch.cpp`), mirroring `StorePeptide()`'s already-fixed pattern exactly: before unconditionally
overwriting the identified worst slot, compare at `float` precision (matching what's actually
stored) and only proceed if the incoming candidate is strictly better, or tied *and* preferred by
the same (sequence, then mod-state) rule the sort functions already use. `XcorrScoreI`/
`StorePeptideI` are reachable from both RTS (which never contends the mutex, per Step 1) and batch
FI_DB/PI_DB search (which does -- `Query::accessMutex`'s existence is itself the evidence, mirroring
`XcorrScore`/`StorePeptide`'s now-fixed batch FASTA_DB equivalent), so this is a real, independently
justified correctness fix for the batch case, applied here rather than deferred.

**Validation.**
- RTS: two replicate 8-thread runs post-fix showed 298 differing PSM lines -- statistically
  indistinguishable from the ~292-300 pre-fix baseline, confirming (as predicted) the fix does not
  change RTS's jitter rate, since RTS's tie-break outcome was already deterministic (just previously
  determined by "whoever the fixed scan order stores last" rather than the sort's own preference
  rule) before this fix.
- Batch PI_DB and batch Fused FI_DB: re-ran both Phase 0 tests post-fix. Both produced **MD5-identical
  output to their Phase 0 pre-fix baseline** (`e0eed274...` and `57dd0db7...` respectively) -- the
  fix is a no-op on this specific dataset (no spectrum in this ~50K-scan test happened to hit an
  exact tie at the eviction boundary), which is expected: the original commit's own description notes
  this requires "many exactly-tied low-scoring candidates (common at degenerate short-peptide
  precursor masses)," a condition this test data doesn't happen to trigger. The fix's correctness
  rests on matching the already-validated `StorePeptide` pattern, not on this dataset proving it; a
  dataset engineered to produce many ties (small peptides, no-enzyme, oversubscribed threads, per the
  original commit's own validation approach) would be needed to observe it firing, and wasn't built
  for this pass since it's out of scope for the RTS jitter question.
- Linux unit suite: 19/19 pass, including T19/T20's AScorePro assertions (confirms the storage-path
  change doesn't disturb downstream mod-site/AScorePro data).

**Conclusion.** RTS's jitter is conclusively **not** an arrival-order/tie-break issue -- RTS never
contends the one mutex that class of bug depends on. This was the last concurrency-adjacent
hypothesis that could be checked cheaply via code-level reasoning; everything remaining (Phase 2
onward) requires either a fast reproducer or direct tooling (TSan/ASan) to make further progress.

### Phase 2 -- build a fast, Thermo-independent reproducer (highest leverage, do early)

The single biggest practical obstacle so far has been iteration speed: each bisection data point
requires a Windows MSBuild rebuild, a ~90-second 49,844-scan RTS run against a 1.4 GB PI_DB index
and a 954 MB raw file, and a full-output diff. This has made the investigation slow and has ruled
out tools (ThreadSanitizer) that need a Linux/Clang build.

1. Write a small, dedicated C++ test driver (new file, e.g. `tests/rts_repro/`) that links directly
   against `libcometsearch` and calls `CometSearchManager::InitializeSingleSpectrumSearch()` +
   `DoSingleSpectrumSearchMultiResults()` repeatedly from N threads against a small, fixed,
   synthetic or pre-extracted set of spectra (e.g. 50-200 representative spectra saved once from the
   existing `20250520_Hela_60min_06.raw` as plain mass/intensity arrays, committed as test fixture
   data) -- no Thermo `RawFileReader`, no C#/CLI boundary, buildable on Linux.
2. This unlocks two things at once: (a) sub-second iteration instead of ~90 seconds, letting Phase 3
   and Phase 4's experiments run in a tight loop; (b) a Linux/GCC-Clang build, which can be compiled
   with `-fsanitize=thread` for a real ThreadSanitizer run -- the tool best suited to directly
   *find* a data race rather than continuing to infer one from bisection.
3. Success criterion: the reproducer shows the same class of jitter (same peptide, different
   E-value, across repeated N-threaded runs of the same fixed spectrum set) that the full RTS harness
   shows, at a rate in the same ballpark (~0.2-0.7% of spectra). If it does not reproduce, that's
   itself informative -- it would point back toward something in the C#/CLI layer or the real
   Thermo reader specifically (elevate hypothesis C).

### Phase 3 -- distinguish the hypotheses empirically

With the Phase 2 reproducer (or, if that's not ready in time, the existing full RTS harness):

1. Extend the diagnostic logging (the removed `DBGHIST` instrumentation is a starting template) to
   dump the **full `iXcorrHistogram[HISTO_SIZE]` array** (or a cheap hash of it) per spectrum, not
   just the top candidate's XCorr. Compare between two runs for every spectrum whose E-value
   differs.
   - If the *histogram content* differs while `uiHistogramCount` and the winner's XCorr are
     identical (expected, per the "what's already known" section above): this proves some
     non-winning candidate's own computed XCorr differs between runs. Proceed to distinguish A vs B
     below.
   - If the histogram is bit-identical and only the *regression fit* differs: re-examine
     `LinearRegression()` for a missed source of nondeterminism (contradicts this session's earlier
     code-trace conclusion that it's pure; would need re-verification, e.g. an uninitialized stack
     variable in the `iStartCorr`/`iNextCorr` boundary logic under specific histogram shapes).
2. To distinguish hypothesis A (stale buffer) from B (fp non-associativity): temporarily change
   `RtsScratch::EnsureInitialized()`/reset logic to fully clear all dense buffers to
   `iArraySizeGlobal` on every spectrum (not just `iArraySize` -- accept the performance regression
   for this experiment only) and re-run the bisection. If jitter drops to zero, hypothesis A is
   confirmed and the fix is to correctly bound-check (not necessarily fully clear -- see Phase 5) the
   specific under-cleared read path. If jitter persists unchanged, hypothesis A is eliminated and
   effort should shift entirely to B/C.
3. To probe hypothesis B: rebuild with `/fp:precise` (MSVC) instead of `/fp:fast` for
   `CometSearch.cpp`/`CometPreprocess.cpp`/`CometPostAnalysis.cpp` only, and re-run the bisection.
   If jitter drops to zero (or measurably decreases), this implicates floating-point
   non-associativity/reassociation specifically, and the fix becomes a scoped `/fp:precise` pragma
   or explicit Kahan-style summation in the specific hot loop identified, rather than a
   project-wide flag change (which would cost real batch-search performance).

### Phase 4 -- ThreadSanitizer / AddressSanitizer pass

If Phases 1-3 don't converge on a specific line, run the Phase 2 reproducer under TSan (races) and
ASan (out-of-bounds/uninitialized reads) on Linux. This is listed after Phases 1-3 rather than first
because building the reproducer (Phase 2) is a prerequisite, and because Phases 1-3's targeted
experiments are cheaper to run first and may make a sanitizer pass unnecessary by localizing the bug
directly.

### Phase 5 -- fix, validate, document

1. Apply the targeted fix identified above. Prefer the narrowest fix that addresses the confirmed
   root cause (e.g. bound-checking a specific read, not blanket-clearing a whole buffer every
   spectrum, which would reintroduce the per-spectrum allocation cost findings 1/4/5 in
   `docs/20260714_rtspostprocessing.md` worked to remove).
2. Validate with the same rigor already established this session: before/after `git stash` A/B
   comparison via a full RTS run diff (not just the reproducer, to confirm the real Thermo/C# path
   is also fixed), Linux unit suite (19 tests), and -- if the fix touches anything performance
   sensitive -- an `RTS_TIMING` before/after comparison to confirm no regression.
3. Success criterion to close this out: replicate RTS runs are **byte-identical** (0 differing
   lines in the full `rts.out` diff), matching the bar the user says batch search already meets --
   not just "reduced," which is where this session's earlier attempt (findings the C# race and the
   AScorePro bug) left things.
4. Update `docs/20260714_rtspostprocessing.md`'s "RTS E-value nondeterminism under concurrency"
   section with the final root cause and resolution, superseding its current "not fully resolved"
   status.

## Non-goals / scope guardrails

- Do not ship a fix that serializes the RTS per-spectrum pipeline in production -- that was only a
  diagnostic tool this session and would eliminate RTS's multithreading benefit entirely (the
  reason findings 1/2/4/5 and the original `docs/20260713_PIopt.md` work exist).
- Do not change `/fp:fast` project-wide without measuring the batch-search performance cost first,
  even if Phase 3.3 implicates it -- prefer a scoped fix.
- The `XcorrScoreI`/`StorePeptideI` tie-break fix (Phase 1.3) is a separate, batch-search-only
  correctness item. Track it, but don't let it block or get conflated with closing the RTS jitter
  investigation, since Phase 1.1 is expected to show it's not the RTS cause.
- Residual jitter below the noise floor of this repo's own pre-existing, separately-documented
  near-tied-score nondeterminism (see commit `7c04814d`'s FASTA_DB fix, which the `git log` history
  shows was itself found via 100+ replicate runs at escalating scale) may not be fully eliminable if
  it turns out to share a root cause with that class of issue in a part of the codebase this plan
  doesn't reach -- if Phase 5 gets close but not to exactly zero, document the residual rate and
  mechanism rather than continuing indefinitely; report back for a scope decision at that point
  rather than assuming further effort is warranted.
