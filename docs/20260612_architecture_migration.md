# Architecture Migration Plan

**Date**: 2026-06-12  
**Scope**: `CometSearch/` library only  
**Goal**: Separate concerns, reduce coupling, increase modularity.  
Behavior is unchanged at every step; each phase is independently compilable and testable.

---

## Background

The codebase has six structural pathologies that this plan addresses in order of
increasing invasiveness:

1. `CometDataInternal.h` (1,554 lines) is a monolith — constants, parameter structs,
   result structs, index structs, and scoring structs all in one file. A one-line
   change rebuilds every translation unit.

2. Pool slot management (`_pbSearchMemoryPool`, `_ppbDuplFragmentArr`,
   `AcquirePoolSlot`) is buried in `CometSearch` static members and bleeds into
   `CometSearchManager` and `SearchThreadData` with no clear ownership.

3. Five result writers (`CometWriteTxt`, `CometWriteSqt`, `CometWritePepXML`,
   `CometWriteMzIdentML`, `CometWritePercolator`) are called via sequential `if`
   chains in `DoSearch()` and access `g_pvQuery` / `g_staticParams` directly.
   There is no shared interface.

4. Twenty-eight mutable globals act as the implicit API between all modules:
   `g_staticParams`, `g_pvQuery`, `g_pvQueryMS1`, `g_cometStatus`,
   `g_searchMemoryPoolMutex`, `g_searchPoolCV`, `g_bPlainPeptideIndexRead`, etc.
   Any file can write any global at any time.

5. `CometSearchManager::DoSearch()` (~1,100 lines) mixes parameter validation,
   index loading, per-file loop logic, file handle management, search dispatch,
   result writing, and progress reporting.

6. Search-path selection (`if iDbType == FI_DB ... else if PI_DB ...`) appears in
   `DoSearch()`, `RunSearch()`, `LoadAndPreprocessSpectra()`,
   `FusedLoadAndSearchSpectra()`, and `InitializeSingleSpectrumSearch()`. Adding
   a new index type requires edits in eight or more places.

---

## Target Folder Structure

```
CometSearch/
├── core/
│   ├── Constants.h              # All compile-time constants (split from CometDataInternal.h)
│   ├── Types.h                  # Results, Query, PepMassInfo, scoring data structs
│   └── Params.h                 # StaticParams and all sub-structs
│
├── params/
│   ├── ParamLoader.h/.cpp       # File/map -> StaticParams  (from CometSearchManager lines 625-1862)
│   └── ParamValidator.h/.cpp    # ValidateOutputFormat, ValidateScanRange, etc.
│
├── index/
│   ├── ISearchIndex.h           # Abstract interface: Load(), GetType(), IsLoaded()
│   ├── fragment/
│   │   ├── FragmentIndex.h/.cpp         # Runtime state + query
│   │   └── FragmentIndexBuilder.h/.cpp  # WriteFIPlainPeptideIndex
│   ├── peptide/
│   │   ├── PeptideIndex.h/.cpp
│   │   └── PeptideIndexBuilder.h/.cpp
│   └── speclib/
│       ├── SpecLib.h/.cpp
│       └── Alignment.h/.cpp
│
├── spectrum/
│   ├── ISpectrumSource.h        # Interface: next(Spectrum&)->bool, scanCount(), seekTo()
│   ├── MSReaderSource.h/.cpp    # MSReader-backed implementation
│   ├── Preprocessor.h/.cpp      # Binning, xcorr prep -- pure computation, no I/O
│   └── BoundedQueue.h           # BoundedSpectrumQueue (moved from CometPreprocess.cpp)
│
├── scoring/
│   ├── XcorrScorer.h/.cpp       # SearchFragmentIndex, XcorrScore
│   ├── SpScorer.h/.cpp          # CalculateSP
│   └── EValueScorer.h/.cpp      # CalculateEValue, CalculateDeltaCn
│
├── search/
│   ├── SearchSession.h          # Owns mutable run state (replaces g_pvQuery etc.)
│   ├── ISearchStrategy.h        # Pure virtual: initialize / execute / finalize
│   ├── FastaStrategy.h/.cpp     # FASTA_DB path
│   ├── FiStrategy.h/.cpp        # FI_DB batch + RTS paths
│   ├── PiStrategy.h/.cpp        # PI_DB path
│   └── Pipeline.h/.cpp          # Selects strategy, drives per-file loop
│
├── output/
│   ├── IResultWriter.h          # Pure virtual: write(results, params)
│   ├── TxtWriter.h/.cpp
│   ├── SqtWriter.h/.cpp
│   ├── PepXmlWriter.h/.cpp
│   ├── MzIdentMlWriter.h/.cpp
│   └── PercolatorWriter.h/.cpp
│
├── threading/
│   ├── ThreadPool.h             # Unchanged
│   └── SearchMemoryPool.h/.cpp  # Extracted from CometSearch statics
│
└── SearchManager.h/.cpp         # Thin ICometSearchManager impl -- delegates to Pipeline
```

