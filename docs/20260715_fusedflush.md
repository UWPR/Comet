# PI_DB batch memory: missing spectrum_batch_size clamp + decoupled fused flush

Status: **IMPLEMENTED.** Two independent fixes: (1) PI_DB batch search was missing the
`spectrum_batch_size` clamp FI_DB already had when searching an existing `.idx` file, leaving it
effectively unbounded by default; (2) the fused FI_DB/PI_DB loader's flush cadence (how many
finished spectra accumulate before results are written and freed) was tied to the legacy
`iSpectrumBatchSize`/`FRAGINDEX_MAX_BATCHSIZE` concept, which doesn't fit the fused streaming
model. Replaced with an independent flush threshold, **scaled by thread count**
(`max(FUSED_FLUSH_MIN_BATCH_SIZE, iNumThreads * FUSED_FLUSH_PER_THREAD)`, `FUSED_FLUSH_PER_THREAD
= 50`) -- an initial flat constant (100) was tried first and found to under-flush on high-thread-
count machines (see "Correction" below). Together these took a real batch PI_DB search from
**7.4GB to ~1.2-1.9GB** peak memory (thread-count-dependent) on the same 20,097-spectrum real
dataset, with byte-identical output, confirmed flat with respect to file size up to 51,816
spectra, and confirmed to reach ~99%+ of peak throughput at both 8 and 20 threads.

## Background

Follow-up to the PI_DB memory investigation in the prior session (RTS `Comet.exe -D...` search
reporting 11.2GB peak working set) and the fused-producer-consumer conversion of PI_DB batch
search to match FI_DB's architecture (see conversation history; no separate doc was written for
that step). That conversion made PI_DB batch search use
`CometPreprocess::FusedLoadAndSearchSpectra()` -- the same bounded producer/consumer pipeline
FI_DB already used -- instead of the legacy load-all-then-search-all sweep. An empirical
before/after test at the time showed **no memory improvement** (5.6GB vs 5.7GB) despite matching
FI_DB's architecture, which prompted this investigation into why.

## Finding 1: PI_DB never had FI_DB's `spectrum_batch_size` clamp

`CometSearchManager.cpp` (~line 1490-1530) detects `.idx` type from the file header and sets
`iDbType`. The FI_DB branch (`"Comet fragment ion index"` header) has always clamped
`iSpectrumBatchSize` to `FRAGINDEX_MAX_BATCHSIZE` (1000) whenever it's unset (0) or too large:

```cpp
else if (!strncmp(szTmp, "Comet fragment ion index", 24))
{
    g_staticParams.iDbType = DbType::FI_DB;
   if (g_staticParams.options.iSpectrumBatchSize > FRAGINDEX_MAX_BATCHSIZE || g_staticParams.options.iSpectrumBatchSize == 0)
      g_staticParams.options.iSpectrumBatchSize = FRAGINDEX_MAX_BATCHSIZE;
}
```

The PI_DB branch (`"Comet peptide index"` header, just above it) set `iDbType = DbType::PI_DB`
and **never applied this clamp** -- grep confirms `FRAGINDEX_MAX_BATCHSIZE` was referenced at
exactly two sites in the whole codebase, both inside the FI_DB branch (this one and the
"index doesn't exist yet, will build FI_DB" branch, which is FI_DB-only by construction).

Effect: `iSpectrumBatchSize` defaults to 0 (`core/Params.h:394`), and `CheckExit()`
(`CometPreprocess.cpp:2214-2216`) treats 0 as "no batch-size limit at all" --
`!bIgnoreSpectrumBatchSize && (iSpectrumBatchSize != 0) && (...)`. So any PI_DB batch search that
didn't explicitly set `spectrum_batch_size` in `comet.params` -- the common case -- held **every**
spectrum's fully-scored `Query` object (each carrying per-spectrum sparse XCorr matrices, see the
prior session's memory breakdown) in memory simultaneously for the entire file before writing
anything out.

Empirical confirmation: real batch PI_DB search, 20,097 real Hela spectra against an ~8.9M-peptide
PI_DB index, `spectrum_batch_size` unset: **7.4GB peak** (`comet_after.exe`, the fused-but-not-yet-
decoupled build from the prior session) -- higher than the 5.6-5.7GB seen with `spectrum_batch_size
= 15000` explicitly set, confirming the unset/default case was the worst one, and the one most
users would actually hit.

Fix: added the identical clamp to the PI_DB branch (`CometSearchManager.cpp:1518-1529`).

## Finding 2: the fused loader shouldn't use the legacy batch-size concept at all

Even with finding 1 fixed, `spectrum_batch_size`/`FRAGINDEX_MAX_BATCHSIZE=1000` is a legacy
concept: it was designed for the load-all-then-search-all sweep (`executeBatchLegacy`), where
"batch size" is the necessary unit of work. The fused loader
(`CometPreprocess::FusedLoadAndSearchSpectra`) already bounds *spectra read but not yet searched*
independently, via `BoundedSpectrumQueue` (depth `4 * iNumThreads`, `CometPreprocess.cpp:3489`) --
a small, thread-count-scaled gate that has nothing to do with `iSpectrumBatchSize`. But the fused
loader's *read loop exit condition* was still wired to the same `iSpectrumBatchSize` value via
`CheckExit()`, meaning it would still accumulate up to that many *finished* `Query` objects before
returning control to `Pipeline::run()` for a write+free cycle. Two consequences:

- Setting `spectrum_batch_size` small (e.g. 100) to control memory works, but it's an
  awkward, non-obvious knob shared with an unrelated legacy path, and its 1000-spectra default
  ceiling is still much larger than necessary for the fused case.
- The `bUseReadahead` fast path (EntireFile analysis + mzXML + `iSpectrumBatchSize == 0`,
  `CometPreprocess.cpp:3510-3513`) had **no exit condition at all** tied to spectra count -- only
  EOF/error/cancel -- so a plain "search this whole file" invocation (the most common real-world
  usage pattern) always read and held the entire file's `Query` pool before any write, regardless
  of `spectrum_batch_size`.

### Fix: a decoupled, thread-scaled flush threshold, independent of `iSpectrumBatchSize`

Added a new flush threshold (`core/Constants.h`), computed once per `FusedLoadAndSearchSpectra`
call as `iFlushBatchSize = max(FUSED_FLUSH_MIN_BATCH_SIZE, iNumThreads * FUSED_FLUSH_PER_THREAD)`
(`FUSED_FLUSH_PER_THREAD = 50`, `FUSED_FLUSH_MIN_BATCH_SIZE = 100`), used only by
`FusedLoadAndSearchSpectra`'s two read loops (readahead and synchronous), replacing the
`iSpectrumBatchSize`-based exit check. See "Correction" below for why this is thread-scaled rather
than a flat constant -- that was the first design tried, and it under-flushed on high-thread-count
machines.

- Synchronous branch (`CometPreprocess.cpp` ~3663-3677): `CheckExit()` is now called with
  `bIgnoreSpectrumBatchSize=true`, and a separate check against `iFlushBatchSize` follows it.
- Readahead branch (~3528-3572): added the same check inside the `while (pRA->next(...))` loop.
  Breaking out here does **not** mean end-of-file -- a new local `bFlushLimitHit` flag distinguishes
  "paused for a flush" from "hit real EOF" so `_bDoneProcessingAllSpectra` is only set in the latter
  case. `pRA->requestStopAndJoin()` (already called unconditionally after the if/else block) cleanly
  tears down that call's readahead thread; the next `executeBatch()` call constructs a fresh
  `SpectrumReadahead` and resumes exactly where `mstReader` left off. This preserves the existing
  "one call, one thread lifetime" invariant the readahead code's comments emphasize -- it's called
  more often, not restructured.

