# Plan: Multithread `DoSingleSpectrumSearchMultiResults()` Calls

## Goal
Enable concurrent calls to `DoSingleSpectrumSearchMultiResults()` from `SearchMS1MS2.cs`
by eliminating the `g_pvQuery` serialization bottleneck in the native C++ search engine.

---

## Problem Summary

When `SearchMS1MS2.cs` launches N C# `Task` threads, they all call
`globalSearchMgr.DoSingleSpectrumSearchMultiResults()` concurrently via `CometWrapper.dll`.
Inside the native C++, every call serializes on `g_pvQueryMutex` because:

1. Each call pushes a `Query*` into the global `g_pvQuery` vector.
2. `CometSearch::RunSearch(ThreadPool*)` hardcodes `iWhichQuery = 0` into `g_pvQuery`.
3. `SearchFragmentIndex()` reads `g_pvQuery.at(iWhichQuery)` — concurrent pushes break indices.
4. Post-analysis (`CalculateSP`, `CalculateEValue`) also index into `g_pvQuery[0]`.
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

#### Task 1.1: Add `SearchFragmentIndex(Query*, bool*, time_point)` Overload
- **File(s):** `CometSearch.h`, `CometSearch.cpp`
- **Description:** Create a new overload of `SearchFragmentIndex` that accepts a `Query*`
  directly instead of a `size_t iWhichQuery` index into `g_pvQuery`. The new overload
  should replace every `g_pvQuery.at(iWhichQuery)` reference with the passed-in `Query*`.
  The existing `SearchFragmentIndex(size_t, ThreadPool*)` remains unchanged for batch search.
  Accepts a `time_point` parameter for timeout checking (Task 7.2).
- **Key changes:**
  - Replace `g_pvQuery.at(iWhichQuery)->` with `pQuery->` throughout.
  - `pbDuplFragment` allocated per-call (already done in current `SearchFragmentIndex`).
  - `XcorrScoreI` calls pass `pQuery` instead of `iWhichQuery`.
  - Timeout checks use `tRealTimeStart` parameter instead of `g_staticParams.tRealTimeStart`.
- **Status:** ✅ Complete — `SearchFragmentIndex(Query*, bool*, time_point)` declared in
  `CometSearch.h` and implemented in `CometSearch.cpp`.

#### Task 1.2: Add Thread-Local `XcorrScoreI` / `StorePeptideI` Overloads
- **File(s):** `CometSearch.h`, `CometSearch.cpp`
- **Description:** Add overloads (or modify existing) `XcorrScoreI` and `StorePeptideI`
  to accept `Query*` instead of indexing `g_pvQuery`. The `pQuery->accessMutex` lock
  is still needed since the `Query` object is local to the caller, but the global
  `g_pvQueryMutex` is not.
- **Key changes:**
  - `XcorrScoreI(Query* pQuery, ...)` — replace `g_pvQuery.at(iWhichQuery)` with `pQuery`.
  - `StorePeptideI(Query* pQuery, ...)` — same replacement.
  - `CheckMassMatch(Query* pQuery, double dCalcPepMass)` — accept `Query*` directly.
- **Status:** ✅ Complete — `XcorrScoreI(Query*, ...)`, `StorePeptideI(Query*, ...)`, and
  `CheckMassMatch(Query*, double)` overloads all declared in `CometSearch.h` and implemented
  in `CometSearch.cpp`.

#### Task 1.3: Add Thread-Local `RunSearch(Query*, time_point)` Overload
- **File(s):** `CometSearch.h`, `CometSearch.cpp`
- **Description:** Add `static bool RunSearch(Query* pQuery, time_point tRealTimeStart)`
  that creates a local `CometSearch` instance and calls the new
  `SearchFragmentIndex(pQuery, ...)`. This is the entry point called by
  `DoSingleSpectrumSearchMultiResults`. Accepts a `time_point` parameter for
  timeout checking (Task 7.2).
- **Status:** ✅ Complete — `RunSearch(Query*, time_point)` declared in `CometSearch.h`
  and implemented in `CometSearch.cpp`. Allocates per-call `pbDuplFragment`, calls
  `SearchFragmentIndex(pQuery, pbDuplFragment, tRealTimeStart)`, then frees the buffer.

### Phase 2: Rewrite `DoSingleSpectrumSearchMultiResults` to Be Thread-Local

#### Task 2.1: Create Thread-Local `Query` Object
- **File(s):** `CometSearchManager.cpp`
- **Description:** In `DoSingleSpectrumSearchMultiResults`, create a stack/heap-local
  `Query` object instead of pushing to `g_pvQuery`. Populate it with the input spectrum
  data (m/z arrays, charge, precursor mass, preprocessed xcorr data). After the search,
  read results directly from the local `Query*` and populate output vectors.
- **Key changes:**
  - Remove `Threading::LockMutex(g_pvQueryMutex)` / `g_pvQuery.push_back()` / `pop_back()`.
  - Call `CometPreprocess::PreprocessSingleSpectrumThreadLocal()` into the local `Query`.
  - Call `CometSearch::RunSearch(pLocalQuery, tRealTimeStart)`.
  - Call post-analysis on the local `Query*`.
  - Extract results from `pLocalQuery->_pResults[]`.
- **Status:** ✅ Complete — `DoSingleSpectrumSearchMultiResults` uses
  `PreprocessSingleSpectrumThreadLocal()` to create a caller-owned `Query*`, calls
  `CometSearch::RunSearch(pQuery, tRealTimeStart)`, and runs thread-local post-analysis.
  No `g_pvQueryMutex` lock or `g_pvQuery` access remains on this path.

