# Migrate the batch FI search path to the RTS fused execution model

## Context

On a hybrid CPU (Intel Core Ultra 265K, 8 P-core + 12 E-core), Comet's batch
search (`comet.exe somefile.raw`) scales poorly with thread count, while the RTS
path (`RealtimeSearch.exe`) scales nearly linearly. Measured on one HeLa file
(66,882 spectra, fragment-index search):

| Threads | Batch Hz | RTS Hz |
|---------|----------|--------|
| 20      | 3,573    | 8,155  |
| 16      | 1,550    | 7,909  |
| 8       | 827      | 6,715  |
| 4       | -        | 4,397  |

RTS at 4 threads beats batch at 20. Root cause (confirmed by reading the code):
the batch path processes each spectrum batch in **three separate parallel
sweeps** - `LoadAndPreprocessSpectra` writes every spectrum's sparse
FastXcorr/SpScore data into `g_pvQuery` (~28 GB resident), then `RunSearch`
reads it all back cold from DRAM, then `PostAnalysis` reads it a third time. On a
shared-LLC machine, DRAM bandwidth saturates and worsens with thread count - the
observed anti-scaling. The batch path also (b) submits ~66k individual
`doJob` tasks each contending on one mutex+CV in `AcquirePoolSlot`, and (c) does
per-spectrum heap allocation of scratch/sparse blocks.

The RTS path already solved all three: it **fuses** preprocess -> search ->
post-analysis per spectrum with a reusable `thread_local RtsScratch` pool, so each
thread's working set is a single spectrum kept hot in L1/L2, with no per-spectrum
malloc/free. This plan brings that model to the batch FI_DB path.

**Outcome:** the batch FI search becomes a fused, thread-local, work-stealing
loop. Expected result: dramatically flatter thread-scaling curve and lower peak
memory, while producing **bit-identical** PSM output to today's batch path.

## Scope

- **In scope:** fragment-index search only (`DbType::FI_DB`), the
  non-Mango, non-speclib database-search case - the path the benchmark exercises.
- **Out of scope, left on the legacy three-sweep path:** the FASTA_DB
  sequential-database path, the Mango path (relies on global g_pvQuery ordering),
  and the spectral-library path. The new branch is guarded so these are untouched.

## Confirmed facts the design relies on

- `Query::bSparseFromPool` already gates the destructor
  (CometDataInternal.h:1429/1445/1460): when true, pool-backed sparse children are
  not freed by `~Query()`; only the small parent `float**` arrays are. So a Query
  can persist in `g_pvQuery` for output while its sparse payload lives in the
  reusable thread-local pool.
- The reusable pool already exists: `RtsScratch` + `thread_local g_rtsScratch`
  (CometPreprocess.cpp:46-134) with `EnsureInitialized()` / `ResetForNewSpectrum()`
  / `AllocSparseChild()`.
- `PreprocessSingleSpectrumCore` already takes `bUseThreadLocalPool`
  (CometPreprocess.cpp:1230-1236); the Spectrum-based `Preprocess()` (line 957)
  does not yet.
- The exact batch post-analysis sequence is `PostAnalysisThreadProc` =
  `AnalyzeSP` -> conditional `CalculateEValue` -> `CalculateDeltaCn` ->
  conditional `CalculateAScorePro` (CometPostAnalysis.cpp:228-260). The fused
  worker must call **this** sequence (not the RTS variant) to keep `usiRankSp`,
  `SortFnMod` tie-breaking, and deltaCn identical.
- For FI_DB, `g_massRange.dMinMass/dMaxMass/bNarrowMassRange` and
  `usiMaxFragmentCharge` are **dead** (only the legacy FASTA scoring functions
  read them); `SearchFragmentIndex` uses per-Query mass tolerance and per-Query
  `usiMaxFragCharge`. Only `g_massRange.uiMaxFragmentArrayIndex` (set once at init)
  is needed. So the per-batch mass sort can be dropped for the new branch.
- `CalculateEValue` / `GenerateXcorrDecoys` are fully per-Query and deterministic
  (no RNG; `thread_local` accumulator; `call_once` for shared read-only data).

## Design decisions (confirmed with user)

- **Work distribution:** read each batch's raw spectra into a `std::vector<Spectrum>`,
  then N long-lived workers pull via a lock-free `std::atomic<size_t>` index
  (mirrors the RTS `ConcurrentQueue` model; no per-scan mutex).
- **Pool slots:** give each worker a fixed `_ppbDuplFragmentArr` slot; add a
  `RunSearch(Query*, int iSlot)` overload that skips `AcquirePoolSlot` entirely.
- **Parity bar:** bit-exact PSMs vs current batch output.

## Implementation stages

### Stage 0 - Pooled support in the Spectrum-based preprocess core
- `CometPreprocess::Preprocess(Query*, Spectrum&, ...)` (CometPreprocess.cpp:957):
  add a `bool bUseThreadLocalPool` parameter. When true, route scratch buffers and
  sparse-child allocation through `g_rtsScratch` (`AllocSparseChild`) and set
  `pScoring->bSparseFromPool = true` - mirroring the existing raw-array path in
  `PreprocessSingleSpectrumCore`.

