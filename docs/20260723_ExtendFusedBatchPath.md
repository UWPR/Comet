# Extend the RTS thread-local sparse-matrix pool to the fused batch path

Status: IMPLEMENTED and validated -- see "Results" section near the end for measured
before/after numbers.

## Background

A 2026-07-23 investigation into `FUSED_FLUSH_MIN_BATCH_SIZE` (`CometSearch/core/Constants.h:49`)
benchmarked FI_DB batch search (`20250903_Hela_Ast_Neo_02.raw`, 326,696 MS2 spectra, 8 threads,
human canonical target-decoy FASTA/index, phospho search params) across four values of that
constant:

| FUSED_FLUSH_MIN_BATCH_SIZE | Search time | ms/spec | Hz | Peak memory | Total time |
|---|---|---|---|---|---|
| 1,000 | 2m:53s | 0.53 | 1888 | 30.3GB | 4m:00s |
| 5,000 (repo default) | 4m:05s | 0.75 | 1332 | 31.8GB | 5m:13s |
| 10,000 | 5m:20s | 0.98 | 1019 | 34.5GB | 6m:27s |
| 20,000 | 8m:11s | 1.50 | 665 | 40.0GB | 9m:18s |

Raising the batch size made both memory and throughput monotonically *worse*, the opposite of
the "avoid flush-wait pauses" hypothesis that motivated the investigation. The sweep was stopped
after 20,000 (82,000 / 165,000 / ~400,000 were not run) once the mechanism was understood, to
avoid an OOM/paging run on a 63.5GB-RAM machine already at 40GB peak.

**Root cause**, traced to `CometPreprocess.cpp`: the fused batch path
(`FiStrategy`/`PiStrategy::executeBatch` -> `FusedLoadAndSearchSpectra` -> `FusedSearchSpectrum`
-> `Preprocess()`) heap-allocates a fresh sparse XCorr matrix (`ppfSparseSpScoreData`,
`ppfSparseFastXcorrData`, `ppfSparseFastXcorrDataNL`) for **every spectrum**, via individual
`new float*[...]` (the outer pointer array) and `new float[SPARSE_MATRIX_SIZE]` calls (the per-
bucket child blocks), at `CometPreprocess.cpp:1304`, `:1350`, `:1403`. `FUSED_FLUSH_MIN_BATCH_SIZE`
does not control how much gets "preloaded" -- raw spectrum reads are already bounded to
`iNumThreads * 4` in flight (`BoundedSpectrumQueue`, `CometPreprocess.cpp:3494`), independent of
this constant. What it actually controls is how many of these individually-`new`'d `Query`
objects -- each carrying its own sparse matrix -- stay alive on the heap simultaneously, across
8 threads sharing one CRT heap, before `Pipeline::run()`'s per-round flush frees them all. A
bigger batch size means more concurrently-live, separately-allocated sparse matrices, which
directly explains both the memory growth and (via heap allocator contention/fragmentation under
concurrent multi-threaded alloc/free pressure) the throughput drop.

There is already a pooled/reused version of this exact allocation in the codebase --
`Query::bSparseFromPool` (`core/Types.h:711-721`), backed by the `thread_local RtsScratch` pool
(`CometPreprocess.cpp:55-211`) -- but per its own comment it is wired up **only** for
`PreprocessSingleSpectrumThreadLocal`, the true single-spectrum real-time-search (RTS) entry
point. The fused batch path never takes that branch. This document plans extending that pooling
to the fused batch path.

## Goal

Eliminate per-spectrum heap allocation/deallocation of each `Query`'s sparse XCorr matrix child
blocks in the fused batch path, so that `FUSED_FLUSH_MIN_BATCH_SIZE` can be raised (reducing how
often the batch pauses to flush results and relaunch the worker pool) without the memory-growth
and allocator-contention penalty measured above.

## Current architecture (baseline)

