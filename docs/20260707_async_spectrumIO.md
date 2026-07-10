# Asynchronous spectrum readahead for FusedLoadAndSearchSpectra

Date: 2026-07-07
Status: proposed
Scope: `CometSearch/CometPreprocess.cpp` (file-local additions + the fused loop),
2 new include lines, and one new private static method declaration in
`CometPreprocess.h` (the shared filter helper, Step 2 -- it must be a class
member because it calls the private `CheckActivationMethodFilter`). No public
API change to CometPreprocess, MSToolkit, or MSReader.

## 1. Current flow and where the read stalls

`FusedLoadAndSearchSpectra` (CometPreprocess.cpp:3246) is a 3-role pipeline today:

- One PRODUCER thread (the caller) runs the `while(true)` read loop
  (CometPreprocess.cpp:3280). Each iteration it calls `PreloadIons`
  (CometPreprocess.cpp:3284/3289), which is a thin wrapper over
  `mstReader.readFile(...)` (CometPreprocess.cpp:1891-1904); then it does light
  per-spectrum filtering (clearMzRange scan at 3333-3349, `iMinPeaks` check at
  3351, activation filter at 3359) and `queue.push(std::move(mstSpectrum))`
  (CometPreprocess.cpp:3361) into a `BoundedSpectrumQueue`
  (CometPreprocess.cpp:153-199).
- N WORKER jobs (launched on the pool at CometPreprocess.cpp:3270-3278, BEFORE
  the read loop) pop from that queue and run preprocess -> search ->
  post-analysis in `FusedSearchSpectrum`.

So I/O already overlaps search: the producer reads while workers search. The
remaining serialization is INSIDE the producer thread: `readFile` (blocking file
read + mzML/mzXML SAX parse + optional gz inflate) runs to completion before the
producer can hand the just-parsed spectrum to the workers and start the next
read. When workers momentarily drain the search queue, they idle until the
producer's current `readFile` returns. The read and the enqueue-to-workers step
are welded together on one thread.

The goal here is to decouple them: a dedicated readahead thread does nothing but
`readFile`, staying N spectra ahead in a small ring buffer, so an in-progress
file read never delays handing an already-parsed spectrum to an idle worker.

## 2. Constraint analysis: what one thread may touch

`MSReader` is not thread-safe on one file handle. For mzML/mzXML it drives
`readMZPFile` (MSReader.cpp:1300), mutating instance members `rampFileIn`,
`pScanIndex`, `rampIndex`, `rampLastScan`, `lastReadScanNum`; `getLastScan`
(MSReader.cpp:259) reads `rampLastScan`/`rampFileIn`. Two threads calling into the
same `MSReader` concurrently is a data race.

Therefore the design keeps a SINGLE dedicated reader thread as the ONLY code that
ever calls `mstReader.readFile()` OR `mstReader.getLastScan()`. The main
(producer) loop must stop calling both while readahead is active. This is safe
because separate threads are fine as long as exactly one touches the handle;
`rampOpenFile` allocates a fresh `RAMPFILE` per open (RAMPface.cpp:378-401) with no
process-global parser state, so one background reader is sufficient and race-free.

Note on TRUE read parallelism (out of scope): distinct `MSReader` instances hold
distinct handles, so K readers each opening the same file and reading disjoint
scan ranges would be thread-safe and would actually multiply read throughput. The
task constrains us to a single `mstReader` and a minimal-change readahead wrapper,
so multi-reader sharding is listed only as the Section 8 alternative.

## 3. Design: a single-thread readahead stage in front of the existing queue

Add a THIRD stage ahead of the current two:

```
[reader thread: readFile]  ->  readyRing (BoundedSpectrumQueue, depth D)
   ->  [producer loop: filter + enqueue + CheckExit]  ->  searchQueue (existing)
   ->  [N workers: preprocess + search + post-analysis]
```

Reuse the existing file-local `BoundedSpectrumQueue` (CometPreprocess.cpp:153-199)
for the readyRing -- it is already a bounded, blocking, move-based `Spectrum`
queue with a `finish()` drain signal, so no new synchronization primitive is
needed. The reader thread is a raw `std::thread`, NOT a pool job: the pool has
exactly `iNumThreads` threads and the loop already launches `iNumThreads` search
jobs that fully occupy it, so a queued reader job would deadlock behind them.

### 3a. Gating -- enable readahead only where it is provably correct

Enable readahead only when all hold:

