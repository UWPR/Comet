# Mutex Serialization in SearchMemoryPool -- Problem and Optimization Plan

## Context

`SearchMemoryPool` (`CometSearch/threading/SearchMemoryPool.h/.cpp`) hands out
duplicate-fragment scratch-buffer slots to search threads. Every call into
`CometSearch::AcquirePoolSlot()` / `releaseSlot()` takes the pool's single
`std::mutex` -- first to scan/pop a free slot, then again to push it back. A prior
benchmarking pass (see Appendix) replaced the original O(n) linear scan of a
`bool[]` array with an O(1) free-list stack, confirming the scan itself was not
the bottleneck: total throughput across all threads stayed flat at roughly
3.8-5M ops/sec from 8 threads up to 512 threads, regardless of slot count. Flat
throughput under increasing thread count, on an operation with no inherent
ordering requirement, is the signature of a single global serialization point --
in this case, the pool's one mutex. This document describes that problem in more
detail and lays out a measurement-gated plan for removing the serialization from
the hottest call site.

## The problem

`acquireSlot()`/`releaseSlot()` (`threading/SearchMemoryPool.cpp`) take the same
`std::mutex _mutex` on every call:

```cpp
int SearchMemoryPool::acquireSlot()
{
   std::unique_lock<std::mutex> lock(_mutex);
   bool found = _cv.wait_for(lock, std::chrono::seconds(240), [this]() { return !_freeSlots.empty(); });
   if (!found) return -1;
   int slot = _freeSlots.back();
   _freeSlots.pop_back();
   return slot;
}

void SearchMemoryPool::releaseSlot(int slot)
{
   { std::lock_guard<std::mutex> lk(_mutex); _freeSlots.push_back(slot); }
   _cv.notify_one();
}
```

Conceptually, "give me any one free slot" and "give this slot back" do not need a
total order across all callers -- any free slot will do, and releases don't need
to be sequenced relative to other releases. But the current design forces every
acquire and every release through one mutex, so N threads doing this concurrently
serialize to roughly the same total throughput as 1 thread doing it N times. The
benchmark in the Appendix confirms this directly: per-operation latency stayed in
the 200-310 ns range across all tested slot/thread counts (8 through 512), with
*total* throughput never scaling up with thread count the way a genuinely
parallel operation would.

### Where this is actually hot

Not every caller of `AcquirePoolSlot()` is on a tight per-spectrum loop. Current
call sites, in descending order of call frequency:

| Call site | File:line | Frequency | Notes |
|---|---|---|---|
| `CometSearch::RunSearch(Query* pQuery)` | `CometSearch.cpp:110` (acquire at lines 122, 164) | **Once per spectrum, per RTS call** | This is `DoSingleSpectrumSearchMultiResults`'s search path -- the RTS thread-local entry point that the project already benchmarks for per-spectrum Hz. Every concurrent RTS caller takes the global mutex twice (acquire + release) per spectrum. |
| `CometSearch::RunSearch(int,int,ThreadPool*,vector<Query*>&)` FI_DB branch | `CometSearch.cpp:218` | Once per query, per batch | Only reached for the legacy (non-fused) batch path, i.e. when Mango or a spectral-library search forces `FiStrategy::executeBatch()` away from the fused path (`search/FiStrategy.cpp:129-131`). |
| `CometSearch::SearchThreadProc` | `CometSearch.cpp:1220` | Once per protein-search job dispatch | Classic FASTA three-sweep search. Per-job, not per-spectrum; each job is comparatively expensive (protein-by-protein FASTA scoring), so lock overhead is a much smaller fraction of total work here. |

### The pattern that already avoids this problem

The fused batch FI_DB path (`CometPreprocess::FusedLoadAndSearchSpectra`,
`CometPreprocess.cpp:3344-3387`) does **not** call `AcquirePoolSlot()` at all. It
launches exactly `iNumThreads` long-lived consumer jobs up front, each one closed
over a fixed slot index `t`:

```cpp
const int iNumSlots = g_staticParams.options.iNumThreads;
std::vector<SlotVec> vSlotQueries(iNumSlots);
std::atomic<size_t> aQueryCount{0};
BoundedSpectrumQueue queue(static_cast<size_t>(iNumSlots) * 4);

for (int t = 0; t < iNumSlots; ++t)
{
   tp->doJob([&queue, t, &vSlotQueries, &aQueryCount]()
   {
      Spectrum spec;
      while (queue.pop(spec))
         FusedSearchSpectrum(std::move(spec), t, vSlotQueries[t].v, aQueryCount);   // pre-assigned slot, no lock
   });
}
```

(Snippet updated 2026-07-10 to match the current signature -- a later change
gave each worker its own result vector plus a shared atomic counter instead of
capturing `session` directly, to remove a separate `queriesMutex` from this
same hot path. The pre-assigned-slot mechanism this section is about --
`RunSearch(Query*, int iSlot)`, next paragraph -- is unchanged.)

Each worker keeps its slot for the worker's entire lifetime instead of
acquiring/releasing it per spectrum. `RunSearch(Query*, int iSlot)`
(`CometSearch.cpp:186`) exists specifically to take this pre-assigned slot,
bypassing `AcquirePoolSlot()` entirely. This is proven, already-shipping code --
it is the model for Phase 1 below, not a new design.

## Why this matters (and the honest caveat)

The RTS path's entire purpose is per-spectrum throughput (Hz) under concurrent
load from multiple C# `Task` threads. Every `DoSingleSpectrumSearchMultiResults`
call pays for two lock/unlock pairs on a global mutex shared with every other
concurrent caller, even when no actual contention exists.

**Caveat:** the Appendix benchmark measures the synchronization primitive in
isolation, with a near-zero critical-section hold time (touch one byte). Real
search work (`SearchFragmentIndex` / `SearchPeptideIndex`) holds the slot for
however long the actual XCorr/peptide-index search takes -- almost certainly
microseconds to low milliseconds, not nanoseconds. If that real hold time
dominates, the relative cost of the lock itself may be a small fraction of total
per-spectrum latency, and this entire effort would not show up in real Hz
numbers. **This needs to be measured in situ before committing to Phase 2.**
Phase 0 below exists specifically to answer that question first.

## Proposed plan

### Phase 0 -- Measure in situ before optimizing further

Use the existing `RTS_TIMING` build flag (see the `comet-build` skill;
`CometSearch.vcxproj` Release config, or `RTS_TIMING_OFF`/`RTS_TIMING`
preprocessor define) to instrument real per-spectrum timing inside
`DoSingleSpectrumSearchMultiResults`, and drive it with a synthetic
high-concurrency load (many concurrent RTS calls, thread count well above
`iNumThreads` to force the pool into contention). Compare:

- Wall time spent inside `AcquirePoolSlot()`/`releaseSlot()` vs. total per-spectrum
  wall time.
- futex wait counts / `perf lock` contention stats under sustained concurrent load,
  if available on the target platform.

**Only proceed to Phase 1/2 if this shows a non-negligible fraction of
per-spectrum latency** (a reasonable bar: >5-10%), or direct evidence of lock
contention at realistic concurrent RTS thread counts. If the real hold time of
the search work dominates, stop here -- the isolated microbenchmark result does
not by itself justify the added code complexity.

### Phase 1 -- Extend the existing pre-assigned-slot pattern to RTS (low risk)

The fused batch path can pre-assign slots because it owns a fixed-size worker
pool it creates itself. RTS callers arrive on whatever thread the .NET `Task`
scheduler happens to run them on, so there is no equivalent fixed "worker index"
threaded through `CometWrapper` today.