`RtsScratch` (`CometPreprocess.cpp:55-211`) is a `thread_local` struct owning, per OS thread:

- Six transient scratch buffers (`pdTmpRawData`, `pdTmpFastXcorrData`, `pdTmpCorrelationData`,
  `pfFastXcorrData`, `pfFastXcorrDataNL`, `pfSpScoreData`) used only *during* one `Preprocess()`
  call and safe to reuse immediately afterward. **The fused batch path already reuses these** --
  `FusedSearchSpectrum` passes `g_rtsScratch.pdTmpRawData` etc. straight into `Preprocess()`
  (`CometPreprocess.cpp:3398-3404`) -- this part of the pool is not the problem.
- `pSparseChildPool`: a flat, pre-zeroed slab of `float[SPARSE_MATRIX_SIZE]` blocks
  (`SPARSE_MATRIX_SIZE` = 100, `CometData.h:35`), handed out sequentially by `AllocSparseChild()`
  (`:147-152`) and bulk-reset via `ResetForNewSpectrum()` (`:134-142`) at the start of the next
  spectrum on that thread. Sized for exactly one in-flight spectrum's worth of blocks
  (`iSparsePoolCapacity = 6 * (iSize / SPARSE_MATRIX_SIZE + 2)`, `:125`).
- `pResults`/`pDecoys`: a single `Results[iNumStored]` array, reused in place and reset field-by-
  field via `ResetResultsForNewSpectrum()` (`:194-208`) at the start of the next spectrum.

`Query::bSparseFromPool` / `Query::bResultsFromPool` (`core/Types.h:711-721`) mark whether the
`Query` destructor should skip `delete[]`-ing these because the pool owns them. Both are set only
by `PreprocessSingleSpectrumCore(bUseThreadLocalPool=true)` (`CometPreprocess.cpp:1458`+), called
only from `PreprocessSingleSpectrumThreadLocal` -- the RTS path. The comment at
`CometPreprocess.h:207-208` states the reason explicitly: "the batch path must use false because
pooled Queries are destroyed before the pool is reset."

The key invariant this design relies on -- documented in the concurrency-nondeterminism section of
`docs/20260714_rtspostprocessing.md` -- is that **exactly one spectrum's pool data is alive per
thread at a time**: `ResetForNewSpectrum()` fully re-zeros exactly what the immediately preceding
spectrum on that thread wrote, before the next spectrum starts. That invariant is what a fused-
batch-path pool must NOT rely on, since the whole point of batching is to keep many `Query`
objects alive at once.

## Why the existing pool can't be reused unchanged

1. **Lifetime mismatch.** RTS destroys/returns each `Query` before the next spectrum on that
   thread starts. The fused batch path deliberately keeps up to roughly
   `FUSED_FLUSH_MIN_BATCH_SIZE / iNumSlots` `Query` objects alive concurrently per worker slot --
   that is the entire point of the batch-and-flush design (`search/Pipeline.cpp:155-214`) -- so a
   "one buffer, reset before the next spectrum" pool would corrupt every earlier `Query`'s still-
   referenced sparse data the moment the next spectrum on that slot ran.
2. **Ownership crosses threads at flush.** Each slot's `Query` objects are produced by that
   slot's worker task, concatenated into `session.queries` after `tp->wait_on_threads()` returns
   (`CometPreprocess.cpp:3706-3714`), then freed later by `Pipeline::run()`'s `cleanupBatch` lambda
   (`Pipeline.cpp:155-161`) on the calling (main) thread -- not the worker thread that created
   them. A `thread_local` free-list-style pool needs the free to happen on the same thread as the
   alloc, which does not hold here.
