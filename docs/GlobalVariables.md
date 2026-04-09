# Global Variables Reference

All globals are defined in `CometSearch/CometSearchManager.cpp` (unless noted) and declared `extern` in `CometSearch/CometDataInternal.h`.

---

## Core search state

| Variable | Type | Thread-safe? | Notes |
|----------|------|:------------:|-------|
| `g_staticParams` | `StaticParams` | Read-only after init | All user-configurable search parameters. Written once by `InitializeStaticParams()`, then read-only for the life of the search. Safe to read from any thread. |
| `g_massRange` | `MassRange` | Batch path: written per-batch | Written in `DoSearch()` at the start of each spectrum batch. The RTS paths (`DoSingleSpectrumSearchMultiResults`, `DoMS1SearchMultiResults`) do **not** write `g_massRange`; they derive mass limits from the per-call `Query*` / `QueryMS1*`. |
| `g_cometStatus` | `CometStatus` | Shared mutable | Error/cancel status. Any thread can call `SetError()` or `IsCancel()`. Internally mutex-protected. |

---

## Spectrum batch containers

Used only in the batch search path (`DoSearch` → `RunSearch`). The RTS paths do not touch these.

| Variable | Type | Thread-safe? | Notes |
|----------|------|:------------:|-------|
| `g_pvQuery` | `vector<Query*>` | Batch path only | One `Query*` per spectrum/charge combination for the current batch. Populated by `CometPreprocess`, consumed by `CometSearch` and `CometPostAnalysis`. Not safe for concurrent writes without `g_pvQueryMutex`. |
| `g_pvQueryMS1` | `vector<QueryMS1*>` | Batch path only | Analogous to `g_pvQuery` for MS1 spectral library batch searches. |
| `g_pvQueryMutex` | `Mutex` | — | Protects `g_pvQuery` insertions during batch preprocessing. |
| `g_pvInputFiles` | `vector<InputFileInfo*>` | Read-only after init | List of input files to search; set before `DoSearch()` begins. |

---

## Fragment index (read-only after loading)

Populated during index build / load; treated as read-only during all searches. Safe for concurrent reads from RTS threads.

| Variable | Type | Notes |
|----------|------|-------|
| `g_iFragmentIndex` | `unsigned int**` | 2D array: `[BIN(fragment mass)][entry index]`. Each row lists which entries in `g_vFragmentPeptides` contain that fragment mass bin. |
| `g_iCountFragmentIndex` | `unsigned int*` | `[BIN(fragment mass)]` — count of entries in each row of `g_iFragmentIndex`. |
| `g_vFragmentPeptides` | `vector<FragmentPeptidesStruct>` | Mass-sorted list of all (peptide, mod-state) combinations. Each entry references a row in `g_vRawPeptides` via `iWhichPeptide`. |
| `g_vRawPeptides` | `vector<PlainPeptideIndexStruct>` | List of unique unmodified peptide sequences with protein file-position pointers. |
| `g_bIndexPrecursors` | `bool*` | Boolean bitmap over precursor mass bins; marks which precursor masses are present in the current input file(s). |
| `g_bPeptideIndexRead` | `std::atomic<bool>` | Set to `true` once the peptide index has been fully loaded. Checked with `acquire` ordering before RTS searches begin. |
| `g_bPlainPeptideIndexRead` | `bool` | Set to `true` if the plain peptide index was read and a fragment index was generated from it. |

---

## Spectral library / MS1 alignment (read-only after loading)

| Variable | Type | Notes |
|----------|------|-------|
| `g_vSpecLib` | `vector<SpecLibStruct>` | In-memory spectral library entries. Each entry holds peaks, charge, RT, and a unit-vector representation for dot-product scoring. |
| `g_vulSpecLibPrecursorIndex` | `vector<vector<unsigned int>>` | Mass index into `g_vSpecLib`; maps precursor mass bins to library entry indices for fast lookup. |
| `g_bSpecLibRead` | `bool` | Set to `true` once the spectral library is fully loaded. |
| `g_bPerformSpecLibSearch` | `bool` | `true` if MS1 speclib search is active for this run. |
| `g_bPerformDatabaseSearch` | `bool` | `true` if FASTA/index database search is active for this run. |
| `RetentionMatchHistory` | `std::deque<RetentionMatch>` | Rolling window of (query RT, reference RT) pairs used by the MS1 RT aligner. Protected by `g_ms1AlignerMutex`. |

---

## Protein database index (read-only after loading)

| Variable | Type | Notes |
|----------|------|-------|
| `g_pvDBIndex` | `vector<DBIndex>` | Peptide index entries (mass-sorted). Each entry holds peptide sequence, mass, var-mod encoding, and a protein file-position pointer. |
| `g_pvProteinNames` | `map<long long, IndexProteinStruct>` | Maps protein file-position to accession string and ordinal. |
| `g_pvProteinsList` | `vector<vector<comet_fileoffset_t>>` | Maps index positions to lists of protein file offsets (for multi-protein peptides). |
| `g_pvDIAWindows` | `vector<double>` | Flat list of DIA isolation window edges (start, end, start, end, …). Empty if not doing DIA. |

---

## AScorePro (read-only after init)

| Variable | Type | Notes |
|----------|------|-------|
| `g_AScoreOptions` | `AScoreProCpp::AScoreOptions` | Configuration for the AScorePro phosphosite localization algorithm. Set once at init. |
| `g_AScoreInterface` | `AScoreProCpp::AScoreDllInterface*` | Pointer to the AScorePro DLL interface. Each RTS thread uses its own copy of data through this interface; the pointer itself is read-only. |