Proposed mechanism: **thread-local lazy slot pinning.** On the first call into
`RunSearch(Query*)` from a given OS thread, claim a slot once via the existing
(mutex-protected) `acquireSlot()` and cache it in a `thread_local int`. Every
subsequent call from that same OS thread reuses the cached slot directly --
no further lock operations for the rest of that thread's lifetime. If the pool
is already fully claimed by other threads when a new thread needs a permanent
slot, fall back to today's per-call dynamic acquire/release for that thread
(graceful degradation, not a hard failure).

- **Implementation surface:** a thin wrapper at the two `AcquirePoolSlot()` call
  sites inside `RunSearch(Query* pQuery)` (`CometSearch.cpp:122,164`). No change
  to `SearchMemoryPool` itself.
- **Risk:** low. Reuses the existing tested mutex/free-list code for the
  one-time claim and the overflow fallback; adds only a `thread_local` cache.
- **Open question to confirm, not assume:** this only pays off if the number of
  distinct OS threads that ever call into RTS search stays bounded near
  `iNumThreads`. `RealtimeSearch.cs`'s `Parallel.ForEach` over the scan queue
  does not currently pin a fixed degree of parallelism matching `iNumThreads` --
  .NET's `ThreadPool` can grow under sustained load. Confirm actual concurrent
  thread counts in production-like load before relying on this assumption; if
  unbounded, either cap `Parallel.ForEach`'s `MaxDegreeOfParallelism` to
  `iNumThreads` on the C# side, or size the pool to comfortably exceed observed
  peak concurrency.

### Phase 2 -- Lock-free fast path (only if Phase 0 justifies it)

If Phase 0 shows the mutex matters and Phase 1's thread-affinity assumption
doesn't hold for some caller, replace the mutex+condition_variable+vector design
with a lock-free atomic bitmask:

- `std::atomic<uint64_t>` for pools up to 64 slots (two words for up to 128;
  `iNumThreads` realistically never exceeds this range).
- `acquireSlot()`: CAS loop -- find the lowest set bit (a free slot), atomically
  clear it. O(1), wait-free in the common case.
- `releaseSlot()`: atomic fetch-or to set the bit back. O(1), wait-free.
- Keep the existing mutex+condition_variable only as the rare fallback path for
  "pool fully exhausted, must block" -- its only remaining job, instead of being
  on every call.

This is more general than Phase 1 (no assumption about caller thread identity or
lifetime) but carries more implementation and review risk: lock-free bitmask
code is straightforward to write but easy to get subtly wrong (memory ordering,
the exhausted-pool fallback path), and is harder to verify by inspection than a
`thread_local` cache. Pursue only if Phase 1 doesn't fully close the gap Phase 0
identified.

### Phase 3 -- Re-benchmark and re-measure after each phase

- Re-run the standalone `SearchMemoryPool` benchmark (Appendix) after each phase
  to confirm the synchronization primitive itself improved.
- Re-run the Phase 0 in-situ `RTS_TIMING` measurement after each phase to confirm
  the improvement is visible in real per-spectrum Hz numbers, not just the
  isolated microbenchmark. A microbenchmark win that doesn't move real Hz numbers
  is not worth the added code complexity or review risk -- don't merge Phase 2
  on the strength of the isolated benchmark alone.

## Other shared mutexes considered and ruled out of scope (for now)

`docs/GlobalVariables.md` lists several other process-wide mutexes:
`g_pvDBIndexMutex`, `g_preprocessMemoryPoolMutex`, `g_ms1AlignerMutex`,
`g_pvQueryMutex`. None of these sit on the per-spectrum hot path the way
`SearchMemoryPool`'s mutex does -- they guard one-time initialization work (DB
index reads, spectral-library loading) or comparatively low-frequency updates
(MS1 RT alignment history, once per MS1 RTS call on a lower-volume path). Revisit
only if a Phase-0-style measurement on one of those specific paths shows an
actual problem; don't speculatively rewrite them without evidence -- that was
exactly the mistake this document is trying to avoid by leading with Phase 0.