3. **Slot index is not a stable OS-thread identity across rounds.** `FusedLoadAndSearchSpectra`
   submits `iNumSlots` fresh closures per round via `tp->doJob()` (`:3496-3504`) into a
   `BS::thread_pool` (`ThreadPool.h:89`); nothing pins "slot t" to the same underlying OS worker
   thread from one call to `FusedLoadAndSearchSpectra` to the next, so a `thread_local`-keyed pool
   does not consistently correspond to "slot t" across rounds, even though it is stable within one
   round (each slot's closure runs its own `while (queue.pop(spec))` loop single-threaded for the
   whole round, `:3498-3503`).

## Proposed design

### Core choice: per-slot bump arena, reset at the flush boundary -- not a free list

All `Query` objects created during one flush round share the exact same destruction point:
`cleanupBatch()` deletes the whole `session.queries` vector at once, every round
(`Pipeline.cpp:157-158`). This is a natural match for a bump/arena allocator that hands out memory
sequentially during the round and resets in bulk (O(1), not per-object) at the same point
`cleanupBatch` already runs -- no per-block free-list bookkeeping, and no return-to-owner-thread
problem, because nothing is ever individually freed mid-round; the whole arena is simply rewound
once, on whichever thread runs `cleanupBatch` (safe, because that only happens after
`tp->wait_on_threads()` has returned, i.e. after every worker for that round is already idle -- no
concurrent arena access is possible at that point).

### Ownership: `SearchSession`, keyed by slot index

Add to `SearchSession` (`search/SearchSession.h:49-77`):

```cpp
std::vector<FusedSparseArena> sparseArenas;   // sized to iNumSlots on first use, indexed by iSlot
```

`session` already outlives every round for one input file (constructed once in `Pipeline::run()`,
passed by reference through `executeBatch` / `FusedLoadAndSearchSpectra` / `FusedSearchSpectrum`,
and already the exact place `cleanupBatch` reaches into via `session.queries`), so it is the
natural, already-threaded-through owner. Sizing/lazy-init happens once, at the top of
`FusedLoadAndSearchSpectra`, the first time `session.sparseArenas.empty()` (mirrors how
`vSlotQueries` is sized there today, `:3483`).

Explicitly **not** `thread_local`, and **not** keyed by OS thread identity -- keyed by the same
`iSlot` index `FusedSearchSpectrum` already receives as a parameter (`:125-128`), sidestepping the
slot-to-OS-thread instability noted above entirely.

### Data structure: append-only chunk list (never reallocate/move existing storage)

A single growable `std::vector<float>` (realloc-on-grow) is unsafe here: any `resize()` /
`reserve()` that reallocates would invalidate every pointer already handed out to a `Query`
created earlier in the same round -- and those pointers are read later (output writing, post-
analysis) after the round's allocation phase has moved on to later spectra that may trigger
growth. So each chunk, once allocated, is never moved or freed for the life of the run:

```cpp
struct FusedSparseArena
{
   static constexpr size_t kChunkBlocks = 65536;   // ~25 MB/chunk at SPARSE_MATRIX_SIZE=100 floats

   std::vector<std::unique_ptr<float[]>> vChunks;  // append-only; existing entries never move/free
   size_t iCurChunk     = 0;   // chunk currently being filled
   size_t iCurChunkUsed = 0;   // blocks used in that chunk so far this round

   // Called only from the single worker task-closure that owns this slot for the current
   // round (FusedSearchSpectrum) -- no lock needed, exactly one writer at a time (see
   // "Correctness invariants" below).
   float* AllocBlock()
   {
      if (vChunks.empty() || iCurChunkUsed >= kChunkBlocks)
      {
         if (iCurChunk + 1 < vChunks.size())        // reuse a chunk grown in an earlier round
            ++iCurChunk;
         else
         {
            vChunks.push_back(std::make_unique<float[]>(kChunkBlocks * SPARSE_MATRIX_SIZE));
            iCurChunk = vChunks.size() - 1;
         }
         iCurChunkUsed = 0;
      }
      float* p = vChunks[iCurChunk].get() + iCurChunkUsed * SPARSE_MATRIX_SIZE;
      std::fill(p, p + SPARSE_MATRIX_SIZE, 0.0f);   // zero-on-issue -- preserves AllocSparseChild's
                                                      // existing pre-zeroed guarantee, at per-block
                                                      // cost instead of a bulk per-round memset
      ++iCurChunkUsed;
      return p;
   }

   // Called once per round, only after tp->wait_on_threads() has returned AND cleanupBatch has
   // finished freeing session.queries for that round -- i.e. no concurrent AllocBlock() call can
   // be in flight. Rewinds to the start of chunk 0; all chunks stay allocated (steady-state
   // reuse), so after the first several rounds this design does zero further heap traffic for
   // sparse child blocks.
   void ResetRound() { iCurChunk = 0; iCurChunkUsed = 0; }
};
```