### Stage 1 - Fused per-scan worker
- Add `CometPreprocess::FusedSearchSpectrum(Spectrum spec, int iSlot)` (Spectrum
  taken by value, matching existing `PreprocessThreadData` copy semantics).
- Body reuses `PreprocessSpectrum`'s charge-state enumeration
  (CometPreprocess.cpp:1969-2116). For **each** `(charge, m/z)` candidate, in
  strict sequence (never batch charges - they share the pool):
  1. `g_rtsScratch.EnsureInitialized(); g_rtsScratch.ResetForNewSpectrum();`
  2. allocate `Query`, fill mass/charge/array-size/tolerance (as
     PreprocessSpectrum:2124-2162);
  3. `Preprocess(pQuery, spec, ..., /*pool=*/true);` (Stage 0);
  4. `CometSearch::RunSearch(pQuery, iSlot);` (Stage 3 overload);
  5. run the batch post-analysis sequence on `pQuery` (reuse the
     `PostAnalysisThreadProc` body: `AnalyzeSP` -> `CalculateEValue` (conditional)
     -> `CalculateDeltaCn` -> `CalculateAScorePro` (conditional));
  6. optionally `pQuery->vfRawFragmentPeakMass.clear()/shrink_to_fit()`;
  7. push `pQuery` into `g_pvQuery` under `g_pvQueryMutex`.

### Stage 2 - FI_DB fused dispatch in DoSearch
- In `CometSearchManager::DoSearch` (CometSearchManager.cpp:2862-3021), add a
  branch `if (iDbType==FI_DB && g_bPerformDatabaseSearch && !bMango && !specLib)`
  that, per batch:
  - reads the batch's spectra into `std::vector<Spectrum>` (extract the read loop
    from `LoadAndPreprocessSpectra` / `PreloadIons`, respecting `iSpectrumBatchSize`
    and scan-range filters);
  - launches `iNumThreads` workers via `tp->doJob`, each:
    `while ((i = ctr.fetch_add(1)) < n) FusedSearchSpectrum(vec[i], myWorkerId);`
    then `tp->wait_on_threads();`
  - **skips** `AllocateResultsMem()` (worker allocates `_pResults`/`_pDecoys`),
    the per-batch `g_massRange.dMin/dMax` assignment, and the sort-by-mass
    (2931-2939);
  - **keeps** the by-scan sort (2978), every output writer (2986-3008), and the
    cleanup `delete` loop (3014) unchanged.
- The existing three-sweep code remains as the `else` path for FASTA/Mango/speclib.

### Stage 3 - Fixed-slot RunSearch overload
- Add `CometSearch::RunSearch(Query*, int iSlot)` that calls
  `SearchFragmentIndex(pQuery, _ppbDuplFragmentArr[iSlot])` directly, skipping
  `AcquirePoolSlot`/CV. Confirm the search memory pool is allocated for the batch
  path before dispatch (it is - the batch already uses `AcquirePoolSlot`).

### Stage 4 - Fix latent pool-fallback leak (do as part of this work)
- `RtsScratch::AllocSparseChild` falls back to `new float[SPARSE_MATRIX_SIZE]()`
  when the pool is exhausted (CometPreprocess.cpp:130), but with
  `bSparseFromPool=true` the destructor never frees it -> leak. Either guarantee
  the pool is sized so it cannot be exhausted for a single charge-state Query, or
  track fallback allocations so they are freed. Verify pool capacity
  (`6*(iSize/SPARSE_MATRIX_SIZE+2)`) is sufficient for the densest single spectrum.

## Critical files
- `CometSearch/CometPreprocess.cpp` - pooled `Preprocess`, new `FusedSearchSpectrum`,
  charge enumeration reuse, pool-leak fix.
- `CometSearch/CometSearchManager.cpp` - FI_DB fused dispatch branch in `DoSearch`.
- `CometSearch/CometSearch.cpp` - `RunSearch(Query*, int iSlot)` overload.
- `CometSearch/CometPostAnalysis.cpp` - reuse `PostAnalysisThreadProc` body
  (`AnalyzeSP`/`CalculateEValue`/`CalculateDeltaCn`/`CalculateAScorePro`).
- `CometSearch/CometDataInternal.h` - `Query` / `bSparseFromPool` (reference; verify
  lifetime, fix fallback leak).

## Verification

1. **Build:** Visual Studio Release/x64 (the user's binary) and `make` on Linux.
2. **Unit tests:** `python tests/unit/run_tests.py --comet /mnt/c/Work/Comet-master/comet.exe`.
3. **Bit-exact PSM parity:** search a HeLa file with the old vs new binary, both to
   `.txt`; diff sorted by (scan, charge): peptide, xcorr, deltaCn, Sp, rankSp,
   e-value, matched/total ions must match exactly. Confirm `compareByScanNumber`
   tie-breaks deterministically for same-scan multi-charge rows so output order is
   stable.
4. **FDR parity:** `python tools/qvalue.py old.txt new.txt --diff` - identical PSM
   counts at 1% FDR for both xcorr and e-value sorting.
5. **Scaling benchmark:** rerun the 4/8/16/20-thread batch search (comet-benchmark
   skill); confirm the curve flattens toward the RTS profile and peak memory drops.
