# Producer/Consumer Queue for Fused Batch FI_DB Path

## Context

`FusedLoadAndSearchSpectra` (added in `batch_FI_optimization`) eliminated the
three-sweep DRAM anti-scaling problem by fusing preprocess -> search ->
post-analysis per spectrum in one pass.  However, it still reads the entire batch
into `std::vector<Spectrum> vSpectra` before dispatching any worker, because the
original work-stealing design required the full vector to be present before
`fetch_add` indexing could begin.

This two-phase structure has a measurable RAM cost:

- `MSToolkit::Spectrum` stores its peaks in a heap-allocated `vector<Peak_T>`
  (12 bytes per peak: 8-byte `double mz` + 4-byte `float intensity`).
- A typical HeLa MS2 spectrum has ~600-800 peaks: ~8 KB per spectrum.
- A 302 MB HeLa `.raw` file contains ~40k MS2 spectra: **~320 MB** held in
  `vSpectra` simultaneously before a single spectrum is processed.
- Peak RAM for the HeLa benchmark is 10.5 GB; that 320 MB is recoverable
  with no algorithmic loss.

There is no correctness reason to read ahead more than one spectrum beyond what
workers can immediately consume.  A bounded producer/consumer queue lets the
read loop and the worker pool run concurrently, capping peak spectrum RAM to
`O(iNumThreads)` regardless of file size.

## Goal

Replace the two-phase (read-all -> process-all) structure of
`FusedLoadAndSearchSpectra` with a single-pass pipeline:

- **Producer** (calling thread): reads spectra from the raw file one at a time
  and pushes them into a bounded concurrent queue, blocking when the queue is
  full.
- **Consumers** (`iNumThreads` workers): pop from the queue and call
  `FusedSearchSpectrum` immediately, with no change to `FusedSearchSpectrum`
  itself.

I/O and compute overlap; peak spectrum RAM drops from ~320 MB to a few hundred KB
(queue depth x spectrum size).

## Confirmed facts the design relies on

- `FusedSearchSpectrum(Spectrum spec, int iSlot)` takes `Spectrum` by value
  (already a copy); the queue can safely `std::move` spectra into and out of
  storage.  No pointer aliasing issue.
- The pool slot index `iSlot` is a per-worker constant (0..iNumThreads-1).
  Each consumer lambda captures its own `t` at launch time -- same as the current
  `fetch_add` dispatch.  The `_ppbDuplFragmentArr` lifetime is the full batch,
  not per-spectrum.  This is unchanged.
- `CheckExit` / `g_pvQueryMutex` are called on the producer thread inside the
  read loop.  This is unchanged; only the producer runs the loop.
- `_bDoneProcessingAllSpectra` is set by the read loop before the function
  returns.  The outer `CometSearchManager` batch while loop reads it after
  `FusedLoadAndSearchSpectra` returns.  This is unchanged.
- `g_pvQuery` is pushed under `g_pvQueryMutex` inside `FusedSearchSpectrum`.
  Multiple consumer threads already do this in the current implementation;
  no change needed.
- PSM output is sorted by scan number (`compareByScanNumber`) in
  `CometSearchManager` after `FusedLoadAndSearchSpectra` returns.  Consumer
  execution order therefore does not need to match read order; only the sort
  at the end matters.  **PSM output remains bit-identical to the current
  fused path** (same `FusedSearchSpectrum`, same post-sort).
- `tp->wait_on_threads()` already blocks until all active jobs finish;
  no new synchronization primitive is needed at the outer level.

## Design: BoundedSpectrumQueue

A simple mutex + two condition-variable queue is sufficient.  The bottleneck
is `FusedSearchSpectrum` (~1.4 ms/spectrum), not queue throughput.  Lock-free
structures would add complexity with no measurable benefit.