1. `iAnalysisType == AnalysisType_EntireFile` -- pure sequential streaming, so the
   producer loop's scan-range / `bSkipToStartScan` seek branches
   (CometPreprocess.cpp:3303-3316, 3323-3331, 3353-3357) never fire and the reader
   thread can own a simple `readFile(NULL)` loop.
2. `g_staticParams.options.iSpectrumBatchSize == 0` -- the whole file is one batch
   (the "large mzML" target). This avoids the batch-boundary spectrum-loss bug:
   with a nonzero batch cap the loop breaks mid-file via CheckExit, but a readahead
   thread would already have pulled spectra past the break point into the ring,
   and tearing the thread down at batch end would silently drop them. Gating to
   `== 0` sidesteps this entirely.
3. `g_staticParams.inputFile.iInputType == InputType_MZXML` -- the mzML/mzXML/.gz
   family (all map to `InputType_MZXML`, SearchUtils.cpp:29-35). Excludes
   `InputType_RAW`: the Thermo path calls managed `cRAW` code (MSReader.cpp:1260-
   1286) under `/clr`, and driving it from a native `std::thread` needs CLR thread
   attach -- out of scope; RAW stays on the synchronous path. (Enabling RAW later
   is Section 8 future work.)

When any condition is false, fall through to the existing synchronous loop
UNCHANGED. This confines all new behavior to exactly the scenario the task targets.

### 3b. Reader-thread termination and EOF

For the gated case, mzML EOF surfaces as `readFile` yielding `getScanNumber()==0`
(readMZPFile returns false / no matching spectrum at end, MSReader.cpp:1374-1380).
The reader pushes every spectrum with `scanNumber != 0` and, on the first
`scanNumber == 0` (or on `g_cometStatus` error/cancel, or on a stop request from
join), calls `readyRing.finish()` and exits. The producer loop ends when
`readyRing.pop()` returns false. This reproduces the current EntireFile
termination, whose only non-error exit is exactly `EntireFile && validInput &&
scanNum==0` (CheckExit, CometPreprocess.cpp:1988-1994).

`getLastScan()` is constant once the file is open, so the reader snapshots it once
after the first read into an atomic; the producer reads the snapshot instead of
calling `mstReader.getLastScan()` (removing the second handle-touch on the main
thread).

Note: "only non-error exit" above means the only branch of `CheckExit` keyed
specifically to `AnalysisType_EntireFile`. `CheckExit` also has a third,
input-type-generic true-return that is NOT keyed to `EntireFile` and is not
mentioned above -- see 3c for why it is kept anyway.

### 3c. Keep CheckExit's third (corrupted-index) safety net -- it's cheap insurance

`CheckExit` has a third true-return beyond error/cancel and EntireFile EOF:
`IsValidInputType(...) && iTotalScans > iReaderLastScan`
(CometPreprocess.cpp:2000-2005), guarding against a scan index that is corrupt
or truncated such that `getScanNumber()` keeps returning non-zero past where it
should have hit EOF. In the normal mzML/mzXML path this is unreachable --
`readMZPFile`'s own `rampIndex > rampLastScan` bound check (MSReader.cpp:1374,
1380) already forces `getScanNumber()==0` before this condition could ever be
true -- but it exists precisely as a defense against a malformed/corrupted
index doing something the normal bound check didn't catch.

There is no reason to drop it in the readahead path: `iTotalScans` is already
tracked by the producer loop and `iSnapshotLastScan` is the exact atomic
equivalent of `iReaderLastScan`, so the check is a single integer comparison
using values already in hand. Step 3 keeps it as an extra break condition in
the readahead loop body -- it costs nothing and preserves full parity with the
synchronous path's exit conditions instead of silently narrowing them.

## 4. Implementation steps (exact changes, all in CometPreprocess.cpp)

### Step 1 -- includes (top of file, near lines 24-27)

```cpp
#include <chrono>
#include <condition_variable>
#include <mutex>
#include <queue>
#include <thread>     // new
#include <atomic>     // new
```

### Step 2 -- file-local readahead state + loop (after BoundedSpectrumQueue, ~line 200)

