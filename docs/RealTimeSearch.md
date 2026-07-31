# Real-Time Search (RTS) Architecture

Comet supports two search modes:

- **Batch search**: `DoSearch()` -- reads a file, processes spectra in configurable batches, writes result files. `DoSearch()` is orchestrated by a `Pipeline` that owns one concrete `ISearchStrategy` (`FiStrategy`, `FastaStrategy`, or `PiStrategy`) and a set of `IResultWriter` implementations. All mutable batch-run state (query lists, per-run flags) lives in a `SearchSession` struct passed by reference through the pipeline.
- **Real-time search (RTS)**: called per-spectrum by an external C# application; returns results synchronously within the same call. Designed for concurrent calls from multiple threads.

This document covers the RTS path. The design history and task-by-task implementation record are in `docs/20260227_RTS_THREAD_PLAN.md` (MS2) and `docs/20260228_MS1_THREAD_PLAN.md` (MS1).

---

## Entry points

All entry points are declared in `CometSearch/CometInterfaces.h` as virtual methods on `ICometSearchManager` and implemented in `CometSearchManager`. They are exposed to C# through `CometWrapper/CometSearchManagerWrapper` (a C++/CLI ref class) and called from `RealtimeSearch/SearchMS1MS2.cs`.

### MS2 search (fragment index or peptide index)

```cpp
bool InitializeSingleSpectrumSearch();
void FinalizeSingleSpectrumSearch();
bool DoSingleSpectrumSearchMultiResults(
    const int topN,
    const int iPrecursorCharge,
    const double dMZ,
    double* pdMass, double* pdInten, int iNumPeaks,
    vector<string>& strReturnPeptides,
    vector<string>& strReturnProteins,
    vector<vector<Fragment>>& matchedFragments,
    vector<CometScores>& scores);
```

Returns up to `topN` hits. Safe for concurrent calls from multiple threads against the same `ICometSearchManager` instance.

### MS1 spectral library search

```cpp
bool InitializeSingleSpectrumMS1Search(const double dMaxQueryRT);
void FinalizeSingleSpectrumMS1Search();
bool DoMS1SearchMultiResults(
    const double dMaxMS1RTDiff,
    const double dMaxQueryRT,
    const int topN,
    const double dRT,
    double* pdMass, double* pdInten, int iNumPeaks,
    vector<CometScoresMS1>& scores);
```

Note this is NOT precursor-charge/m-z based like the MS2 call -- it takes the
query's retention time (`dRT`) directly, since MS1 scoring matches against
spectral-library retention times rather than a precursor mass window.

Scores against the in-memory spectral library (`g_vSpecLib`). Also safe for concurrent calls; uses `g_ms1AlignerMutex` only for the RT alignment history update.

---

## Initialization

### MS2 (`InitializeSingleSpectrumSearch`)

Uses a double-checked locking pattern with `std::atomic<bool> singleSearchInitializationComplete`:

**PI_DB and FI_DB share one unified `.idx` format and one builder/reader**
(`CometPeptideIndex::WritePeptideIndex()` / `ReadPeptideIndex()`;
`docs/20260730_PI_reduction.md` Phase 0). Since the file no longer implies a mode on its
own, which mode to build/search comes from `g_staticParams.options.iIndexSearchType`
(`0`=PI_DB, `1`=FI_DB, default when unset) -- for RTS this is set via a
`CometSearchManagerWrapper::SetParam("index_search_type", ...)` call in
`RealtimeSearch/SearchMS1MS2.cs` (a new `[index_search_type]` CLI argument), since RTS has
no `comet.params` file to read a `index_search_type` key from the way batch does.

