# Code Review: architecture_update branch (2026-06-17)

## Scope

Deep review of the `architecture_update` branch versus `master` (commit `c971a2dd`).
The diff covers the Strategy/Pipeline refactor: `ISearchStrategy` + `Pipeline` replace
the monolithic `CometSearchManager::DoSearch` per-file loop; `SearchSession` replaces the
batch-path globals `g_pvQuery` / `g_pvQueryMS1`; `SearchMemoryPool` encapsulates the
thread scratch-array pool; and a new `output/IResultWriter` layer wraps the existing
`CometWrite*` classes.

Review method: 8 parallel finder angles (line-by-line diff scan, removed-behavior audit,
cross-file tracer, reuse, simplification, efficiency, altitude, conventions), each
surfacing up to 6 candidates, followed by a 1-vote verification pass on the strongest
findings.

---

## 1. Summary

The refactor successfully decouples per-batch mutable state from process-wide globals and
introduces a clean strategy/pipeline separation. The CometWrapper layer is fully insulated
(all calls go through the unchanged `ICometSearchManager` vtable). However, three
correctness bugs were introduced -- two silent data-corruption paths in hand-written
`operator=` overloads, and one functional regression that drops the batch MS1 spectral-
library search path entirely.

---

## 2. Critical Issues

### 2a. Batch MS1 speclib search silently dead (functional regression)

**File:** `CometSearch/search/SearchUtils.h:283`

`RunSearchAndPostAnalysis` (the shared batch body called by all three strategies) invokes
`CometSearch::RunSearch` and `CometSearch::RunSpecLibSearch` but never calls
`CometSearch::RunMS1Search(ThreadPool*, ...)`. Separately, `CometPreprocess::
PreprocessMS1SingleSpectrum(session&)` -- the only function that populates
`session.ms1Queries` -- has zero callers in any strategy or pipeline code path.

Result: a batch run with `bPerformSpecLibSearch = true` produces no MS1 spectral-library
matches and emits no error or warning. The MS1 speclib batch path was present in
`CometSearchManager::DoSearch` on `master` and is now dead code.

**Fix:** wire `PreprocessMS1SingleSpectrum(session)` and `RunMS1Search(tp, ...,
session.ms1Queries)` into `RunSearchAndPostAnalysis` when `session.bPerformSpecLibSearch`
is true, mirroring the MS2 speclib path already present.

**Status (2026-06-17):** Investigation confirmed this was not a regression -- the batch
MS1 speclib path (`PreprocessMS1SingleSpectrum` / `RunMS1Search(ThreadPool*,...)`) had zero
callers on `master` as well. Two partial fixes applied: (1) `Pipeline::cleanupBatch` lambda
now also deletes and clears `session.ms1Queries` so any future wiring will not leak; (2) a
TODO comment at `SearchUtils.h:287` documents the RT-range parameters required before the
batch MS1 path can be wired in.

---

### 2b. VarModParams::operator= drops two fields -- protein-filter var-mod searches silently broken

**File:** `CometSearch/core/Params.h:273`

`VarModParams::operator=` (called via `StaticParams::operator=` line 451) assigns every
field except `sProteinLModsListFile` (std::string) and `mmapProteinModsList`
(multimap<int,string>). After any `StaticParams` copy, `bVarModProteinFilter` is true but
`mmapProteinModsList` is empty, so the filter silently matches every protein and the
restriction is ignored.

`Options::operator=` (line 98) has the same structural problem: `iSpecLibMSLevel` (int,
declared line 48) is never assigned. After copy, the speclib MS-level filter uses
whatever value was already in the destination.

**Root cause:** All five hand-written `operator=` bodies in `Params.h` (`Options`,
`DBInfo`, `StaticMod`, `PrecalcMasses`, `VarModParams`) copy fields one by one and have
drifted from their struct declarations. The compiler-synthesised `operator=` would copy
all members correctly for free -- every member is a trivially-copyable scalar, a fixed
array of scalars, or a `std::string` / `std::vector` / `std::multimap` with correct copy
semantics.

