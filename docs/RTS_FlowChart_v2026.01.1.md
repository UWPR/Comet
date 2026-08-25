# Real-Time Search (RTS) Flow: v2026.01.1

This document traces the complete call chain for PI (Peptide Index) and FI (Fragment Ion Index)
searches through the real-time search path: C# -> CometWrapper.dll -> CometSearchManager.

---

## Shared Scaffold (both PI and FI)

```
C# SearchMS1MS2.cs
  |
  |  (once, before first spectrum)
  +---> CometSearchManagerWrapper::InitializeSingleSpectrumSearch()
          |
          v
        CometSearchManager::InitializeSingleSpectrumSearch()  [CometSearchManager.cpp:3206]
          |
          |-- if (singleSearchInitializationComplete) return  <- plain bool, not atomic
          |-- InitializeStaticParams()   sets iIndexDb (1=FI, 2=PI)
          |-- ValidateSequenceDatabaseFile()
          |     reads .idx header -> sets iIndexDb if .idx provided
          |-- CometPreprocess::AllocateMemory()
          |-- CometSearch::AllocateMemory()
          |-- tp->fillPool()
          |
          |  [see FI or PI init below]
          |
          +-- singleSearchInitializationComplete = true  (plain bool, no mutex)
```

---

## FI Initialization (iIndexDb == 1)

```
InitializeSingleSpectrumSearch()
  |
  |-- CometSearch::AllocateMemory()   allocates _pbSearchMemoryPool + _ppbDuplFragmentArr
  |
  |-- bCreateFragmentIndex == true?   (.idx absent, FASTA present)
  |   YES: CometSearchManager::CreateFragmentIndex()
  |         -> CometSearchManager::DoSearch() with m_bRTSIndexBuild=true
  |              -> CometFragmentIndex::WriteFIPlainPeptideIndex(tp)
  |                   (scans FASTA, writes plain-peptide .idx to disk)
  |              -> CometSearch::DeallocateMemory()   frees pool allocated above
  |              -> return early (skips spec-lib load and batch-search logic)
  |         CometSearch::AllocateMemory()   re-allocate pool freed by DoSearch()
  |
  |-- CometFragmentIndex::ReadPlainPeptideIndex()
  |     populates g_vRawPeptides
  |     sets g_bPlainPeptideIndexRead = true
  |
  +-- CometFragmentIndex::CreateFragmentIndex(tp)
        builds g_iFragmentIndex[] (CSR posting lists, in-memory)
```

---

## PI Initialization (iIndexDb == 2)

```
InitializeSingleSpectrumSearch()
  |
  |-- bCreatePeptideIndex == true?
  |   YES: CometPeptideIndex::WritePeptideIndex(tp)
  |        (scans FASTA, writes .idx to disk)
  |
  |-- ** NO in-memory load at init **
  |   PI index is loaded lazily on first SearchPeptideIndex() call
  +-- (done)
```

---

## Per-Spectrum Search (both PI and FI)

