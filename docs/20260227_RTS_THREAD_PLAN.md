# Plan: Multithread `DoSingleSpectrumSearchMultiResults()` Calls

## Goal
Enable concurrent calls to `DoSingleSpectrumSearchMultiResults()` from `SearchMS1MS2.cs`
by eliminating the `g_pvQuery` serialization bottleneck in the native C++ search engine.

---

## Problem Summary

When `SearchMS1MS2.cs` launches N C# `Task` threads, they all call
`globalSearchMgr.DoSingleSpectrumSearchMultiResults()` concurrently via `CometWrapper.dll`.
Inside the native C++, every call serialized on `g_pvQueryMutex` because:

1. Each call pushed a `Query*` into the global `g_pvQuery` vector.
2. `CometSearch::RunSearch(ThreadPool*)` hardcoded `iWhichQuery = 0` into `g_pvQuery`.
3. `SearchFragmentIndex()` read `g_pvQuery.at(iWhichQuery)` — concurrent pushes broke indices.
4. Post-analysis (`CalculateSP`, `CalculateEValue`) also indexed into `g_pvQuery[0]`.
5. `CometSearch::_ppbDuplFragmentArr` is a static shared scratch buffer indexed by thread pool slot.

## What Is Already Thread-Safe (READ-ONLY After Init)

| Global                     | Safe? | Notes                                      |
|----------------------------|-------|--------------------------------------------|
| `g_staticParams`           | ✅     | Set once in `InitializeSingleSpectrumSearch()` |
| `g_iFragmentIndex`         | ✅     | Built once, never modified during search   |
| `g_vFragmentPeptides`      | ✅     | Built once                                 |
| `g_vRawPeptides`           | ✅     | Built once                                 |
| `g_pvProteinNames`         | ✅     | Built once                                 |
| `g_pvProteinsList`         | ✅     | Built once                                 |
| `g_pvQuery`                | ❌     | **SHARED MUTABLE — the bottleneck**        |
| `g_cometStatus`            | ❌     | Shared mutable error reporting             |
| `_ppbDuplFragmentArr`      | ❌     | Static per-thread-pool-slot scratch arrays |

---

## Task List

### Phase 1: Core C++ Changes — Eliminate `g_pvQuery` Dependency

#### Task 1.1: Add `SearchFragmentIndex(Query*, bool*)` Overload
- **File(s):** `CometSearch.h`, `CometSearch.cpp`
- **Description:** Create a new overload of `SearchFragmentIndex` that accepts a `Query*`
  directly instead of a `size_t iWhichQuery` index into `g_pvQuery`. The new overload
  replaces every `g_pvQuery.at(iWhichQuery)` reference with the passed-in `Query*`.
  The existing `SearchFragmentIndex(size_t, ThreadPool*)` remains unchanged for batch search.
  Timeout checking uses `pQuery->tSearchStart` (see Task 7.2) instead of the global
  `g_staticParams.tRealTimeStart`.
- **Key changes:**
  - Replace `g_pvQuery.at(iWhichQuery)->` with `pQuery->` throughout.
  - `pbDuplFragment` allocated per-call by the caller (`RunSearch(Query*)`).
  - `XcorrScoreI` calls pass `pQuery` instead of `iWhichQuery`.
  - Timeout checks use `pQuery->tSearchStart` instead of a parameter or global.
- **Status:** ✅ Complete — `SearchFragmentIndex(Query*, bool*)` declared in
  `CometSearch.h` and implemented in `CometSearch.cpp`.

#### Task 1.2: Add Thread-Local `XcorrScoreI` / `StorePeptideI` Overloads
- **File(s):** `CometSearch.h`, `CometSearch.cpp`
- **Description:** Add overloads of `XcorrScoreI` and `StorePeptideI` that accept `Query*`
  instead of indexing `g_pvQuery`. The `pQuery->accessMutex` lock is still used since
  multiple threads on the batch path might score into the same query object, but the
  global `g_pvQueryMutex` is not used.
- **Key changes:**
  - `XcorrScoreI(Query* pQuery, ...)` — replace `g_pvQuery.at(iWhichQuery)` with `pQuery`.
  - `StorePeptideI(Query* pQuery, ...)` — same replacement.
  - `CheckMassMatch(Query* pQuery, double dCalcPepMass)` — accept `Query*` directly.
