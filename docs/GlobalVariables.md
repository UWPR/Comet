# Global Variables Reference

All globals are defined in `CometSearch/CometSearchManager.cpp` (unless noted). Most are declared `extern` reachable through `CometSearch/CometDataInternal.h`, which is a compatibility shim that just `#include`s `core/Constants.h`, `core/Params.h`, and `core/Types.h` -- so e.g. `g_staticParams`/`g_massRange` are actually declared extern in `core/Params.h`. `g_cometStatus` is the one exception: it's declared extern in the standalone `CometSearch/CometStatus.h`, which `CometDataInternal.h` does **not** include. A few single-translation-unit globals (e.g. `g_ms1AlignerMutex`) are never declared `extern` anywhere since nothing outside `CometSearchManager.cpp` uses them.

---

## Core search state

| Variable | Type | Thread-safe? | Notes |
|----------|------|:------------:|-------|
| `g_staticParams` | `StaticParams` | Read-only after init | All user-configurable search parameters. Written once by `InitializeStaticParams()`, then read-only for the life of the search. Safe to read from any thread. |
| `g_massRange` | `MassRange` | Mixed -- see below | `dMinMass`/`dMaxMass` are set once at init on *either* path -- `DoSearch()` (batch) or `InitializeSingleSpectrumSearch()` (RTS, `CometSearchManager.cpp:2221-2222`) -- as outer bounds; not batch-only. On the legacy three-sweep batch paths (FASTA, PI_DB/FI_DB legacy fallback), `RunSearchAndPostAnalysis()` (`search/SearchUtils.cpp:236-238`) then re-narrows them every batch from that batch's `SearchSession.queries` -- the fused batch path and RTS do not do this re-narrowing (see `docs/DataStructures.md`'s `MassRange` section for detail). `bNarrowMassRange` is likewise set once during `CreateFragmentIndex()`, which both paths call during their respective init. Only `usiMaxFragmentCharge` is genuinely batch-only: it's updated under `_maxChargeMutex` inside `CometPreprocess::PreprocessSpectrum` (the batch preprocessing path), which the RTS thread-local preprocessing path never calls. |
| `g_cometStatus` | `CometStatus` | Shared mutable | Error/cancel status. Any thread can call `SetStatus(CometResult[, msg])` or `IsCancel()`/`IsError()` (there is no `SetError()`). Internally mutex-protected. |

---

## Spectrum batch containers

Used only in the batch search path (`DoSearch` -> `Pipeline` -> strategies). The RTS paths do not touch these. Query lists (`g_pvQuery`/`g_pvQueryMS1`) were moved into `SearchSession` (defined in `search/SearchSession.h`) as part of the Phase 4-5 architecture migration; per-run flags were only partly moved -- see the note below.

| Variable | Type / Location | Thread-safe? | Notes |
|----------|----------------|:------------:|-------|
| `SearchSession::queries` | `vector<Query*>` | Mixed -- see below | One `Query*` per spectrum/charge combination for the current batch. Populated by `CometPreprocess`, consumed by `CometSearch` and `CometPostAnalysis`. Replaces the former global `g_pvQuery`. On the fused FI_DB batch path (`FusedLoadAndSearchSpectra`), workers append lock-free to their own per-slot vectors and this is filled by one serial concatenation after the thread-pool join -- `queriesMutex` is not used there. Other batch paths (`LoadAndPreprocessSpectra`, etc.) still guard direct pushes with `queriesMutex`. |
| `SearchSession::ms1Queries` | `vector<QueryMS1*>` | Guarded by `queriesMutex` | Analogous to `queries` for MS1 spectral library batch searches. Replaces the former global `g_pvQueryMS1`. |
| `SearchSession::queriesMutex` | `std::mutex` | -- | Protects `queries` / `ms1Queries` insertions on the non-fused batch paths (see above). Replaces the former `g_pvQueryMutex`. |
| `g_pvInputFiles` | `vector<InputFileInfo*>` | Read-only after init | List of input files to search; set before `DoSearch()` begins. |
| `g_bPerformDatabaseSearch` / `g_bPerformSpecLibSearch` / `g_bIdxNoFasta` | `bool` | Written once per run | Still live bare globals (`CometSearchManager.cpp:94,95,99`), not `SearchSession` members -- despite being conceptually per-run flags, they were never migrated. Written in `DoSearch()` (`g_bPerformDatabaseSearch`, lines ~1953-1962) and `ValidateSequenceDatabaseFile()` (`g_bIdxNoFasta`, lines ~183-188), then copied *into* `session.bPerformDatabaseSearch` etc. rather than replaced. `ValidateSequenceDatabaseFile()` is also called from the RTS init path (`InitializeSingleSpectrumSearch()`), so `g_bIdxNoFasta` is touched by both paths, not batch-only. |

