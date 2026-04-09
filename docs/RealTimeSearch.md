# Real-Time Search (RTS) Architecture

Comet supports two search modes:

- **Batch search**: `DoSearch()` — reads a file, processes spectra in configurable batches, writes result files.
- **Real-time search (RTS)**: called per-spectrum by an external C# application; returns results synchronously within the same call. Designed for concurrent calls from multiple threads.

This document covers the RTS path. The design history and task-by-task implementation record are in `docs/20260227_RTS_THREAD_PLAN.md` (MS2) and `docs/20260228_MS1_THREAD_PLAN.md` (MS1).

---

## Entry points

All entry points are declared in `CometSearch/CometInterfaces.h` as virtual methods on `ICometSearchManager` and implemented in `CometSearchManager`. They are exposed to C# through `CometWrapper/CometSearchManagerWrapper` (a C++/CLI ref class) and called from `RealtimeSearch/SearchMS1MS2.cs`.

### MS2 fragment index search

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
    vector<Scores>& scores);
```

Returns up to `topN` hits. Safe for concurrent calls from multiple threads against the same `ICometSearchManager` instance.

### MS1 spectral library search

```cpp
bool InitializeSingleSpectrumMS1Search(const double dMaxQueryRT);
void FinalizeSingleSpectrumMS1Search();
bool DoMS1SearchMultiResults(
    const double dMaxMS1RTDiff,
    const int iPrecursorCharge,
    const double dMZ,
    double* pdMass, double* pdInten, int iNumPeaks,
    vector<Scores>& scores);
```

Scores against the in-memory spectral library (`g_vSpecLib`). Also safe for concurrent calls; uses `g_ms1AlignerMutex` only for the RT alignment history update.

---

## Initialization

### MS2 (`InitializeSingleSpectrumSearch`)

Uses a double-checked locking pattern with `std::atomic<bool> singleSearchInitializationComplete`:

```
fast path: singleSearchInitializationComplete.load(acquire) → return true if set
slow path: mutex-guarded check + initialization
  → InitializeStaticParams()       populates g_staticParams
  → ValidateSequenceDatabaseFile() validates FASTA / index
  → ReadFragmentIndex()            loads g_iFragmentIndex, g_vFragmentPeptides, g_vRawPeptides
  → AllocateMemory()               preprocessing + search thread buffers
  → singleSearchInitializationComplete.store(true, release)
```

The `release` store ensures all threads that subsequently load the flag with `acquire` see a fully initialized `g_iFragmentIndex` and all other globals.

### MS1 (`InitializeSingleSpectrumMS1Search`)

Same double-checked pattern with `singleSearchMS1InitializationComplete`:

```
  → InitializeSingleSpectrumSearch()   ensures MS2 init is also complete
  → CometSpecLib::LoadSpecLib()        loads g_vSpecLib + g_vulSpecLibPrecursorIndex
  → CometAlignment::InitAlignment()   sets up RT aligner
  → singleSearchMS1InitializationComplete.store(true, release)
