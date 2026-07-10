# Code Review -- architecture_update (Follow-Up)

**Date:** 2026-06-16
**Reviewer:** Claude Code (claude-sonnet-4-6)
**Scope:** Follow-up review of the 9 fixes applied after the initial code review (20260616_codereview.md).
Branch: `architecture_update` vs `master` (working tree included).
**Method:** 7-angle Phase 1 (A-G, up to 6 candidates each) + Phase 2 per-candidate
verification (CONFIRMED / PLAUSIBLE / REFUTED). Only CONFIRMED and PLAUSIBLE findings
are reported below.

---

## 1. Summary

The nine fixes from the first review are largely sound: the mutex, PEFF seek-back,
dead-field removal, BuildNames consolidation, and Percolator const-ref changes are all
correct and clean. Three confirmed defects remain in newly added code: a file-descriptor
leak in MzIdentMlWriter, a misplaced memset in AllocateResultsMem, and an int-to-short
narrowing in StorePeptideI's new decoy branch. One plausible concurrency hazard exists
in the dual slot-tracking representation carried over from the refactor.

---

## 2. Critical Issues

### [C1] MzIdentMlWriter -- mkstemp fd leaked on every OpenTmp() call (Linux)

**File:** `CometSearch/output/MzIdentMlWriter.h`
**Lines:** ~94-101

On Linux, `OpenTmp()` calls `mkstemp(&sTmp[0])` (which creates and opens the temp file,
returning a live fd), uses the return value only as an error sentinel (`== -1`), then
calls `fopen(sTmp.c_str(), "w")` to open a second handle to the same path. The fd
returned by `mkstemp()` is never passed to `close()`. One fd is leaked per `OpenTmp()`
invocation -- once per mzIdentML output file per search batch.

**Failure scenario:** With a small `spectrum_batch_size` or many concurrent mzIdentML
writers, the process exhausts its open-fd limit, causing subsequent `fopen()` calls to
return `nullptr` and triggering "cannot write to temporary mzIdentML file" errors that
abort the search.

**Fix:**
```cpp
int fd = mkstemp(&sTmp[0]);
if (fd == -1)
{
   // error path
   return false;
}
close(fd);   // release the fd; fopen below opens its own handle
fp = fopen(sTmp.c_str(), "w");
```

---

### [C2] SearchUtils.h -- iXcorrHistogram memset inside per-result slot loop

**File:** `CometSearch/search/SearchUtils.h`
**Lines:** ~190 (inside `AllocateResultsMem`)

`iXcorrHistogram` is a per-`Query` array (declared `int iXcorrHistogram[HISTO_SIZE]` on
the `Query` struct in `core/Types.h:593`), not a per-`Results` slot field. The
`memset(pQuery->iXcorrHistogram, 0, sizeof(pQuery->iXcorrHistogram))` call is placed
inside the inner `for (int j = 0; j < g_staticParams.options.iNumStored; ++j)` loop, so
it zeroes the same query-level array `iNumStored` times instead of once. On iterations
j > 0, it resets the histogram, destroying any accumulation from prior j iterations.

**Failure scenario:** Currently harmless because histogram population happens after
`AllocateResultsMem` returns (during the search phase). However, if histogram data were
ever partially populated before the j-loop completes, iteration j=1 would silently
destroy accumulations from j=0. It also wastes `iNumStored - 1` redundant memset calls
per query.

**Fix:** Move `memset(pQuery->iXcorrHistogram, ...)` to just after
`pQuery->iDecoyMatchPeptideCount = 0`, before the for-j loop begins, so it executes
exactly once per query.

---

### [C3] CometSearch.cpp -- int-to-short narrowing in StorePeptideI decoy index

**File:** `CometSearch/CometSearch.cpp`
**Lines:** ~8724-8733 (new decoy branch in `StorePeptideI`)

The new decoy branch recomputes the lowest-scoring decoy slot index with
`for (int i = 1; ...)` and assigns `siLowestDecoyXcorrScoreIndex = i` where the local
variable is declared `short`. This is an implicit int-to-short narrowing conversion.
The analogous loop in `StorePeptide()` (FASTA path, line ~5227) uses `short siA`
throughout, keeping the type consistent with the `short siLowestDecoyXcorrScoreIndex`
field on `Query` (declared `core/Types.h:603`).

**Failure scenario:** Safe at current `iNumStored` values (typically <= 10). If
`iNumStored` were ever set to >= 32,768 the narrowing truncation would produce a wrong
or negative index, causing `_pDecoys[]` to be accessed out of bounds in the next
`StorePeptideI` call and silently corrupting decoy results.

**Fix:** Change the loop variable to `short` to match `StorePeptide()`:
```cpp
for (short siA = 1; siA < (short)g_staticParams.options.iNumStored; ++siA)
{
   if (pQuery->_pDecoys[siA].fXcorr < pQuery->_pDecoys[siLowestDecoyXcorrScoreIndex].fXcorr)
      siLowestDecoyXcorrScoreIndex = siA;
}
pQuery->siLowestDecoyXcorrScoreIndex = siLowestDecoyXcorrScoreIndex;
```