---

## Phase 1 — Split `CometDataInternal.h`

**Effort**: ~1 day  **Risk**: Low (mechanical split, no logic changes)

### Problem

`CometDataInternal.h` is included by every `.cpp` in the library. It contains:
- Physical/algorithmic constants (`#define` macros, lines 33–114)
- Fourteen parameter sub-structs (`Options`, `ToleranceParams`, `IonInfo`,
  `MassUtil`, `VarModParams`, `StaticMod`, `PrecalcMasses`, `DBInfo`,
  `SpecLibInfo`, `PEFFInfo`, `EnzymeInfo`, `MassRange`, lines 116–980)
- `StaticParams` aggregate (lines 890–1172)
- Result/query structs (`Results`, `Query`, `QueryMS1`, `PepMassInfo`,
  `SpectrumInfoInternal`, `PreprocessStruct`, `SpecLibResults`, lines 248–1490)
- Index-related structs (`PlainPeptideIndexStruct`, `FragmentPeptidesStruct`,
  `DBIndex`, `PepGenTuple`, `PepGenTupleShort`, `IndexProteinStruct`,
  `ProteinsListCSR`, lines 454–1277)
- PEFF structs (`PeffModStruct`, `PeffVariantSimpleStruct`,
  `PeffVariantComplexStruct`, `PeffPositionStruct`, `PeffSearchStruct`,
  lines 340–424)
- Scoring/output structs (`MatchedIonsStruct`, `IonSeriesStruct`,
  `ModificationNumber`, lines 1278–1555)
- `DbType` enum (line 882)

### Action

Create `CometSearch/core/` and split into three headers:

**`core/Constants.h`** — all `#define` constants replaced with `constexpr`:

```
Source lines in CometDataInternal.h: 33-114
Contents:
  PROTON_MASS, C13_DIFF, FLOAT_ZERO
  MIN/MAX_PEPTIDE_LEN, MAX_PEPTIDE_LEN_P2
  FRAGINDEX_* (8 constants)
  MS1_* (4 constants)
  MAX_PEFFMOD_LEN, SIZE_MASS, SIZE_NATIVEID
  NUM_SP_IONS, NUM_ION_SERIES, VMODS, HISTO_SIZE
  WIDTH_REFERENCE, MAX_PROTEINS, EXPECT_DECOY_SIZE
  NO_PEFF_VARIANT, ASCORE_CUTOFF_TO_ACCEPT, FRAGINDEX_VMODS
  COMPOUNDMODS_OFFSET, VMOD_*_INDEX (15 constants)
  ENZYME_* (4 constants)
  ION_SERIES_* (7 constants)
  XCORR_CUTOFF, SPECLIB_CUTOFF
  DbType enum (move from line 882)
Change: #define -> constexpr int/double. DbType moves here from line 882.
```

**`core/Params.h`** — all parameter structs that StaticParams aggregates:

```
Source lines in CometDataInternal.h: 116-246 (Options)
                                     828-854 (ToleranceParams)
                                     856-878 (IonInfo)
                                     721-789 (PrecalcMasses)
                                     790-826 (MassUtil)
                                     697-720 (StaticMod)
                                     741-789 (VarModParams)
                                     436-453 (DBInfo)
                                     645-649 (SpecLibInfo)
                                     691-696 (PEFFInfo)
                                     890-1172 (StaticParams)
                                     321-333 (MassRange)
Also includes: EnzymeInfo (from CometData.h -- leave in place, just #include it)
Depends on: core/Constants.h, CometData.h
```

**`core/Types.h`** — runtime data structs (per-spectrum, per-query):

```
Source lines in CometDataInternal.h:
  248-278   Results
  280-295   SpecLibResults, SpecLibResultsMS1
  296-320   PepMassInfo, SpectrumInfoInternal
  334-339   PreprocessStruct
  340-424   PEFF structs (5 structs)
  425-435   sDBEntry
  454-602   DBIndex, PepGenTuple, PepGenTupleShort
  510-602   PepGenTuple / PepGenTupleShort
  603-644   PlainPeptideIndexStruct, FragmentPeptidesStruct
  650-689   SpecLibStruct, RetentionMatch
  684-690   IndexProteinStruct
  1175-1277 ProteinsListCSR
  1278-1310 ModificationNumber
  1312-1491 Query
  1492-1536 QueryMS1
  1537-1555 IonSeriesStruct, MatchedIonsStruct
  352-364   ProteinEntryStruct
  365-424   Peff structs
Depends on: core/Constants.h, core/Params.h, CometData.h, Threading.h, AScore headers
```