---

## Combinatorics / mod permutation tables (read-only after init)

Used by the variable mod permutation engine (`CometModificationsPermuter`).

| Variable | Notes |
|----------|-------|
| `MOD_NUMBERS` | `vector<ModificationNumber>` — precomputed modification number combinations. |
| `MOD_SEQS` | `vector<string>` — unique modifiable sequences. |
| `MOD_SEQ_MOD_NUM_START` / `MOD_SEQ_MOD_NUM_CNT` | `int*` — index into `MOD_NUMBERS` per modifiable sequence. |
| `PEPTIDE_MOD_SEQ_IDXS` | `int*` — maps peptides to their modifiable sequence index. |
| `MOD_NUM` | `int` — total number of distinct modification combinations. |

---

## Threading primitives

| Variable | Type | Notes |
|----------|------|-------|
| `g_pvQueryMutex` | `Mutex` | Protects `g_pvQuery` insertions during batch preprocessing. |
| `g_pvDBIndexMutex` | `Mutex` | Protects database index reads where concurrent access is possible. |
| `g_preprocessMemoryPoolMutex` | `Mutex` | Protects the shared preprocessing memory pool. |
| `g_searchMemoryPoolMutex` | `Mutex` | Protects the shared search memory pool. |
| `g_ms1AlignerMutex` | `Mutex` | Protects `RetentionMatchHistory` updates in `DoMS1SearchMultiResults`. |
| `g_vSpecLibMutex` | `Mutex` | Protects speclib access where needed. |
| `g_dbIndexMutex` | `Mutex` | Protects DB index access where needed. |

---

## RTS initialization flags

| Variable | Type | Notes |
|----------|------|-------|
| `singleSearchInitializationComplete` | `std::atomic<bool>` | Set to `true` (with `release` ordering) after `InitializeSingleSpectrumSearch()` completes. Checked with `acquire` ordering at the top of `DoSingleSpectrumSearchMultiResults()`. Ensures all RTS threads see fully initialized globals. |
| `singleSearchMS1InitializationComplete` | `std::atomic<bool>` | Same pattern for `InitializeSingleSpectrumMS1Search()` / `DoMS1SearchMultiResults()`. |

---

## Memory allocation state flags

| Variable | Notes |
|----------|-------|
| `g_bCometPreprocessMemoryAllocated` | `true` when `CometPreprocess::AllocateMemory()` has been called. |
| `g_bCometSearchMemoryAllocated` | `true` when `CometSearch::AllocateMemory()` has been called. |
| `g_bIdxNoFasta` | `true` when searching a `.idx` file without the corresponding `.fasta` present. |

---

## Version and environment

| Variable | Type | Notes |
|----------|------|-------|
| `g_sCometVersion` | `string` | Full version string including git hash. Assembled once in `main()` or the first RTS call. |
| `g_psGITHUB_SHA` | `string` | Raw `GITHUB_SHA` env var trimmed to 7 characters; empty string if not set. |

---

## StaticParams sub-structs (key members)

`g_staticParams` is a large aggregate. The most-referenced sub-structs during search:

| Member | Type | Purpose |
|--------|------|---------|
| `options` | `Options` | All boolean/integer user options (decoy search mode, output formats, charge limits, index creation, etc.). |
| `tolerances` | `ToleranceParams` | Precursor tolerance, fragment bin size + offset, isotope error setting. |
| `massUtility` | `MassUtil` | AA mass tables (`pdAAMassParent[]`, `pdAAMassFragment[]`), used in hot loops. |
| `precalcMasses` | `PrecalcMasses` | Pre-computed N/C-terminus proton values; `iMinus17`/`iMinus18` BIN'd values. |
| `variableModParameters` | `VarModParams` | Variable mod definitions (`varModList[VMODS]`), compound mod list, flags. |
| `ionInformation` | `IonInfo` | Which ion series (a/b/c/x/y/z) are active; water/ammonia loss flag. |
| `enzymeInformation` | `EnzymeInfo` | Enzyme cut rules, missed cleavage count, search/sample enzyme distinction. |
| `databaseInfo` | `DBInfo` | FASTA path; `iTotalNumProteins` and `uliTotAACount` updated during batch scan. |
| `dInverseBinWidth` / `dOneMinusBinOffset` | `double` | Used in the `BIN(mass)` macro on every fragment — computed once at init. |

---

## Thread-safety summary

```
Safe to read from any concurrent RTS thread (after init):
  g_staticParams, g_iFragmentIndex, g_iCountFragmentIndex,
  g_vFragmentPeptides, g_vRawPeptides, g_pvProteinNames, g_pvProteinsList,
  g_vSpecLib, g_vulSpecLibPrecursorIndex, g_pvDIAWindows,
  g_AScoreOptions, g_AScoreInterface, MOD_NUMBERS, MOD_SEQS,
  g_massRange.iMaxFragmentCharge (after batch setup)

Written per batch (batch path only — not touched by RTS):
  g_pvQuery, g_pvQueryMS1,
  g_massRange.dMinMass / dMaxMass / bNarrowMassRange,
  g_staticParams.databaseInfo.uliTotAACount

Protected by mutex (safe to call from any thread):
  RetentionMatchHistory (g_ms1AlignerMutex)

Always shared mutable — use sparingly from hot paths:
  g_cometStatus (SetError/IsCancel are mutex-protected internally)

Atomic, checked with acquire/release ordering:
  singleSearchInitializationComplete, singleSearchMS1InitializationComplete,
  g_bPeptideIndexRead
```