```
C# SearchMS1MS2.cs
  |
  |  (per spectrum -- called one at a time, NOT concurrent-safe)
  +---> CometSearchManagerWrapper::DoSingleSpectrumSearchMultiResults()
          |
          v
        CometSearchManager::DoSingleSpectrumSearchMultiResults()  [CometSearchManager.cpp:3341]
          |
          |-- InitializeSingleSpectrumSearch()     (no-op: bool flag already true)
          |-- g_staticParams.tRealTimeStart = now() <- SHARED GLOBAL written here
          |-- CometPreprocess::Reset()
          |-- new double[iArraySizeGlobal]           heap alloc per call
          |
          |-- CometPreprocess::PreprocessSingleSpectrum()
          |     bins peaks, creates Query object
          |     pushes Query* into g_pvQuery          <- SHARED GLOBAL VECTOR
          |
          |-- AllocateResultsMem()
          |-- pQuery = g_pvQuery.at(0)
          |-- g_massRange.dMinMass/dMaxMass = ...    <- SHARED GLOBALS written here
          |
          |-- CometSearch::RunSearch(tp)   [CometSearch.cpp:94]
          |
          |       iIndexDb == 1 (FI)?                 iIndexDb == 2 (PI)?
          |      YES                                   YES
          |       |                                     |
          |       v                                     v
          |   SearchFragmentIndex(iWhichQuery=0, tp)  SearchPeptideIndex(tp)
          |       |                                     |
          |       for each peak in g_pvQuery[0]:        fopen(.idx)  <- PER SPECTRUM
          |         BIN(mass * z)                       |
          |         lookup g_iFragmentIndex[]           if first call:
          |           (O(1), in-memory, no I/O)           fread header
          |         count matches per peptide ID          fread g_pvProteinsList
          |       filter by count threshold              g_bPeptideIndexRead = true
          |       ScoreByFI()                           |
          |         full Xcorr for candidates           fread lReadIndex[]  <- PER SPECTRUM
          |       -> g_pvQuery[0]->_pResults              (mass index table)
          |                                             fseek to mass window
          |                                             ReadPeptideIndexEntry() loop
          |                                               AnalyzePeptideIndex()
          |                                                 generate b/y ions
          |                                                 compute Xcorr
          |                                             fclose(.idx)
          |                                            -> g_pvQuery[0]->_pResults
          |
          |-- sort _pResults by Xcorr
          |-- CometPostAnalysis::CalculateSP(pQuery->_pResults, ...)
          |-- CometPostAnalysis::CalculateEValue(iWhichQuery=0, ...)
          |-- CometPostAnalysis::CalculateDeltaCn(iWhichQuery=0)
          |-- CometPostAnalysis::CalculateAScorePro(...)   (if phospho enabled)
          |
          |-- fopen(.idx)  <- SECOND file open (always, for protein names)
          |-- for each top-N result:
          |     GetProteinNameString(fp, ...)  <- file seek per result
          |-- fclose(.idx)
          |
          +-- delete[] pdTmpSpectrum
              clear g_pvQuery
```

---

## Full Flowchart

```
C# SearchMS1MS2.cs
  |
  |  (once, before first spectrum)
  +---> CometSearchManagerWrapper::InitializeSingleSpectrumSearch()
          |
          v
        CometSearchManager::InitializeSingleSpectrumSearch()  [CometSearchManager.cpp:3206]
          |
          |-- if (singleSearchInitializationComplete) return  <- plain bool, not atomic
          |-- InitializeStaticParams()   sets iIndexDb (1=FI, 2=PI)
          |-- ValidateSequenceDatabaseFile()
          |     reads .idx header -> sets iIndexDb if .idx provided
          |-- CometPreprocess::AllocateMemory()
          |-- CometSearch::AllocateMemory()
          |-- tp->fillPool()
          |
          |                  iIndexDb == 1 (FI)?
          +----YES-------------------------------------------+
          |   bCreateFragmentIndex==true?  (.idx absent)     |
          |   YES: CreateFragmentIndex()                     |
          |         -> DoSearch() [m_bRTSIndexBuild=true]    |
          |              WriteFIPlainPeptideIndex(tp)         |
          |              DeallocateMemory()                   |
          |              return early                         |
          |         CometSearch::AllocateMemory()  re-alloc  |
          |                                                   |
          |   ReadPlainPeptideIndex()                         |
          |     g_vRawPeptides, g_bPlainPeptideIndexRead=T    |
          |   CreateFragmentIndex(tp)                         |
          |     g_iFragmentIndex (CSR posting lists)          |
          +----------------------------------------------------+
          |
          |                  iIndexDb == 2 (PI)?
          +----YES-------------------------------------------+
          |   bCreatePeptideIndex==true?                     |
          |   YES: CometPeptideIndex::WritePeptideIndex(tp)  |
          |        (scan FASTA, write .idx)                  |
          |                                                  |
          |   ** NO in-memory load **                        |
          |   PI index loaded on first SearchPeptideIndex()  |
          +---------------------------------------------------+
          |
          +-- singleSearchInitializationComplete = true  (plain bool)

  |  (per spectrum -- one call at a time, NOT concurrent-safe)
  +---> CometSearchManagerWrapper::DoSingleSpectrumSearchMultiResults()
          |
          v
        CometSearchManager::DoSingleSpectrumSearchMultiResults()  [CometSearchManager.cpp:3341]
          |
          |-- InitializeSingleSpectrumSearch()   (no-op: bool flag already true)
          |-- g_staticParams.tRealTimeStart = now()   SHARED GLOBAL written here
          |-- CometPreprocess::Reset()
          |-- new double[iArraySizeGlobal]            heap alloc per call
          |
          |-- CometPreprocess::PreprocessSingleSpectrum()
          |     bins peaks, creates Query, pushes into g_pvQuery  <- GLOBAL VECTOR
          |
          |-- AllocateResultsMem()
          |-- pQuery = g_pvQuery.at(0)
          |-- g_massRange.dMinMass/dMaxMass = ...     SHARED GLOBALS written here
          |
          |-- CometSearch::RunSearch(tp)   [CometSearch.cpp:94]
          |
          |           iIndexDb == 1?                     iIndexDb == 2?
          |    YES-----------+                    YES---------+
          |    |                                  |
          |    SearchFragmentIndex(0, tp)          SearchPeptideIndex(tp)
          |    |                                  |
          |    for each peak in g_pvQuery[0]:     fopen(.idx)  PER SPECTRUM
          |      BIN(mass*z)                      |
          |      lookup g_iFragmentIndex[]         if first call:
          |        (O(1), in-memory, no I/O)        fread header
          |      count matches per pep ID           fread g_pvProteinsList
          |    filter, sort, ScoreByFI()             g_bPeptideIndexRead=true
          |      full Xcorr                        |
          |    -> g_pvQuery[0]->_pResults           fread lReadIndex[]  PER SPECTRUM
          |                                          (mass index table)
          |                                         fseek to mass window
          |                                         ReadPeptideIndexEntry()  loop
          |                                           AnalyzePeptideIndex()
          |                                             gen b/y ions, Xcorr
          |                                         fclose(.idx)
          |                                        -> g_pvQuery[0]->_pResults
          |
          |-- sort _pResults by Xcorr
          |-- CalculateSP(pQuery->_pResults, ...)
          |-- CalculateEValue(iWhichQuery=0, ...)
          |-- CalculateDeltaCn(iWhichQuery=0)
          |-- CalculateAScorePro(...)   (if enabled)
          |
          |-- fopen(.idx)  <- SECOND file open for protein names (always)
          |-- for each top-N result:
          |     GetProteinNameString(fp, ...)  <- file seeks per result
          |-- fclose(.idx)
          |
          +-- delete[] pdTmpSpectrum; clear g_pvQuery
```

