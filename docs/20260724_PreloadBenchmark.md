# Batch vs. RTS FI_DB throughput: isolating read time from search time

Status: IMPLEMENTED and measured.

## Background

`docs/20260723_ExtendFusedBatchPath.md` closed most of the gap between the fused batch path's
throughput at different `FUSED_FLUSH_MIN_BATCH_SIZE` values, but that investigation's own batch
numbers (up to ~3500-5500 Hz) were dwarfed by a separate RTS benchmark
(`20260420-human-phosho/run2.out/results/index.html`) showing FI_DB real-time search reaching
12,681 Hz at 20 threads on `20240924_Hela_01.raw` (63,488 MS2 spectra). This document investigates
that gap directly.

## First pass: same file, same index, batch's own reported Hz

Running the current batch code (`Comet.exe -D<idx> <raw>`) against the exact file/index/thread
counts RTS was tested at:

| Threads | RTS FI_DB (Hz) | Batch FI_DB (Hz) | Batch / RTS |
|---|---|---|---|
| 1 | 1,356 | 1,154 | 85% |
| 2 | 2,651 | 2,241 | 85% |
| 4 | 5,209 | 3,944 | 76% |
| 8 | 8,991 | 5,474 | 61% |
| 20 | 12,681 | 4,303 | 34% |

Batch not only falls further behind as thread count rises, it gets *slower* in absolute terms past
8 threads.

## Second pass: quantifying read time directly

Temporary instrumentation (since reverted) timed every `PreloadIons()` call in the fused batch
path's single-threaded producer loop:

| Threads | Total wall time | Cumulative raw-read time | Read % of wall time |
|---|---|---|---|
| 1 | 54s | 8.2s | 15% |
| 8 | 11s | 8.55s | 78% |
| 20 | 13s | 11.59s | 89% |

At 8+ threads, raw-file read/parse -- a single-threaded operation in the fused batch path (see
`docs/20260723_ExtendFusedBatchPath.md`'s producer/consumer design) -- is nearly the entire
measured wall time, and that read time itself grows with thread count (8.55s -> 11.59s, +35% from
8 to 20 threads) due to `BoundedSpectrumQueue` mutex contention from more idle/blocking consumers.
RTS's own reported Hz explicitly excludes an equivalent preload phase
(`SearchMS1MS2.cs` reads all spectra into memory first; only the subsequent search loop is timed --
see the `preload_s`/`search_s` columns in `run2.out/results/index.html`), so the two Hz numbers were
never measuring the same thing.

## Third pass: a real preload-then-search-only implementation

To get a number genuinely comparable to RTS's methodology (not just an estimate from subtracting
read time, which is unreliable here since the producer and consumers run concurrently and most
search work happens *during* the read window, not after it), implemented a real preload-then-search
path for the fused batch pipeline.

### Design

- **`CometMassSpecUtils::GetCurrentWorkingSetKB()`** (`CometMassSpecUtils.h`/`.cpp`, new) -- current
  (not peak) resident memory, for before/after deltas. `GetPeakMemory()` (used elsewhere in this
  codebase) is a monotonic process-lifetime high-water mark and can't isolate one phase's cost.
- **`CometPreprocess::ApplySpectrumFilters()`** (`CometPreprocess.h`/`.cpp`) -- the `clearMzRange`/
  `iMinPeaks`/activation-method filter logic, factored out of `FilterAndEnqueueSpectrum` (which now
  calls it) into its own method so the new preload path's read loop can reuse it without a second,
  hand-duplicated copy -- same rationale as every other shared-logic extraction in this
  investigation series.