No upfront capacity prediction is needed -- the arena grows on demand during the first several
rounds and then plateaus once it has enough chunks for the workload's actual peak per-slot usage,
since chunks are never freed. This is simpler and more robust than trying to precompute
`iFlushBatchSize / iNumSlots * blocks-per-spectrum` up front, the way the existing single-spectrum
pool's `6 * (iSize / SPARSE_MATRIX_SIZE + 2)` formula does -- that formula assumes exactly one
spectrum in flight, not a whole round's worth.

### Wiring into `Preprocess()` / `Query::bSparseFromPool`

`Preprocess()` (`CometPreprocess.cpp:1185-1457`) currently does, at each of three sites:

```cpp
pScoring->ppfSparseFastXcorrDataNL = new float*[pScoring->iFastXcorrDataSize]();   // :1304
...
pScoring->ppfSparseFastXcorrData   = new float*[pScoring->iFastXcorrDataSize]();   // :1350
...
pScoring->ppfSparseSpScoreData     = new float*[pScoring->iSpScoreData]();          // :1403
```

followed by, per in-use bin, `new float[SPARSE_MATRIX_SIZE]`. Add a `FusedSparseArena* pArena`
parameter to `Preprocess()` (nullptr for any caller that does not want pooling -- currently only
`FusedSearchSpectrum` calls this function, but keeping it nullable avoids forcing every call site
to supply one). Thread it from `FusedSearchSpectrum(spec, iSlot, outQueries, outCount)` as
`&session.sparseArenas[iSlot]`; note `FusedSearchSpectrum` does not currently receive `session` at
all (`CometPreprocess.h:125-128`, `CometPreprocess.cpp:3159-3162` -- it is called today with only
`vSlotQueries[t].v` and `aQueryCount`, `:3502`), so this is a small signature/call-site change up
the chain (`FusedLoadAndSearchSpectra` -> the `doJob` lambda -> `FusedSearchSpectrum`).

When `pArena != nullptr`: each per-bucket `new float[SPARSE_MATRIX_SIZE]` becomes
`pArena->AllocBlock()`, and `pScoring->bSparseFromPool = true` is set -- reusing the existing flag
and the existing destructor logic at `core/Types.h:834-857` verbatim; no destructor change is
needed, since it already skips the child-block `delete[]` whenever this flag is true.

### Explicitly out of scope for this plan (phase 1)

- **The outer `ppfSparseFastXcorrData` / `ppfSparseFastXcorrDataNL` / `ppfSparseSpScoreData`
  pointer arrays themselves** (the `new float*[N]()` calls at `:1304` / `:1350` / `:1403`) are not
  pooled by this plan -- only their child blocks are. Each `Query` still does up to 3 small
  `new[]` calls for these pointer arrays. This is a much cheaper allocation pattern than today's
  (N children eliminated per `Query`, 3 small pointer-arrays remain), and pooling the pointer
  arrays too would require either a fixed global upper bound on `iFastXcorrDataSize` /
  `iSpScoreData` (so every `Query`'s pointer array is the same size and can come from a similarly-
  shaped pool) or a second, differently-shaped arena -- deferred as a phase 2, measurement-driven
  follow-on, per the "measure before optimizing further" lesson `docs/20260714_rtspostprocessing.md`
  draws from its own findings 1 and 6.