```
fast path: singleSearchInitializationComplete.load(acquire) -> return true if set
slow path: mutex-guarded check + initialization
  -> InitializeStaticParams()         populates g_staticParams
  -> ValidateSequenceDatabaseFile()   validates FASTA / index. If the requested .idx is
                                      absent but the underlying FASTA exists, sets EITHER
                                      bCreatePeptideIndex=true + iDbType=PI_DB OR
                                      bCreateFragmentIndex=true + iDbType=FI_DB, from
                                      iIndexSearchType -- the only place either gets set
                                      for a missing .idx (the block below only runs when
                                      the file already exists).
  -> if requested database ends in ".idx" and the file exists: read its first line as a
       sanity check only (must start "Comet index database"; does NOT select a mode), then
       set iDbType from iIndexSearchType (same as ValidateSequenceDatabaseFile's branch above)
  -> CometPreprocess::AllocateMemory()  preprocessing thread buffers
  -> CometSearch::AllocateMemory()      search thread pool (s_pool, a SearchMemoryPool
                                        instance; aliased into _ppbDuplFragmentArr)
                                        used by AcquirePoolSlot()
  -> tp->fillPool()
  -> if iDbType == FI_DB:
       if bCreateFragmentIndex:
         CreateFragmentIndex()         (CometSearchManager's own overload) calls DoSearch()
                                       to scan FASTA and write the unified .idx; DoSearch()
                                       calls CometSearch::DeallocateMemory() internally
                                       before returning
         CometSearch::AllocateMemory() re-allocate search pool freed by DoSearch() above
       if !g_bPlainPeptideIndexRead:
         CometPeptideIndex::ReadPeptideIndex(true)   loads g_vRawPeptides from the .idx file,
                                                      then (Phase 0.5) regenerates
                                                      MOD_SEQS/MOD_NUMBERS/etc. in memory from
                                                      g_vRawPeptides + whatever variable mods
                                                      comet.params has active right now --
                                                      neither is read from disk any more
         sqSearch.CreateFragmentIndex(tp, true)       builds g_iFragmentIndex /
                                                      g_iFragmentIndexOffset in memory
                                                      (CSR posting lists) from those
                                                      just-regenerated tables
         if iPrintAScoreProScore: SetAScoreOptions() + CreateAScoreDllInterface()
       g_bPlainPeptideIndexRead = true
  -> if iDbType == PI_DB and !g_bPeptideIndexRead:
       if bCreatePeptideIndex:
         CometPeptideIndex::WritePeptideIndex(tp)    builds the unified .idx from FASTA
                                                      (strips/restores the ".idx" suffix on
                                                      g_staticParams.databaseInfo.szDatabase
                                                      internally, since that field holds the
                                                      not-yet-existent .idx path at this
                                                      point, not a FASTA path); calls
                                                      CometSearch::DeallocateMemory()
                                                      internally before returning
         CometSearch::AllocateMemory()               re-allocate search pool freed above
       CometPeptideIndex::ReadPeptideIndex(true)      loads g_vRawPeptides from the .idx file,
                                                       then (Phase 0.5) regenerates
                                                       MOD_SEQS/MOD_NUMBERS/etc. and
                                                       g_vDBIndexVariants in memory from
                                                       g_vRawPeptides + whatever variable mods
                                                       comet.params has active right now
       CometSearch::InitializeMassesFromPeptideIndex() re-parses the .idx header
                                                        (MassType/StaticMod/DecoySearch/
                                                        Enzyme/Enzyme2 -- no variable-mod
                                                        fields, Phase 0.5 removed those from
                                                        the header entirely) into
                                                        g_staticParams so fragment masses match
                                                        the static mods baked into the index
                                                        (otherwise InitializeStaticParams()'s
                                                        values may have double-applied them);
                                                        does not touch variable mods, which
                                                        ReadPeptideIndex() already regenerated
                                                        from live comet.params above
       if iPrintAScoreProScore: SetAScoreOptions() + CreateAScoreDllInterface()
                                (mirrors the FI_DB branch's AScore setup, since
                                EnsurePeptideIndexLoaded()'s own AScore-creation code is
                                gated on g_bPeptideIndexRead being false, and this branch
                                already sets it true below)
       g_bPeptideIndexRead = true
  -> singleSearchInitializationComplete.store(true, release)
```

Note: search memory (the `pbDuplFragment` scratch arrays) is already allocated by the
unconditional `CometSearch::AllocateMemory()` call earlier in this sequence; the
`bCreatePeptideIndex`/`bCreateFragmentIndex` build sub-branches each re-allocate it after
their respective builder internally deallocates it, mirroring each other.

The `release` store ensures all threads that subsequently load the flag with `acquire` see a fully initialized `g_iFragmentIndex`, `g_iFragmentIndexOffset`, `g_pvProteinNameCache`, and all other globals.