- **`CometPreprocess::FusedPreloadThenSearch()`** (`CometPreprocess.cpp`, new) -- diagnostic-only,
  same signature as `FusedLoadAndSearchSpectra`. Two phases, each separately timed and
  memory-snapshotted:
  1. **Preload**: the same single-threaded read loop as the normal fused path (same
     `PreloadIons`/`ApplySpectrumFilters` calls, same scan-range logic), but appending into a
     `std::vector<Spectrum>` instead of pushing into `BoundedSpectrumQueue` -- and with no
     `FUSED_FLUSH_MIN_BATCH_SIZE` flush cap, since the whole point is one uninterrupted preload of
     the entire remaining range.
  2. **Search**: the preloaded vector is dispatched across `iNumThreads` workers via a plain
     `std::atomic<size_t>` work-stealing index (`fetch_add`) instead of the queue -- each worker
     calls the same `FusedSearchSpectrum()` used by the normal path, with the same per-slot arenas
     (`sparseArenas`/`resultsArenas`/`pointerArenas`).
- **Opt-in via `COMET_PRELOAD_BENCHMARK` environment variable**, checked in
  `FiStrategy::executeBatch` / `PiStrategy::executeBatch`: unset (default) runs the normal path,
  completely unchanged; set, runs `FusedPreloadThenSearch` instead and prints
  `[PRELOAD]`/`[SEARCH]` lines with per-phase spectra/time/Hz/memory. (From WSL, invoking the
  Windows binary needs `WSLENV=COMET_PRELOAD_BENCHMARK` alongside setting the variable itself, or
  it won't cross the interop boundary into the Windows process.)

### Correctness validation

- Full Linux unit suite (T1-T20, 19/19) passes with the default (non-preload) path, confirming the
  `ApplySpectrumFilters` refactor didn't change default behavior.
- Byte-identical output diff: same FI_DB search (scan range 1-20000 on `20240924_Hela_01.raw`) run
  with `COMET_PRELOAD_BENCHMARK` unset and set. Every PSM data row identical; only the output
  file's embedded `-N` basename text differed.

## Results

Full file (63,488 MS2 spectra), `FUSED_FLUSH_MIN_BATCH_SIZE` at its shipped default (1,000; note
this path ignores it anyway since it never flushes mid-run):

| Threads | RTS search-only (Hz) | Batch preload search-only (Hz) | Batch / RTS | Preload time | Preload memory | Search memory | Total peak |
|---|---|---|---|---|---|---|---|
| 1 | 1,356 | 1,141 | 84% | 6.29s (10,112 Hz) | +1.34GB | +16.26GB | 36.5GB |
| 2 | 2,651 | 2,188 | 83% | 6.01s (10,577 Hz) | +1.34GB | +16.30GB | 36.5GB |
| 4 | 5,209 | 4,214 | 81% | 6.00s (10,594 Hz) | +1.34GB | +16.36GB | 36.6GB |
| 8 | 8,991 | 5,652 | 63% | 5.87s (10,824 Hz) | +1.34GB | +16.48GB | 36.7GB |
| 20 | 12,681 | 6,387 | 50% | 5.89s (10,796 Hz) | +1.35GB | +16.89GB | 37.3GB |

**The hypothesis is confirmed at low-to-moderate thread counts.** Excluding read time closes most
of the gap at 1-4 threads (81-84% of RTS's throughput, up from 76-85% -- comparable, since at those
thread counts read was never the dominant cost in the first place per the second-pass table above)
and dramatically improves the picture at 8-20 threads versus the original raw-Hz comparison (63%/50%
of RTS here, vs. 61%/34% before) -- and unlike the original numbers, search-only Hz no longer
*declines* from 8 to 20 threads. Preload itself is fast and cheap: ~6s and ~1.3-1.4GB regardless of
thread count, for the entire file's spectra.

**But a real gap remains at 8-20 threads even with read fully excluded.** Batch's search-only
throughput barely improves from 8 to 20 threads (5,652 -> 6,387 Hz, +13%) while RTS keeps climbing
strongly over the same range (8,991 -> 12,681 Hz, +41%). Both now use a comparable low-overhead
work-distribution mechanism (batch: atomic work-stealing index; RTS: lock-free
`ConcurrentQueue<int>`), so simple dispatch overhead is an unlikely explanation. The RTS benchmark's
own writeup flags a candidate mechanism for exactly this kind of scaling-past-8-threads pattern:
the fragment-ion index scan reads a large (3.7-billion-entry), shared, read-only structure with
almost no compute per match, making it inherently more exposed to memory-bandwidth contention across
this CPU's P-cores/E-cores than a more compute-heavy workload would be -- consistent with both RTS
and batch sharing the same underlying `SearchFragmentIndex()` call and hitting a similar
architecture-level ceiling once thread count is high enough, independent of which harness is driving
it. This was not directly measured here and would need its own investigation (e.g. hardware
performance counters for memory bandwidth/cache-miss rate) to confirm rather than infer.

**Memory, first look: preloading every spectrum for the file costs relatively little** (~1.3-1.4GB
for 63,563 spectra) -- `Spectrum` objects are lightweight (centroided peaks + metadata). Holding
*every search result* in memory simultaneously (no flush cap, by design of this benchmark) costs far
more: +16.3-16.9GB, ~12x the preload cost, consistent with Phase 1/2's own finding that per-spectrum
`Query` result objects (not the raw spectra) are the dominant per-spectrum memory cost. Total peak
(36.5-37.3GB) is correspondingly far above the normal streaming path's ~20-22GB for the same file,
and grows only modestly with thread count (+2% from 1 to 20 threads) -- unlike the original
`FUSED_FLUSH_MIN_BATCH_SIZE` investigation's batch-size-dependent growth, here every configuration
holds the same total result set, just distributed across a different number of worker threads.

**This first look was confounded by file size -- see the Follow-up section below for the corrected,
apples-to-apples number.** The 63,488-spectrum file used throughout this document is a different,
much smaller file than the 326,696-spectrum file `docs/20260723_ExtendFusedBatchPath.md`'s 35.8GB
streaming number came from. Comparing the two directly (36.7GB vs. 35.8GB, "not much different") was
a mistake made in a follow-up conversation about this document and is corrected next.

## Follow-up: same-file comparison and a real memory-pressure finding

A natural follow-up question: `docs/20260723_ExtendFusedBatchPath.md` reports 35.8GB as Comet's own
peak memory for the streaming path (`FUSED_FLUSH_MIN_BATCH_SIZE=1,000`) on
`20250903_Hela_Ast_Neo_02.raw` (326,696 spectra) -- not far from this document's 36.7GB for
preload-everything at 8 threads. Does that mean holding tens of thousands of extra spectra and
results costs very little?

**No -- that comparison is invalid.** The 36.7GB figure above is from a *different, much smaller*
file (`20240924_Hela_01.raw`, 63,488 spectra -- 5.1x fewer than the 326,696-spectrum file the 35.8GB
figure came from). Re-running `FusedPreloadThenSearch` against the *same* 326,696-spectrum file,
same 8 threads, same params as the 35.8GB streaming baseline gives the real, apples-to-apples
answer:

```
[PRELOAD] 326696 spectra in 95.450s (3422.7 Hz) -- memory 18.93GB -> 37.42GB (+18.50GB)
[SEARCH]  326696 spectra in 246.262s (1326.6 Hz) -- memory 37.42GB -> 4.31GB (-33.11GB)
- done. (7m:54s, 48.0GB)
```

**Comet's own reported peak: 48.0GB vs. streaming's 35.8GB -- a real +12.2GB, 34% increase**, not
"not much different." Preloading everything does *not* come cheap at realistic file sizes; the
earlier-looking similarity was purely an artifact of testing on a 5x-smaller file.

**A second finding, caught by this run: the `[SEARCH]` line's memory reads as a large *decrease*
(37.42GB -> 4.31GB), which is not something `FusedPreloadThenSearch`'s own logic can produce** (it
only ever accumulates `Query` objects during the search phase; nothing frees memory until the
function returns). The only explanation is that the OS trimmed this process's working set under real
memory pressure at some point during the search phase -- unsurprising, since Comet's own peak (48.0GB)
was competing against everything else running on a 63.5GB-RAM machine. `GetCurrentWorkingSetKB()` is
a point-in-time snapshot, not a monotonic counter, so a decrease across two snapshots is a real,
legitimate possibility, not a bug in the measurement approach itself -- but the original `printf`
computed the delta as an unsigned (`size_t`) subtraction, which silently underflowed on this negative
delta into a nonsensical `+17592186044382.88GB`. Fixed by switching both `[PRELOAD]` and `[SEARCH]`
delta calculations to a signed `double` subtraction (`CometPreprocess.cpp`).