## Appendix: benchmark methodology

Standalone harness, compiled directly against the real
`threading/SearchMemoryPool.cpp` and `Threading.cpp` (no need to link the rest of
Comet's dependency tree -- `logout`/`logerr` are macros over `cout`/`cerr`, and
`CometStatus` is fully defined inline in its header):

```cpp
// bench_pool.cpp
#include "threading/SearchMemoryPool.h"
#include "CometStatus.h"
#include <atomic>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <thread>
#include <vector>

CometStatus g_cometStatus;   // extern required by SearchMemoryPool.cpp's bad_alloc path

int main(int argc, char** argv)
{
   int nSlots   = argc > 1 ? atoi(argv[1]) : 8;
   int nThreads = argc > 2 ? atoi(argv[2]) : 32;
   long nIters  = argc > 3 ? atol(argv[3]) : 200000;

   SearchMemoryPool pool;
   if (!pool.allocate(nSlots, 16)) { fprintf(stderr, "allocate failed\n"); return 1; }

   std::atomic<long> totalOps{0};
   std::vector<std::thread> threads;
   auto tStart = std::chrono::steady_clock::now();

   for (int t = 0; t < nThreads; ++t)
   {
      threads.emplace_back([&pool, nIters, &totalOps]() {
         for (long i = 0; i < nIters; ++i)
         {
            int slot = pool.acquireSlot();
            if (slot < 0) continue;
            SearchMemoryPoolSlotGuard guard{pool, slot};
            volatile bool* p = pool.duplFragmentArr(slot);
            p[0] = !p[0];                       // simulate minimal real work
            totalOps.fetch_add(1, std::memory_order_relaxed);
         }
      });
   }
   for (auto& th : threads) th.join();

   double sec = std::chrono::duration<double>(std::chrono::steady_clock::now() - tStart).count();
   long ops = totalOps.load();
   printf("slots=%d threads=%d total_ops=%ld time=%.4fs ops/sec=%.0f avg_latency_ns=%.1f\n",
          nSlots, nThreads, ops, sec, ops / sec, (sec * 1e9) / ops);

   pool.deallocate();
   return 0;
}
```

Compile from `CometSearch/`:

```bash
g++ -O3 -std=c++20 -fpermissive -I. -I../MSToolkit/include \
    -I../MSToolkit/extern/expat-2.2.9/lib -I../MSToolkit/extern/zlib-1.2.11 \
    -D_LARGEFILE_SOURCE -D_FILE_OFFSET_BITS=64 -D__LINUX__ -D_NOSQLITE \
    bench_pool.cpp threading/SearchMemoryPool.cpp Threading.cpp -lpthread -o bench_pool
./bench_pool <nSlots> <nThreads> <itersPerThread>
```

Results from the prior benchmarking pass (O(n) linear-scan implementation vs. the
O(1) free-list that replaced it -- both still mutex-bound, which is the point of
this document):

| Slots/Threads | O(n) scan ops/sec | O(1) free-list ops/sec | Delta |
|---|---|---|---|
| 8/8 | 3.81M | 4.00M | +5% |
| 8/32 | 4.28M | 4.05M | -5% (noise) |
| 16/64 | 3.91M | 3.87M | -1% (noise) |
| 32/128 | 3.89M | 4.11M | +6% |
| 256/256 | 3.81M | 5.05M | +33% |
| 512/512 | 3.26M | 4.39M | +35% |

At realistic pool sizes (`iNumThreads`, typically <= 64), throughput is flat
across both implementations within noise -- confirming the mutex, not the scan,
sets the ceiling. The free-list version only pulls ahead once slot counts grow
well past any realistic `iNumThreads` value, which is informative for
understanding *why* the scan wasn't the bottleneck but does not by itself
indicate a production-relevant win. Phase 0 of this plan is how to find out
whether removing the mutex itself would be.