---

## Key Differences: v2026.01.1 vs Current (master)

| Aspect                     | v2026.01.1                               | Current (master)                                 |
|----------------------------|------------------------------------------|--------------------------------------------------|
| Init guard                 | `bool singleSearchInitializationComplete` (no mutex) | `atomic<bool>` + `std::mutex` double-check locking |
| Concurrency                | NOT concurrent-safe (one call at a time) | Fully concurrent (multiple C# Tasks in parallel) |
| Timeout reference          | `g_staticParams.tRealTimeStart` (shared global, overwritten per call) | `pQuery->tSearchStart` (per-Query, thread-local) |
| Spectrum preprocessing     | `PreprocessSingleSpectrum()` -> pushes into `g_pvQuery` | `PreprocessSingleSpectrumThreadLocal()` -> caller-owned `Query*` |
| PI in-memory load at init  | No -- loaded lazily per spectrum         | Yes -- `g_vRawPeptides` + `g_vDBIndexVariants` (mass-sorted per-variant array, built once per session by `CometPeptideIndex::GenerateVariantArray()`) loaded/built at init. **Not** `g_pvDBIndex` -- that type is build-time-only since `docs/20260730_PI_reduction.md`'s PI/FI unification (copied into `g_vRawPeptides` and cleared before `WritePeptideIndex()` returns); `DBIndex` the *type* survives only as a stack-local reconstruction target `MaterializeOneEntry()` builds per surviving search candidate. See `docs/DataStructures.md`'s `DBIndex`/`FragmentPeptidesStruct` sections. |
| PI per-spectrum I/O        | `fopen` + `fread` + `fclose` per spectrum | Binary search in `g_vDBIndexVariants` by mass (no I/O), then `CometPeptideIndex::MaterializeOneEntry()` reconstructs a full `DBIndex` per surviving candidate |
| Protein name lookup        | `fopen` + file seeks per result (always) | `g_pvProteinNameCache` (in-memory unordered_map) |
| `RunSearch()` overload     | `RunSearch(ThreadPool* tp)` (shared batch/RTS) | `RunSearch(Query* pQuery)` (RTS-only, no globals) |