Two practical consequences:
1. **The large-file search-only Hz (1,326.6) should not be trusted as a clean measurement** -- it was
   very likely degraded by the paging/working-set-trim event, not a pure reflection of search
   throughput. The 63,488-spectrum results table above, which showed no such signature, is the more
   reliable read on search-only scaling.
2. **This is itself a finding, not just a measurement artifact to route around**: preload-everything
   on a realistically large file pushed this specific machine into genuine memory pressure. That is
   exactly the kind of risk `docs/20260723_ExtendFusedBatchPath.md` flagged as a reason to treat large
   `FUSED_FLUSH_MIN_BATCH_SIZE` values with OOM caution -- preload-everything is the extreme end of
   that same spectrum, and this run shows the caution was warranted.

## What this does and doesn't settle

- **Settled**: batch's Hz looking so much lower than RTS's was substantially a measurement artifact
  at low-to-moderate thread counts -- RTS's number excludes preload time, batch's didn't. The
  underlying search implementation is not the primary story there.
- **Settled**: a second, independent problem exists in the fused batch path specifically at 8+
  threads -- read time itself grows with thread count due to `BoundedSpectrumQueue` contention
  (second-pass table above), which the preload-then-search design sidesteps entirely by not using
  that queue at all.
- **Not settled**: why search-only throughput itself (read fully excluded) still scales much worse
  for batch than RTS past 8 threads. The memory-bandwidth-contention hypothesis is plausible and
  consistent with what RTS's own writeup already flags, but unconfirmed here.