**Note on the index-build path:** the "requested .idx is missing, build it from the
underlying FASTA" scenario is symmetric between the two modes now (it wasn't reachable for
PI_DB at all before `index_search_type` existed, since RTS previously had no way to request
PI_DB mode without a PI-formatted `.idx` already on disk to sniff). For FI_DB,
`CreateFragmentIndex()` calls `DoSearch()` with `m_bRTSIndexBuild=true`; `DoSearch()` writes
the `.idx` file, calls `CometSearch::DeallocateMemory()` to free the large FASTA-parse
memory, then returns early (skipping the spec-lib and batch-search logic that follows in
`DoSearch()`). For PI_DB, `CometPeptideIndex::WritePeptideIndex()` does the equivalent
directly (no `DoSearch()`/`m_bRTSIndexBuild` involved). Either way,
`InitializeSingleSpectrumSearch()` re-allocates the search pool before proceeding to load
the index it just built.

### MS1 (`InitializeSingleSpectrumMS1Search`)

Same double-checked pattern with `singleSearchMS1InitializationComplete`:

```
  -> InitializeStaticParams()
  -> CometSpecLib::LoadSpecLibMS1Raw()  loads g_vSpecLib from the MS1 reference .raw file
  -> singleSearchMS1InitializationComplete.store(true, release)
```

The C# caller invokes `InitializeSingleSpectrumMS1Search` and `InitializeSingleSpectrumSearch` as **separate, independent calls** (in that order). MS1 init does not call MS2 init internally.

**Important:** Both init calls must complete before any thread calls `DoMS1SearchMultiResults()` or `DoSingleSpectrumSearchMultiResults()`. This ordering is enforced by the C# caller, not by the C++ code itself.

`FinalizeSingleSpectrumSearch()` frees allocated memory (`CometSearch::DeallocateMemory`/`CometPreprocess::DeallocateMemory`/`DeleteAScoreDllInterface`) and resets `singleSearchInitializationComplete`. `FinalizeSingleSpectrumMS1Search()` only resets `singleSearchMS1InitializationComplete` -- by design it frees nothing, since MS1 init never allocates the `CometSearch` pool (it only loads `g_vSpecLib`), and that pool belongs to the independent MS2 lifecycle which may still be in use. Call both when the search session ends.

---

## Per-call flow

### MS2 (`DoSingleSpectrumSearchMultiResults`)

```
DoSingleSpectrumSearchMultiResults(topN, charge, mz, masses, intensities, nPeaks, ...)
  |
  +- Guard: iNumPeaks == 0, or dMZ*charge exceeds dPeptideMassHigh -- reject early
  +- InitializeSingleSpectrumSearch() -- SELF-INITIALIZING, not abort-only: this
  |     call IS the double-checked-locking init path (see "Initialization" above).
  |     The first caller across all threads pays the index-load cost; every
  |     other concurrent/later caller takes the atomic fast path and returns
  |     immediately once init has completed.
  |
  +- pdTmpSpectrum = CometPreprocess::GetRtsRawDataBuffer()   <- thread-local
  |     pooled buffer (RtsScratch), not a per-call new[]/delete[]
  |
  +- CometPreprocess::PreprocessSingleSpectrumThreadLocal(charge, mz, masses, intensities, nPeaks, pdTmpSpectrum)
  |     -> allocates caller-owned Query* on the heap
  |     -> fills it with binned spectrum data
  |     -> does NOT touch SearchSession::queries
  |     -> returns nullptr on failure (caller checks and returns false)
  |
  +- pQuery->tSearchStart = now()                    <- member of the per-call Query;
  |                                                      not a local passed through calls
  |
  +- CometSearch::RunSearch(pQuery)
  |     -> AcquirePoolSlot() reserves one slot of the shared SearchMemoryPool
  |        (s_pool), guarded by a SearchMemoryPoolSlotGuard so the slot is
  |        released on any exit path (including exceptions)
  |     -> branches on g_staticParams.iDbType (CometSearch.cpp:171-215):
  |        FI_DB -> SearchFragmentIndex(pQuery, _ppbDuplFragmentArr[iSlot])
  |                   reads g_iFragmentIndex / g_iFragmentIndexOffset (READ-ONLY) [x]
  |                   reads g_vFragmentPeptides (READ-ONLY) [x]
  |                   XcorrScoreI(pQuery, ...) -- updates only pQuery->_pResults
  |                   CheckMassMatch(pQuery, dMass) -- reads only pQuery->_pepMassInfo
  |                   timeout checked periodically against pQuery->tSearchStart
  |        PI_DB -> EnsurePeptideIndexLoaded(true) then
  |                 SearchPeptideIndex(pQuery, _ppbDuplFragmentArr[iSlot], iSlot)
  |                   thread-local peptide-index search; binary-searches
  |                   g_vDBIndexVariants (READ-ONLY) [x] by mass, then
  |                   CometPeptideIndex::MaterializeOneEntry() reconstructs a
  |                   stack-local DBIndex per surviving candidate from
  |                   g_vRawPeptides (READ-ONLY) [x] -- see
  |                   docs/20260730_PI_reduction.md; also checks
  |                   pQuery->tSearchStart for the iMaxIndexRunTime timeout
  |        any other iDbType -> error
  |     -> guard destructor releases the pool slot on return
  |
  +- timeout re-checked against pQuery->tSearchStart (iMaxIndexRunTime) before
  |     each of CalculateSP/CalculateEValue/CalculateDeltaCn (each is skipped
  |     past the deadline); CalculateAScorePro is NOT individually gated -- it
  |     always runs once CalculateDeltaCn completes, and is only implicitly
  |     bounded by the next timeout check below (CometSearchManager.cpp:2582,
  |     2599, 2613 have explicit pre-checks; 2640's CalculateAScorePro call
  |     does not; the next check is at 2658, after AScorePro has already run)
  +- CometPostAnalysis::CalculateSP(pQuery->_pResults, pQuery, iSize)
  +- CometPostAnalysis::CalculateEValue(pQuery, bSkip)
  +- CometPostAnalysis::CalculateDeltaCn(pQuery)
  +- CometPostAnalysis::CalculateAScorePro(pQuery, g_AScoreInterface)
  |
  +- sort _pResults by XCorr, extract top topN hits into output vectors
  |     protein names resolved via g_pvProteinNameCache.find(offset) [READ-ONLY, O(1)] [x]
  |
  +- cleanup_results:
       delete pQuery          (destructor frees sparse arrays, _pResults[], accessMutex)
       return
```