```cpp
// ---------------------------------------------------------------------------
// Single-thread spectrum readahead (EntireFile, spectrum_batch_size==0, mzML/
// mzXML only).  One dedicated thread is the ONLY code that touches mstReader
// while readahead is active, satisfying MSReader's single-handle thread-safety
// rule.  Parsed spectra are buffered in readyRing so a slow file read never
// stalls hand-off of an already-parsed spectrum to an idle search worker.
// ---------------------------------------------------------------------------
struct SpectrumReadahead
{
   BoundedSpectrumQueue readyRing;
   std::thread          ioThread;
   std::atomic<int>     iSnapshotLastScan{-1};
   std::atomic<bool>    bStop{false};

   explicit SpectrumReadahead(size_t depth) : readyRing(depth) {}

   // Producer loop calls this instead of PreloadIons.  false => no more spectra.
   bool next(Spectrum& out) { return readyRing.pop(out); }

   void requestStopAndJoin()
   {
      bStop.store(true, std::memory_order_relaxed);
      readyRing.finish();                       // unblock a full-queue push()
      if (ioThread.joinable()) ioThread.join();
   }
};

// Runs on the dedicated reader thread.  bFirstRead honors the cross-batch
// _bFirstScan state captured by the caller before this thread was spawned.
static void ReadaheadLoop(MSReader& mstReader, SpectrumReadahead& ra, bool bFirstRead)
{
   Spectrum spec;

   if (bFirstRead)
      CometPreprocess::PreloadIons(mstReader, spec, false, 0);   // opens the file
   else
      CometPreprocess::PreloadIons(mstReader, spec, true);

   ra.iSnapshotLastScan.store(mstReader.getLastScan(), std::memory_order_relaxed);

   while (!ra.bStop.load(std::memory_order_relaxed))
   {
      if (g_cometStatus.IsError() || g_cometStatus.IsCancel())
         break;
      if (spec.getScanNumber() == 0)            // EOF for a valid input type
         break;
      ra.readyRing.push(std::move(spec));        // move parsed spectrum to consumer
      CometPreprocess::PreloadIons(mstReader, spec, true);
   }

   ra.readyRing.finish();
}
```

`PreloadIons` is already public (CometPreprocess.h:136); `_bFirstScan` is private,
so the caller (a CometPreprocess method) reads/sets it and passes `bFirstRead` in.

### Step 2b -- shared filter helper (eliminates the duplicate-filter-block risk)

The clearMzRange scrub + `iMinPeaks` gate + activation-method filter + `queue.push`
sequence (CometPreprocess.cpp:3349-3378) currently exists once, inline in the
synchronous loop. Step 3 needs the same sequence in the readahead loop. Copying
it a second time -- even as "identical" inline code, as an earlier draft of this
plan proposed -- means a future filter change (a new clearMzRange variant, a
different `iMinPeaks` rule, a new activation method) can land in one copy and
not the other with nothing to catch the divergence. Factor it into one function
instead, called from both loops.

It must be a private static member of `CometPreprocess`, not a free file-local
function like `ReadaheadLoop`: it calls `CheckActivationMethodFilter`, which is
private (CometPreprocess.h:167), and a non-member function has no access to it.
This is the one header change this design needs beyond the two includes.

`CometPreprocess.h` (private section, next to `CheckActivationMethodFilter`):

```cpp
   // Shared by both the synchronous and readahead producer loops in
   // FusedLoadAndSearchSpectra so a future filter change cannot land in only
   // one of the two call sites and silently diverge between them.
   static bool FilterAndEnqueueSpectrum(Spectrum& mstSpectrum,
                                        BoundedSpectrumQueue& queue);
```

(`BoundedSpectrumQueue` is currently file-local to CometPreprocess.cpp; this
declaration requires either moving that struct's definition into the header
above the class, or forward-declaring it there and keeping the definition in
the .cpp. Either is a mechanical detail to settle during implementation, not a
design question.)

`CometPreprocess.cpp` (defined near the other private methods, or immediately
above `FusedLoadAndSearchSpectra`):

```cpp
// Mutates mstSpectrum in place (clearMzRange zeroes cleared-range intensities)
// and moves it into queue if it survives all three filters.  Returns true iff
// the spectrum was enqueued.  This is the ONLY copy of this filter sequence;
// both the synchronous and readahead loops in FusedLoadAndSearchSpectra call
// it (Step 3) so they cannot drift apart.
bool CometPreprocess::FilterAndEnqueueSpectrum(Spectrum& mstSpectrum, BoundedSpectrumQueue& queue)
{
   int iNumClearedPeaks = 0;

   if (g_staticParams.options.clearMzRange.dEnd > 0.0
         && g_staticParams.options.clearMzRange.dStart <= g_staticParams.options.clearMzRange.dEnd)
   {
      int ip = 0;
      while (true)
      {
         if (ip >= mstSpectrum.size() || mstSpectrum.at(ip).mz > g_staticParams.options.clearMzRange.dEnd)
            break;
         if (mstSpectrum.at(ip).mz >= g_staticParams.options.clearMzRange.dStart
               && mstSpectrum.at(ip).mz <= g_staticParams.options.clearMzRange.dEnd)
         {
            mstSpectrum.at(ip).intensity = 0.0;
            iNumClearedPeaks++;
         }
         ip++;
      }
   }

   if (mstSpectrum.size() - iNumClearedPeaks < g_staticParams.options.iMinPeaks)
      return false;

   if (!CheckActivationMethodFilter(mstSpectrum.getActivationMethod()))
      return false;

   queue.push(std::move(mstSpectrum));
   return true;
}
```