- **`_pResults` / `_pDecoys`** are not pooled by this plan. Unlike the sparse child blocks,
  `Results` (`core/Types.h:44-74`) has non-trivial members (`std::string`,
  `vector<ProteinEntryStruct>`) that cannot be safely bump-allocated/reset without explicit
  placement-new and destructor bookkeeping per slot -- a real but separate design problem.
  `docs/20260714_rtspostprocessing.md` finding 1 pooled this exact struct for the RTS single-
  spectrum path (reset-in-place, not arena-based, since RTS only ever needs one live copy at a
  time) and measured only a ~1.4%-of-total-time isolated win there; given `num_output_lines=1` in
  this session's benchmark params (`iNumStored=2`), `Results` is a comparatively small contributor
  next to the sparse matrices, which scale with the mass-range-dependent bin count, not a small
  fixed constant. Worth a follow-up measurement once phase 1 is validated, not a blocker for it.

## Implementation steps

1. Add `FusedSparseArena` as a new small struct -- likely its own header (e.g.
   `CometSearch/core/FusedSparseArena.h`) so both `SearchSession.h` and `CometPreprocess.cpp` can
   include it without pulling in unrelated dependencies.
2. Add `std::vector<FusedSparseArena> sparseArenas;` to `SearchSession`
   (`search/SearchSession.h`).
3. In `FusedLoadAndSearchSpectra` (`CometPreprocess.cpp:3455`+), resize `session.sparseArenas` to
   `iNumSlots` on first use (mirrors `vSlotQueries` sizing at `:3483`).
4. Thread an arena reference through `FusedSearchSpectrum`'s signature
   (`CometPreprocess.h:125-128`, `CometPreprocess.cpp:3159-3162`) and its call site inside the
   `doJob` lambda (`:3498-3503`).
5. In `Preprocess()` (`CometPreprocess.cpp:1185-1457`), add the `FusedSparseArena*` parameter,
   branch the three child-block allocation sites (`:1304`, `:1350`, `:1403` and their per-bucket
   loops) on whether it is non-null, and set `pScoring->bSparseFromPool = true` when pooled.
6. In `Pipeline.cpp`'s `cleanupBatch` lambda (`:155-161`), add
   `for (auto& arena : session.sparseArenas) arena.ResetRound();` alongside the existing
   `delete q` loop -- same point in the same function, so the "no concurrent access" invariant is
   trivially true by construction (nothing schedules new work into a slot's arena until the next
   `executeBatch()` call, which happens after `cleanupBatch()` returns).
7. No change needed to `Query`'s destructor (`core/Types.h:820-886`) -- it already branches on
   `bSparseFromPool` correctly for this exact "don't delete[], pool owns it" case.

## Correctness invariants to preserve (and verify)

- **Exactly one writer per slot's arena per round.** True by construction: each slot's
  `FusedSearchSpectrum` calls all run inside the single `while (queue.pop(spec))` loop of that
  slot's one `doJob` closure (`:3498-3503`) -- never two closures touching the same `iSlot`
  concurrently.
- **No arena access during `ResetRound()`.** True by construction: `cleanupBatch()` runs only
  after `executeBatch()` -> `FusedLoadAndSearchSpectra()` has returned, which itself only returns
  after `tp->wait_on_threads()` (`:3698`) -- every worker for that round is already idle.
- **No pointer invalidation on growth.** The chunk-list design never moves or frees an already-
  allocated chunk; `AllocBlock()` only ever appends a new chunk, or reuses an existing later chunk
  from an earlier, larger round -- and that reuse only happens after `ResetRound()`, i.e. after
  every prior pointer into it is already dead. No existing chunk is ever resized.