```cpp
struct BoundedSpectrumQueue
{
   std::queue<Spectrum>     q;
   std::mutex               mtx;
   std::condition_variable  cvNotFull;
   std::condition_variable  cvNotEmpty;
   size_t                   maxDepth;
   bool                     bDone = false;

   explicit BoundedSpectrumQueue(size_t depth) : maxDepth(depth) {}

   // Producer calls this.  Blocks when queue is full.
   void push(Spectrum&& spec)
   {
      std::unique_lock<std::mutex> lk(mtx);
      cvNotFull.wait(lk, [&]{ return q.size() < maxDepth || bDone; });
      if (!bDone)
      {
         q.push(std::move(spec));
         cvNotEmpty.notify_one();
      }
   }

   // Consumer calls this.  Returns false when done and queue is empty.
   bool pop(Spectrum& spec)
   {
      std::unique_lock<std::mutex> lk(mtx);
      cvNotEmpty.wait(lk, [&]{ return !q.empty() || bDone; });
      if (q.empty()) return false;
      spec = std::move(q.front());
      q.pop();
      cvNotFull.notify_one();
      return true;
   }

   // Producer calls after the read loop ends.
   void finish()
   {
      std::unique_lock<std::mutex> lk(mtx);
      bDone = true;
      cvNotEmpty.notify_all();
      cvNotFull.notify_all();
   }
};
```

**Queue depth**: `iNumThreads * 4`.  At steady state, each consumer holds one
spectrum (inside `FusedSearchSpectrum`).  A depth of 4x threads means the
producer can stay up to 4 spectra/thread ahead without blocking.  For 20 threads,
peak in-flight spectra = 20 (being processed) + 80 (in queue) = 100 spectra x
~8 KB = **800 KB**, down from ~320 MB.

## Implementation changes

### Stage 1 -- Add `BoundedSpectrumQueue` (CometPreprocess.cpp)

Define the struct near the top of `CometPreprocess.cpp`, alongside the
`RtsScratch` definition.  It is a local implementation detail and does not need
its own header.

### Stage 2 -- Restructure `FusedLoadAndSearchSpectra`

Remove `std::vector<Spectrum> vSpectra` and the `std::atomic<size_t> ctr`
dispatch block.  Replace with:

```
1. Construct BoundedSpectrumQueue with depth = iNumThreads * 4.

2. Launch iNumThreads consumer workers BEFORE the read loop:

      for (int t = 0; t < iNumSlots; ++t)
      {
         tp->doJob([&queue, t]()
         {
            Spectrum spec;
            while (queue.pop(spec))
               FusedSearchSpectrum(std::move(spec), t);
         });
      }

3. Run the read loop on the calling thread (unchanged logic).
   Replace:
      vSpectra.push_back(mstSpectrum);
   with:
      queue.push(std::move(mstSpectrum));

4. After the read loop: call queue.finish().

5. tp->wait_on_threads() (unchanged).
```

Note: workers are launched before reading starts so that the first spectrum
pushed is consumed immediately with no dead time.  If launched after, the read
loop could fill the queue and stall before any worker starts.

### Stage 3 -- Error/cancel handling

If `g_cometStatus.IsError()` or `IsCancel()` is detected inside the read loop
(via `CheckExit`), the read loop breaks.  `queue.finish()` is called
unconditionally after the loop and before `wait_on_threads`, so consumers drain
any buffered spectra and exit cleanly.  This matches the current behavior where
spectra already in `vSpectra` were still processed after an early break.  If
strict cancellation is desired (drop buffered spectra on error), consumers can
check `g_cometStatus` at the top of their loop and call `queue.finish()`
themselves to unblock the producer.

## Files changed

| File | Change |
|------|--------|
| `CometSearch/CometPreprocess.cpp` | Add `BoundedSpectrumQueue`; restructure `FusedLoadAndSearchSpectra` |
| `CometSearch/CometPreprocess.h` | No change (no new public API) |
| Everything else | No change |

## Memory impact summary

| Metric | Before (batch_FI_optimization) | After (this plan) |
|--------|-------------------------------|-------------------|
| Spectrum buffer RAM | ~320 MB (40k spectra) | ~800 KB (100 spectra) |
| Peak total (HeLa) | 10.5 GB | ~10.2 GB (est.) |
| Dominant cost | Fragment index (~9.5 GB) | Fragment index (~9.5 GB) |

The fragment index dominates; this change recovers the spectrum-buffer overhead
entirely.

## Verification

1. **Unit tests**: `python tests/unit/run_tests.py --comet comet.exe` -- all 17
   tests must pass.
2. **PSM parity**: Run on HeLa `.raw` with both the `batch_FI_optimization`
   binary (before this change) and the new binary.  `diff` on the `.txt` outputs
   must show only the header line (run name + timestamp), as verified for the
   prior change.  `tools/qvalue.py --diff` must show zero unique PSMs at 1% and
   5% FDR.
3. **Memory**: Run under `/usr/bin/time -v` and confirm `Maximum resident set
   size` drops by ~300 MB relative to the prior binary on the same HeLa file.