- **Status:** ✅ Complete — `XcorrScoreI(Query*, ...)`, `StorePeptideI(Query*, ...)`, and
  `CheckMassMatch(Query*, double)` overloads all declared in `CometSearch.h` and implemented
  in `CometSearch.cpp`.

#### Task 1.3: Add Thread-Local `RunSearch(Query*)` Overload
- **File(s):** `CometSearch.h`, `CometSearch.cpp`
- **Description:** Add `static bool RunSearch(Query* pQuery)` that dispatches on
  `g_staticParams.iDbType` (see Task 7.3) to call either `SearchFragmentIndex` or
  `SearchPeptideIndex` with a per-call `pbDuplFragment` allocation. This is the entry
  point called by `DoSingleSpectrumSearchMultiResults`. Timeout checking uses
  `pQuery->tSearchStart` (see Task 7.2) which is set by the caller before invoking
  `RunSearch`.
- **Note on signature:** An earlier design passed `time_point tRealTimeStart` as a
  parameter; the final implementation stores it in `pQuery->tSearchStart` instead,
  keeping the call site cleaner and avoiding an extra parameter at every call level.
- **Status:** ✅ Complete — `RunSearch(Query*)` declared in `CometSearch.h` and
  implemented in `CometSearch.cpp`. Allocates per-call `pbDuplFragment[]`,
  dispatches to `SearchFragmentIndex(pQuery, pbDuplFragment)` or
  `SearchPeptideIndex(pQuery, pbDuplFragment)`, then frees the buffer.

#### Task 1.4: Add Thread-Local `SearchPeptideIndex(Query*, bool*)` Overload
- **File(s):** `CometSearch.h`, `CometSearch.cpp`
- **Description:** Analogous to Task 1.1 but for peptide index (`.idx`) searches.
  Accepts `Query*` and a per-call `pbDuplFragment` buffer allocated by `RunSearch(Query*)`.
  Does not access `g_pvQuery`.
- **Status:** ✅ Complete — `SearchPeptideIndex(Query*, bool*)` declared in `CometSearch.h`
  and implemented in `CometSearch.cpp`. Called from `RunSearch(Query*)` when
  `g_staticParams.iDbType == DbType::PI_DB`.

### Phase 2: Rewrite `DoSingleSpectrumSearchMultiResults` to Be Thread-Local

#### Task 2.1: Create Thread-Local `Query` Object
- **File(s):** `CometSearchManager.cpp`
- **Description:** In `DoSingleSpectrumSearchMultiResults`, create a heap-local
  `Query` object instead of pushing to `g_pvQuery`. Populate it with the input spectrum
  data via `PreprocessSingleSpectrumThreadLocal`. After the search, read results directly
  from the local `Query*` and populate output vectors.
- **Key changes:**
  - Remove `Threading::LockMutex(g_pvQueryMutex)` / `g_pvQuery.push_back()` / `pop_back()`.
  - Call `CometPreprocess::PreprocessSingleSpectrumThreadLocal()` into a local `Query*`.
  - Set `pQuery->tSearchStart = now()` after preprocessing (see Task 7.2).
  - Call `CometSearch::RunSearch(pQuery)`.
  - Call post-analysis on the local `Query*`.
  - Extract results from `pQuery->_pResults[]`.
  - Each concurrent call opens its own `FILE* fp` for protein name retrieval.
- **Status:** ✅ Complete — `DoSingleSpectrumSearchMultiResults` uses
  `PreprocessSingleSpectrumThreadLocal()` to create a caller-owned `Query*`, sets
  `pQuery->tSearchStart`, calls `CometSearch::RunSearch(pQuery)`, and runs thread-local
  post-analysis. No `g_pvQueryMutex` lock or `g_pvQuery` access remains on this path.

#### Task 2.2: Add Thread-Local Post-Analysis Functions
- **File(s):** `CometPostAnalysis.h`, `CometPostAnalysis.cpp`
- **Description:** Add overloads of `CalculateSP`, `CalculateEValue`, `CalculateDeltaCn`,
  `AnalyzeSP`, and `CalculateAScorePro` that accept `Query*` directly instead of indexing
  `g_pvQuery`.