Note: `pdTmpSpectrum` above is a thread-local buffer owned by the per-thread
`RtsScratch` pool (`CometPreprocess::GetRtsRawDataBuffer()`), not a per-call
heap allocation -- this replaced an earlier per-call `new[]`/`delete[]` design.

### MS1 (`DoMS1SearchMultiResults`)

```
DoMS1SearchMultiResults(dMaxMS1RTDiff, dMaxQueryRT, topN, dRT, masses, intensities, nPeaks, scoresMS1)
  |
  +- Guard: singleSearchMS1InitializationComplete.load(acquire) -- ABORTS with an
  |     error (unlike MS2's self-initializing InitializeSingleSpectrumSearch()
  |     call above) if InitializeSingleSpectrumMS1Search() has not already
  |     completed. The C# caller is responsible for calling it first.
  +- Guard: iNumPeaks == 0 -- reject early
  |
  +- CometPreprocess::PreprocessMS1SingleSpectrumThreadLocal(pdMass, pdInten, iNumPeaks)
  |     -> allocates caller-owned QueryMS1* on the heap (no charge/mz -- MS1
  |        scoring is retention-time based, not precursor-mass based)
  |     -> does NOT touch SearchSession::ms1Queries
  |
  +- CometSearch::RunMS1Search(pQueryMS1, dRT, dMaxMS1RTDiff, dMaxSpecLibRT, dMaxQueryRT, localScores)
  |     reads g_vSpecLib / g_vulSpecLibPrecursorIndex (READ-ONLY) [x]
  |     fills the out-parameter vector<CometScoresMS1> localScores directly
  |     (pQueryMS1->_pSpecLibResultsMS1 is internal mid-scoring scratch, not
  |     the source the caller reads from)
  |
  +- if localScores is non-empty:
  |     dMatchedSpecLibRT = localScores[0].fRTime
  |     Threading::LockMutex(g_ms1AlignerMutex)
  |       dLinearRegressionRT = pMS1Aligner.processRetentionMatch(dRT, dMatchedSpecLibRT)
  |          <- pMS1Aligner is a global CometMassSpecAligner instance that
  |             accumulates RT history across the whole run; the mutex
  |             protects that shared history, not localScores itself
  |     Threading::UnlockMutex(g_ms1AlignerMutex)
  |     localScores[0].fRTime = dLinearRegressionRT   <- overwrite with the
  |                                                       regression-adjusted RT
  |     scoresMS1 = std::move(localScores)
  |
  +- delete pQueryMS1 (after freeing pQueryMS1->pfFastXcorrData if non-null)
       return bSucceeded
```

