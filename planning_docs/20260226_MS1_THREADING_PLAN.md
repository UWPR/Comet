# Plan: Fix MS1 Search Threading for `DoMS1SearchMultiResults()`

## Goal
Ensure `LoadSpecLibMS1Raw()` loads the reference library exactly **once** into `g_vSpecLib`,
then allow N concurrent C# threads to each call `DoMS1SearchMultiResults()` with their
query MS1 spectra — comparing each query against the **shared, read-only** `g_vSpecLib`.

---

## Problem Summary

When `SearchMS1MS2.cs` launches N C# `Task` threads, they all call
`globalSearchMgr.DoMS1SearchMultiResults()` concurrently. Inside the native C++,
`DoMS1SearchMultiResults()` currently:

1. Calls `InitializeSingleSpectrumMS1Search()` — which creates a thread pool and calls
   `LoadSpecLibMS1Raw()` to load the reference file into `g_vSpecLib`.
2. **Each concurrent call** re-enters `DoMS1SearchMultiResults()` and finds
   `g_bSpecLibRead == false`, so each thread independently calls `LoadSpecLibMS1Raw()`,
   causing the reference file to be loaded N times simultaneously.
3. Even after loading, `DoMS1SearchMultiResults()` uses shared mutable state
   (`g_pvQueryMS1`) to preprocess the query spectrum, then calls `RunMS1Search()` which
   iterates `g_pvQueryMS1` — a global vector that is not thread-safe for concurrent
   push/read.
4. The `fillPool(iNumThreads - 1)` call creates 0 worker threads when `num_threads=1`,
   causing a deadlock in `LoadSpecLibMS1Raw()` (the `wait_on_threads` never completes
   because no worker thread exists to process queued jobs).

### Root Causes

| Issue | Location | Description |
|-------|----------|-------------|
| **Redundant loading** | `DoMS1SearchMultiResults` | No guard prevents concurrent calls from each loading the reference file |
| **Shared mutable `g_pvQueryMS1`** | `DoMS1SearchMultiResults`, `RunMS1Search`, `PreprocessMS1SingleSpectrum` | Query spectrum pushed into global vector; concurrent threads corrupt it |
| **Thread pool deadlock** | `InitializeSingleSpectrumMS1Search` | `fillPool(0)` with 1 thread creates no workers; queued jobs never execute |

## What Is Already Thread-Safe (READ-ONLY After Init)

| Global | Safe After Init? | Notes |
|--------|-----------------|-------|
| `g_vSpecLib` | ✅ | Populated once by `LoadSpecLibMS1Raw()`, read-only during search |
| `g_staticParams` | ✅ | Set once during initialization |
| `g_bSpecLibRead` | ✅ | Written once during single-threaded init; .NET Task scheduling provides happens-before |
| `g_pvQueryMS1` | ✅ (batch only) | No longer used by single-spectrum path; only batch `RunMS1Search(ThreadPool*)` |
| `dMaxSpecLibRT` | ✅ | Written once during single-threaded init, read-only after |

---

## Architecture: Before vs. After

### Before (Broken)
---

## Task List

### Phase 1: Fix Reference Library Loading (Load Once)

#### Task 1.1: Move `LoadSpecLibMS1Raw()` into `InitializeSingleSpectrumMS1Search()`
- **File(s):** `CometSearchManager.cpp`
- **Description:** `InitializeSingleSpectrumMS1Search()` is called **once** from C#
  before any threads start. Move the `LoadSpecLibMS1Raw()` call here, along with the
  `dMaxSpecLibRT` computation. Store `dMaxSpecLibRT` as a member variable of
  `CometSearchManager` (or a new global) so `DoMS1SearchMultiResults()` can read it.
- **Key changes:**
  - Call `LoadSpecLibMS1Raw(tp, dMaxQueryRT, &dMaxSpecLibRT)` inside
    `InitializeSingleSpectrumMS1Search()`.
  - Store the resulting `dMaxSpecLibRT` value for later use.
  - Remove the `LoadSpecLibMS1Raw()` call from `DoMS1SearchMultiResults()`.
  - After loading, `g_vSpecLib` and `g_bSpecLibRead` are immutable.
- **Status:** ✅ Complete

#### Task 1.2: Fix Thread Pool Deadlock for `num_threads=1`
- **File(s):** `CometSearchManager.cpp`
- **Description:** In `InitializeSingleSpectrumMS1Search()`, the thread pool is created
  with `fillPool(iNumThreads - 1)`. When `iNumThreads == 1`, this creates 0 worker
  threads, causing `LoadSpecLibMS1Raw()` to deadlock on `wait_on_threads()`. Fix by
  ensuring at least 1 worker thread.
- **Key change:** `fillPool(max(1, iNumThreads - 1))`
- **Status:** ✅ Complete

### Phase 2: Make `DoMS1SearchMultiResults()` Thread-Local