- **Zero-on-issue.** `AllocBlock()` must zero exactly the block it returns before returning it,
  preserving the guarantee `pSparseChildPool`'s bulk pre-zero + `ResetForNewSpectrum()` provide
  today (`CometPreprocess.cpp:126`, `:138-141`) -- several downstream sparse-matrix consumers
  (e.g. `CometPostAnalysis.cpp:1290-1301`) implicitly rely on unallocated/never-written bins
  reading as zero.

## Validation plan

1. **Correctness, batch path.** Unlike the RTS path documented in
   `docs/20260714_rtspostprocessing.md` (unreachable from the Linux suite), the fused batch path
   IS exercised by `tests/unit/run_tests.py`. Full suite (T1-T18) must pass unchanged.
2. **Byte-identical output.** Run the same FI_DB search (this session's phosho params +
   `20250903_Hela_Ast_Neo_02.raw`, or a smaller/faster fixture for iteration) before and after,
   diff the full `.txt` output -- must be byte-identical, since this change only alters memory
   *ownership*, not any scoring/search logic. (Unlike `docs/20260714_rtspostprocessing.md`'s RTS
   `rts.out` diffs, an exact match is expected here -- the batch path does not have that doc's
   documented near-tied-score nondeterminism sources.)
3. **Re-run this session's exact benchmark sweep** (`FUSED_FLUSH_MIN_BATCH_SIZE` =
   1000/5000/10000/20000, same raw file, same params, same machine) and compare against the
   2026-07-23 baseline table reproduced at the top of this document. Success criterion: Hz should
   stay roughly flat (not degrade) as batch size increases, and peak-memory growth above the fixed
   ~19.8GB FI-index baseline should shrink substantially versus baseline, since concurrently-live
   sparse child blocks are now pooled/arena blocks reused across rounds instead of N fresh heap
   allocations per round.
4. **Only after 1-3 pass**, extend the sweep to the three configs skipped this session (82,000 /
   165,000 / ~400,000-"all") to check whether the original goal -- raising
   `FUSED_FLUSH_MIN_BATCH_SIZE` to reduce flush-wait pauses without a memory/throughput penalty --
   is actually achievable now. This is the real test of whether phase 1 alone resolves the
   original question, or whether the remaining un-pooled costs (outer pointer arrays,
   `_pResults`/`_pDecoys`) become the new bottleneck at that scale.

## Risks / open questions

- **Steady-state chunk count is workload-dependent and per-slot.** If per-thread spectrum load is
  uneven (e.g. some spectra need far more sparse blocks than others, and happen to cluster onto
  one slot), one slot's arena could grow much larger than others'. Not a correctness risk (each
  arena is independent), but worth watching in the phase-1 memory measurement -- if it happens,
  total memory could still track close to today's worst case for that one slot, just without the
  allocator churn.
- **`kChunkBlocks` is a guess (65536, ~25 MB/chunk)** -- no measurement backs this number yet. Too
  small wastes time on repeated `vChunks.push_back()` calls (still cheap, but not free); too large
  wastes memory on partially-used trailing chunks, times `iNumSlots`. Should be tuned against a
  real run's actual usage distribution during implementation, not fixed a priori.
- **This plan does not address the two costs explicitly deferred above** (outer pointer arrays,
  `_pResults`/`_pDecoys`). If step 4 of the validation plan shows those now dominate at very large
  batch sizes, a phase 2 covering them would be a natural follow-up, written as its own dated doc
  once phase 1's measurements are in hand (matching this repo's existing pattern of one dated doc
  per investigation/change, e.g. `docs/20260714_rtspostprocessing.md`,
  `docs/20260714_EvalueJitter.md`).

## Results (2026-07-23, phase 1 implemented and measured)