---

## Fragment index (read-only after loading)

Populated during index build / load; treated as read-only during all searches. Safe for concurrent reads from RTS threads.

The fragment index uses a **CSR (Compressed Sparse Row)** layout. For a given fragment mass bin `b`, the entries in `g_vFragmentPeptides` are at positions `g_iFragmentIndexOffset[b]` through `g_iFragmentIndexOffset[b+1] - 1` (half-open interval), and the values stored there are indices into `g_vFragmentPeptides`.

| Variable | Type | Notes |
|----------|------|-------|
| `g_iFragmentIndex` | `unsigned int*` | Flat CSR data array. Each element is an index into `g_vFragmentPeptides`. Entries for bin `b` span `[g_iFragmentIndexOffset[b], g_iFragmentIndexOffset[b+1])`. |
| `g_iFragmentIndexOffset` | `uint64_t*` | CSR offset array; length = (max bin + 1) + 1. Must be 64-bit -- the total entry count can exceed UINT_MAX for large databases with many variable mods. |
| `g_vFragmentPeptides` | `vector<FragmentPeptidesStruct>` | Mass-sorted list of all (peptide, mod-state) combinations. Each entry references a row in `g_vRawPeptides` via `iWhichPeptide`. |
| `g_vRawPeptides` | `RawPeptideTable` (pooled, core/Types.h) | List of unique unmodified peptide sequences with protein row references; sequences in one flat NUL-terminated pool + parallel fixed-field arrays (docs/20260827_PI_memory.md Phase 3). |
| `g_bIndexPrecursors` | `bool*` | Boolean bitmap over precursor mass bins; marks which precursor masses are present in the current input file(s). |
| `g_bPeptideIndexRead` | `std::atomic<bool>` | Set to `true` by `CometPeptideIndex::ReadPeptideIndex()` once the `.idx` file itself has been read -- **not** the same as "fully initialized" (see `g_bPeptideIndexFullyInitialized` below, which gates on more than this). |
| `g_bPeptideIndexFullyInitialized` | `std::atomic<bool>` | Set to `true` only after `CometSearch::EnsurePeptideIndexLoaded()` has completed *all* of: `ReadPeptideIndex()` (which sets `g_bPeptideIndexRead`), `InitializeMassesFromPeptideIndex()`, and (if `print_ascorepro_score`) AScorePro interface creation. This exists because a second concurrent caller (another RTS Task's thread, which the API permits) could otherwise observe `g_bPeptideIndexRead == true` while the first caller is still finishing mass-init/AScorePro setup under the lock, and search with masses that aren't ready yet. `EnsurePeptideIndexLoaded()`'s unlocked fast-path check gates on this flag, not on `g_bPeptideIndexRead`. |
| `g_bPlainPeptideIndexRead` | `bool` (plain, not atomic) | FI_DB analogue of `g_bPeptideIndexRead` -- set once `g_vRawPeptides` has been fully loaded (`CometFragmentIndex.cpp:1604`, re-set at `CometSearchManager.cpp:2291`), then read from RTS search threads (`CometSearch.cpp:175,240`). Not atomic, unlike its sibling; safe in practice only because writes are confined to the init path under the `singleSearchInitializationComplete` happens-before edge. |

---

## Spectral library / MS1 alignment (read-only after loading)

| Variable | Type | Notes |
|----------|------|-------|
| `g_vSpecLib` | `vector<SpecLibStruct>` | In-memory spectral library entries. Each entry holds peaks, charge, RT, and a unit-vector representation for dot-product scoring. |
| `g_vulSpecLibPrecursorIndex` | `vector<vector<unsigned int>>` | Mass index into `g_vSpecLib`; maps precursor mass bins to library entry indices for fast lookup. |
| `pMS1Aligner` | `CometMassSpecAligner` (`CometSearchManager.cpp:110`) | The RT-regression aligner object itself. Its `processRetentionMatch(dQueryRT, dMatchedSpecLibRT)` method is called under `g_ms1AlignerMutex` from `DoMS1SearchMultiResults` and internally maintains the rolling RT-match history (`RetentionMatchHistory` below is the deque it manipulates, not a separate globally-visible container). |
| `RetentionMatchHistory` | `std::deque<RetentionMatch>` | Rolling window of (query RT, reference RT) pairs used by the MS1 RT aligner, owned internally by `pMS1Aligner`. Protected by `g_ms1AlignerMutex`. |
| `dMaxSpecLibRT` | `double` (`CometSearchManager.cpp:106`) | Set once during `InitializeSingleSpectrumMS1Search()` -> `LoadSpecLibMS1Raw()`; read on every RTS MS1 call to rescale query RT against the library's RT range. |
| `g_bSpecLibRead` | `bool` (plain, not atomic) | Set once `g_vSpecLib` has been fully loaded (`CometSpecLib.cpp:93,687`), read at `CometSpecLib.cpp:42`. Same not-atomic-but-init-only-write pattern as `g_bPlainPeptideIndexRead` above. |

---

## Protein database index (read-only after loading)

PI_DB and FI_DB share one unified `.idx` format and one reader,
`CometPeptideIndex::ReadPeptideIndex()` (`docs/20260730_PI_reduction.md` Phase 0). Both
modes populate `g_vRawPeptides` and the protein-list globals below identically -- these are
the only peptide-level data actually read from disk. `g_dbIndexVariants` is PI_DB-specific,
built once per session (not read from disk, see Phase 0.5) only when `iDbType == PI_DB`, and
`g_iFragmentIndex`/`g_iFragmentIndexOffset`/`g_vFragmentPeptides` (built separately by
FI_DB, once per session, from the same `g_vRawPeptides` + regenerated permutation tables --
not listed in this table, see `RealTimeSearch.md`'s thread-safety table) are FI_DB-specific.

| Variable | Type | Notes |
|----------|------|-------|
| `g_pvDBIndex` | `vector<DBIndex>` | **Build-time only, no longer the search-time index.** Phase A digestion output (`CometFragmentIndex::GeneratePlainPeptideIndex()`) inside `CometPeptideIndex::WritePeptideIndex()`: one entry per unique raw peptide, copied into `g_vRawPeptides` and cleared before the function returns. `DBIndex` itself is also used as a transient, stack-local, per-candidate reconstruction target at PI_DB search time (`CometPeptideIndex::MaterializeOneEntry()`, called from `CometSearch::SearchPeptideIndex()`) -- that usage never touches this global. |
| `g_vRawPeptides` | `RawPeptideTable` (pooled, core/Types.h) | One entry per unique unmodified peptide (sequence, protein reference, flank AAs, unmodified mass), loaded from the `.idx` file at search init and kept resident for the whole session; sequences live NUL-terminated in one flat char pool with fixed fields in parallel arrays (~24B/entry + seq bytes, formerly 72B/entry AoS -- docs/20260827_PI_memory.md Phase 3), accessed via `RawPeptideView`. Shared by both search modes. |
| `g_dbIndexVariants` | `PiVariantArray` (SoA, core/Types.h) | PI_DB's compact per-variant array (one entry per (peptide, mod combination) pair: a 4-byte fixed-point mass key + a reference back into `g_vRawPeptides`; 13B/entry across four parallel arrays -- docs/20260827_PI_memory.md Phase 2), mass-sorted. Built once per search session by `CometPeptideIndex::GenerateVariantArray()` from `g_vRawPeptides` + whichever variable mods are active in `g_staticParams.variableModParameters` at that moment -- as of `docs/20260811_restore_idx_header_mods.md`, that's the `.idx` file's own `VariableMod:` header line (parsed into `g_staticParams` by `ParsePeptideIndexHeader()`, overwriting whatever `comet.params` supplied), not live `comet.params` directly. The array itself is still never persisted to disk -- rebuilt fresh every session. `CometSearch::SearchPeptideIndex()` binary-searches this by mass; `CometPeptideIndex::MaterializeOneEntry()` reconstructs a full `DBIndex` per surviving candidate. Only populated when `iDbType == PI_DB`. |
| `g_pvProteinNames` | `map<long long, IndexProteinStruct>` | Maps protein file-position to accession string and ordinal. Used for FASTA searches and legacy index paths. |
| `g_pvProteinsList` | `ProteinsListCSR` | Maps peptide index positions to lists of protein file offsets (for multi-protein peptides). `ProteinsListCSR` is a CSR-layout replacement for `vector<vector<comet_fileoffset_t>>`; exposes the same `operator[]`/`size()`/range-for interface but uses only two heap allocations total. |
| `g_pvProteinNameCache` | `unordered_map<comet_fileoffset_t, string>` | Protein name lookup cache for index-based searches. Populated at index load time from the protein name blocks in the `.idx` file. Maps protein file-position offsets to accession strings. ~7 MB for a human target-decoy database. Allows O(1) protein name resolution during RTS without file I/O. |

---

## AScorePro (read-only after init)

| Variable | Type | Notes |
|----------|------|-------|
| `g_AScoreOptions` | `AScoreProCpp::AScoreOptions` | Configuration for the AScorePro phosphosite localization algorithm. Set once at init. |
| `g_AScoreInterface` | `AScoreProCpp::AScoreDllInterface*` | Pointer to the AScorePro DLL interface. Each RTS thread uses its own copy of data through this interface; the pointer itself is read-only. |

---

## Combinatorics / mod permutation tables (read-only after init)

Used by the variable mod permutation engine (`CometModificationsPermuter`). As of Phase 0.5
(`docs/20260730_PI_reduction.md`), rebuilt once per search session by
`CometFragmentIndex::PermuteIndexPeptideMods(g_vRawPeptides)`, called from
`CometPeptideIndex::ReadPeptideIndex()`, from whichever variable mods are active in
`g_staticParams.variableModParameters` at that moment -- the tables themselves are
**not** persisted in the `.idx` file, still rebuilt fresh every session. (An earlier
design, Phase 0, persisted these tables directly and read them back as-is; Phase 0.5
reverted that in favor of always regenerating them.) What *does* now come from the `.idx`
file again is the mod settings feeding that regeneration: `docs/20260811_restore_idx_header_mods.md`
restored the `VariableMod:`/`ProteinModList:`/`RequireVariableMod:` header lines Phase 0.5
had removed, so `ParsePeptideIndexHeader()` overwrites `g_staticParams.variableModParameters`
from the file before this regeneration runs, the same way `StaticMod:` already did --
`comet.params`/RTS `SetParam()` values are no longer authoritative for an indexed search.

| Variable | Notes |
|----------|-------|
| `MOD_NUMBERS_POOL` | `vector<char>` -- flat pool holding every precomputed mod-combination entry's `modifications[]` array, concatenated (docs/20260827_PI_memory.md Phase 1; formerly `vector<ModificationNumber>` with one heap allocation per entry). Entries are addressed arithmetically via `GetModNumEntry()` (core/Types.h). |
| `MOD_SEQ_MOD_NUM_POOL_START` | `uint64_t*` -- per modifiable sequence, offset of its first entry in `MOD_NUMBERS_POOL`. |
| `MOD_SEQS_POOL` / `MOD_SEQS_OFFSET` | `vector<char>` / `vector<unsigned int>` -- unique modifiable sequences, flat-pooled (formerly `vector<string> MOD_SEQS`); sequence text is not NUL-terminated, accessed via `GetModSeq()`. |
| `MOD_SEQ_MOD_NUM_START` / `MOD_SEQ_MOD_NUM_CNT` | `int*` -- mod-combination entry index range per modifiable sequence. |
| `PEPTIDE_MOD_SEQ_IDXS` | `int*` -- maps peptides to their modifiable sequence index. |
| `MOD_NUM` | `int` -- total number of distinct modification combinations. |
| `g_vvvPepGenShort` / `g_vvvPepGenLong` | Per-thread peptide generation scratch buffers; populated during index build and reused across peptides to avoid repeated allocation. |

---

## Threading primitives

| Variable | Type | Notes |
|----------|------|-------|
| `g_pvDBIndexMutex` | `Mutex` | Protects database index reads where concurrent access is possible. |
| `g_preprocessMemoryPoolMutex` | `Mutex` | Protects the shared preprocessing memory pool. |
| `g_pvQueryMutex` | `Mutex` | Protects `g_vSpecLib` load/access (`CometSpecLib.cpp`, `CometPreprocess.cpp`). Name is a holdover from before the architecture migration, when it also guarded the now-removed `g_pvQuery` global; it was repurposed rather than renamed. Remains a global (not a `SearchSession` member) because it is also used by the RTS path -- see `search/SearchSession.h`'s header comment. |
| `g_ms1AlignerMutex` | `Mutex` | Protects `RetentionMatchHistory` updates in `DoMS1SearchMultiResults`. |

**Note:** `g_searchMemoryPoolMutex` and the paired `g_searchPoolCV` condition variable were removed during the architecture migration; the search memory pool's locking is now encapsulated inside the `SearchMemoryPool` class (see below) instead of living as bare globals.

**Dead declarations:** `core/Types.h:984-985` declares `extern Mutex g_dbIndexMutex;` and `extern Mutex g_vSpecLibMutex;`. Neither has a definition anywhere in the repo, and neither is ever locked. They are distinct from the real, in-use `g_pvDBIndexMutex` and `g_pvQueryMutex` above -- `g_vSpecLibMutex` in particular looks like an abandoned attempt to give `g_vSpecLib`'s protection a correctly-named mutex (the doc's own `g_pvQueryMutex` entry already notes that name is a holdover). Don't assume either does anything.

### SearchMemoryPool (`threading/SearchMemoryPool.h`)

Not a global variable, but the direct replacement for the old `_pbSearchMemoryPool` static array, `g_searchMemoryPoolMutex`, and `g_searchPoolCV` trio, so it is documented here for anyone updating this table. `CometSearch.cpp` holds a single file-static instance, `s_pool`, owning its own `std::mutex` and `std::condition_variable`. `CometSearch::AllocateMemory(N)` calls `s_pool.allocate(maxNumThreads, g_staticParams.iArraySizeGlobal, nBinnedIonMassesElems, nBinnedPrecursorNLElems)` (`CometSearch.cpp:63` -- the latter two size the PI_DB target/decoy scratch buffers, not just the two originally-documented arguments); `AcquirePoolSlot()` / `releaseSlot()` forward to `s_pool.acquireSlot()` / `s_pool.releaseSlot()`. Every acquire site wraps the returned slot in a `SearchMemoryPoolSlotGuard` (RAII; releases on scope exit, including exception unwind) so a throw out of a search body cannot leak a slot and stall the next `acquireSlot()` caller for up to 240 s.

---

## RTS initialization flags

**Not process-wide globals**, unlike everything else in this document: `singleSearchInitializationComplete`/`singleSearchMS1InitializationComplete` are private, non-static members of the `CometSearchManager` class (`CometSearchManager.h:115-116`) -- one pair per instance. They only *behave* like globals in practice because the C# side (`RealtimeSearch/SearchMS1MS2.cs`) keeps exactly one `globalSearchMgr` instance alive for the life of an RTS session; that's an external convention, not a language guarantee (see `docs/20260615_multiple_rts_instances.md` for what breaks if a process ever needs two concurrent `ICometSearchManager` instances).

| Variable | Type | Notes |
|----------|------|-------|
| `singleSearchInitializationComplete` | `std::atomic<bool>` | Set to `true` (with `release` ordering) after `InitializeSingleSpectrumSearch()` completes. Checked with `acquire` ordering at the top of `DoSingleSpectrumSearchMultiResults()`. Ensures all RTS threads see fully initialized globals. |
| `singleSearchMS1InitializationComplete` | `std::atomic<bool>` | Same pattern for `InitializeSingleSpectrumMS1Search()` / `DoMS1SearchMultiResults()`. |

The happens-before edge that makes the `acquire`/`release` ordering meaningful comes from a single file-static `std::mutex`, `g_initSingleSpectrumMutex` (`CometSearchManager.cpp:2309`), shared by both `InitializeSingleSpectrumSearch()` and `InitializeSingleSpectrumMS1Search()`. It used to be two separate function-local statics, one per function; they were unified because both functions call the same `InitializeStaticParams()` (guarded only by a plain, non-atomic bool) and both `fillPool()` the same `ThreadPool` (no internal lock of its own) -- concurrent first-time MS1+MS2 init from two C# Tasks (which the API permits) could otherwise run both functions' bodies at once and race on either. `g_initSingleSpectrumMutex` is file-static (no `extern`, inaccessible outside the translation unit), so it's not a full "global" in the sense of the rest of this table, but it's what actually serializes concurrent first-callers during double-checked locking.

---

## Memory allocation state flags

| Variable | Notes |
|----------|-------|
| `g_bCometPreprocessMemoryAllocated` | `true` when `CometPreprocess::AllocateMemory()` has been called. |
| `g_bCometSearchMemoryAllocated` | `true` when `CometSearch::AllocateMemory()` has been called. |

---

## Version and environment

| Variable | Type | Notes |
|----------|------|-------|
| `g_sCometVersion` | `string` | Version string, assembled independently at three sites: `main()` in `Comet.cpp:44-52` (batch via CLI) and `DoSearch()` in `CometSearchManager.cpp:1985-1993` (batch via DLL -- its comment notes this duplicates `main()`'s logic "as main() is skipped when search invoked via DLL") both append the `GITHUBSHA` macro (below) to `comet_version` when non-empty, producing e.g. `"2026.02 rev. 1 (a1b2c3d)"`. **`InitializeSingleSpectrumSearch()` (RTS, `CometSearchManager.cpp:2217`) does not** -- it sets `g_sCometVersion = comet_version;` with no SHA appended, so RTS builds report a version string with no git hash. This looks like a real gap in the RTS path, not just a doc issue -- worth fixing at the code level if a git hash in the reported RTS version is expected. |
| `GITHUBSHA` | `#define` macro (`CometSearch/githubsha.h`) | Not a variable -- a compile-time string literal. Defaults to `#define GITHUBSHA ""` in the tracked file; CI workflows overwrite `githubsha.h` with the actual commit SHA before building a release, so a locally-built binary normally has an empty `GITHUBSHA`. There is no `g_psGITHUB_SHA` global; earlier drafts of this doc described one, but the mechanism has always been this compile-time macro. |

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
| `dInverseBinWidth` / `dOneMinusBinOffset` | `double` | Used in the `BIN(mass)` macro on every fragment -- computed once at init. |

---

## Thread-safety summary

```
Safe to read from any concurrent RTS thread (after init):
  g_staticParams, g_iFragmentIndex, g_iFragmentIndexOffset,
  g_vFragmentPeptides, g_vRawPeptides, g_pvProteinNames, g_pvProteinsList,
  g_pvProteinNameCache, g_vSpecLib, g_vulSpecLibPrecursorIndex,
  g_AScoreOptions, g_AScoreInterface, MOD_NUMBERS_POOL, MOD_SEQS_POOL, MOD_SEQS_OFFSET,
  g_massRange.dMinMass / dMaxMass / bNarrowMassRange (written once at init on
  either path -- not batch-only, see "Core search state" above)

Written once per call, batch path only (RTS never calls PreprocessSpectrum):
  g_massRange.usiMaxFragmentCharge

Written per batch (batch path only -- not touched by RTS):
  SearchSession::queries, SearchSession::ms1Queries,
  g_staticParams.databaseInfo.uliTotAACount

Protected by mutex (safe to call from any thread):
  RetentionMatchHistory (g_ms1AlignerMutex, via pMS1Aligner.processRetentionMatch())

Always shared mutable -- use sparingly from hot paths:
  g_cometStatus (SetStatus()/IsCancel()/IsError() are mutex-protected
  internally; there is no SetError())

Atomic, checked with acquire/release ordering:
  singleSearchInitializationComplete, singleSearchMS1InitializationComplete
  (CometSearchManager instance members, not free globals -- see "RTS
  initialization flags" above), g_bPeptideIndexRead,
  g_bPeptideIndexFullyInitialized (the one EnsurePeptideIndexLoaded()'s
  fast-path actually gates on -- see "Fragment index" above)

Plain bool, NOT atomic, but written only inside the init paths (relies on the
above happens-before edge for safety -- see their entries above):
  g_bPlainPeptideIndexRead, g_bSpecLibRead
```