### Transition

Keep `CometDataInternal.h` as a compatibility shim that just includes the three
new headers. This means zero changes to existing `.cpp` files in Phase 1:

```cpp
// CometDataInternal.h after Phase 1 -- pure forwarding
#pragma once
#include "core/Constants.h"
#include "core/Params.h"
#include "core/Types.h"
```

In Phase 2+, files that only need one of the three headers update their own
`#include` to the specific header. `CometDataInternal.h` can be retired once
no `.cpp` includes it directly.

### Verification

```
make cclean && make   # must compile clean
python3 tests/unit/run_tests.py --comet comet.exe  # all 17 must pass
```

---

## Phase 2 — Extract `SearchMemoryPool`

**Effort**: ~1 day  **Risk**: Low (self-contained, well-tested at runtime)

### Problem

The duplicate-fragment scratch arrays and the pool-slot semaphore are spread
across three locations with no single owner:

| Location | What it does |
|----------|-------------|
| `CometSearch.cpp` lines 23-24 | Defines `_pbSearchMemoryPool`, `_ppbDuplFragmentArr` as class statics |
| `CometSearch.cpp` lines 45-116 | `AllocateMemory()`, `DeallocateMemory()`, `AcquirePoolSlot()` |
| `CometSearch.h` lines 50-59 | `SearchThreadData::~SearchThreadData()` releases the slot directly |
| `CometSearch.cpp` lines 139-140, 182-183, 227-228, 272+ | Inline slot release at each `RunSearch` call site |
| `CometSearchManager.cpp` lines 2741-2748 | Calls `AllocateMemory` / `DeallocateMemory` |
| `CometSearchManager.cpp` line 60 | Defines `g_searchMemoryPoolMutex` |
| `CometSearchManager.cpp` line 67 | Defines `g_searchPoolCV` |
| `CometSearchManager.cpp` line 94 | Defines `g_bCometSearchMemoryAllocated` |

### New File: `threading/SearchMemoryPool.h`

```cpp
#pragma once
#include <condition_variable>
#include <mutex>

// Owns the per-thread duplicate-fragment scratch arrays used during search.
// Replaces CometSearch::_pbSearchMemoryPool, _ppbDuplFragmentArr,
// AllocateMemory(), DeallocateMemory(), AcquirePoolSlot() and the paired globals
// g_searchMemoryPoolMutex, g_searchPoolCV, g_bCometSearchMemoryAllocated.
class SearchMemoryPool
{
public:
   SearchMemoryPool() = default;
   ~SearchMemoryPool() { if (_allocated) deallocate(_nSlots); }

   // Allocates nSlots scratch arrays each of size iArraySize bools.
   // Corresponds to CometSearch::AllocateMemory(nThreads).
   bool allocate(int nSlots, int iArraySize);

   // Frees all scratch arrays.
   // Corresponds to CometSearch::DeallocateMemory(nThreads).
   void deallocate(int nSlots);

   // Blocks up to 240 s until a slot is free. Returns index in [0, nSlots)
   // or -1 on timeout. Corresponds to CometSearch::AcquirePoolSlot().
   int  acquireSlot();

   // Returns the slot and signals one waiting acquireSlot() caller.
   // Corresponds to the inline release blocks in CometSearch::RunSearch.
   void releaseSlot(int slot);

   // Direct access to the scratch array for a claimed slot.
   bool* duplFragmentArr(int slot) const { return _pool[slot]; }

   int slotCount() const { return _nSlots; }

private:
   int     _nSlots    = 0;
   bool*   _inUse     = nullptr;   // was _pbSearchMemoryPool
   bool**  _pool      = nullptr;   // was _ppbDuplFragmentArr
   bool    _allocated = false;

   std::mutex              _mutex;
   std::condition_variable _cv;
};
```

### New File: `threading/SearchMemoryPool.cpp`

The implementations are direct ports of the existing functions:

```
allocate()    <- CometSearch::AllocateMemory() lines 45-72
               reads: g_staticParams.iArraySizeGlobal (pass as parameter instead)
               writes: g_bCometSearchMemoryAllocated (becomes _allocated member)

deallocate()  <- CometSearch::DeallocateMemory() lines 75-92
               reads: g_bCometSearchMemoryAllocated (becomes _allocated member)

acquireSlot() <- CometSearch::AcquirePoolSlot() lines 97-116
               reads: g_staticParams.options.iNumThreads (becomes _nSlots)
               uses: g_searchMemoryPoolMutex -> _mutex
                     g_searchPoolCV          -> _cv
                     _pbSearchMemoryPool     -> _inUse

releaseSlot() <- inline blocks at CometSearch.cpp lines 139, 182, 227, 272+
               uses: g_searchMemoryPoolMutex -> _mutex
                     g_searchPoolCV          -> _cv
                     _pbSearchMemoryPool     -> _inUse
```

### `SearchThreadData` update

`CometSearch.h` `SearchThreadData::~SearchThreadData()` (lines 50-59) currently
releases the slot directly into globals. Update it to hold a `SearchMemoryPool*`
and call `releaseSlot()`:

```cpp
struct SearchThreadData
{
   sDBEntry          dbEntry;
   int               iPoolSlot  = -1;
   SearchMemoryPool* pPool      = nullptr;
   ThreadPool*       tp         = nullptr;

   ~SearchThreadData()
   {
      if (pPool && iPoolSlot >= 0)
      {
         pPool->releaseSlot(iPoolSlot);
         iPoolSlot = -1;
      }
      dbEntry.vectorPeffMod.clear();
      dbEntry.vectorPeffVariantSimple.clear();
   }
};
```

### Call-site changes

Every call site that calls `CometSearch::AllocateMemory()`,
`CometSearch::DeallocateMemory()`, or `CometSearch::AcquirePoolSlot()` is
updated to use the `SearchMemoryPool` object. The object is constructed in
`CometSearchManager::DoSearch()` and passed by reference to all functions that
need it:

```
CometSearchManager.cpp line 2741: CometPreprocess::AllocateMemory() -- unchanged
CometSearchManager.cpp line 2746: CometSearch::AllocateMemory()  ->  pool.allocate(n, arraySize)
CometSearchManager.cpp ~line 2332: CometSearch::DeallocateMemory() -> pool.deallocate(n)
CometSearch.cpp line 132: AcquirePoolSlot() -> pool.acquireSlot()
CometSearch.cpp line 139: inline release -> pool.releaseSlot(iSlot)
CometSearch.cpp line 175: (PI_DB path) same pattern
CometSearch.cpp lines 220, 227, 272+: (batch path) same pattern
CometPreprocess.cpp FusedLoadAndSearchSpectra: pool.duplFragmentArr(t) replaces
                                               _ppbDuplFragmentArr[t]
```

Globals retired after this phase: `g_searchMemoryPoolMutex`, `g_searchPoolCV`,
`g_bCometSearchMemoryAllocated`.

### Verification

```
make cclean && make
python3 tests/unit/run_tests.py --comet comet.exe   # all 17 must pass
# run HeLa mzXML batch search and confirm PSM count matches pre-change baseline
```

---

## Phase 3 — Extract `IResultWriter`

**Effort**: ~2 days  **Risk**: Medium (touches writer internals)

### Problem

Five writer classes are dispatched from `DoSearch()` via 300+ lines of sequential
`if (bOutputXxx)` blocks (lines 2446–2900 in `CometSearchManager.cpp`). Each
writer reads `g_pvQuery` and `g_staticParams` directly. There is no shared
interface, so the dispatch cannot be driven polymorphically.

### New File: `output/IResultWriter.h`

```cpp
#pragma once
#include "core/Types.h"
#include "core/Params.h"
#include <vector>

// Abstract result serializer. One concrete implementation per output format.
// Replaces the sequential if (bOutputTxtFile) / if (bOutputPepXMLFile) / ...
// dispatch in CometSearchManager::DoSearch().
class IResultWriter
{
public:
   virtual ~IResultWriter() = default;

   // Open output file(s) and write format header.
   // baseName: g_staticParams.inputFile.szBaseName + szOutputSuffix
   // Called once per input file, before any spectra are searched.
   virtual bool open(const std::string& baseName, const StaticParams& params) = 0;

   // Write all results for one batch of spectra.
   // results is sorted by scan number (compareByScanNumber already applied).
   // Called once per spectrum batch within a file.
   virtual void write(const std::vector<Query*>& results,
                      const StaticParams&         params) = 0;

   // Flush and close output file(s). Write format footer if needed (e.g. pepXML).
   // Called once per input file, after all batches are complete.
   virtual void close(const StaticParams& params) = 0;
};
```

### Writer refactoring

Each existing writer becomes a concrete `IResultWriter`. The key behavioral
change is: instead of reading `g_pvQuery` directly, receive `results` as a
parameter. `g_staticParams` access is replaced by the `params` parameter.