- **Status:** ✅ Complete — `CalculateSP(Results*, Query*, int)`,
  `CalculateEValue(Query*, bool)`, `CalculateDeltaCn(Query*)`, `AnalyzeSP(Query*)`, and
  `CalculateAScorePro(Query*, AScoreDllInterface*)` overloads all declared in
  `CometPostAnalysis.h` and implemented in `CometPostAnalysis.cpp`. `CalculateAScorePro`
  is called conditionally: only when `iPrintAScoreProScore` is enabled and
  `pQuery->_pResults[0].cHasVariableMod == HasVariableModType_AScorePro`.

### Phase 3: Thread-Safe Scratch Memory

#### Task 3.1: Per-Call `pbDuplFragment` Allocation
- **File(s):** `CometSearch.cpp`
- **Description:** The new `RunSearch(Query*)` allocates `pbDuplFragment` on the heap
  per-call and passes it to `SearchFragmentIndex` / `SearchPeptideIndex`. Verify no
  static `_ppbDuplFragmentArr` is accessed on the RTS code path.
- **Status:** ✅ Verified — `RunSearch(Query*)` in `CometSearch.cpp` allocates
  `bool* pbDuplFragment = new bool[g_staticParams.iArraySizeGlobal]()` per-call and
  frees it after the search returns. The static `_ppbDuplFragmentArr` is only accessed
  on the batch path via `SearchForPeptides` called from `RunSearch(ThreadPool*)`.

### Phase 4: Wrapper Layer

#### Task 4.1: Verify `CometWrapper.cpp` Needs No Changes
- **File(s):** `CometWrapper.cpp`
- **Description:** The existing wrapper already pins managed arrays, creates local native
  vectors, and converts results per-call. No changes needed since threading happens at
  the C# level. Confirm the wrapper method calls the (now thread-safe) native method.
- **Status:** ✅ Verified — `CometWrapper.cpp` pins arrays with `pin_ptr`, creates
  stack-local `std::vector` containers, calls through the `ICometSearchManager` virtual
  interface (signature unchanged), and converts results to managed `List<>` objects.
  No shared mutable state exists in the wrapper layer.

### Phase 5: C# Caller

#### Task 5.1: Fix `dAScoreScore` → `dAScorePro` in `SearchMS1MS2.cs`
- **File(s):** `RealtimeSearch/SearchMS1MS2.cs`
- **Description:** The C# code references `result.Scores[0].dAScoreScore` which should
  be `result.Scores[0].dAScorePro` to match the corrected `ScoreWrapper` property name.
- **Status:** ✅ Complete — No `dAScoreScore` references exist in the codebase. The
  `ScoreWrapper` property is already named `dAScorePro` and all C# consumers use that name.

#### Task 5.2: Fix `dAScoreScore` → `dAScorePro` in `CometDataWrapper.h`
- **File(s):** `CometWrapper/CometDataWrapper.h`
- **Description:** Rename property `dAScoreScore` to `dAScorePro` in `ScoreWrapper`.
- **Status:** ✅ Complete — `ScoreWrapper` in `CometDataWrapper.h` already exposes
  `property double dAScorePro` backed by `pScores->dAScorePro`. Also exposes
  `property String^ sAScoreProSiteScores`. No stale naming exists.

#### Task 5.3: Add Default Constructors and Setters to Wrapper Classes
- **File(s):** `CometWrapper/CometDataWrapper.h`
- **Description:** Add default constructors and property setters to `ScoreWrapper` and
  `FragmentWrapper` if needed by the wrapper conversion code.
- **Status:** ✅ Not needed — `CometWrapper.cpp` constructs `ScoreWrapper` and
  `FragmentWrapper` via their existing copy-constructors. No default construction or
  property setting is required.

### Phase 6: Testing & Validation

#### Task 6.1: Single-Threaded Correctness — Code Review
- **Description:** Review the modified code path and verify structural correctness against
  the original serial implementation.
- **Status:** ✅ Complete — Code review confirmed:
  - `Query` constructor properly initializes all fields.
  - Thread-local path: `PreprocessSingleSpectrumThreadLocal()` → set `pQuery->tSearchStart`
    → `CometSearch::RunSearch(pQuery)` → `CalculateSP/CalculateEValue/CalculateDeltaCn(Query*)`
    → result extraction → `delete pQuery` — all use `pQuery->` throughout, never index `g_pvQuery`.
  - E-value computation: `CalculateEValue(Query*)` → `GenerateXcorrDecoys(Query*)` uses
    only `pQuery->` fields and the read-only static `decoyIons[]` array. Deterministic.
  - All post-analysis `Query*` overloads use the same algorithmic logic as the original
    `int iWhichQuery` counterparts, just replacing `g_pvQuery.at(i)->` with `pQuery->`.

