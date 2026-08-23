---
name: comet-benchmark
description: Run and interpret Comet performance benchmarks comparing batch search vs real-time search (RTS) throughput. Use when measuring search speed, comparing before/after changes, or analyzing Hz and ms/spectrum metrics.
---

# Comet Benchmark

## Test files (in `RealtimeSearch\bin\x64\Release\`)

- Raw file: `20170103_HelaQC_01.raw` (39,970 spectra)
- Database: `human.target-decoy.fasta.idx` (410 MB, ~40,858 proteins)

These are the original small fixtures the 2026-04-28/29 baseline below used.
A separate, much larger fixture pair (`human.target-decoy.fasta`,
211,090 protein entries, ~5x bigger) was used for the 2026-08-22 comparison
further down — **database size and thread count are the two biggest levers
on absolute timing, so don't compare raw numbers across benchmarks run on
different fixtures without normalizing for both.**

## Batch search command

```bash
cd /mnt/c/Work/Comet-master/RealtimeSearch/bin/x64/Release
../../../x64/Release/Comet.exe -Dhuman.target-decoy.fasta.idx 20170103_HelaQC_01.raw
```

Key output line: `searching "20170103_HelaQC_01" ... 10s (39970 spectra, 0.25ms/spec, 3930Hz)` —
this same line's trailing `(...MB)` (visible in the full "Run stats" line) also reports peak memory.

## RTS search command

```bash
cd /mnt/c/Work/Comet-master/RealtimeSearch/bin/x64/Release
./RealtimeSearch.exe --query 20170103_HelaQC_01.raw --ms1ref 20170103_HelaQC_01.raw --db human.target-decoy.fasta.idx --threads 20
```

(As of docs/20260805_carafe.md Phase 4, args are flags, not positional -- `--help` lists all of
them, including `--mask <path>` for a Carafe predicted-fragment mask file, FI_DB only.)

(Or run via PowerShell from that directory.)

Key output lines (current `RealtimeSearch/SearchMS1MS2.cs`):
```
 initialize elapsed time: X.XX s
 raw file preload elapsed time: X.XX s
 MS2 search elapsed time: X.XX s
 MS2 average search time: X.XX ms/spectrum (N spectra), ZZZZ Hz
 MS1 search elapsed time: X.XX s
      total elapsed time: X.XX s

 Done. (X.XGB)
```
The final `Done. (X.XGB)` line (printed once, after the raw file is closed)
is peak memory for the whole process, from `CometSearchManager::GetPeakMemory()`.

## What the RTS metrics mean (current instrumentation)

Four independent `Stopwatch`es, each started/stopped exactly once (no
shared/toggled stopwatch across threads, so none of them can race or
under-count):

- `watchIndexCreate` -> **initialize elapsed time**: wraps
  `InitializeSingleSpectrumSearch()` — loading the `.idx` file and building
  `g_pvProteinNameCache`, plus (for FI_DB) regenerating the in-memory
  fragment index from the persisted raw-peptide list every session (the
  fragment index itself isn't persisted in the `.idx` — see
  `docs/20260730_PI_reduction.md` Phase 0.5). **This is not a fixed ~28s
  cost** — it scales with database size and is dramatically affected by
  whether the fragment-index build is parallelized (see
  `docs/20260819_fablereview.md` P1: this phase alone dropped from 351.85s to
  4.10s across a `v2026.02.2`-vs-current-branch comparison on a 528 MB `.idx`).
- `watchPreload` -> **raw file preload elapsed time**: a single-threaded
  upfront pass that reads every scan's peak data (and, for MS2, precursor
  m/z/charge from the trailer) into an in-memory `PreloadedScan[]` array
  before any search thread starts. This is the *only* code that ever touches
  the shared `IRawDataPlus rawFile` — eliminates the need for a lock on it
  during the parallel phase entirely (see "why RTS was slow" below).
- `watchParallelPhase` -> **MS2 search elapsed time**: wraps
  `Task.Run(...)` for all `numThreads` workers through `Task.WaitAll(tasks)`
  — i.e. the wall-clock duration of the actual concurrent search phase,
  nothing else. **`ms/spectrum` and `Hz` are derived directly from this
  wall-clock span** (`watchParallelPhase.Elapsed.TotalMilliseconds /
  scansProcessedMS2`, then `1000 / that`) — this *is* genuine aggregate
  wall-clock throughput across every thread, not a single-thread-equivalent
  rate. (An earlier version of this doc described these as computed from a
  `cumulativeElapsedMS2` sum of each thread's own per-call time instead —
  that field doesn't exist in the current code; each worker's per-call
  `Stopwatch` result is only used for the slowest-runs list and the
  histogram, not for the aggregate Hz/ms-per-spectrum stat.)
- `watchGlobal` -> **total elapsed time**: wraps the entire search (opening
  the raw file through the parallel phase finishing) — roughly
  initialize + preload + MS2 search + MS1 search, plus file-open overhead.

## Key architectural difference: why RTS was slow

The original RTS bottleneck was `numThreads` C# threads sharing one
`IRawDataPlus rawFile`. Thermo's reader serializes all calls internally, so
most thread time was idle waiting for file I/O. Fix: a single-threaded
upfront pass (`watchPreload`, above) reads every scan into an in-memory
`PreloadedScan[]` array before any worker thread starts; workers then pull
scan numbers off a `ConcurrentQueue<int>` (`scanQueue`) and operate entirely
on the preloaded in-memory arrays — no lock on `rawFile` is needed because
nothing touches it concurrently anymore.

## Baseline numbers — small fixture (established 2026-04-28/29)

410 MB `.idx`, ~40,858 proteins, 20 threads:

| Mode | Time | ms/spec | Hz |
|------|------|---------|-----|
| Batch (Comet.exe) | 10 s | 0.25 | 3930 |
| RTS original | 33.2 s | 0.83 | 1204 |
| RTS after pre-read fix | ~4 s search | ~0.10 | ~10000 |

("RTS original"/"after pre-read fix" reflect the codebase as of April 2026,
before most of the bug-fix and performance work in
`docs/20260819_fablereview.md` — this table is a historical record of the
pre-read fix's own impact, not a general Comet-version comparison.)

## Baseline numbers — full proteome, `v2026.02.2` vs. current branch (2026-08-22)

528 MB `.idx` built fresh by each binary, 211,090 protein entries, 16
threads, FI_DB, real HeLa QC data (56,152 total scans / 40,302 MS2 scans).
See `docs/20260819_fablereview.md`'s end-to-end RTS validation section
(after P11) for full methodology and the 1% FDR correctness comparison.

| Metric | Baseline (`v2026.02.2`) | Current (`fablereview`, P1-P11) | Change |
|---|---|---|---|
| Initialize (index load + FI regen) | 351.85 s | 4.10 s | ~86x faster |
| Raw file preload | 3.99 s | 2.34 s | ~1.7x faster |
| MS2 search elapsed | 18.54 s | 16.21 s | ~13% faster |
| MS2 avg search speed | 0.46 ms/spec, 2174 Hz | 0.40 ms/spec, 2487 Hz | ~14% faster |
| Total elapsed | 374.61 s | 22.86 s | ~16.4x faster |
| Peak memory | 2.8 GB | 2.8 GB | unchanged |

Don't average or otherwise combine this table with the 2026-04-28/29 one
above — different database size, thread count, and Comet version on both
sides make the absolute numbers non-comparable.