---

## Thread-safety of concurrent RTS calls

| State | RTS path | Notes |
|-------|:--------:|-------|
| `g_staticParams` | Read-only [x] | Set once at init; never written during search. |
| `g_iFragmentIndex` / `g_iFragmentIndexOffset` | Read-only [x] | CSR index loaded at init; never modified. |
| `g_vFragmentPeptides` / `g_vRawPeptides` | Read-only [x] | `g_vRawPeptides` loaded from the `.idx` file at init and never modified after; shared with PI_DB (both modes read the same unified `.idx` file -- `docs/20260730_PI_reduction.md`). `g_vFragmentPeptides` is built once at init from `g_vRawPeptides` + live `comet.params` mods (Phase 0.5, not read from disk), then likewise never modified during search. FI_DB uses it for its own posting-list resolution. |
| `g_vDBIndexVariants` | Read-only [x] | PI_DB's compact per-variant array (mass-sorted), built once at init from `g_vRawPeptides` + live `comet.params` mods (Phase 0.5, not read from disk), then read-only for the rest of the session; `MaterializeOneEntry()` reconstructs a stack-local `DBIndex` per candidate rather than mutating anything shared. Only populated when `iDbType == PI_DB`. |
| `g_vSpecLib` / `g_vulSpecLibPrecursorIndex` | Read-only [x] | Loaded at init. |
| `g_pvProteinNames` / `g_pvProteinsList` / `g_pvProteinNameCache` | Read-only [x] | Loaded at init. |
| `g_AScoreOptions` / `g_AScoreInterface` | Read-only [x] | Pointer set at init; each call uses its own data. |
| `SearchSession::queries` / `SearchSession::ms1Queries` | Not touched [x] | `SearchSession` is batch-path only. RTS path uses per-call `Query*` / `QueryMS1*`. |
| `g_massRange` | Not written [x] | Mass limits derived from per-call `Query*._pepMassInfo`. |
| `pQuery->tSearchStart` | Per-Query member [x] | Set once per call on the caller-owned `Query*`, not a bare local threaded through function params; each call's clock is independent. |
| `Query*` / `QueryMS1*` | Per-call heap [x] | Each call allocates and owns its object; freed at end. |
| `_ppbDuplFragmentArr[iSlot]` | Pool-slot, not per-call heap [x] | `RunSearch(Query*)` reuses a pre-allocated scratch array from the shared `SearchMemoryPool` (`AcquirePoolSlot()`, released by `SearchMemoryPoolSlotGuard`) rather than allocating a fresh array per call; each concurrent caller gets its own slot, so no data race, but it is not the per-call `new[]`/`delete[]` an earlier design used. |
| `RetentionMatchHistory` | Mutex-guarded [x] | `g_ms1AlignerMutex` protects writes in `DoMS1SearchMultiResults`. |
| `g_cometStatus` | Shared mutable [!] | Pre-existing; `IsCancel()` is safe, `SetStatus()` is mutex-protected (there is no `SetError()` -- errors are reported via `SetStatus(CometResult_Failed, msg)`). RTS path checks `IsCancel()` only -- avoids poisoning all threads on one failure. |

---

## C# / wrapper integration

### Layer stack

```
RealtimeSearch/SearchMS1MS2.cs          (C# application)
      ->  calls via COM-visible interface
CometWrapper/CometSearchManagerWrapper  (C++/CLI ref class, managed heap)
      ->  calls native C++
CometSearch/CometSearchManager          (native library)
```

### CometWrapper marshaling

`CometSearchManagerWrapper` holds a native `ICometSearchManager*` and marshals per-call:
- Managed `array<double>^` <-> native `double*` via `pin_ptr` (zero-copy).
- Native `CometScores` -> managed `ScoreWrapper` via copy-constructor; native `CometScoresMS1` -> managed `ScoreWrapperMS1` likewise.
- Native `Fragment` -> managed `FragmentWrapper` via copy-constructor.
- Native `string` <-> managed `String^` via `marshal_as<>`.