#### Task 6.2: Multi-Threaded Safety — Code Review
- **Description:** Review with N threads for data races, shared mutable state, and
  determinism.
- **Status:** ✅ Complete — Two data races identified and fixed in Phase 7:
  1. **`g_massRange` concurrent writes** — Fixed in Task 7.1 (writes removed from RTS path).
  2. **`g_staticParams.tRealTimeStart` concurrent writes** — Fixed in Task 7.2
     (moved to per-call `pQuery->tSearchStart`).
  - All other shared state (`g_staticParams`, `g_iFragmentIndex`, `g_vFragmentPeptides`,
    `g_vRawPeptides`, `g_pvProteinNames`, `g_pvProteinsList`, `decoyIons[]`) confirmed
    read-only after initialization. ✅
  - `g_cometStatus` has unsynchronized reads/writes (pre-existing). Low impact on the
    RTS path: only `IsCancel()` is checked (not `IsError()`), preventing one thread's
    failure from cancelling all other concurrent searches.
  - Results are deterministic regardless of thread count: each thread operates on its
    own `Query*` with no cross-thread data dependencies.

#### Task 6.3: Memory Leak Check — Code Review
- **Description:** Verify that per-call `Query` objects and `pbDuplFragment` arrays
  are properly freed after each search call.
- **Status:** ✅ Complete — All allocations have matching deallocations:
  - `Query* pQuery` — `delete pQuery` at `cleanup_results` label. Destructor frees
    `ppfSparseSpScoreData`, `ppfSparseFastXcorrData`, `ppfSparseFastXcorrDataNL`,
    `_pResults[]`, `_pDecoys[]`, and destroys `accessMutex`.
  - `double* pdTmpSpectrum` — `delete[] pdTmpSpectrum` at `cleanup_results` label.
  - `bool* pbDuplFragment` — allocated in `RunSearch(Query*)`, freed with
    `delete[]` after the search function returns. Not accessible outside `RunSearch`.
  - `FILE* fp` — opened per-call for protein name retrieval; `fclose(fp)` is called
    within the result extraction block before `cleanup_results`.
  - All early-exit `goto cleanup_results` paths reach the deallocation code.

### Phase 7: Fix Remaining Data Races

#### Task 7.1: Remove Vestigial `g_massRange` Writes from RTS Path
- **File(s):** `CometSearchManager.cpp`
- **Description:** Remove the `g_massRange` assignments from
  `DoSingleSpectrumSearchMultiResults`. The `SearchFragmentIndex(Query*, bool*)`
  and `SearchPeptideIndex(Query*, bool*)` overloads read mass range from
  `pQuery->_pepMassInfo` directly, never from `g_massRange`.
- **Status:** ✅ Complete — `g_massRange.dMinMass`, `g_massRange.dMaxMass`, and
  `g_massRange.bNarrowMassRange` are not written in `DoSingleSpectrumSearchMultiResults`.
  `g_massRange` is only written during batch search and during `InitializeSingleSpectrumSearch`
  (`g_massRange.dMinMass/dMaxMass` set from `g_staticParams.options` and
  `g_massRange.uiMaxFragmentArrayIndex` set once at index load time).

#### Task 7.2: Replace `g_staticParams.tRealTimeStart` with `pQuery->tSearchStart`
- **File(s):** `CometSearchManager.cpp`, `CometSearch.cpp`, `CometDataInternal.h`
- **Description:** `g_staticParams.tRealTimeStart` was written at the start of every RTS
  call, creating a data race when multiple threads called `DoSingleSpectrumSearchMultiResults`
  concurrently. The fix moves the timeout clock to a per-`Query` field `tSearchStart`
  (a `chrono::high_resolution_clock::time_point` added to `struct Query`) set immediately
  after preprocessing, before `RunSearch(pQuery)` is called. Timeout checks inside
  `SearchFragmentIndex` and `SearchPeptideIndex` read `pQuery->tSearchStart` instead.