- **Settled (Follow-up section)**: preloading every spectrum is cheap; holding every search result
  until the end is not. On the same 326,696-spectrum file used for the streaming baseline,
  preload-everything's real peak memory is 48.0GB vs. streaming's 35.8GB (+34%), and the run showed
  direct evidence of OS-level memory pressure (a working-set trim event) on a 63.5GB-RAM machine.
  `FUSED_FLUSH_MIN_BATCH_SIZE`'s streaming design stays flat regardless of file size (35.8GB whether
  the file has 63K or 326K spectra); preload-everything's memory cost scales with file size instead,
  with no throughput benefit large enough to justify that trade-off in production.

## Status of `FusedPreloadThenSearch`

Implemented as an opt-in diagnostic (`COMET_PRELOAD_BENCHMARK`), not the default search path, and
not proposed as a replacement for the normal fused streaming path -- it deliberately has no flush
cap and therefore no bound on peak memory for a large file, which is the opposite trade-off from
everything else in this investigation series, and the Follow-up section above shows that trade-off is
a real one, not just theoretical. Kept in the codebase as a reusable tool for future "is this a
read-path problem or a search problem" questions, following the same precedent as `RTS_TIMING` (see
`docs/20260714_rtspostprocessing.md`) -- a documented, code-reviewed, opt-in-only instrumentation
flag rather than a one-off deleted after use, not a recommendation to run it against production-scale
files without the same OOM caution `docs/20260723_ExtendFusedBatchPath.md` already recommends for
large `FUSED_FLUSH_MIN_BATCH_SIZE` values.
