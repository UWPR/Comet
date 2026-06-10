---
name: comet-benchmark
description: Run and interpret Comet performance benchmarks comparing batch search vs real-time search (RTS) throughput. Use when measuring search speed, comparing before/after changes, or analyzing Hz and ms/spectrum metrics.
---

# Comet Benchmark

## Test files (in `RealtimeSearch\bin\x64\Release\`)

- Raw file: `20170103_HelaQC_01.raw` (39,970 spectra)
- Database: `human.target-decoy.fasta.idx` (410 MB, ~40,858 proteins)

## Batch search command

```bash
cd /mnt/c/Work/Comet-master/RealtimeSearch/bin/x64/Release
../../../x64/Release/Comet.exe -Dhuman.target-decoy.fasta.idx 20170103_HelaQC_01.raw
```

Key output line: `searching "20170103_HelaQC_01" ... 10s (39970 spectra, 0.25ms/spec, 3930Hz)`

## RTS search command

```bash
cd /mnt/c/Work/Comet-master/RealtimeSearch/bin/x64/Release
./RealtimeSearch.exe 20170103_HelaQC_01.raw 20170103_HelaQC_01.raw human.target-decoy.fasta.idx 20
```

(Or run via PowerShell from that directory.)

Key output lines:
```
Pre-read N scans in X.XX s
initialize search elapsed time: XX.XX s
search elapsed time: X.XX s, avg Y.YY ms/spectrum (39970 spectra), ZZZZ Hz
```

## Baseline numbers (established 2026-04-28/29)

| Mode | Time | ms/spec | Hz |
|------|------|---------|-----|
| Batch (Comet.exe) | 10 s | 0.25 | 3930 |
| RTS original | 33.2 s | 0.83 | 1204 |
| RTS after pre-read fix | ~4 s search | ~0.10 | ~10000 |

## What the RTS metrics mean

`search elapsed time` = wall-clock time for the parallel search phase only (excludes pre-read and index load).

`avg ms/spectrum` and `Hz` are computed from `cumulativeElapsedMS2` — the **sum of per-call wall time inside `DoSingleSpectrumSearchMultiResults` only**, divided by `scansProcessedMS2`. This measures pure C++ search speed, excluding raw file I/O and marshaling overhead. It is a **single-thread-equivalent** rate, not wall-clock throughput.

Wall-clock throughput = `scansProcessedMS2 / watchGlobal.Elapsed.TotalSeconds` (not printed directly).

## Key architectural difference: why RTS was slow

The original RTS bottleneck was 20 C# threads sharing one `IRawDataPlus rawFile`. Thermo's reader serializes all calls internally, so ~87% of thread time was idle waiting for file I/O. Fix: pre-read all scan data sequentially before launching parallel workers (`PrereadScan` queue). Workers then operate entirely from in-memory arrays.

## Initialize time (~28 s)

This is `InitializeSingleSpectrumSearch()` loading the `.idx` file and building `g_pvProteinNameCache`. It is a fixed one-time cost, not part of the search phase timer.