- **Note on design choice:** An earlier approach passed `time_point` as a parameter through
  `RunSearch(Query*, time_point)` → `SearchFragmentIndex(Query*, bool*, time_point)`. The
  final implementation stores it in `pQuery->tSearchStart` instead, which avoids threading
  the parameter through every call level and keeps `Query` self-contained.
- **Status:** ✅ Complete — `tSearchStart` field added to `struct Query` in
  `CometDataInternal.h`. Set in `DoSingleSpectrumSearchMultiResults` as
  `pQuery->tSearchStart = std::chrono::high_resolution_clock::now()` after preprocessing.
  `g_staticParams.tRealTimeStart` is no longer written or read on the RTS path.

#### Task 7.3: Add `DbType` Enum and Dispatch in `RunSearch(Query*)`
- **File(s):** `CometSearch.h`, `CometSearch.cpp`, `CometDataInternal.h`
- **Description:** `RunSearch(Query*)` needs to dispatch to the correct thread-local search
  function depending on whether the active database is a fragment index (`.fi`) or a peptide
  index (`.idx`). A `DbType` enum (`FI_DB`, `PI_DB`, `FASTA_DB`) stored in
  `g_staticParams.iDbType` provides the dispatch key.
  For `PI_DB`, the peptide index may not yet be loaded at first call (lazy loading via
  `ReadPeptideIndex()`); a double-checked lock on `g_pvDBIndexMutex` guards this one-time
  load safely under concurrent RTS calls.
- **Status:** ✅ Complete — `DbType` enum defined; `g_staticParams.iDbType` set during
  init. `RunSearch(Query*)` dispatches:
  - `FI_DB` → `SearchFragmentIndex(pQuery, pbDuplFragment)` (fragment index built at init)
  - `PI_DB` → double-checked lock → `ReadPeptideIndex()` if needed → `SearchPeptideIndex(pQuery, pbDuplFragment)`
  - Other → error via `g_cometStatus.SetStatus()`

---

## Architecture: Thread-Local RTS Flow

```
C# Task thread N
  │
  └─ DoSingleSpectrumSearchMultiResults(topN, charge, mz, peaks...)
        │
        ├─ Guard: singleSearchInitializationComplete.load(acquire)
        ├─ Guard: g_cometStatus.IsCancel()  [not IsError() — avoids poisoning all threads]
        │
        ├─ pdTmpSpectrum = new double[iArraySizeGlobal]     ← per-call
        ├─ pQuery = PreprocessSingleSpectrumThreadLocal(...)  ← per-call Query* on heap
        │     does NOT touch g_pvQuery
        ├─ pQuery->tSearchStart = now()                     ← per-call timeout clock
        │
        ├─ CometSearch::RunSearch(pQuery)
        │     ├─ pbDuplFragment = new bool[iArraySizeGlobal]  ← per-call
        │     ├─ if FI_DB:  SearchFragmentIndex(pQuery, pbDuplFragment)
        │     │     reads g_iFragmentIndex / g_vFragmentPeptides  [READ-ONLY] ✅
        │     │     XcorrScoreI(pQuery, ...)  → pQuery->_pResults only
        │     │     timeout via pQuery->tSearchStart
        │     ├─ if PI_DB:  [double-checked lock] ReadPeptideIndex() if needed
        │     │             SearchPeptideIndex(pQuery, pbDuplFragment)
        │     │     reads g_pvDBIndex  [READ-ONLY after load] ✅
        │     └─ delete[] pbDuplFragment
        │
        ├─ CalculateSP(pQuery->_pResults, pQuery, N)        ← pQuery only
        ├─ CalculateEValue(pQuery, false)                   ← pQuery only
        ├─ CalculateDeltaCn(pQuery)                         ← pQuery only
        ├─ CalculateAScorePro(pQuery, g_AScoreInterface)    ← conditional; g_AScoreInterface READ-ONLY ✅
        │
        ├─ fp = fopen(szDatabase, "rb")                     ← per-call FILE*
        │     reads g_pvProteinsList  [READ-ONLY] ✅
        ├─ extract results → output vectors
        ├─ fclose(fp)
        │
        └─ cleanup_results:
               delete pQuery
               delete[] pdTmpSpectrum
```

Globals written per-call on this path: **none**. All state is either read-only after init
or owned by the per-call `Query*`.