**Fix:** Delete all five hand-written `operator=` definitions and rely on the compiler-
generated versions. If explicit copy control is needed for a specific reason, add a
static_assert or a comment naming that reason.

**Status (2026-06-17):** Fixed. All nine hand-written `operator=` bodies in `Params.h`
replaced with `= default` (correct `const Type&` signature). The full scope was larger
than initially identified -- beyond the five listed above, `MassUtil`, `ToleranceParams`,
`IonInfo`, and `StaticParams` had the same drift bug. `StaticParams::operator=` was missing
`peffInfo`, `iDbType`, `sDecoyPrefix` (string), `bSkipToStartScan`, and `tRealTimeStart`.
Build verified clean after replacement.

---

### 2c. SearchThreadProc has no RAII guard for the pool slot -- bad_alloc during index build causes 240-second deadlock

**File:** `CometSearch/CometSearch.cpp:1253`

```cpp
int i = AcquirePoolSlot();
// ...
CometSearch* sqSearch = new CometSearch();
sqSearch->DoSearch(...);
delete sqSearch;
s_pool.releaseSlot(i);   // never reached if DoSearch throws
```

`DoSearch` contains two re-throwing `catch` blocks (lines ~3563 and ~7558) inside
`g_pvDBIndex.push_back()` failure paths, reachable when `bCreateFragmentIndex` or
`bCreatePeptideIndex` is set. If the system OOMs mid index-build, the exception propagates
past `releaseSlot`. The old `SearchThreadData::~SearchThreadData` released the slot
unconditionally; that safety net was removed in this diff.

**Fix:** Wrap the slot in a simple RAII guard:

```cpp
struct SlotGuard {
   int slot;
   ~SlotGuard() { if (slot >= 0) s_pool.releaseSlot(slot); }
};
SlotGuard guard{i};
```

**Status (2026-06-17):** Fixed. Local `SlotGuard` struct added to `SearchThreadProc`
immediately after the slot is acquired. The explicit `s_pool.releaseSlot(i)` call was
removed; the guard destructor handles release on both normal exit and exception unwind.

---

## 3. Code Quality & Maintainability

### 3a. FusedLoadAndSearchSpectra batch-size check fires early

**File:** `CometSearch/CometPreprocess.cpp:3362`

`iNumSpectraLoaded` is incremented when a spectrum is pushed onto the bounded queue
(before any consumer thread processes it). `CheckExit` fires when
`iNumSpectraLoaded >= iSpectrumBatchSize`. With a queue depth of `iNumThreads * 4`, the
read loop can stop up to `iNumThreads * 4` entries before the configured batch size is
actually searched.

The non-fused `LoadAndPreprocessSpectra` path sets `iNumSpectraLoaded =
session.queries.size()` (post-preprocessing count), so the two paths have different
batch-size semantics. Users relying on `spectrum_batch_size` for memory control in FI_DB
mode will observe smaller-than-configured batches.

**Status (2026-06-17):** Fixed. Removed the local `iNumSpectraLoaded` variable and its
queue-push increment from `FusedLoadAndSearchSpectra`. The `CheckExit` call (which already
holds `session.queriesMutex`) now passes `(int)session.queries.size()` directly, matching
the non-fused path semantics: the count reflects spectra that have been fully preprocessed
and stored in `session.queries`.

### 3b. SearchThreadData::pQueries latent null deref

**File:** `CometSearch/CometSearch.h:43` / `CometSearch/CometSearch.cpp:1269`

Both `SearchThreadData` constructors initialise `pQueries = nullptr`. `SearchThreadProc`
dereferences it at line 1269 with no null check. All current callers correctly set
`pQueries = &queries` before dispatching, but the type provides no enforcement. A future
dispatch path that forgets the assignment will crash inside a thread with no useful
diagnostic.

**Fix:** make `pQueries` a required constructor parameter (remove the default-null
initialiser) or add an assert before the dereference.