Deliberately NOT folded in: the `AnalysisType_SpecificScanRange` recheck
(CometPreprocess.cpp:3369-3373, `iAnalysisType == AnalysisType_SpecificScanRange
&& iLastScan > 0 && iScanNumber > iLastScan`). `AnalysisType_EntireFile` and
`AnalysisType_SpecificScanRange` are mutually exclusive at assignment
(Comet.cpp:784 vs. 789/801/811 -- a run is one or the other, never both), so
this recheck is unreachable from the readahead path by construction. Pulling it
into the shared helper would just be dead code on that side; it stays inline in
the synchronous loop only, called before `FilterAndEnqueueSpectrum` in the same
position it occupies today.

### Step 3 -- branch the read loop in FusedLoadAndSearchSpectra

At the top of `FusedLoadAndSearchSpectra`, after the workers are launched
(CometPreprocess.cpp:3278) compute the gate and, when on, spawn the reader:

```cpp
   const bool bUseReadahead =
         (iAnalysisType == AnalysisType_EntireFile)
      && (g_staticParams.options.iSpectrumBatchSize == 0)
      && (g_staticParams.inputFile.iInputType == InputType_MZXML);

   // Modest fixed depth: one reader cannot usefully run far ahead, and this
   // bounds extra in-flight Spectrum RAM independently of thread count.
   const size_t iReadDepth = 16;
   std::unique_ptr<SpectrumReadahead> pRA;

   if (bUseReadahead)
   {
      bool bFirstRead = _bFirstScan;
      _bFirstScan = false;                       // reader thread now owns the reads
      pRA.reset(new SpectrumReadahead(iReadDepth));
      pRA->ioThread = std::thread(ReadaheadLoop, std::ref(mstReader), std::ref(*pRA), bFirstRead);
   }
```

Then wrap the existing `while(true)` read loop (CometPreprocess.cpp:3296-3413)
so the readahead path uses a simplified body and the synchronous path is
behavior-identical to the current code. The one deliberate textual change to
the synchronous side is that its filter block (CometPreprocess.cpp:3349-3378)
now calls the shared `FilterAndEnqueueSpectrum()` from Step 2b instead of
containing the clearMzRange/iMinPeaks/activation-filter/push code inline, so
the two loops share one copy of that logic instead of two:

```cpp
   if (bUseReadahead)
   {
      Spectrum mstSpectrum;
      int iFileLastScan = pRA->iSnapshotLastScan.load(std::memory_order_relaxed);
      bool bAborted = false;

      while (pRA->next(mstSpectrum))              // false at EOF/error/stop
      {
         if (g_cometStatus.IsError() || g_cometStatus.IsCancel())
         {
            bAborted = true;
            break;
         }

         // iScanNumber guaranteed != 0 here (EOF is consumed by the reader
         // thread and surfaces as next() returning false, above).
         FilterAndEnqueueSpectrum(mstSpectrum, queue);

         iTotalScans++;

         // Same defensive safety net as CheckExit's third branch
         // (CometPreprocess.cpp:2000-2005, see design doc 3c): bail out if
         // the producer has consumed more matching-level spectra than the
         // file's own scan index reports.  Unreachable in practice --
         // readMZPFile's own EOF bound already prevents it -- but free to
         // keep given iTotalScans and iFileLastScan are already in hand.
         if (iTotalScans > iFileLastScan)
            break;
      }

      // Mirror CheckExit exactly: error/cancel return true WITHOUT setting
      // _bDoneProcessingAllSpectra (CometPreprocess.cpp:1961-1969); only a
      // clean EOF or the safety net above means EntireFile is actually done.
      // Setting this unconditionally (as an earlier draft of this plan did)
      // would diverge from the synchronous path -- harmless only by accident
      // today, because Pipeline.cpp's outer loop exits via bSucceeded==false
      // before it would ever re-check DoneProcessingAllSpectra() again.
      if (!bAborted)
         _bDoneProcessingAllSpectra = true;
   }
   else
   {
      // ... existing synchronous while(true) loop, UNCHANGED except that its
      // filter block (3349-3378) calls FilterAndEnqueueSpectrum(mstSpectrum,
      // queue) in place of the inline clearMzRange/iMinPeaks/activation/push
      // code.  The AnalysisType_SpecificScanRange recheck (3369-3373) stays
      // inline, evaluated before the call, exactly where it is today.
   }
```