**`CometWriteTxt` -> `output/TxtWriter`**

```
Current:  void CometWriteTxt::PrintResults(int iWhichQuery, bool bDecoy,
                 FILE* fpout, FILE* fpoutd, int iPrintTargetDecoy)
          reads g_pvQuery.at(iWhichQuery) and g_staticParams directly

After:    write() iterates over the results vector instead of g_pvQuery.
          The file handles (fpout, fpoutd) become private members opened in open().
          g_staticParams references become params parameter.

open()    <- file open + PrintTxtHeader() call (lines 2500-2550 in SearchManager)
write()   <- current PrintResults() loop body, receiving vector<Query*>
close()   <- fclose(fpout); fclose(fpoutd);
```

**`CometWriteSqt` -> `output/SqtWriter`**

```
open()   <- file open + PrintSqtHeader() call (lines 2446-2498 in SearchManager)
write()  <- existing PrintResults() but receiving vector<Query*>
close()  <- fclose(fpout); fclose(fpoutd);
```

**`CometWritePepXML` -> `output/PepXmlWriter`**

```
open()   <- file open + WritePepXMLHeader() (lines 2553-2627 in SearchManager)
write()  <- existing PrintPepXMLResults() receiving vector<Query*>
close()  <- WritePepXMLFooter() + fclose
Note: pepXML has a two-pass pattern (tmp file + finalize). The tmp-file logic
      (currently lines 2659-2724) moves into close().
```

**`CometWriteMzIdentML` -> `output/MzIdentMlWriter`**

```
open()   <- file open + header (lines 2628-2724 in SearchManager)
write()  <- existing per-scan output
close()  <- footer + tmp file merge + fclose
```

**`CometWritePercolator` -> `output/PercolatorWriter`**

```
open()   <- file open + WritePercolatorHeader() (lines 2724-2734 in SearchManager)
write()  <- existing PrintPercolatorResults() receiving vector<Query*>
close()  <- fclose
```

### `DoSearch()` dispatch replacement

The 300-line dispatch block in `DoSearch()` (lines 2446-2900) becomes a factory
that builds a `vector<unique_ptr<IResultWriter>>` once per input file:

```cpp
// In DoSearch() -- replaces lines 2446-2734
vector<unique_ptr<IResultWriter>> writers;
if (g_staticParams.options.bOutputTxtFile)
   writers.push_back(make_unique<TxtWriter>());
if (g_staticParams.options.bOutputSqtFile || g_staticParams.options.bOutputSqtStream)
   writers.push_back(make_unique<SqtWriter>());
if (g_staticParams.options.bOutputPepXMLFile)
   writers.push_back(make_unique<PepXmlWriter>());
if (g_staticParams.options.iOutputMzIdentMLFile)
   writers.push_back(make_unique<MzIdentMlWriter>());
if (g_staticParams.options.bOutputPercolatorFile)
   writers.push_back(make_unique<PercolatorWriter>());

// open all writers before first search batch
for (auto& w : writers)
   if (!w->open(baseName, g_staticParams)) { /* handle error */ }

// after each batch sort+write:
for (auto& w : writers)
   w->write(g_pvQuery, g_staticParams);

// after all batches:
for (auto& w : writers)
   w->close(g_staticParams);
```

Note: `g_pvQuery` and `g_staticParams` are still globals at this phase. That
coupling is eliminated in Phase 4. Phase 3 only introduces the interface and
moves file-handle lifetime into the writer objects.

### Verification

```
make cclean && make
python3 tests/unit/run_tests.py --comet comet.exe
# Run HeLa mzXML; diff txt output against pre-Phase-3 baseline -- must be identical
# (header line timestamp will differ; all PSM data must match exactly)
```

---

## Phase 4 — Introduce `SearchSession`

**Effort**: ~3 days  **Risk**: Medium-high (many call sites)

### Problem

The mutable state for one search run is scattered across 28 globals. Any code
can modify any of them without any indication of ownership or lifetime.

### New File: `search/SearchSession.h`