**Status (2026-06-17):** Fixed. Removed the no-arg `= default` constructor (unused).
`pQueries` is now a required second parameter of the `sDBEntry` constructor:
`SearchThreadData(const sDBEntry&, const vector<Query*>*)`. The one call site in
`RunSearch` updated to `new SearchThreadData(dbe, &queries)`, eliminating the
post-construction assignment step.

### 3c. Pipeline::cleanupBatch skips session.ms1Queries

**File:** `CometSearch/search/Pipeline.cpp:136`

The `cleanupBatch` lambda deletes and clears `session.queries` but never touches
`session.ms1Queries`. Currently `session.ms1Queries` is never populated (see 2a above),
so there is no active leak. If batch MS1 search is re-wired, every batch will leak its
`QueryMS1*` objects across all batches and all input files.

**Status (2026-06-17):** Fixed as part of 2a. `cleanupBatch` now also iterates and
deletes `session.ms1Queries` and calls `session.ms1Queries.clear()`.

### 3d. session.params member is vestigial

**File:** `CometSearch/search/SearchSession.h:48`

`SearchSession` carries `const StaticParams& params` that is never read by any caller.
Every strategy, pipeline, and utility accesses `g_staticParams` directly. The member
implies an in-progress migration that has not started, misleading future readers.

**Status (2026-06-17):** Fixed. `const StaticParams& params` member and the accompanying
comment removed from `SearchSession`. Constructor simplified to
`explicit SearchSession(CometStatus& st)`. The one construction site in
`CometSearchManager.cpp` updated accordingly.

### 3e. Non-ASCII characters in SearchSession.h

**File:** `CometSearch/search/SearchSession.h:20,21,47`

Lines 20 (U+2026 HORIZONTAL ELLIPSIS) and 21, 47 (U+2014 EM DASH) are UTF-8 multi-byte
sequences. CLAUDE.md rule: \"No non-ASCII characters allowed in the code or documentation.\"
All other new files are pure ASCII. Replace with ASCII equivalents (`...` and `--`).

**Status (2026-06-17):** Fixed. The EM DASH on old line 47 was removed along with the
vestigial `params` member (3d). The HORIZONTAL ELLIPSIS on line 20 replaced with `...`
and the EM DASH on line 21 replaced with `--`. Verified with `grep -P "[^\x00-\x7F]"`:
no non-ASCII bytes remain.

### 3f. Trailing whitespace in Params.h

**File:** `CometSearch/core/Params.h:154,155,257`

Line 154 has 4 trailing spaces, line 155 has 2 trailing spaces and a stray space before
the semicolon (`iFragIndexMinIonsReport ;`), and line 257 has 1 trailing space. CLAUDE.md
rule: \"No trailing whitespace.\"

**Status (2026-06-17):** Fixed. The stray space before the semicolon and the two lines of
trailing spaces on old lines 154-155 were eliminated when the hand-written `operator=`
bodies were replaced with `= default` (issue 2b), which removed those lines entirely. The
remaining trailing space on old line 257 (`bVarProteinCTermMod` declaration, now line 162)
was stripped directly. Verified with `grep -P "[\t ][\r]?$"`: no trailing whitespace
remains.

---

## 4. Actionable Improvements

### 4a. Delete hand-written operator= in Params.h

Replace all five with `= default` or remove them entirely:

```cpp
// Before (drift-prone):
Options& operator=(Options& a) { iNumPeptideOutputLines = a.iNumPeptideOutputLines; ... }

// After:
Options& operator=(const Options&) = default;
```

If the non-const signature `operator=(Options& a)` was intentional (e.g., to allow
modification of the source), document why; otherwise make it `const Options&`.

**Status (2026-06-17):** Done as part of issue 2b. All nine hand-written `operator=`
bodies (not just the five originally identified) were replaced with `= default` using the
correct `const Type&` signature.

### 4b. Move RunSearchAndPostAnalysis out of SearchUtils.h

**File:** `CometSearch/search/SearchUtils.h:244`

`SearchUtils.h` is included by 5 translation units and contains 65-line non-trivial
functions marked `inline static`. Each TU gets its own copy. Move `RunSearchAndPostAnalysis`,
`AllocateResultsMem`, and `UpdateInputFile` into a `SearchUtils.cpp` and keep only
declarations in the header. The three small comparator helpers
(`compareByPeptideMass`, etc.) are genuinely inline-worthy and can stay.