`iSpectrumBatchSize`/`FRAGINDEX_MAX_BATCHSIZE` remain unchanged for the legacy path (still used by
`executeBatchLegacy`, which `PiStrategy`/`FiStrategy` fall back to for Mango/speclib runs -- finding
1's clamp fix still matters there).

## Modeling/testing writer I/O batching cost

Before picking a flush size, checked whether more frequent `IResultWriter::write()` calls would
hurt I/O throughput. Read `TxtWriter`, `MzIdentMlWriter`, `PepXmlWriter`, `PercolatorWriter`,
`SqtWriter`: none call `fflush()` inside `write()` (only `TxtWriter::open()` flushes, once, after
writing the header), none do batch-count-dependent work (no O(batch^2) patterns, no XML-tree
rebuild per call -- `MzIdentMlWriter` appends to a temp file per `write()` call and only assembles
the final XML once at `close()`). `write()`'s only argument dependency is `ctx.pQueries` (whatever
this flush's `Query*` vector holds) -- total rows formatted and written is identical regardless of
how many `write()` calls it takes to cover them. Conclusion: writer I/O itself should be
insensitive to flush granularity; the empirical sweep below confirms this (see "no benchmark
noise from writer batching" note).

## Empirical sweep (initial, 8 threads -- see "Correction" for why this alone was insufficient)

Real batch PI_DB search, 20,097 real Hela spectra (`20250520_Hela_60min_06.mzXML`, `scan_range 1
25000`), 8 threads, against a freshly-built ~8.9M-peptide PI_DB index (`human.small.fasta`,
no-enzyme, length 8-13), `spectrum_batch_size` unset. Six full clean rebuilds (`make cclean &&
make`) with the flush threshold (then still a flat constant) swept 20/50/100/500/1000/2000;
`/usr/bin/time -v` for peak RSS; output diffed byte-for-byte against a known-good legacy-path
baseline.

| flush size | peak RSS | throughput | output |
|---|---|---|---|
| 20 | 1.19GB | 1182 Hz | byte-identical |
| 50 | 1.20GB | 1255 Hz | byte-identical |
| **100** | **1.22GB** | **1271 Hz** | byte-identical |
| 500 | 1.36GB | 1311 Hz | byte-identical |
| 1000 | 1.53GB | 1314 Hz | byte-identical |
| 2000 | 1.87GB | 1310 Hz | byte-identical |

Memory grows roughly linearly above the ~1.1-1.2GB floor (dominated by the resident PI_DB index):
about 0.4MB per additional held spectrum, consistent with the prior session's per-spectrum sparse
XCorr matrix estimate. Throughput rises from 20 to ~500 (barrier/thread-relaunch overhead
dominates at very small flush sizes) then plateaus -- 500/1000/2000 are within noise of each other
(1310-1314 Hz). No benchmark noise attributable to writer batching was observed (monotonic,
low-variance curve), consistent with the code-reading conclusion above.

Initially chose **100** as a flat default: ~97% of the throughput ceiling, at the low end of the
memory curve -- at 8 threads.

## Correction: a flat constant under-flushes at high thread counts

Question raised after landing the flat-100 design: since each flush round requires every worker
thread to fully drain (`tp->wait_on_threads()`) before the next round's jobs are dispatched (and,
for readahead, its background reader thread torn down and recreated), the barrier/relaunch cost is
amortized over each thread's *share* of a round, not the round's total size. A flat flush count
means that share shrinks as thread count grows -- exactly the reasoning that already justifies
`BoundedSpectrumQueue`'s depth being `4 * iNumThreads` rather than a flat constant
(`CometPreprocess.cpp:3489`), which the flush threshold should have mirrored from the start.

Re-ran the sweep at 20 threads (max available in this environment; not 64, but enough to show the
trend), same dataset:

| threads | flush | spectra/thread | throughput | vs. ceiling |
|---|---|---|---|---|
| 8 | 100 | 12.5 | 1271 Hz | 96.7% |
| 20 | 100 | 5 | 2206 Hz | 93.7% |
| 20 | 250 | 12.5 | 2269 Hz | 96.4% |
| 20 | 500 | 25 | 2308 Hz | 98.0% |
| 20 | 1000 | 50 | 2354 Hz | 100% (ceiling) |
| 20 | 2500 | 125 | 2314 Hz | 98.3% (past the ceiling, noise) |

The flat-100 deficit nearly doubles from 8 to 20 threads (3.3% -> 6.3% below ceiling), and the
ceiling itself is reached consistently around 50 spectra/thread regardless of total thread count
(1000/20 = 50; 500-1000/8 = 62-125 was already flat at 8 threads) -- confirming the two values
*are* tied together, and confirming the fix: scale the flush threshold by thread count rather than
hardcoding it.

**Fix**: `iFlushBatchSize = max(FUSED_FLUSH_MIN_BATCH_SIZE, iNumThreads * FUSED_FLUSH_PER_THREAD)`,
`FUSED_FLUSH_PER_THREAD = 50` (the conservative end of the 50-125 ceiling-reaching range),
`FUSED_FLUSH_MIN_BATCH_SIZE = 100` (matches the original flat default, as a floor for low thread
counts). Re-validated after the fix:

| threads | old (flat 100) | new (thread-scaled) | ceiling |
|---|---|---|---|
| 8 | 1271 Hz / 1.22GB | 1323 Hz / 1.36GB | ~1314 Hz |
| 20 | 2206 Hz / 1.56GB | 2342 Hz / 1.89GB | ~2354 Hz |

20-thread throughput recovers to 99.5% of ceiling (up from 93.7%), landing almost exactly on the
computed value (`max(100, 20*50) = 1000`, matching the measured ceiling point above). At 64
threads the formula gives `max(100, 64*50) = 3200`; extrapolating the ~0.4MB/spectrum memory slope,
that's still only ~2.5GB total (on top of the ~1.2GB index floor) -- nowhere near the original
problem. Full unit + integration suite (21/21) and byte-identical output re-confirmed at both
thread counts after the fix.

## Validation

- Full unit + integration suite: 21/21 pass at `FUSED_FLUSH_BATCH_SIZE=100`, including T19 (FI_DB
  build+search+AScorePro) and T20 (PI_DB build+search+AScorePro regression).
- Real-scale correctness: all six sweep runs plus the final flush=100 build produced **byte-identical
  output** (`diff`, 0 lines) against a legacy-path baseline captured before any of this session's
  changes.
- Scaling confirmation: same flush=100 build against the **entire** mzXML file (51,816 spectra, no
  `scan_range` limit, exercising the readahead branch since this is a plain `EntireFile` mzXML
  search) -- peak memory **1.26GB**, statistically flat versus the 20,097-spectrum test's 1.22GB.
  Confirms memory no longer scales with file size.
- FI_DB sanity check at real scale (same mechanism, shared function): 20,097 spectra against an
  ~8.9M-peptide FI_DB index, 1.8GB peak (FI_DB's larger in-memory fragment index, ~1.34e8 entries,
  accounts for the higher floor versus PI_DB's ~1.2GB -- unrelated to this change), 4157 Hz, no
  errors.

## Net result

Real batch PI_DB search, unset `spectrum_batch_size` (the common case): **7.4GB -> ~1.2-1.9GB**
peak memory (thread-count-dependent, see table above), byte-identical output, ~99%+ of achievable
throughput at both tested thread counts, no user configuration required.