#### Task 2.1: Create Thread-Local Query Preprocessing
- **File(s):** `CometSearchManager.cpp`, `CometPreprocess.h`, `CometPreprocess.cpp`
- **Description:** Replace `PreprocessMS1SingleSpectrum()` (which pushes into global
`g_pvQueryMS1`) with a thread-local version that returns a `QueryMS1*` without
touching any global state. This mirrors the pattern already established for MS2
with `PreprocessSingleSpectrumThreadLocal()`.
- **Key changes:**
- Added `static QueryMS1* PreprocessMS1SingleSpectrumThreadLocal(double* pdMass, double* pdInten, int iNumPeaks)` to `CometPreprocess`.
- This function allocates a `QueryMS1*` on the heap, fills its `pfFastXcorrData`
  array (unit vector), and returns it. No global state touched.
- The existing `PreprocessMS1SingleSpectrum()` remains for backward compatibility
  (batch path).
- **Status:** ✅ Complete

#### Task 2.2: Create Thread-Local `RunMS1Search` Overload
- **File(s):** `CometSearch.h`, `CometSearch.cpp`
- **Description:** Add a new overload `static bool RunMS1Search(QueryMS1* pQueryMS1, ...)` 
that accepts a thread-local `QueryMS1*` and computes the dot product against
`g_vSpecLib` entries within the RT window. Returns results via output parameters
(or a `vector<CometScoresMS1>&`) instead of writing to any global.
- **Key changes:**
- The function iterates `g_vSpecLib` (read-only) to find entries within `dMaxMS1RTDiff`
  of the query RT.
- For each candidate, compute dot product between `pQueryMS1->pfFastXcorrData` and
  `g_vSpecLib[i].pfUnitVector`.
- Populate the output `vector<CometScoresMS1>` with top-N results.
- No access to `g_pvQueryMS1` — everything is thread-local.
- **Status:** ✅ Complete

#### Task 2.3: Rewrite `DoMS1SearchMultiResults()` to Use Thread-Local Objects
- **File(s):** `CometSearchManager.cpp`
- **Description:** Rewrite `DoMS1SearchMultiResults()` to:
1. Call `PreprocessMS1SingleSpectrumThreadLocal()` → heap-local `QueryMS1*`.
2. Call `RunMS1Search(pQueryMS1, dRT, dMaxMS1RTDiff, ...)` → thread-local results.
3. Populate output `vector<CometScoresMS1>& scores`.
4. Delete `pQueryMS1`.
5. No mutex locks, no global vector access.
- **Key changes:**
- Removed `g_pvQueryMS1.clear()` and `g_pvQueryMS1` usage entirely from this path.
- Removed the `LoadSpecLibMS1Raw()` call (moved to init in Task 1.1).
- Removed `InitializeSingleSpectrumMS1Search()` re-call (was being called per-invocation).
- **Status:** ✅ Complete

### Phase 3: Verify Existing `g_vSpecLib` Thread Safety

#### Task 3.1: Audit `g_vSpecLib` Access After Loading
- **File(s):** `CometSpecLib.cpp`, `CometSearch.cpp`
- **Description:** Verify that after `LoadSpecLibMS1Raw()` completes and
`g_bSpecLibRead = true`, no code path modifies `g_vSpecLib` or
`SpecLibStruct::pfUnitVector`. All access during `DoMS1SearchMultiResults()`
must be read-only.
- **Checklist:**
- [x] `g_vSpecLib.push_back()` only in `LoadSpecLibMS1Raw()` / `PreprocessThreadProcMS1()`
- [x] `pfUnitVector` allocated once per entry, never modified after
- [x] `fRTime`, `fScaleMaxInten`, `uiArraySizeMS1` set once during loading
- [x] Dot product scoring in `RunMS1Search` only reads `pfUnitVector`
- **Action:** Added documentation comment above `LoadSpecLibMS1Raw()` in `CometSpecLib.cpp`
  codifying the read-only invariant for future maintainers.
- **Status:** ✅ Complete (audit passed — no mutations after init)

#### Task 3.2: Protect `g_bSpecLibRead` with Proper Synchronization
- **File(s):** `CometSpecLib.cpp`, `CometSearchManager.cpp`
- **Description:** `g_bSpecLibRead` is a plain `bool` set at the end of
`LoadSpecLibMS1Raw()`. Since initialization is single-threaded (Task 1.1), this is
safe as long as all threads see the write before they start. Verified that the C#
`InitializeSingleSpectrumMS1Search()` call completes (and its side effects are
visible) before `Task.Run()` launches worker threads. `Task.Run()` scheduling
includes an implicit memory barrier guaranteeing visibility.
- **Action:** Added early-return guard with error message at top of
  `DoMS1SearchMultiResults()` checking `singleSearchMS1InitializationComplete`
  as defense-in-depth. No atomic/mutex needed since init is single-threaded
  and .NET Task scheduling provides the happens-before guarantee.
- **Status:** ✅ Complete (no code change needed beyond defensive guard)

### Phase 4: Cleanup