**Status (2026-06-17):** Done. Created `CometSearch/search/SearchUtils.cpp` containing the
definitions of `UpdateInputFile`, `SetMSLevelFilter`, `AllocateResultsMem`, and
`RunSearchAndPostAnalysis`. `GetInputType` became a `static` helper in that .cpp (not
exported). `SearchUtils.h` now contains only declarations plus the three inline comparators;
added self-contained includes (`MSReader.h`, `SearchSession.h`) so the header compiles
standalone. `search/SearchUtils` added to `SEARCH_SRC` in the Makefile and
`search\SearchUtils.cpp` added to `CometSearch.vcxproj`.

### 4c. Factor out the shared legacy batch body in FiStrategy and FastaStrategy

**Files:** `CometSearch/search/FiStrategy.cpp:147`, `CometSearch/search/FastaStrategy.cpp:60`

The two \"legacy three-sweep\" paths (`LoadAndPreprocess` -> `AllocateResultsMem` ->
`RunSearchAndPostAnalysis`) are structurally identical except for a verbosity flag. The
difference is already encoded in the `bLogPrePostAnalysis` parameter that `RunSearchAndPostAnalysis`
accepts. Extract a shared free function:

```cpp
bool executeBatchLegacy(MSToolkit::MSReader& mstReader, int iFirstScan, int iLastScan,
                        int iAnalysisType, int& iPercentStart, int& iPercentEnd,
                        ThreadPool* tp, SearchSession& session, bool bVerbose);
```

**Status (2026-06-17):** Done. `executeBatchLegacy` added to `SearchUtils.cpp` /
declared in `SearchUtils.h`. The `bVerbose` flag controls the three per-strategy
differences: the \"Load spectra:\" console log before loading, the spectra-count
`logout` after allocation, and whether to pass `bLogPrePostAnalysis=true` to
`RunSearchAndPostAnalysis`. All three strategy `executeBatch` bodies replaced with a
single call; this covered `PiStrategy` as well (not mentioned in the original finding
but structurally identical to the `FiStrategy` non-fused path).

### 4d. Fix iNumSpectraLoaded semantics in FusedLoadAndSearchSpectra

**File:** `CometSearch/CometPreprocess.cpp:3362`

Either (a) increment `iNumSpectraLoaded` inside `FusedSearchSpectrum` after a spectrum
completes preprocessing (requires an atomic counter shared with the worker lambdas), or
(b) document that the fused-path batch size is approximate (+/- queue depth) and update
any user-facing documentation for `spectrum_batch_size` accordingly.

**Status (2026-06-17):** Done as part of 3a (option a). The local `iNumSpectraLoaded`
variable and its queue-push increment were removed entirely. `CheckExit` now receives
`(int)session.queries.size()` directly under the already-held `queriesMutex`, which counts
only spectra that have been fully preprocessed -- the same semantics as the non-fused path.

---

## Appendix: Findings Not Requiring Code Changes

- **CometWrapper isolation confirmed**: all CometWrapper calls go through the
  `ICometSearchManager` vtable; no internal signature changes propagate to the wrapper
  layer.
- **s_pool singleton (TODO acknowledged)**: the file-static `SearchMemoryPool s_pool` in
  `CometSearch.cpp` prevents multiple concurrent RTS instances. The TODO comment at line
  30 correctly identifies this. No concurrent RTS path currently invokes the batch pool,
  so this is a known deferred item, not a regression.
- **FiStrategy::finalize() redundant iDbType check**: the `if (g_staticParams.iDbType ==
  DbType::FI_DB)` guard is always true when called by the pipeline (which selected
  FiStrategy precisely because iDbType == FI_DB). Harmless today.
- **Redundant #include lines in CometSearchManager.cpp**: the five `CometWrite*.h`
  includes at lines 21-25 are already pulled in transitively by the new `output/*Writer.h`
  includes. Dead includes, no functional impact.
