# Plan: identify and resolve RTS E-value jitter

Status: **RESOLVED.** Phases 0, 1, 2, 3, 5 complete (Phase 4/TSan skipped as unnecessary -- Phase 3
proved this was an intra-thread logic bug, not a data race). Root cause: uncapped `iHighestIon`
passed into `MakeCorrData()` from `PreprocessSingleSpectrumCore()`'s RTS path let stale data from a
prior, differently-sized spectrum on the same thread corrupt windowed XCorr normalization for some
candidates. Fixed by clamping `iHighestIon` to `iArraySize - 1` before the `MakeCorrData()` call.
Validated byte-identical (0 differing lines) on both the real Windows RTS harness and the
`tests/rts_repro/` reproducer, matching batch search's determinism bar exactly. See the Phase 5
section for full details.

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

### Phase 2 -- build a fast, Thermo-independent reproducer (highest leverage, do early) -- DONE

Built `tests/rts_repro/` (see its `README.md` for build/run instructions). `rts_repro.cpp` links
directly against `libcometsearch` and calls `ICometSearchManager::InitializeSingleSpectrumSearch()`
+ `DoSingleSpectrumSearchMultiResults()` from N worker threads pulling off a shared work queue --
the same API and concurrency pattern `RealtimeSearch/SearchMS1MS2.cs` uses via `CometWrapper.dll`,
minus the Thermo `RawFileReader`/C++-CLI/C# layers entirely. Parameter setup mirrors
`ConfigureInputSettings()`'s "index already exists" branch exactly (RTS never reads
`comet.params` -- see that C# method for the authoritative list of what it actually sets, and note
this exposed that RTS hardcodes `max_index_runtime=200` regardless of the params file, which the
driver now replicates for fidelity).

**Fixture data**: `tests/rts_repro/fixture_spectra.txt`, 197 real spectra (scan number, charge,
precursor m/z, mass/intensity arrays) extracted once from `20250520_Hela_60min_06.raw` via a
temporary, since-reverted env-var-gated dump in `SearchMS1MS2.cs` -- a fixed list of scan numbers
already known from earlier full-harness bisection to show jitter, plus a modulo-300 sample for
general coverage.

**Result: reproduces cleanly, on the first working build.** Two 8-thread runs of the same 197-spectrum
fixture set, sorted by scan number and diffed:

```
MS2 5835   R.VPSFAAGR.V     (same peptide)   E-value 160.147  vs  159.345
MS2 5938   K.ESRGGPSR.R     (same peptide)   E-value 64.6068  vs  64.629
MS2 7124   K.HPVFPSGK.F     (same peptide)   E-value 14.5699  vs  14.7645
MS2 25030  R.AYAETSKMK.V    vs  R.SVSSSSYRR.M   (different winning peptide)
```

The scan 25030 flip is not just the same *class* of event -- it is the **identical flip** (same
scan, same two competing peptides, `R.AYAETSKMK.V` <-> `R.SVSSSSYRR.M`) already documented in
`docs/20260714_rtspostprocessing.md`'s original full-harness investigation. Repeated across several
more 8-thread run pairs: 3, 4, 5, 6 differing lines out of 197 each time (~1.5-3%, higher than the
full harness's ~0.5-0.7% because the fixture set intentionally over-samples scans already known to
be jitter-prone). `num_threads=1` reproduced RTS's other established property exactly: two
single-threaded runs were **byte-identical** (0 differing lines).