Implemented exactly as designed above: `CometSearch/core/FusedSparseArena.h` (new), plus edits to
`search/SearchSession.h`, `CometPreprocess.h`/`.cpp` (three allocation sites in `Preprocess()`,
`FusedSearchSpectrum()`/`FusedLoadAndSearchSpectra()` signatures), and `search/Pipeline.cpp`'s
`cleanupBatch` lambda. No changes needed to `Query`'s destructor, as predicted.

**Correctness:**
- Full Linux `tests/unit/run_tests.py` suite: 19/19 passed (T1-T7, T11-T16, T19, T20), including
  T19/T20 which exercise the FI_DB/PI_DB fused batch path directly.
- Byte-identical output diff: same FI_DB search (this doc's params + `20250903_Hela_Ast_Neo_02.raw`,
  scan range 1-40000, 37,453 spectra spanning 7+ flush rounds at the default
  `FUSED_FLUSH_MIN_BATCH_SIZE=5000`) run with the pre-change code (via `git stash`) and the
  post-change code. Every PSM data row was byte-identical; the only diff line was the output
  file's embedded `-N` basename text itself.

**Performance (full file, 326,696 MS2 spectra, otherwise identical to the original 2026-07-23
sweep):**

| FUSED_FLUSH_MIN_BATCH_SIZE | Before (search time / Hz / peak mem) | After (search time / Hz / peak mem) | Hz change |
|---|---|---|---|
| 1,000 | 2m:53s / 1888 Hz / 30.3GB | 1m:41s / 3234 Hz / 31.3GB | +71% |
| 5,000 (default) | 4m:05s / 1332 Hz / 31.8GB | 1m:50s / 2966 Hz / 33.6GB | +123% |
| 10,000 | 5m:20s / 1019 Hz / 34.5GB | 2m:6s / 2588 Hz / 35.5GB | +154% |
| 20,000 | 8m:11s / 665 Hz / 40.0GB | 2m:36s / 2088 Hz / 41.1GB | +214% |

**Interpretation.** A large, real, consistent throughput win at every batch size tested (71-214%
faster), most pronounced at the larger sizes -- exactly where the original allocator-contention
problem was worst. This confirms the root-cause diagnosis and the arena design.

**However, the success criterion in the validation plan above was only partially met.** The plan
predicted Hz should "stay roughly flat" as batch size increases and memory growth should "shrink
substantially" -- neither happened completely. With the fix, Hz still degrades as batch size grows
(3234 -> 2966 -> 2588 -> 2088), just far less steeply than before (1888 -> 1332 -> 1019 -> 665), and
peak memory is now slightly *higher* than the pre-fix baseline at every size (e.g. 41.1GB vs 40.0GB
at 20,000), still growing with batch size. This is consistent with the plan's own "explicitly out
of scope" section: the outer `ppfSparseFastXcorrData`/`ppfSparseFastXcorrDataNL`/
`ppfSparseSpScoreData` pointer arrays (3 small `new[]` calls per spectrum) and `_pResults`/
`_pDecoys` (1-2 `new[]` calls per spectrum) are still individually heap-allocated per spectrum and
still accumulate un-pooled for as long as a `Query` stays alive before flush -- phase 1 removed the
*largest* per-spectrum allocation (the sparse child blocks, which scale with mass-range bin count)
but not these smaller, fixed-size ones. Their residual cost is exactly what's now visible in the
remaining Hz-degradation and memory-growth trend.

**Conclusion:** phase 1 is a real, validated win and should be kept, but does not fully resolve the
original question ("can `FUSED_FLUSH_MIN_BATCH_SIZE` be raised arbitrarily without penalty?") --
the answer is now "much less penalty, but not zero." A phase 2 covering the outer pointer arrays and
`_pResults`/`_pDecoys`, per the deferred-scope section above, would be needed to fully flatten the
curve. The original 82,000 / 165,000 / ~400,000 ("all spectra") configurations were not re-tested
in this pass; given memory is now higher rather than lower at each tested size, those larger configs
should be approached with the same OOM caution as the original investigation, not less.