No shared mutable state exists in the wrapper layer. Each managed call creates stack-local native containers for results.

**Key wrapper types (`CometDataWrapper.h`):**

| Wrapper type | Native type | Key properties |
|---|---|---|
| `ScoreWrapper` | `CometScores` | `xCorr`, `dSp`, `dCn`, `dExpect`, `mass`, `MatchedIons`, `TotalIons`, `dAScorePro`, `sAScoreProSiteScores` |
| `ScoreWrapperMS1` | `CometScoresMS1` | `fDotProduct`, `fRTime`, `iScanNumber` |
| `FragmentWrapper` | `Fragment` | `Mass`, `MZ` (computed via `ToMz()`), `IsNeutralLossFragment`, `NeutralLossMass`, `Intensity`, `Number`, `Type` (`IonSeries` enum), `Charge` |

**Note:** If `CometData.h` enums or struct layouts change, `CometDataWrapper.h` must be updated in parallel. The file contains this reminder for `AnalysisType` and `InputType`.

### C# search loop (`SearchMS1MS2.cs`)

The loop is a **single-threaded preload pass followed by a purely in-memory parallel
search pass** -- not concurrent raw-file reads from worker threads. `rawFile` (the
Thermo reader) is touched only by the preload loop; by the time any worker thread
starts, all scan data already lives in memory:

```csharp
// Single-threaded: reads every scan from rawFile into preloadedScans[] up front.
// "no lock is needed because nothing else runs concurrently with this loop."
var preloadedScans = new PreloadedScan[totalScans];
for (int iScanNumber = iFirstScan; iScanNumber <= iLastScan; ++iScanNumber)
{
    // ... GetScanStatsForScanNumber / GetCentroidStream or GetSegmentedScanFromScanNumber /
    //     GetTrailerExtraInformation for precursor m/z + charge ...
    preloadedScans[iScanNumber - iFirstScan] = new PreloadedScan { ScanNumber = iScanNumber, Mass = pdMass, Intensity = pdInten, ... };
}

// Populate scan queue (numbers only -- data already in preloadedScans[])
for (int i = iFirstScan; i <= iLastScan; ++i)
    scanQueue.Enqueue(i);

void ProcessScans(int threadId)
{
    // Purely in-memory, no shared-resource locking: every scan's data was
    // already read by the single-threaded preload pass above.
    while (scanQueue.TryDequeue(out int iScanNumber))
    {
        PreloadedScan pre = preloadedScans[iScanNumber - iFirstScan];
        if (!pre.Valid || pre.NumPeaks < 10) continue;

        if (bPerformMS1Search && pre.ScanType == MSOrderType.Ms)
            globalSearchMgr.DoMS1SearchMultiResults(dMaxMS1RTDiff, dMaxQueryRT, iMS1TopN, pre.RT,
                pre.Mass, pre.Intensity, pre.NumPeaks, out List<ScoreWrapperMS1> vScores);
        else if (bPerformMS2Search && pre.ScanType == MSOrderType.Ms2)
            globalSearchMgr.DoSingleSpectrumSearchMultiResults(topN, pre.PrecursorCharge, pre.PrecursorMZ,
                pre.Mass, pre.Intensity, pre.NumPeaks, out ...);
        // ... result stored on a per-scan ScanResult, collected after all tasks join ...
    }
}

Task[] tasks = new Task[numThreads];
for (int i = 0; i < numThreads; ++i)
{
    int threadId = i;
    tasks[i] = Task.Run(() => ProcessScans(threadId));
}
```

`InitializeSingleSpectrumMS1Search`/`InitializeSingleSpectrumSearch` are called
once before the preload pass, not inside the per-scan loop. MS1 scans are only
fully read (profile trace, up to ~20K points) when `bPerformMS1Search` is on;
otherwise only cheap metadata (RT/scan type) is captured during preload so
downstream skip logic still works.

---

## Timeout mechanism

`g_staticParams.options.iMaxIndexRunTime` (milliseconds; 0 = no limit) limits fragment index search time per call.