```

**Important:** `InitializeSingleSpectrumMS1Search()` must return before any thread calls `DoMS1SearchMultiResults()`. This is a precondition enforced by the C# caller, not by the C++ code itself.

`FinalizeSingleSpectrumSearch()` and `FinalizeSingleSpectrumMS1Search()` free allocated memory and reset the atomic flags. Call them when the search session ends.

---

## Per-call flow

### MS2 (`DoSingleSpectrumSearchMultiResults`)

```
DoSingleSpectrumSearchMultiResults(topN, charge, mz, masses, intensities, nPeaks, ...)
  │
  ├─ Guard: singleSearchInitializationComplete.load(acquire) — abort if not ready
  ├─ Guard: IsCancel() — abort if cancelled (not IsError(), which would poison all threads)
  │
  ├─ tRealTimeStart = now()                         ← LOCAL variable; no shared write
  │
  ├─ CometPreprocess::PreprocessSingleSpectrumThreadLocal(charge, mz, masses, intensities)
  │     → allocates caller-owned Query* on the heap
  │     → fills it with binned spectrum data
  │     → does NOT touch g_pvQuery
  │     → returns nullptr on failure (caller checks and returns false)
  │
  ├─ pdTmpSpectrum = new double[iArraySize]          ← per-call allocation
  │
  ├─ CometSearch::RunSearch(pQuery, tRealTimeStart)
  │     → allocates per-call bool* pbDuplFragment[]
  │     → SearchFragmentIndex(pQuery, pbDuplFragment, tRealTimeStart)
  │           reads g_iFragmentIndex / g_vFragmentPeptides (READ-ONLY) ✅
  │           XcorrScoreI(pQuery, ...) — updates only pQuery->_pResults
  │           CheckMassMatch(pQuery, dMass) — reads only pQuery->_pepMassInfo
  │           timeout checked against local tRealTimeStart
  │     → delete[] pbDuplFragment
  │
  ├─ CometPostAnalysis::CalculateSP(pQuery->_pResults, pQuery, iSize)
  ├─ CometPostAnalysis::CalculateEValue(pQuery, bSkip)
  ├─ CometPostAnalysis::CalculateDeltaCn(pQuery)
  ├─ CometPostAnalysis::CalculateAScorePro(pQuery, g_AScoreInterface)
  │
  ├─ sort _pResults by XCorr, extract top topN hits into output vectors
  │
  └─ cleanup_results:
       delete pQuery          (destructor frees sparse arrays, _pResults[], accessMutex)
       delete[] pdTmpSpectrum
       return
```

### MS1 (`DoMS1SearchMultiResults`)

```
DoMS1SearchMultiResults(dMaxMS1RTDiff, charge, mz, masses, intensities, nPeaks, ...)
  │
  ├─ Guard: singleSearchMS1InitializationComplete.load(acquire)
  │
  ├─ CometPreprocess::PreprocessMS1SingleSpectrumThreadLocal(charge, mz, masses, intensities)
  │     → allocates caller-owned QueryMS1* on the heap
  │     → does NOT touch g_pvQueryMS1
  │
  ├─ CometSpecLib::RunMS1Search(pQueryMS1, ...)
  │     reads g_vSpecLib / g_vulSpecLibPrecursorIndex (READ-ONLY) ✅
  │
  ├─ Threading::LockMutex(g_ms1AlignerMutex)
  │     CometAlignment::UpdateAlignment(RetentionMatchHistory, ...)  ← guarded write
  │   Threading::UnlockMutex(g_ms1AlignerMutex)
  │
  ├─ extract results from pQueryMS1->_pSpecLibResultsMS1 into output vectors
  │
  └─ delete pQueryMS1
       return