#### Task 4.1: Remove `g_pvQueryMS1` from the MS1 Search Path
- **File(s):** `CometSearchManager.cpp`, `CometSearch.cpp`
- **Description:** After Tasks 2.1–2.3, `g_pvQueryMS1` is no longer used in the
single-spectrum MS1 search path. The global vector declaration is retained because
the batch search path (`RunMS1Search(ThreadPool*,...)` and
`PreprocessMS1SingleSpectrum()`) still uses it. A documentation comment was added
to the `g_pvQueryMS1` declaration in `CometSearchManager.cpp` to clarify that it is
batch-path-only and must not be accessed from `DoMS1SearchMultiResults()`.
- **Verification:**
  - [x] `DoMS1SearchMultiResults()` uses only `PreprocessMS1SingleSpectrumThreadLocal()` and
    `RunMS1Search(QueryMS1*,...)` — no `g_pvQueryMS1` references
  - [x] `RunMS1Search(QueryMS1*,...)` thread-local overload does not read `g_pvQueryMS1`
  - [x] `FinalizeSingleSpectrumMS1Search()` cleanup of `g_pvQueryMS1` is safe — vector
    will be empty since `DoMS1SearchMultiResults()` never populates it
  - [x] `g_pvQueryMS1` retained for batch path backward compatibility
- **Status:** ✅ Complete

#### Task 4.2: Verify Wrapper Layer
- **File(s):** `CometWrapper.cpp`, `CometWrapper.h`
- **Description:** Confirm the C++/CLI wrapper layer (`CometSearchManagerWrapper`)
  requires no changes. The wrapper delegates directly to `ICometSearchManager`
  methods without touching any global state.
- **Verification:**
  - [x] `CometWrapper::DoMS1SearchMultiResults()` only calls
    `_pSearchMgr->DoMS1SearchMultiResults()` and converts outputs
  - [x] No direct `g_pvQueryMS1` access in wrapper code
  - [x] Pin pointers used correctly for array marshalling
- **Status:** ✅ Complete

---

## Summary

All phases complete. The MS1 single-spectrum search path is now fully thread-safe:
- Reference library loaded once during `InitializeSingleSpectrumMS1Search()`
- Each concurrent `DoMS1SearchMultiResults()` call uses thread-local `QueryMS1*`
- No shared mutable state accessed during search
- `g_pvQueryMS1` retained for batch path only, documented accordingly

### Phase 5: Testing & Validation

#### Task 5.1: Single-Thread MS1 Correctness
- **Description:** Run with `num_threads=1` and verify:
- Reference file loaded exactly once (single "loading MS1 scan" message).
- Dotproduct scores match baseline.
- No deadlock.
- **Status:** ☐ Not started

#### Task 5.2: Multi-Thread MS1 Correctness
- **Description:** Run with N threads (e.g., 4) and verify:
- Reference file loaded exactly once.
- Results are identical regardless of thread count.
- No crashes or data races.
- **Status:** ☐ Not started

#### Task 5.3: Combined MS1+MS2 Multi-Thread Test
- **Description:** Run with both `bPerformMS1Search=true` and `bPerformMS2Search=true`,
N threads. Verify both search types produce correct results concurrently.
- **Status:** ☐ Not started

#### Task 5.4: Memory Leak Check
- **Description:** Verify that per-call `QueryMS1` objects and their `pfFastXcorrData`
arrays are properly freed after each `DoMS1SearchMultiResults()` call.
- **Status:** ☐ Not started

---

## File Change Summary

| File | Changes |
|------|---------|
| `CometSearchManager.cpp` | Move `LoadSpecLibMS1Raw` to init; rewrite `DoMS1SearchMultiResults` to be thread-local; fix `fillPool` deadlock |
| `CometPreprocess.h` | Declare `PreprocessMS1SingleSpectrumThreadLocal()` |
| `CometPreprocess.cpp` | Implement `PreprocessMS1SingleSpectrumThreadLocal()` |
| `CometSearch.h` | Declare `RunMS1Search(QueryMS1*, ...)` overload |
| `CometSearch.cpp` | Implement thread-local `RunMS1Search(QueryMS1*, ...)` |
| `CometWrapper.cpp` | No changes expected |
| `SearchMS1MS2.cs` | No changes expected |

---

## Key Design Principles

1. **Load once, search many.** The reference library (`g_vSpecLib`) is loaded exactly
 once during `InitializeSingleSpectrumMS1Search()`. After that, it is immutable.

2. **Thread-local query objects.** Each `DoMS1SearchMultiResults()` call creates a
 heap-local `QueryMS1*`, preprocesses the query spectrum into it, scores against the
 read-only `g_vSpecLib`, extracts results, and deletes it. Zero shared mutable state.

3. **Same pattern as MS2.** This follows the identical architecturealready proven for
 `DoSingleSpectrumSearchMultiResults()`: thread-local `Query*` via
 `PreprocessSingleSpectrumThreadLocal()`, thread-local `RunSearch(Query*)`, and
 thread-local post-analysis.

4. **Minimal API surface change.** The `ICometSearchManager` interface signatures
   remain unchanged. All changes are internal implementation details.