The timeout clock is `pQuery->tSearchStart`, a `chrono::time_point` member set
once on the per-call `Query*` right after preprocessing (`CometSearchManager.cpp`,
inside `DoSingleSpectrumSearchMultiResults`). `RunSearch(Query*)` and
`SearchFragmentIndex(Query*, bool*)` take no separate time parameter -- they read
the deadline directly off `pQuery` at several checkpoints during the search, and
`DoSingleSpectrumSearchMultiResults` re-checks it again before each post-analysis
step (`CalculateSP`/`CalculateEValue`/`CalculateDeltaCn`), skipping ahead to
cleanup once the deadline has passed. Each concurrent call has an independent
clock via its own `Query*`; there is no shared timeout state.

---

## Memory model

**Per-call heap allocations:**

| Allocation | Freed at |
|-----------|----------|
| `Query* pQuery` | `cleanup_results` (`delete pQuery`; destructor frees nested arrays + mutex) |
| `QueryMS1* pQueryMS1` | End of `DoMS1SearchMultiResults` (`delete pQueryMS1`; also frees `pQueryMS1->pfFastXcorrData` if non-null) |

**Shared pools (allocated once at init or once per thread, reused across calls):**

- `CometPreprocess::GetRtsRawDataBuffer()` -- returns the thread-local raw-data buffer owned by a per-thread `RtsScratch` pool, initializing it on first use per thread. This replaced an earlier per-call `new[] iArraySizeGlobal doubles` / `delete[]` design (`pdTmpSpectrum` is no longer a per-call heap allocation).
- `CometSearch::AllocateMemory(N)` -- calls `s_pool.allocate(N, g_staticParams.iArraySizeGlobal, nBinnedIonMassesElems, nBinnedPrecursorNLElems)` (`CometSearch.cpp:63`; the latter two args size the PI_DB target/decoy scratch buffers) (`s_pool` is a file-static `SearchMemoryPool` instance in `CometSearch.cpp`; see `threading/SearchMemoryPool.h`) and aliases each slot's scratch buffer into `_ppbDuplFragmentArr[N][]`. `AcquirePoolSlot()` / `releaseSlot()` forward to `s_pool.acquireSlot()` / `s_pool.releaseSlot()`. Every acquire site wraps the slot in a `SearchMemoryPoolSlotGuard` so the slot is released on scope exit even if the search body throws. Must be valid before any call reaches `RunSearch(Query*, ...)`. If the index-build path was taken during init, this pool is freed inside `DoSearch()` and re-allocated by `InitializeSingleSpectrumSearch()` before proceeding.
- **Known limitation:** `s_pool` is a single process-wide instance, so it does not support multiple concurrent `ICometSearchManager` instances performing RTS searches against different fragment indexes in the same process -- see the `TODO` comment at the top of `CometSearch.cpp` and `docs/20260615_multiple_rts_instances.md`.

---

## Adding a new RTS entry point

For a new **search or per-spectrum call** that routes through `ICometSearchManager`:

1. Declare the method in `CometInterfaces.h` (`ICometSearchManager`).
2. Implement in `CometSearchManager.cpp` using the thread-local pattern:
   - Use `PreprocessSingleSpectrumThreadLocal()` (not `PreprocessSingleSpectrum()`).
   - Set `pQuery->tSearchStart` right after preprocessing, then call `CometSearch::RunSearch(pQuery)` (not `RunSearch(ThreadPool*)` or the pre-assigned-slot `RunSearch(Query*, int iSlot)` overload used by the fused batch path).
   - Never write `SearchSession` fields, `g_massRange`, or `g_staticParams` from within the call.
3. Add a managed wrapper method in `CometWrapper/CometWrapper.cpp` with `pin_ptr` for array parameters.
4. If new return types are needed, add wrapper structs to `CometDataWrapper.h` (and mirror in `CometData.h`).
5. Call from `RealtimeSearch/SearchMS1MS2.cs`.

For a **utility method** that does not route through `ICometSearchManager` (e.g. `GetPeakMemory()`):

1. Implement as a free function in the relevant `CometSearch/*.cpp` file. Do **not** include headers that pull in `CometDataInternal.h` directly into the C++/CLI translation unit -- instead expose a plain free function wrapper (e.g. `GetPeakMemoryStr()`) and forward-declare it in `CometWrapper.cpp`.
2. Declare and implement the managed method directly on `CometSearchManagerWrapper` in `CometWrapper.h` / `CometWrapper.cpp`.
3. Call from `RealtimeSearch/SearchMS1MS2.cs`.