```

---

## Thread-safety of concurrent RTS calls

| State | RTS path | Notes |
|-------|:--------:|-------|
| `g_staticParams` | Read-only ✅ | Set once at init; never written during search. |
| `g_iFragmentIndex` / `g_vFragmentPeptides` / `g_vRawPeptides` | Read-only ✅ | Loaded at init; never modified. |
| `g_vSpecLib` / `g_vulSpecLibPrecursorIndex` | Read-only ✅ | Loaded at init. |
| `g_pvProteinNames` / `g_pvProteinsList` | Read-only ✅ | Loaded at init. |
| `g_AScoreOptions` / `g_AScoreInterface` | Read-only ✅ | Pointer set at init; each call uses its own data. |
| `g_pvQuery` / `g_pvQueryMS1` | Not touched ✅ | RTS path uses per-call `Query*` / `QueryMS1*`. |
| `g_massRange` | Not written ✅ | Mass limits derived from per-call `Query*._pepMassInfo`. |
| `tRealTimeStart` | Per-call local ✅ | Each call has its own `chrono::time_point`. |
| `Query*` / `QueryMS1*` | Per-call heap ✅ | Each call allocates and owns its object; freed at end. |
| `pbDuplFragment[]` | Per-call heap ✅ | Allocated in `RunSearch`, freed before return. |
| `RetentionMatchHistory` | Mutex-guarded ✅ | `g_ms1AlignerMutex` protects writes in `DoMS1SearchMultiResults`. |
| `g_cometStatus` | Shared mutable ⚠️ | Pre-existing; `IsCancel()` is safe, `SetError()` is mutex-protected. RTS path checks `IsCancel()` only — avoids poisoning all threads on one failure. |

---

## C# / wrapper integration

### Layer stack

```
RealtimeSearch/SearchMS1MS2.cs          (C# application)
      ↓  calls via COM-visible interface
CometWrapper/CometSearchManagerWrapper  (C++/CLI ref class, managed heap)
      ↓  calls native C++
CometSearch/CometSearchManager          (native library)
```

### CometWrapper marshaling

`CometSearchManagerWrapper` holds a native `ICometSearchManager*` and marshals per-call:
- Managed `array<double>^` ↔ native `double*` via `pin_ptr` (zero-copy).
- Native `Scores` → managed `ScoreWrapper` via copy-constructor.
- Native `Fragment` → managed `FragmentWrapper` via copy-constructor.
- Native `string` ↔ managed `String^` via `marshal_as<>`.

No shared mutable state exists in the wrapper layer. Each managed call creates stack-local native containers for results.

**Key wrapper types (`CometDataWrapper.h`):**

| Wrapper type | Native type | Key properties |
|---|---|---|
| `ScoreWrapper` | `Scores` | `xCorr`, `dCn`, `dExpect`, `mass`, `matchedIons`, `totalIons`, `dAScorePro`, `sAScoreProSiteScores` |
| `FragmentWrapper` | `Fragment` | `mass`, `intensity`, `type`, `number`, `charge`, `neutralLoss`, `neutralLossMass` |

**Note:** If `CometData.h` enums or struct layouts change, `CometDataWrapper.h` must be updated in parallel. The file contains this reminder for `AnalysisType` and `InputType`.

### C# search loop (`SearchMS1MS2.cs`)

Multiple `Task.Run()` workers share a single `globalSearchMgr` instance:

```csharp
// Concurrent Task threads — safe because native C++ is fully thread-local:
Parallel.ForEach(scanQueue, scan => {
    globalSearchMgr.DoSingleSpectrumSearchMultiResults(charge, mz, masses, ...);
    // result stored into ConcurrentBag<ScanResult>
});
```

MS1 searches run similarly via `DoMS1SearchMultiResults`, with `InitializeSingleSpectrumMS1Search` called once before the loop.

---

## Timeout mechanism

`g_staticParams.options.iMaxIndexRunTime` (milliseconds; 0 = no limit) limits fragment index search time per call.

The timeout clock is a `chrono::time_point tRealTimeStart` local to each call, passed through `RunSearch(pQuery, tRealTimeStart)` → `SearchFragmentIndex(pQuery, pbDuplFragment, tRealTimeStart)`. Each concurrent call has an independent clock; there is no shared timeout state.

---

## Memory model

**Per-call heap allocations:**

| Allocation | Freed at |
|-----------|----------|
| `Query* pQuery` | `cleanup_results` (`delete pQuery`; destructor frees nested arrays + mutex) |
| `QueryMS1* pQueryMS1` | End of `DoMS1SearchMultiResults` (`delete pQueryMS1`) |
| `double* pdTmpSpectrum` | `cleanup_results` |
| `bool* pbDuplFragment[]` | End of `RunSearch(Query*, ...)` |

**Shared pools (allocated once at init, reused across calls):**

- `CometPreprocess::AllocateMemory(N)` — per-thread preprocessing buffers for the batch path. The RTS thread-local path bypasses this pool and allocates directly.
- `CometSearch::AllocateMemory(N)` — per-thread search buffers. Each `CometSearch` instance constructed in `RunSearch(Query*, ...)` uses its own member arrays, not the shared pool.

---

## Adding a new RTS entry point

1. Declare the method in `CometInterfaces.h` (`ICometSearchManager`).
2. Implement in `CometSearchManager.cpp` using the thread-local pattern:
   - Use `PreprocessSingleSpectrumThreadLocal()` (not `PreprocessSingleSpectrum()`).
   - Call `CometSearch::RunSearch(pQuery, tRealTimeStart)` (not `RunSearch(ThreadPool*)`).
   - Never write `g_pvQuery`, `g_massRange`, or `g_staticParams` from within the call.
3. Add a managed wrapper method in `CometWrapper/CometWrapper.cpp` with `pin_ptr` for array parameters.
4. If new return types are needed, add wrapper structs to `CometDataWrapper.h` (and mirror in `CometData.h`).
5. Call from `RealtimeSearch/SearchMS1MS2.cs`.