Notes on the readahead body:
- No `PreloadIons`, no `_bFirstScan`, no `bSkipToStartScan`, no scan-range
  checks, no `AnalysisType_SpecificScanRange` recheck (all inapplicable to
  EntireFile, which is mutually exclusive with SpecificScanRange -- see
  Comet.cpp:784 vs. 789/801/811); this is why the gate requires EntireFile.
- `iFileLastScan` comes from the snapshot; the main thread never calls
  `mstReader.getLastScan()` in this branch (the old call at
  CometPreprocess.cpp:3308-3309, and the `iReaderLastScan` argument CheckExit
  would otherwise need, are both replaced by the snapshot).
- The old per-scan `CheckExit` (CometPreprocess.cpp:3407-3412) is replaced by
  two inline checks that reproduce all of its reachable true-returns for this
  gate: error/cancel (checked before filtering, same as `CheckExit`'s own
  first two branches) and the corrupted-index safety net (`iTotalScans >
  iFileLastScan`, mirroring `CheckExit`'s third branch per 3c). `CheckExit`'s
  EntireFile-EOF branch (`scanNum==0`) is redundant by construction here,
  since a `scanNum==0` spectrum never reaches this loop body at all -- it is
  consumed by the reader thread and surfaces only as `next()` returning
  `false`.

### Step 4 -- join the reader after the workers drain (near CometPreprocess.cpp:3398)

```cpp
   queue.finish();
   tp->wait_on_threads();

   if (pRA)
      pRA->requestStopAndJoin();                  // idempotent; safe on normal EOF

   return !g_cometStatus.IsError() && !g_cometStatus.IsCancel();
```

`requestStopAndJoin()` sets stop + `finish()` (both idempotent) then joins, so it
is correct whether the reader already exited on EOF or is still blocked on a full
`readyRing.push()`.

## 5. Why this is safe (correctness argument)

- Single-writer to the handle: only `ReadaheadLoop` calls `mstReader.readFile`/
  `getLastScan` while readahead is active; the producer branch calls neither
  (uses the atomic snapshot). No concurrent MSReader access.
- No lost spectra: gate condition 2 (`iSpectrumBatchSize == 0`) guarantees a
  single batch, so the reader is spawned and joined within one
  `FusedLoadAndSearchSpectra` call; nothing it reads is discarded.
- Order preserved into `searchQueue`: the reader is a single thread pushing in
  file order into `readyRing`; the producer pops in that order and pushes to the
  search queue in that order -- identical to today. (Final result order is
  scan-sorted downstream at Pipeline.cpp:184 regardless, so even reordering would
  be output-invariant, but no reordering is introduced.)
- Termination parity: EntireFile's sole non-error exit (scn==0 EOF) is
  preserved, plus `CheckExit`'s corrupted-index safety net (3c); error/cancel
  are checked on both the reader and producer sides, and
  `_bDoneProcessingAllSpectra` is set true only on those same non-error exits
  -- matching `CheckExit`, which returns `true` on error/cancel without
  touching that flag (CometPreprocess.cpp:1961-1969).
- Fallback is a no-op change: when the gate is false the original synchronous loop
  runs unchanged (aside from calling the shared `FilterAndEnqueueSpectrum` in
  place of its former inline filter block, Step 2b), so all non-EntireFile /
  batched / RAW / MGF paths are untouched behaviorally.

## 6. Honest performance expectation

This is a latency-hiding / jitter-smoothing change, NOT a read-throughput
multiplier. There is still ONE thread parsing the file, so a file whose search is
bound by single-thread parse speed will not go faster than that ceiling; for that
case the real fix is multi-reader sharding (Section 8). The win here is:

- Idle workers get the next already-parsed spectrum instantly instead of waiting
  for the producer's in-flight `readFile` to finish -- benefits bursty cases and
  high thread counts where the search queue occasionally drains.
- The producer's own non-read work (clearMzRange peak scan, filtering, enqueue)
  now overlaps the next read instead of serializing behind it.

Expect a modest wall-clock improvement that grows with thread count and with
clearMzRange enabled; expect roughly break-even on a small file or when search is
the hard bottleneck. Measure before committing to it as a default.

## 7. Validation

### 7a. Identical output (correctness gate)

Because results are scan-sorted before writing (Pipeline.cpp:184), old vs new
output must match. Build a baseline binary and a readahead binary, run both on a
large mzML at several thread counts, and diff:

```bash
for T in 1 8 20; do
  for BIN in old new; do
    /tmp/comet.$BIN -Pcomet.params.high --num_threads=$T -N/tmp/o.$BIN.$T big.mzML
  done
  tail -n +3 /tmp/o.old.$T.txt | sort > /tmp/a
  tail -n +3 /tmp/o.new.$T.txt | sort > /tmp/b
  diff /tmp/a /tmp/b && echo "T=$T IDENTICAL"
done
python tests/unit/run_tests.py --comet /mnt/c/Work/Comet-master/comet.exe
```

Also verify determinism (two new-binary runs at 20 threads -> identical sorted
output) and that a batched run (`spectrum_batch_size` nonzero, gate off) and a RAW
run still produce unchanged output (confirming the fallback path).

### 7b. Wall-clock benchmark at high thread count on a large file

```bash
for BIN in old new; do
  for i in 1 2 3; do
    /usr/bin/time -v /tmp/comet.$BIN -Pcomet.params.high \
        --num_threads=20 -N/tmp/bench.$BIN.$i big_100k.mzML 2>>/tmp/t.$BIN
  done
done
grep 'wall clock' /tmp/t.old /tmp/t.new
```

Report median of 3; also use the `comet-benchmark` skill for Hz / ms-per-spectrum
old vs new. Test at least one gz-compressed mzML (parse+inflate is heavier, so the
overlap win is larger) and one uncompressed.

### 7c. Confirm the reader stays ahead

Temporarily instrument `readyRing.pop()` to count how often it blocks on an empty
ring (reader fell behind) vs returns immediately. A near-zero empty-block rate at
20 threads confirms the readahead is effective; a high rate confirms the run is
read-bound and points to Section 8.

## 8. Alternatives considered

- Double buffer (D=2) instead of a ring: simpler but one slow parse stalls the
  hand-off; the ring at D=16 costs a few extra buffered Spectrum objects and
  absorbs parse-time variance. Recommend the ring (reusing BoundedSpectrumQueue).
- Fold filtering into the reader thread: would shrink the producer to a pure
  splice, but mixes I/O and CPU filtering on one thread and complicates the diff;
  rejected for minimality. (Distinct from Step 2b's shared filter helper, which
  keeps filtering on the producer side in both loops and only de-duplicates the
  code -- that one is adopted; this alternative would move filtering onto the
  reader thread and is rejected.)
- Multi-reader sharding (TRUE read parallelism): K `MSReader` instances, each its
  own handle, reading disjoint scan-index ranges of the same file in parallel.
  This is the only design that beats the single-parse ceiling for read-bound
  files, and it is thread-safe (distinct handles). It is larger (needs per-range
  scan-index partitioning and violates the single-`mstReader` constraint), so it
  is deferred; if Section 7c shows runs are read-bound, this is the next step.

## 9. Risk notes

- Thread count rises by 1 (the reader) beyond `iNumThreads` search workers; the
  reader is I/O-bound and mostly blocked, so oversubscription is negligible.
- Extra RAM: up to `iReadDepth` (16) additional in-flight Spectrum objects on top
  of the existing `4 * iNumThreads` search-queue depth. Tune `iReadDepth` down if
  peak-RAM sensitive.
- The `_bFirstScan` handoff must set `_bFirstScan = false` on the main thread
  BEFORE spawning the reader (done in Step 3) so the static is not read/written
  concurrently by both threads.
- Gate is deliberately narrow (EntireFile + batchSize==0 + mzML/mzXML). Widening
  it (RAW, batched, scan-range) requires the additional handling in Sections 3a/8
  and must not be done by simply relaxing the condition.
- Filter logic (clearMzRange / iMinPeaks / activation method) lives in exactly
  one place -- `CometPreprocess::FilterAndEnqueueSpectrum` (Step 2b) -- called
  from both the synchronous and readahead loops, so a future filter change
  cannot land in only one of the two call sites and silently diverge.