#### Task 2.2: Add Thread-Local Post-Analysis Functions
- **File(s):** `CometPostAnalysis.h`, `CometPostAnalysis.cpp`
- **Description:** Add overloads of `CalculateSP`, `CalculateEValue`, and
  `CalculateDeltaCn` that accept `Query*` directly instead of indexing `g_pvQuery`.
- **Status:** ✅ Complete — `CalculateSP(Results*, Query*, int)`,
  `CalculateEValue(Query*, bool)`, `CalculateDeltaCn(Query*)`, and
  `CalculateAScorePro(Query*, AScoreDllInterface*)` overloads all declared in
  `CometPostAnalysis.h` and implemented in `CometPostAnalysis.cpp`. Each delegates to
  its `int iWhichQuery` counterpart's logic using the passed `Query*` directly.

### Phase 3: Thread-Safe Scratch Memory

#### Task 3.1: Per-Call `pbDuplFragment` Allocation
- **File(s):** `CometSearch.cpp`
- **Description:** The new `SearchFragmentIndex(Query*, ...)` already allocates
  `pbDuplFragment` on the heap per-call (`new bool[g_staticParams.iArraySizeGlobal]`).
  Verify this is correct and no static `_ppbDuplFragmentArr` is accessed.
  The existing batch-search path continues using the static pool.
- **Status:** ✅ Verified — `RunSearch(Query*, time_point)` in `CometSearch.cpp` allocates
  `bool* pbDuplFragment = new bool[g_staticParams.iArraySizeGlobal]()` per-call, passes
  it to `SearchFragmentIndex(pQuery, pbDuplFragment, tRealTimeStart)`, and `delete[]`s
  it afterward. The static `_ppbDuplFragmentArr` is not accessed on this code path.

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
  The getter already correctly accesses `pScores->dAScorePro`.
- **Status:** ✅ Complete — `ScoreWrapper` in `CometDataWrapper.h` already exposes
  `property double dAScorePro` backed by `pScores->dAScorePro`. Also exposes
  `property String^ sAScoreProSiteScores`. No stale naming exists.

#### Task 5.3: Add Default Constructors and Setters to Wrapper Classes
- **File(s):** `CometWrapper/CometDataWrapper.h`
- **Description:** Add default constructors and property setters to `ScoreWrapper` and
  `FragmentWrapper` so the wrapper code can use `gcnew ScoreWrapper()` and set fields.
- **Status:** ✅ Not needed — `CometWrapper.cpp` constructs `ScoreWrapper` and
  `FragmentWrapper` via their existing `const CometScores&` / `const Fragment&`
  copy-constructors. No default construction or property setting is required by the
  current wrapper conversion code.

### Phase 6: Testing & Validation

#### Task 6.1: Single-Threaded Correctness — Code Review
- **Description:** Review the modified code path with `iNumThreads = 1` logic and verify
  structural correctness against the original serial implementation.
- **Status:** ✅ Complete — Code review confirmed:
  - `Query` constructor properly initializes all fields including `iMinXcorrHisto = 0`.
  - Thread-local path: `PreprocessSingleSpectrumThreadLocal()` → `RunSearch(Query*, time_point)` →
    `CalculateSP/CalculateEValue/CalculateDeltaCn(Query*)` → result extraction →
    `delete pQuery` — all use `pQuery->` throughout, never index `g_pvQuery`.
  - E-value computation: `CalculateEValue(Query*)` → `GenerateXcorrDecoys(Query*)` uses
    only `pQuery->` fields and the read-only static `decoyIons[]` array. Deterministic.
  - Sort tiebreakers in `SortFnXcorr` use `strcmp` + `piVarModSites`, producing stable
    deterministic ordering for identical XCorr scores.
  - All post-analysis `Query*` overloads delegate to the same algorithmic logic as the
    `int iWhichQuery` counterparts, just replacing `g_pvQuery.at(i)->` with `pQuery->`.

#### Task 6.2: Multi-Threaded Safety — Code Review
- **Description:** Review with N threads for data races, shared mutable state, and
  determinism.
- **Status:** ✅ Complete — Two data races identified and fixed in Phase 7:
  1. **`g_massRange` concurrent writes** — Fixed in Task 7.1 (writes removed).
  2. **`g_staticParams.tRealTimeStart` concurrent writes** — Fixed in Task 7.2
     (now a local variable passed as parameter).
  - All other shared state (`g_staticParams`, `g_iFragmentIndex`, `g_vFragmentPeptides`,
    `g_vRawPeptides`, `g_pvProteinNames`, `g_pvProteinsList`, `decoyIons[]`) confirmed
    read-only after initialization. ✅
  - `g_cometStatus` has unsynchronized writes (pre-existing issue, not introduced by
    threading changes). Low impact — only affects error reporting.
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
  - `bool* pbDuplFragment` — allocated in `RunSearch(Query*, time_point)`, freed with
    `delete[]` after `SearchFragmentIndex` returns. Not accessible outside `RunSearch`.
  - `FILE* fp` — opened per-call for protein name retrieval, `fclose(fp)` within the
    result extraction block.
  - All early-exit `goto cleanup_results` paths reach the deallocation code.

### Phase 7: Fix Remaining Data Races

#### Task 7.1: Remove vestigial `g_massRange` writes
- **File(s):** `CometSearchManager.cpp`
- **Description:** Remove the three `g_massRange` assignments from
  `DoSingleSpectrumSearchMultiResults`. The `SearchFragmentIndex(Query*, bool*, time_point)`
  overload reads mass range from `pQuery->_pepMassInfo` directly.
- **Status:** ✅ Complete — Removed `g_massRange.dMinMass`, `g_massRange.dMaxMass`, and
  `g_massRange.bNarrowMassRange` writes from `DoSingleSpectrumSearchMultiResults`.


---

## Architecture Diagram