```cpp
#pragma once
#include "core/Params.h"
#include "core/Types.h"
#include "CometStatus.h"
#include <mutex>
#include <vector>

// Owns mutable state for one search run.
// Created at the start of DoSearch() / InitializeSingleSpectrumSearch().
// Passed by reference to all pipeline functions that write results.
// Read-only index state (g_iFragmentIndex, g_vFragmentPeptides, g_vSpecLib,
// g_vRawPeptides, g_pvProteinsList, g_pvProteinNameCache) is NOT moved here --
// those are large, initialized once, and shared read-only across all searches.
// They remain as const globals. (See note on pragmatic globals below.)
struct SearchSession
{
   // Run parameters -- set once before searching, then read-only.
   // The params reference outlives the session (owned by CometSearchManager).
   const StaticParams& params;

   // Per-batch result accumulator.
   // Guarded by queriesMutex in the batch path; not accessed concurrently in RTS.
   std::vector<Query*>    queries;
   std::vector<QueryMS1*> ms1Queries;
   std::mutex             queriesMutex;

   // Run-time flags (currently globals)
   bool bPerformDatabaseSearch = false;
   bool bPerformSpecLibSearch  = false;
   bool bIdxNoFasta            = false;
   bool bPlainPeptideIndexRead = false;
   bool bSpecLibRead           = false;

   // Error / cancel state for this run.
   // Replaces g_cometStatus for per-run isolation.
   CometStatus status;

   explicit SearchSession(const StaticParams& p) : params(p) {}
   SearchSession(const SearchSession&)            = delete;
   SearchSession& operator=(const SearchSession&) = delete;
};
```

### Globals replaced by SearchSession

```
Global (CometSearchManager.cpp)        -> SearchSession member
-------------------------------------------------------
g_pvQuery                              -> session.queries
g_pvQueryMS1                           -> session.ms1Queries
g_pvQueryMutex                         -> session.queriesMutex
g_bPerformDatabaseSearch               -> session.bPerformDatabaseSearch
g_bPerformSpecLibSearch                -> session.bPerformSpecLibSearch
g_bIdxNoFasta                          -> session.bIdxNoFasta
g_bPlainPeptideIndexRead               -> session.bPlainPeptideIndexRead
g_bSpecLibRead                         -> session.bSpecLibRead
g_cometStatus                          -> session.status
```

### Globals intentionally NOT moved (pragmatic globals)

The following globals remain as globals. They are large, allocated once,
read-only after initialization, and shared by concurrent threads. Moving them
into a session object would require reference or pointer threading through
hundreds of scoring call sites with no correctness benefit:

```
g_staticParams         -- read-only after DoSearch() init; replace with session.params
g_iFragmentIndex       -- read-only after index load; stays global
g_iFragmentIndexOffset -- same
g_vFragmentPeptides    -- same
g_vRawPeptides         -- same
g_pvProteinsList       -- same
g_pvProteinNameCache   -- same
g_vSpecLib             -- same
g_pvDBIndex            -- read-only after FASTA scan; stays global
g_vvvPepGenShort/.Long -- same
g_massRange            -- derived from params; can be computed on demand
g_pvProteinNames       -- read-only after load; stays global
g_pvInputFiles         -- owned by CometSearchManager; stays
g_sCometVersion        -- constant after init; stays
g_AScoreOptions        -- constant after init; stays
g_AScoreInterface      -- constant after init; stays
g_bPeptideIndexRead    -- atomic, read-only after set; stays
RetentionMatchHistory  -- deque used by alignment; keep as module-local in Alignment
```

### Migration strategy

Introduce `SearchSession` alongside the existing globals. In Phase 4, both exist
in parallel. Each function signature that currently reads a global gets a
`SearchSession&` parameter added. The global is then read from `session.member`
instead of the global directly. Once all reads/writes go through the session,
the global definition is removed.

Recommended order within Phase 4 (lowest risk first):

```
Step 4a: Add session to DoSearch() and the per-file loop. Pass to writer open()/write()/close().
Step 4b: Thread session into CometPreprocess::LoadAndPreprocessSpectra() and
         FusedLoadAndSearchSpectra(). Remove g_pvQuery push under mutex; use
         session.queries.push_back() under session.queriesMutex.
Step 4c: Thread session into CometSearch::RunSearch() overloads. RunSearch(Query*)
         and RunSearch(int, int, ThreadPool*) no longer read g_pvQuery directly.
Step 4d: Thread session into CometPostAnalysis. PostAnalysisThreadProc currently
         iterates g_pvQuery; replace with session.queries.
Step 4e: Remove global definitions for the nine replaced globals. Compiler errors
         will identify any remaining direct accesses.
```

### Verification

After each step 4a-4e:
```
make cclean && make
python3 tests/unit/run_tests.py --comet comet.exe
# batch HeLa mzXML diff against Phase 3 baseline
```

---

## Phase 5 — Extract `ISearchStrategy` and `Pipeline`

**Effort**: ~1 week  **Risk**: High (most invasive refactor)

### Problem

`DoSearch()` selects the search path via cascading `if (iDbType == FI_DB)` chains
that appear in at minimum these locations:

```
CometSearchManager.cpp ~line 2252: bCreatePeptideIndex path
CometSearchManager.cpp ~line 2324: bCreateFragmentIndex path
CometSearchManager.cpp ~line 2352: FI_DB precursor pre-read
CometSearchManager.cpp ~line 2808: FI_DB index load
CometSearchManager.cpp ~line 2900: FASTA_DB vs FI/PI_DB file opens
CometSearch.cpp line 122: RunSearch(Query*) dispatch
CometSearch.cpp line 206: RunSearch(ThreadPool*) dispatch
CometPreprocess.cpp: LoadAndPreprocessSpectra vs FusedLoadAndSearchSpectra
CometSearchManager.cpp ~line 3283: InitializeSingleSpectrumSearch dispatch
```

### New File: `search/ISearchStrategy.h`

```cpp
#pragma once
#include "SearchSession.h"
#include "threading/SearchMemoryPool.h"
#include "ThreadPool.h"

struct InputFileInfo;

// One implementation per database type: FastaStrategy, FiStrategy, PiStrategy.
// Pipeline selects the correct one at startup and holds it for the run.
class ISearchStrategy
{
public:
   virtual ~ISearchStrategy() = default;

   // Called once before the first input file.
   // Responsible for index loading / building (e.g. ReadPlainPeptideIndex,
   // CreateFragmentIndex, WriteFIPlainPeptideIndex, WritePeptideIndex).
   // Returns false on error.
   virtual bool initialize(SearchSession& session, ThreadPool& pool) = 0;

   // Called once per input file. Opens the spectrum source, reads/searches
   // all batches, appends fully scored Query* objects to session.queries.
   // Returns false on error or cancel.
   virtual bool execute(const InputFileInfo& file,
                        SearchSession&       session,
                        SearchMemoryPool&    pool,
                        ThreadPool&          tp) = 0;

   // Called once after all files. Cleanup (index dealloc, etc.).
   virtual void finalize(SearchSession& session, ThreadPool& pool) = 0;
};
```

### Strategy implementations

**`search/FiStrategy.h/.cpp`** — FI_DB batch path

```
initialize():
  If bCreateFragmentIndex: call WriteFIPlainPeptideIndex(tp) then return.
  Else: pre-read precursors (if !iFragIndexSkipReadPrecursors),
        call ReadPlainPeptideIndex() + CreateFragmentIndex(tp).
  Source: DoSearch() lines 2324-2414.

execute():
  Opens MSReader, calls FusedLoadAndSearchSpectra() in batch loop.
  Source: DoSearch() lines 2808-3220 (FI_DB branch).

finalize():
  CometSearch::DeallocateMemory(), CometPreprocess::DeallocateMemory().
```

**`search/FastaStrategy.h/.cpp`** — FASTA_DB path

```
initialize():
  ReadProteinVarModFilterFile() if configured.
  CometSearch::AllocateMemory().
  Source: DoSearch() lines 2252-2277.

execute():
  Opens MSReader and FASTA file handle.
  Runs LoadAndPreprocessSpectra() + RunSearch() in batch loop.
  Source: DoSearch() lines 2800-3220 (FASTA_DB branch).

finalize():
  DeallocateMemory().
```

**`search/PiStrategy.h/.cpp`** — PI_DB path

```
initialize():
  If bCreatePeptideIndex: call WritePeptideIndex(tp) then return.
  Else: load peptide index.
  Source: DoSearch() lines 2245-2252.

execute():
  Same loop structure as FiStrategy but calls SearchPeptideIndex().

finalize():
  DeallocateMemory().
```

### New File: `search/Pipeline.h/.cpp`

```
// Pipeline.h
class Pipeline
{
public:
   Pipeline(unique_ptr<ISearchStrategy> strategy,
            vector<unique_ptr<IResultWriter>> writers);

   // Drives the full batch search for all files.
   // Replaces the main body of CometSearchManager::DoSearch().
   bool run(SearchSession& session,
            const vector<InputFileInfo*>& files,
            ThreadPool& pool);

private:
   void flushAndWrite(SearchSession& session);
   unique_ptr<ISearchStrategy>          _strategy;
   vector<unique_ptr<IResultWriter>>    _writers;
};
```

### Strategy factory

A free function in `SearchManager.cpp` selects the right strategy based on
`g_staticParams.iDbType` and the index-build flags. This is the single location
where the `if (iDbType == FI_DB)` logic lives after Phase 5:

```cpp
static unique_ptr<ISearchStrategy> makeStrategy(const StaticParams& p)
{
   if (p.iDbType == DbType::FI_DB || p.options.bCreateFragmentIndex)
      return make_unique<FiStrategy>();
   if (p.iDbType == DbType::PI_DB || p.options.bCreatePeptideIndex)
      return make_unique<PiStrategy>();
   return make_unique<FastaStrategy>();
}
```

### `DoSearch()` after Phase 5

The 4,585-line `CometSearchManager::DoSearch()` body reduces to approximately:

```cpp
bool CometSearchManager::DoSearch()
{
   if (!InitializeStaticParams()) return false;
   if (!ValidateOutputFormat())   return false;
   if (!ValidateScanRange())      return false;
   if (!ValidatePeptideLengthRange()) return false;

   try { _tp->fillPool(g_staticParams.options.iNumThreads); }
   catch (...) { /* error */ return false; }

   SearchSession session(g_staticParams);
   session.bPerformDatabaseSearch = ValidateSequenceDatabaseFile();
   session.bPerformSpecLibSearch  = ValidateSpecLibFile();

   auto strategy = makeStrategy(g_staticParams);
   auto writers  = makeWriters(g_staticParams);    // builds IResultWriter vector
   Pipeline pipeline(move(strategy), move(writers));

   return pipeline.run(session, g_pvInputFiles, *_tp);
}
```

### RTS path

The RTS entry points (`InitializeSingleSpectrumSearch`,
`DoSingleSpectrumSearchMultiResults`, `FinalizeSingleSpectrumSearch`) are
**not moved into the strategy pattern** in Phase 5. They are thread-safe,
well-tested, and called from C# via `CometWrapper`. Refactoring them carries
high wrapper-compatibility risk. They remain in `CometSearchManager` and use
`g_staticParams` / `g_iFragmentIndex` etc. directly. This is explicitly out
of scope for Phase 5.

### Verification

```
make cclean && make
python3 tests/unit/run_tests.py --comet comet.exe   # all 17 must pass
# batch HeLa mzXML diff against Phase 4 baseline -- identical PSM data
# run integration test (T17/T18) against human.small.fasta
# confirm RTS path still compiles and executes via RealtimeSearch.exe smoke test
```

---

## Build System

The Linux `Makefile` currently globs `CometSearch/*.cpp`. After Phase 1 it needs
to include subdirectory sources. Update the `SRCS` variable in `CometSearch/Makefile`:

```makefile
SRCS := $(wildcard *.cpp) \
        $(wildcard core/*.cpp) \
        $(wildcard threading/*.cpp) \
        $(wildcard output/*.cpp) \
        $(wildcard search/*.cpp)
```

The Windows `CometSearch.vcxproj` needs a new `<ClCompile>` entry for each new
`.cpp` added. Use `<Filter>` entries to create matching Solution Explorer folders.

---

## Line-Ending Rule

All new `.h` and `.cpp` files must use CRLF line endings (Windows `\r\n`).
Verify after creating each file:
```bash
file CometSearch/threading/SearchMemoryPool.h   # must show "CRLF line terminators"
```
If not, run `unix2dos <file>` before committing.

---

## Phase Summary

| Phase | Target | Key Files Changed | Globals Retired | Risk |
|-------|--------|-------------------|-----------------|------|
| 1 | Split `CometDataInternal.h` | `core/Constants.h`, `core/Params.h`, `core/Types.h` | None (shim kept) | Low |
| 2 | `SearchMemoryPool` | `threading/SearchMemoryPool.h/.cpp`, `CometSearch.h/.cpp`, `CometSearchManager.cpp` | `g_searchMemoryPoolMutex`, `g_searchPoolCV`, `g_bCometSearchMemoryAllocated` | Low |
| 3 | `IResultWriter` | `output/IResultWriter.h`, 5 writer files, `CometSearchManager.cpp` | None yet | Medium |
| 4 | `SearchSession` | `search/SearchSession.h`, `CometSearchManager.cpp`, `CometPreprocess.cpp`, `CometSearch.cpp`, `CometPostAnalysis.cpp` | `g_pvQuery`, `g_pvQueryMS1`, `g_pvQueryMutex`, `g_bPerformDatabaseSearch`, `g_bPerformSpecLibSearch`, `g_bIdxNoFasta`, `g_bPlainPeptideIndexRead`, `g_bSpecLibRead`, `g_cometStatus` | Medium-high |
| 5 | `ISearchStrategy` + `Pipeline` | `search/ISearchStrategy.h`, `FiStrategy`, `FastaStrategy`, `PiStrategy`, `Pipeline.h/.cpp`, `SearchManager.cpp` | Search-path `if/else` chains | High |
