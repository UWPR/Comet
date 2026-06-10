---
name: comet-codebase
description: Navigate the Comet codebase. Use when looking for where a feature is implemented, which file owns a function, how data flows between layers, or what global variables are involved in a code path.
---

# Comet Codebase Navigation

## Layer overview

```
C# RealtimeSearch  →  C++/CLI CometWrapper  →  C++ CometSearch
(SearchMS1MS2.cs)     (CometWrapper.vcxproj)   (libcometsearch)
```

The batch path (`Comet.exe`) bypasses C# and the wrapper entirely.

## File responsibility map

### CometSearch/ (core library)

| File | Owns |
|------|------|
| `CometSearchManager.cpp` | Top-level orchestration; `DoSingleSpectrumSearchMultiResults` (RTS entry point); `DoMS1SearchMultiResults`; parameter loading |
| `CometSearch.cpp` | Fragment index query (`SearchFragmentIndex`); XCorr scoring; `AcquirePoolSlot`; `RunSearch` overloads |
| `CometFragmentIndex.cpp` | Index build (`CreateFragmentIndex`); index write/read (`.idx` file); `g_pvProteinNameCache` population |
| `CometPeptideIndex.cpp` | Peptide index build/read; also populates `g_pvProteinNameCache` |
| `CometPreprocess.cpp` | Spectrum binning and noise reduction; `PreprocessSingleSpectrumThreadLocal` (RTS path); `GetRtsRawDataBuffer` |
| `CometPostAnalysis.cpp` | SP score, E-value, delta-Cn, AScore localization |
| `CometDataInternal.h` | All global variable declarations; key structs (`Query`, `Results`, `sDBEntry`); `WIDTH_REFERENCE = 512` |
| `CometSearch.h` | `CometSearch` class; `SearchThreadData` struct (batch thread pool data + destructor) |
| `CometFragmentIndexReader.h` | Read-only wrapper (`FragmentIndexReader`) around global index arrays |

### RealtimeSearch/

| File | Owns |
|------|------|
| `SearchMS1MS2.cs` | Main RTS loop; pre-read phase (`PrereadScan`); parallel task launch; timing (`cumulativeElapsedMS2`) |

## Key global variables

| Global | Type | Thread-safe? | Notes |
|--------|------|-------------|-------|
| `g_staticParams` | struct | Read-only after init | All search parameters |
| `g_iFragmentIndex` | `unsigned int*` | Read-only after init | CSR flat data; elements are peptide indices |
| `g_iFragmentIndexOffset` | `uint64_t*` | Read-only after init | CSR bin offsets; **must be 64-bit** — can exceed UINT_MAX |
| `g_vFragmentPeptides` | `vector<FragmentPeptidesStruct>` | Read-only after init | One entry per (peptide × mod), mass-sorted |
| `g_vRawPeptides` | `vector<PlainPeptideIndexStruct>` | Read-only after init | Unmodified peptides + protein file pointers |
| `g_pvProteinsList` | `vector<vector<comet_fileoffset_t>>` | Read-only after init | Maps peptide index → protein file offsets |
| `g_pvProteinNameCache` | `unordered_map<comet_fileoffset_t, string>` | Read-only after init | Protein names; ~7 MB for human target-decoy |
| `g_pvQuery` | `vector<Query*>` | Shared mutable | Batch path only; guarded by mutex |
| `_pbSearchMemoryPool` | `bool*` | Guarded by `g_searchMemoryPoolMutex` | Pool slot availability |
| `g_searchMemoryPoolMutex` | `mutex` | — | Guards pool + `g_searchPoolCV` |
| `g_searchPoolCV` | `condition_variable` | — | Signals free pool slot |

## RTS search path (per spectrum)

```
C#: scanDataQueue.TryDequeue(PrereadScan)
  → globalSearchMgr.DoSingleSpectrumSearchMultiResults(...)   [CometWrapper → C++]
    → PreprocessSingleSpectrumThreadLocal()                    [CometPreprocess.cpp]
    → CometSearch::RunSearch(Query*)                           [CometSearch.cpp]
      → AcquirePoolSlot()                                      [condition_variable wait]
      → SearchFragmentIndex(pQuery, pbDuplFragment)            [main scoring loop]
      → release slot + notify_one
    → CalculateSP / CalculateEValue / CalculateDeltaCn         [CometPostAnalysis.cpp]
    → protein name lookup: g_pvProteinNameCache.find(offset)   [in-memory, no I/O]
```

## Batch search path

```
Comet.exe main()
  → CometSearchManager::DoSearch()
    → reads all spectra via MSToolkit into g_pvQuery
    → ThreadPool dispatches RunSearch(int iStart, int iEnd, ThreadPool*)
      → per thread: AcquirePoolSlot via SearchThreadData
      → SearchFragmentIndex(g_pvQuery[i], ...)
    → CometPostAnalysis (batch)
    → write results (txt/pepXML/sqt/percolator/mzIdentML)
```

## Key struct locations

- `Query` — `CometDataInternal.h` (~line 400): holds preprocessed spectrum, results array
- `Results` — `CometDataInternal.h`: per-hit data (peptide, xcorr, proteins, mods)
- `SearchThreadData` — `CometSearch.h`: batch thread wrapper; destructor releases pool slot
- `FragmentPeptidesStruct` — `CometDataInternal.h`: (peptide index, mod index, mass)
- `PrereadScan` — `SearchMS1MS2.cs`: C# struct holding pre-read scan data for RTS

## Index file format (.idx)

- Text header (search parameters)
- Protein name blocks: N × 512 bytes (`WIDTH_REFERENCE`), one per protein
- Peptide records: length + sequence + prevAA + nextAA + mass + siVarModFilter + protein offset
- `g_pvProteinsList`: vector-of-vectors of file offsets
- Permutations / modification sequences
- Fragment ion permutation data

The fragment ion index itself (`g_iFragmentIndex`, `g_iFragmentIndexOffset`) is **NOT stored in the .idx file** — it is rebuilt in memory on every run from the peptide records.