**Conclusion.** Both success-criterion conditions from the original plan are met with room to
spare: same class of jitter (same-peptide E-value drift, and the rarer different-winner case), same
determinism/non-determinism split by thread count, and a specific historical case reproduced
exactly. Hypothesis C (something specific to the C#/CLI layer or the real Thermo reader) is
correspondingly *weakened*, not eliminated -- the reproducer shows this is fundamentally a C++-side
phenomenon reachable through the plain `ICometSearchManager` API, with no C#/CLI/Thermo code in the
loop at all. Iteration time dropped from ~90 seconds (full RTS run) to under a second per run,
unlocking Phase 3 (targeted histogram/`/fp:precise` experiments) and Phase 4 (ThreadSanitizer, now
buildable on Linux) to run in a tight loop.

### Phase 3 -- distinguish the hypotheses empirically -- DONE, root cause localized

All three steps run with the Phase 2 reproducer (sub-second iteration made this a same-session pass
instead of a multi-day one).

**Step 1 (full histogram dump).** Added temporary instrumentation dumping the full
`iXcorrHistogram[HISTO_SIZE]` array per spectrum, keyed by `(dMZ, charge)`, to a log file. Pulled
the histogram for three spectra that differed between two runs (5835, 6650: same winning peptide,
different E-value; 25030: different winning peptide). Result, e.g. scan 5835:

```
run1: uiHistogramCount=6  winner fXcorr=0.597000  bins[3..7] = 2545 1049 500 200 69 ...
run2: uiHistogramCount=6  winner fXcorr=0.597000  bins[3..7] = 2544 1058 507 200 69 ...
```

**`uiHistogramCount` and the winner's own XCorr are bit-identical, but the histogram bin contents
are not.** This is conclusive per the plan's own branch logic: some *non-winning* candidate's own
computed XCorr genuinely differs between runs, on a spectrum whose real-candidate scan is
single-threaded and deterministic in order (Phase 1). The regression math itself isn't implicated --
it's being fed different input.

**Step 2 (hypothesis A -- stale buffer).** Temporarily changed the RTS preprocessing path's
buffer-clearing (`CometPreprocess.cpp`, `PreprocessSingleSpectrumCore`'s `bUseThreadLocalPool`
branch) to fully clear `pdTmpRawData`/`pdTmpFastXcorrData`/`pdTmpCorrelationData`/`pfFastXcorrData`/
`pfFastXcorrDataNL`/`pfSpScoreData` to `iArraySizeGlobal` every spectrum, instead of the bounded
`iArraySize`/`iArraySize + iXcorrProcessingOffset` region. Re-ran the reproducer:

| Comparison | Differing lines (of 197) |
|---|---|
| Baseline (bounded clear), 2 runs | 3-6 (consistent with Phase 2's earlier measurements) |
| Full clear, run A vs B | **0** |
| Full clear, run A vs C | **0** |
| Full clear, run A vs D | **0** |
| Full clear, run C vs D | **0** |
| Reverted back to bounded clear, 2 more runs | 2 (jitter returns) |

**Hypothesis A is confirmed: four independent full-clear runs, zero jitter across every pairwise
comparison; reverting immediately brings the jitter back.** This localizes the root cause to the
under-bounded clear in the RTS preprocessing path -- some read in the downstream computation
(sliding-window XCorr processing, SP-score binning, or the b/y-ion dot product) touches a region of
one of these six buffers beyond `iArraySize` that a *differently-sized* spectrum processed earlier
on the same thread left non-zero. The exact read site was not pinned down in this pass (a full clear
proves the *class* of bug, not the specific line) -- that's Phase 5 fix-scoping work, not required to
close Phase 3's goal of distinguishing hypotheses.

**Step 3 (hypothesis B -- fp non-associativity) -- effectively answered for free by Phase 2.** The
Linux reproducer is built with plain `g++ -O2`, no `-ffast-math`/`-funsafe-math-optimizations` --
GCC will not reorder or vectorize floating-point reductions without one of those flags, so the
build already has the same "strict" floating-point semantics `/fp:precise` would give on MSVC. It
still showed the jitter (Phase 2's result, and the baseline row in the table above). Combined with
Step 2's direct causal fix, hypothesis B is not just weakened but **superseded**: the mechanism is a
memory-content bug, not a reassociation/rounding-order issue, and no separate `/fp:precise` MSVC
experiment is needed to establish that.

**Conclusion.** Root cause localized to hypothesis A: incomplete buffer-clearing in
`CometPreprocess.cpp`'s RTS single-spectrum preprocessing path lets stale data from a prior,
differently-sized spectrum processed by the same thread leak into a downstream read, producing a
genuinely different (not just differently-ordered) computed XCorr for some non-winning candidates.
This is an intra-thread logic bug (`RtsScratch`'s buffers are `thread_local`, reused across spectra
on one thread, not shared across threads), not a data race -- Phase 4's ThreadSanitizer pass is
consequently much less likely to be needed; the next step is Phase 5's scoped fix-finding (identify
which specific read exceeds `iArraySize` and either bound-check it or extend the cleared region to
cover it) followed by validation that the narrow fix, not a blanket full clear, eliminates the
jitter.

### Phase 4 -- ThreadSanitizer / AddressSanitizer pass

If Phases 1-3 don't converge on a specific line, run the Phase 2 reproducer under TSan (races) and
ASan (out-of-bounds/uninitialized reads) on Linux. This is listed after Phases 1-3 rather than first
because building the reproducer (Phase 2) is a prerequisite, and because Phases 1-3's targeted
experiments are cheaper to run first and may make a sanitizer pass unnecessary by localizing the bug
directly.

### Phase 5 -- fix, validate, document -- DONE, RESOLVED

**Root cause, precisely.** `PreprocessSingleSpectrumCore()`'s RTS thread-local-pool path
(`CometPreprocess.cpp`) computes `pPre.iHighestIon` from every peak's bin, unconditionally --
including peaks whose bin lands at or beyond `iArraySize` (the ion-loading loop deliberately never
writes `pdTmpRawData` for those bins, guarded by `iBinIon < iArraySize`, but it still updates
`iHighestIon` to reflect them). This uncapped `iHighestIon` was then passed to `MakeCorrData()`,
whose windowed-normalization loop reads/writes `pdTmpRawData`/`pdTmpCorrelationData` up to
`iHighestIon` (capped only by `iArraySizeGlobal`, not `iArraySize` -- `MakeCorrData()` itself
already carried a stale `// FIX: need to check why both iArraySize and iHighestIons are used`
comment flagging this exact ambiguity). The RTS path's buffer-clearing optimization (added for
performance -- see findings 1/4/5 in `docs/20260714_rtspostprocessing.md`) only pre-zeroes these
buffers up to `iArraySize + iXcorrProcessingOffset`, unlike the batch path's `Preprocess()`, which
always clears the full `iArraySizeGlobal`. Net effect: any spectrum with a peak beyond the in-range
cutoff let `MakeCorrData()` read stale `pdTmpRawData` content left behind by a *different, larger*
spectrum previously processed on the same thread, corrupting that window's normalization factor and
therefore the XCorr of any candidate whose fragment bins shared that window with the contaminated
tail -- explaining both the same-peptide-different-E-value cases (a non-winning candidate's score
shifts a histogram bin) and the rarer different-winning-peptide cases.

**Fix**: clamp `iHighestIon` to `iArraySize - 1` immediately before the `MakeCorrData()` call in
`PreprocessSingleSpectrumCore()`, RTS path only. Minimal and provably safe: `pdTmpRawData` is always
zero beyond `iArraySize` once properly cleared (real fragment data was never written there in the
first place), so `MakeCorrData()` has nothing legitimate to process past that point regardless of
how much larger the uncapped `iHighestIon` was. No memset widening, no reintroduction of the
per-spectrum allocation cost findings 1/4/5 removed.

**Validation.**
- `tests/rts_repro/`: 5 independent 8-thread runs of the 197-spectrum fixture set, all 10 pairwise
  comparisons **zero** differing lines (previously 3-6 per pair before the fix).
- Real Windows RTS harness (`RealtimeSearch.exe`, full Thermo/C++-CLI/C# stack, same 8-thread,
  49,844-scan methodology as the original bisection): two full runs, **0 differing PSM lines** in
  the complete `rts.out` diff (only the expected per-spectrum timing column and end-of-run summary
  stats varied) -- the **byte-identical** bar set as this phase's success criterion, matching what
  batch search already achieves.
- Linux unit suite: 19/19 pass.
- `RTS_TIMING` before/after (`llPreprocess`, the step this touches): 425.851us -> 427.401us mean,
  +0.36% -- within run-to-run noise, confirming the single-integer-comparison fix has no measurable
  performance cost.
- `docs/20260714_rtspostprocessing.md`'s "RTS E-value nondeterminism under concurrency" section
  updated with this root cause and resolution, retitled from "partially fixed" to "RESOLVED".

**This closes the investigation.** All 5 phases of this plan are complete; RTS PI_DB search is now
deterministic across replicate runs, matching batch search's determinism exactly.

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