---

## 3. Code Quality and Maintainability

### [C4] CometSearch -- dual slot-tracking systems alias the same scratch buffers

**File:** `CometSearch/CometSearch.cpp`, `CometSearch/threading/SearchMemoryPool.h`
**Lines:** `CometSearch.cpp:1267`, `SearchMemoryPool.cpp:80`

The refactor introduced `SearchMemoryPool` (`s_pool`) but retained the legacy
`_pbSearchMemoryPool[]` + `g_searchMemoryPoolMutex` slot-tracking used by the FASTA
batch path (`SearchThreadProc`). The RTS path calls `s_pool.acquireSlot()` /
`releaseSlot()` (guarded by `s_pool._mutex`), while `SearchThreadProc` scans
`_pbSearchMemoryPool[]` under `g_searchMemoryPoolMutex`. Both systems alias the same
physical scratch buffers (`_ppbDuplFragmentArr[i]` = `s_pool._pool[i]`), but neither
delegates to the other -- they are genuinely independent availability-tracking arrays.

**Failure scenario (PLAUSIBLE):** If FASTA batch search and RTS search ever ran
concurrently in the same process, slot `i` could be claimed by `SearchThreadProc` via
`_pbSearchMemoryPool[i]` and simultaneously by `AcquirePoolSlot()` via
`s_pool._inUse[i]`, handing the same scratch buffer to two threads and silently
corrupting XCorr scores. The `TODO(Phase N)` comment at `CometSearch.cpp:31`
acknowledges the singleton design is not yet multi-instance safe.

**Recommendation:** Route `SearchThreadProc` through `s_pool.acquireSlot()` /
`releaseSlot()` and remove `_pbSearchMemoryPool`, `g_searchMemoryPoolMutex`, and
`g_searchPoolCV` once all paths use the single `s_pool` authority.

---

### [C5] CometSearch -- dead RunSpecLibSearch(ThreadPool*) overload

**File:** `CometSearch/CometSearch.cpp` (~line 1000), `CometSearch/CometSearch.h` (~line 94)

The 1-argument overload `RunSpecLibSearch(ThreadPool* /*tp*/)` is declared and defined
but has no callers in the current codebase. Its body is a commented-out debug printf
followed by `return true`. The live path is the 4-argument overload
`RunSpecLibSearch(int, int, ThreadPool*, vector<Query*>&)` called from
`SearchUtils.h::RunSearchAndPostAnalysis()`.

**Failure scenario:** If any future code resolves a call with a single `ThreadPool*`
argument to this overload -- by mistake or through a partial refactor -- all speclib
scoring is silently skipped with no error. The two overloads are visually similar and
the compiler produces no diagnostic.

**Fix:** Remove the dead 1-argument overload from both the `.h` declaration and the
`.cpp` definition.

---

### [C6] FastaStrategy -- dead if-block in initialize()

**File:** `CometSearch/search/FastaStrategy.cpp`
**Lines:** ~27-48

The block conditioned on `session.bPerformDatabaseSearch && sProteinLModsListFile.length() > 0`
in `FastaStrategy::initialize()` contains only a multi-line comment explaining why
nothing is done here (the filter is loaded before `makeStrategy()` is called). There
are no executable statements inside the block.

**Failure scenario:** No runtime defect. The risk is a future developer placing
initialization code inside this block expecting it to execute, unaware that the comment
explains the work is already complete by the time `initialize()` is called.

**Fix:** Remove the dead block entirely, or replace it with a one-line comment at the
top of `initialize()` stating the precondition.

---

## 4. Actionable Improvements

### [I1] PercolatorWriter -- inline filename construction should use BuildNames

**File:** `CometSearch/output/PercolatorWriter.h`
**Lines:** ~28-35

`PercolatorWriter::open()` constructs its output filename using the same
`base + range + ".pin"` pattern as `IResultWriter::BuildNames()`, but does so inline
rather than calling the shared helper. It is the only concrete writer that does not call
`BuildNames()`. Any future change to naming conventions (e.g., a new suffix format or
CRUX conditional) must be applied in two places.

**Fix:** Call `BuildNames(ctx, ".pin", ".decoy.pin", ".target.pin", _sPath, _sDecoyPath)`
and drop the local `base`/`range` variables, matching the pattern used by all other
writers.

---

### [I2] IResultWriter::BuildNames -- extTargetCrux should default to nullptr

**File:** `CometSearch/output/IResultWriter.h`
**Lines:** ~72-86

The `extTargetCrux` parameter of `BuildNames()` is unconditionally `(void)`-cast and
discarded in non-CRUX builds. All four call sites must pass a dummy string literal that
is silently ignored at compile time, leaking the CRUX/non-CRUX conditional into every
call site.

**Improvement:** Add `= nullptr` as the default for `extTargetCrux`:
```cpp
static void BuildNames(const WriterOpenCtx& ctx,
                       const char* ext,
                       const char* extDecoy,
                       std::string& sTarget,
                       std::string& sDecoy,
                       const char* extTargetCrux = nullptr);
```
Non-CRUX callers can then omit the argument entirely.
