# Core Data Structures

Key types used throughout `CometSearch/`. Struct definitions were reorganized in Phase 3-4 of the architecture migration:
- `core/Types.h` -- per-spectrum, index, and runtime structs (`Results`, `Query`, `QueryMS1`, `DBIndex`, `PlainPeptideIndexStruct`, `FragmentPeptidesStruct`, `ProteinsListCSR`, etc.)
- `core/Params.h` -- `StaticParams` and all its nested sub-structs
- `core/Constants.h` -- compile-time constants (`MAX_PEPTIDE_LEN`, `VMODS`, `HISTO_SIZE`, etc.)
- `CometData.h` -- public API types that cross the library boundary into `CometWrapper` and `RealtimeSearch`

`CometDataInternal.h` `#include`s all three `core/` headers; existing code that includes `CometDataInternal.h` continues to see everything.

---

## Query

The central per-spectrum data object. One `Query` is allocated for each spectrum/charge combination in a batch.

```cpp
struct Query  // core/Types.h
```

**Scoring state:**

| Field | Purpose |
|-------|---------|
| `iXcorrHistogram[HISTO_SIZE]` | Histogram of XCorr scores for E-value estimation (152 bins). |
| `uiHistogramCount` | Number of entries in the histogram. |
| `fPar[4]` | Fitted LMA regression parameters from `LinearRegression()`. |
| `siMaxXcorr` | Bin index of the histogram maximum. |
| `iMinXcorrHisto` | Minimum xcorr bin used in histogram; adjusts E-value floor for sparse spectra. |
| `dLowestXcorrScore` / `dLowestDecoyXcorrScore` | Current minimum stored XCorr; gates whether a new hit is kept. |
| `siLowestXcorrScoreIndex` / `siLowestDecoyXcorrScoreIndex` | Index of the current lowest-scoring result slot. |
| `fLowestSpecLibScore` | Current minimum stored speclib score for the MS2 speclib path. |
| `iMatchPeptideCount` / `iDecoyMatchPeptideCount` | Number of results actually stored. |
| `_uliNumMatchedPeptides` / `_uliNumMatchedDecoyPeptides` | Total peptides scored (including those below cutoff). |
| `dMangoIndex` | Decimal scan-number encoding for Mango TMT-precursor searches. |

**Spectrum data (set by CometPreprocess):**

| Field | Purpose |
|-------|---------|
| `ppfSparseSpScoreData[][]` | Sparse 2D binned intensity array for SP scoring. |
| `ppfSparseFastXcorrData[][]` | Sparse 2D preprocessed intensity array for XCorr calculation. |
| `ppfSparseFastXcorrDataNL[][]` | Same with NH3/H2O neutral loss contributions. |
| `iSpScoreData` / `iFastXcorrDataSize` | Outer dimension of the respective sparse arrays. |
| `bSparseFromPool` | `true` when the sparse child arrays belong to a pool -- the RTS thread-local `RtsScratch` pool, or the fused batch path's `FusedSparseArena` -- rather than a per-`Query` `new[]`; the destructor must **not** `delete[]` them in this case. |
| `bSparsePointerArraysFromPool` | Same pool-ownership flag, for the outer pointer arrays as distinct from the sparse child arrays above. |
| `bResultsFromPool` | `true` when `_pResults`/`_pDecoys` come from a pool (RTS `RtsScratch` or the fused batch path's `FusedResultsArena`) rather than a per-`Query` `new[]`; gates the destructor the same way. |
| `vfRawFragmentPeakMass` | Raw fragment peak masses for fragment index search (intensity not needed at scoring stage). |
| `vRawFragmentPeakMassIntensity` | Raw peaks as `AScoreProCpp::Centroid` pairs; populated when AScorePro is enabled. |
| `_pepMassInfo` | Experimental mass and tolerance window (see `PepMassInfo`). |
| `_spectrumInfoInternal` | Scan number, charge state, RT, array size, nativeID (see `SpectrumInfoInternal`). |
| `tSearchStart` | Per-query search start time; used to enforce `iMaxIndexRunTime` timeout. |

**Results:**

| Field | Purpose |
|-------|---------|
| `_pResults` | Heap-allocated `Results[iNumStored]` array for target hits. |
| `_pDecoys` | Same for decoy hits (separate decoy mode only; `iDecoySearch == 2`). |
| `_pSpecLibResults` | MS2 spectral library results (`SpecLibResults[iNumStored]`). |
| `accessMutex` | Per-query mutex; guards `_pResults` updates in concurrent search threads. |

**Lifecycle:** Allocated in `CometPreprocess`, freed in `Query::~Query()`. In batch mode, all `Query*` objects live in `SearchSession.queries`. In the RTS thread-local path, each call owns its own heap `Query*` and frees it at the end of the call.

---

## Results

Holds one peptide hit. Each `Query` owns an array of `Results[iNumStored]`.

```cpp
struct Results  // core/Types.h
```

| Field | Type | Purpose |
|-------|------|---------|
| `fXcorr` | `float` | Cross-correlation score. |
| `fScoreSp` | `float` | Preliminary SP score. |
| `fDeltaCn` | `float` | Delta-Cn (score difference to next-best hit). |
| `fLastDeltaCn` | `float` | Delta-Cn to the last stored hit. |
| `fAScorePro` | `float` | AScorePro phosphosite localization score. |
| `dExpect` | `double` | E-value from LMA-fitted histogram. |
| `dPepMass` | `double` | Calculated peptide MH+ mass. |
| `usiRankXcorr` | `unsigned short` | Xcorr rank. |
| `usiRankSp` | `unsigned short` | SP rank. |
| `usiMatchedIons` | `unsigned short` | Number of matched fragment ions. |
| `usiTotalIons` | `unsigned short` | Total theoretical fragment ions. |
| `usiLenPeptide` | `unsigned short` | Peptide length. |
| `lProteinFilePosition` | `comet_fileoffset_t` | File offset into the FASTA for the matched protein; for index searches, an entry index into `g_pvProteinsList`. |
| `lWhichProtein` | `long` | Which entry in `g_pvProteinsList[]` contains the matched proteins. |
| `szPeptide[MAX_PEPTIDE_LEN]` | `char[]` | Peptide sequence (no flanking AAs). |
| `cPrevAA` / `cNextAA` | `char` | Preceding and following amino acid. |
| `bClippedM` | `bool` | `true` if this is a new N-terminal peptide due to a clipped methionine. |
| `cHasVariableMod` | `char` | `HasVariableModType` enum: 0 = none, 1 = variable mod, 2 = AScorePro mod. |
| `piVarModSites[MAX_PEPTIDE_LEN_P2]` | `int[]` | Per-position variable mod encoding. Values 1-9 map to `varModList[0-8]`. Values >= `COMPOUNDMODS_OFFSET` (100) encode compound mods. Indices `iLenPeptide` and `iLenPeptide+1` hold N/C-terminal mod codes. |
| `pdVarModSites[MAX_PEPTIDE_LEN_P2]` | `double[]` | Mass delta at each modified position. |
| `pszMod[MAX_PEPTIDE_LEN][MAX_PEFFMOD_LEN]` | `char[][]` | PEFF modification strings, one per position. |
| `sPeffOrigResidues` | `string` | Original residues for PEFF variants. |
| `sAScoreProSiteScores` | `string` | Comma-separated per-site AScorePro scores. |
| `pWhichProtein` | `vector<ProteinEntryStruct>` | All proteins sharing this peptide (sorted by file offset). |
| `pWhichDecoyProtein` | `vector<ProteinEntryStruct>` | Decoy proteins (concatenated search mode). |
| `iPeffOrigResiduePosition` | `int` | Position of a PEFF variant substitution; `-1` = N-term, `iLenPeptide` = C-term, `-9` = unused. |
| `iPeffNewResidueCount` | `int` | Count of new residues from a PEFF variant: >0 means substitution (if orig count is 1) or insertion (if orig count >1). |

---

## StaticParams

The global parameter aggregate. Fully populated before any search thread starts; treated as read-only during search.

```cpp
struct StaticParams  // core/Params.h
extern StaticParams g_staticParams;
```

Contains nested sub-structs (defined in `core/Params.h`, except `enzymeInformation` -- see below):

| Sub-struct | Type | Key contents |
|------------|------|-------------|
| `options` | `Options` | ~40 integer/bool flags controlling output formats, decoy mode, charge limits, clipping, indexing, etc. |
| `tolerances` | `ToleranceParams` | Precursor tolerance + units, fragment bin size + offset, isotope error mode. |
| `massUtility` | `MassUtil` | `pdAAMassParent[128]` and `pdAAMassFragment[128]` look-up tables; mono vs. average flag. |
| `precalcMasses` | `PrecalcMasses` | Pre-computed `dNtermProton`, `dCtermOH2Proton`, `dOH2ProtonCtermNterm`, BIN'd H2O/NH3 values. |
| `staticModifications` | `StaticMod` | Per-AA static mod deltas (`pdStaticMods[128]`), peptide/protein terminal additions. |
| `variableModParameters` | `VarModParams` | `varModList[VMODS]` (`VMODS = 15` slots), mod symbol codes, max-per-peptide limit, compound mod list. |
| `ionInformation` | `IonInfo` | Active ion series bitmask, water/ammonia loss flag, flanking peak mode. |
| `enzymeInformation` | `EnzymeInfo` | Search enzyme, sample enzyme, 2nd enzyme, allowed missed cleavages, offset directions. **Defined in `CometData.h`, not `core/Params.h`** (it's a public API type shared with `CometWrapper`; see "Public API types" below). |
| `databaseInfo` | `DBInfo` | FASTA path; `iTotalNumProteins` and `uliTotAACount` updated during batch scan. |
| `dInverseBinWidth` / `dOneMinusBinOffset` | `double` | Used in the `BIN(mass)` macro on every fragment ion -- computed once to turn division into multiplication. |
| `tRealTimeStart` | `chrono::time_point` | Declared but unused -- grep confirms no code anywhere reads or assigns it besides its own declaration. Both the FI_DB and PI_DB RTS paths instead use a genuinely per-call clock, `pQuery->tSearchStart` on the heap-allocated `Query`, set fresh by `DoSingleSpectrumSearchMultiResults()` for every call and checked by both `SearchFragmentIndex()` and the PI_DB thread-local search (see `docs/RealTimeSearch.md`). |

---

## VarMods / VarModParams

```cpp
struct VarMods      // CometData.h   (one entry per mod slot)
struct VarModParams // core/Params.h (all mod config)
```

`VarModParams` contains:

| Field | Purpose |
|-------|---------|
| `varModList[VMODS]` | Array of `VarMods` entries. `VMODS = 15` (`core/Constants.h`), indexed 0-14. |
| `cModCode[VMODS]` | Output symbol for each mod slot. Slots 0-8 use fixed symbols `*`, `#`, `@`, `^`, `~`, `$`, `%`, `!`, `+`; slots 9-14 use an ASCII-derived fallback code (`core/Params.h`, `int iAscii = 88 + i`) so up to 15 simultaneous variable mods can each get a distinct output character. |
| `bVarModSearch` | Set to `true` if any mod has a non-zero mass; gates the `WithVariableMods` code path. |
| `iMaxVarModPerPeptide` | Total modified residues allowed per peptide across all mods. |
| `iMaxPermutations` | Cap on permutation count in `WithVariableMods`. |
| `vdCompoundMasses` | `vector<double>` of masses from the compound mods file. |
| `uiNumCompoundMasses` | `unsigned int` size of `vdCompoundMasses`; `0` when the feature is disabled. |
| `bVarTermModSearch` | Set if any mod is N-term/C-term constrained. |
| `bVarProteinNTermMod` / `bVarProteinCTermMod` | Set if a protein-terminus (not peptide-terminus) variable mod is specified. |
| `bBinaryModSearch` | Set if any mod has `iBinaryMod` set. |
| `bUseFragmentNeutralLoss` | Set if any custom neutral loss is set; applied only to 1+/2+ fragments. |
| `bRareVarModPresent` | Set if any mod has `iRequireThisMod == -1`. |
| `bVarModProteinFilter` | Set if a protein mod filter list is applied. |
| `iRequireVarMod` | `0` = no requirement; otherwise a bitmask of which var mods are required. |
| `sProteinLModsListFile` / `mmapProteinModsList` | Path to, and parsed `<varmod#, protein name>` contents of, the protein mod filter file. |
| `sCompoundModsFile` | Path to the compound mods mass file backing `vdCompoundMasses`; empty = disabled. |

Each `VarMods` entry:

| Field | Purpose |
|-------|---------|
| `dVarModMass` | Mass delta (monoisotopic or average per `massUtility` setting). |
| `dNeutralLoss` | Fragment neutral loss mass for this mod. |
| `szVarModChar[MAX_VARMOD_AA]` | AAs this mod applies to (e.g. `"STY"`). |
| `iMaxNumVarModAAPerMod` / `iMinNumVarModAAPerMod` | Per-mod occurrence limits. |
| `iBinaryMod` | If 1, either all eligible residues in the peptide are modified or none. |
| `iRequireThisMod` | Tri-state, not a bool: `0` = not required; `1` = required (only report peptides carrying this mod); negative = "exactly one from this set of mods" grouping. |
| `iVarModTermDistance` / `iWhichTerm` | Terminal-distance constraint. |
| `dNeutralLoss2` | Second fragment neutral-loss mass for this mod. |
| `bNtermMod` / `bCtermMod` | Set if this mod is constrained to the peptide N-/C-terminus. |
| `bUseMod` | Set if this mod slot has a non-zero mass (i.e. is active). |

---

## DBIndex

**Build-time and per-candidate transient use only -- not the search-time index.**
Historically one entry in `g_pvDBIndex`, used both during index generation and as PI_DB's
resident search-time array; as of `docs/20260730_PI_reduction.md`, `g_pvDBIndex` is
build-time-only (Phase A digestion output inside `CometPeptideIndex::WritePeptideIndex()`,
copied into `g_vRawPeptides` and cleared before the function returns -- see
`PlainPeptideIndexStruct`/`FragmentPeptidesStruct` below for what replaced it at search
time). `DBIndex` the *type* is still used, but only as a stack-local, per-candidate
reconstruction target: `CometPeptideIndex::MaterializeOneEntry()` builds one on demand from
a `g_vDBIndexVariants` entry for each mass-window candidate PI_DB search scores, then
discards it. Since Phase 0.5, `g_vDBIndexVariants` itself is also transient in a different
sense: it's rebuilt once per search session (`CometPeptideIndex::GenerateVariantArray()`,
called from `ReadPeptideIndex()`) from `g_vRawPeptides` + whatever variable mods
`comet.params` has active, rather than read from disk.

```cpp
struct DBIndex  // core/Types.h
```

| Field | Type | Purpose |
|-------|------|---------|
| `sPeptide[MAX_PEPTIDE_LEN]` | `char[]` | Peptide amino acid sequence (null-terminated). |
| `cPrevAA` / `cNextAA` | `char` | Flanking residues (for enzyme termini check). |
| `pcVarModSites` | `VarModSites` | Compact fixed-size `(position, residue)` pair list (`MAX_SITES = 8`), not a `vector<char>` -- avoids a per-entry heap allocation. Empty (`cNumSites == 0`) = unmodified; `operator[](pos)` mirrors the old dense-array lookup used by `piVarModSites`. See the type's doc comment in `core/Types.h` (and `docs/20260716_pidbmemory.md`) for the sizing rationale. |
| `dPepMass` | `double` | MH+ mass; used as sort key within equal sequences. |
| `siVarModProteinFilter` | `unsigned short` | Bitwise filter derived from the protein filter file; `0` when not filtering. Initialized to `0`. |
| `lIndexProteinFilePosition` | `comet_fileoffset_t` | Index into `g_pvProteinsList` mapping to the list of protein file offsets. |

`DBIndex` provides `operator==` (sequence + mass + mod-sites) and `operator<` (sequence -> mass -> mod-sites -> protein position).

---

## PlainPeptideIndexStruct

Compact fixed-size tuple stored in the unified `.idx` file (shared by PI_DB and FI_DB,
`docs/20260730_PI_reduction.md` Phase 0) and loaded into `g_vRawPeptides` at runtime by
both search modes. Same core fields as `DBIndex` but without the `VarModSites` mod-site
field (only unmodified peptides are stored here; modifications are layered on in
`g_vFragmentPeptides` for FI_DB, or the structurally-identical `g_vDBIndexVariants` for
PI_DB). As of Phase 0.5, `g_vRawPeptides` is the *only* peptide-level data persisted in the
`.idx` file at all -- `g_vFragmentPeptides`/`g_vDBIndexVariants` and the mod-permutation
tables (`MOD_NUMBERS`/`MOD_SEQS`/etc.) are generated fresh from it, once per search
session, from live `comet.params` rather than read back from disk.

```cpp
struct PlainPeptideIndexStruct  // core/Types.h
```

| Field | Purpose |
|-------|---------|
| `szPeptide[MAX_PEPTIDE_LEN]` | Peptide sequence (null-terminated). |
| `cPrevAA` / `cNextAA` | Flanking residues. |
| `dPepMass` | Unmodified MH+ mass. |
| `siVarModProteinFilter` | Protein filter bitfield. |
| `lIndexProteinFilePosition` | Row index into `g_pvProteinsList`. |

---

## FragmentPeptidesStruct

One entry in the fragment index peptide list (`g_vFragmentPeptides`). Represents one (peptide, mod-state) combination. Sorted by mass so that RunSearch can binary-search for mass-matching candidates.

**Also used, as the same type, for PI_DB's compact per-variant array** (`g_vDBIndexVariants`,
`docs/20260730_PI_reduction.md`) -- a separate global rather than literally sharing
`g_vFragmentPeptides` with FI_DB (the two backends don't yet share a build/dispatch path
for this, tracked as a follow-up), but identical in layout and semantics. PI_DB's
`CometSearch::SearchPeptideIndex()` binary-searches `g_vDBIndexVariants` by `dPepMass`
exactly as FI_DB does with `g_vFragmentPeptides`, then reconstructs a full `DBIndex` per
surviving candidate via `CometPeptideIndex::MaterializeOneEntry()` instead of resolving a
fragment-ion posting list.

```cpp
struct FragmentPeptidesStruct  // core/Types.h
```

| Field | Purpose |
|-------|---------|
| `iWhichPeptide` | Index into `g_vRawPeptides`; provides sequence and protein info. |
| `modNumIdx` | Index into `MOD_NUMBERS`; 0 = unmodified. |
| `dPepMass` | Modified MH+ mass (= unmodified mass + sum of applied mod masses). |
| `cNtermMod` / `cCtermMod` | N/C-terminal variable mod codes (index into `varModList`). |

---

## ProteinsListCSR

CSR (Compressed Sparse Row)-style storage for the per-peptide protein list. Replaces `vector<vector<comet_fileoffset_t>>` to eliminate the ~190 M individual heap allocations (one per inner vector) that caused a multi-minute free-time tail when building large MHC `.idx` files.

```cpp
class ProteinsListCSR  // core/Types.h
extern ProteinsListCSR g_pvProteinsList;
```

The external interface mirrors `vector<vector<comet_fileoffset_t>>`: `size()`, `empty()`, `clear()`, `reserve()`, `push_back(vector&&)` / `push_back(const vector&)`, `append_flat()`, `operator[](i)`, `at(i)`, range-for. `operator[](i)` returns a lightweight `Row` proxy (`ptr` + `n`) with `size()`, `operator[]`, `begin()`/`end()`. Only two internal heap allocations regardless of how many rows are stored (`m_flat`: all protein file offsets concatenated; `m_off`: `[N+1]` uint64 CSR offsets).

---

## SearchSession

Owns all mutable state for one batch search run. Created once at the top of `CometSearchManager::DoSearch()` and passed by reference through `Pipeline` to `ISearchStrategy` implementations.

```cpp
struct SearchSession  // search/SearchSession.h
```

| Field | Purpose |
|-------|---------|
| `queries` | `vector<Query*>` -- per-batch MS2 query accumulator (replaces global `g_pvQuery` for the batch path). Protected by `queriesMutex`. |
| `ms1Queries` | `vector<QueryMS1*>` -- per-batch MS1 query accumulator (replaces global `g_pvQueryMS1`). |
| `queriesMutex` | `std::mutex` -- guards `queries` and `ms1Queries` during parallel spectrum loading. |
| `bPerformDatabaseSearch` | Replaces the former global `g_bPerformDatabaseSearch`. |
| `bPerformSpecLibSearch` | Replaces the former global `g_bPerformSpecLibSearch`. |
| `bIdxNoFasta` | Replaces the former global `g_bIdxNoFasta`. |
| `sparseArenas` / `pointerArenas` / `resultsArenas` | `vector<FusedSparseArena>` / `vector<FusedPointerArena>` / `vector<FusedResultsArena>` -- per-thread scratch-memory arenas for the fused FI_DB/PI_DB batch path (`FusedLoadAndSearchSpectra`); freed in `Pipeline`'s `cleanupBatch()`. |
| `statusRef` | `CometStatus&` -- a **reference** to the process-wide singleton `g_cometStatus`, not a per-run copy. Pipeline and strategy code use `session.statusRef` so they are not coupled to the global name, but both spellings touch the same object. |

`SearchSession` has no `params` member -- code reads `g_staticParams` directly throughout; an earlier draft carried a `const StaticParams& params` field but it was unused and removed. There is also no `bPlainPeptideIndexRead` / `bSpecLibRead` member: `g_bPlainPeptideIndexRead`, `g_bSpecLibRead`, and `g_pvQueryMutex` remain plain globals rather than `SearchSession` fields, specifically because the RTS path (which never constructs a `SearchSession`) also reads/writes them -- see the header comment in `search/SearchSession.h` and the `g_pvQueryMutex` entry in `docs/GlobalVariables.md`.

`SearchSession` is non-copyable. The RTS paths (`DoSingleSpectrumSearchMultiResults`, `DoMS1SearchMultiResults`) do **not** use `SearchSession`; they use per-call `Query*`/`QueryMS1*` objects directly.

---

## Pipeline and ISearchStrategy

Added in Phase 5. `DoSearch()` instantiates a `Pipeline` + one concrete `ISearchStrategy` and calls `pipeline.run()`.

```cpp
class ISearchStrategy  // search/ISearchStrategy.h
class Pipeline         // search/Pipeline.h
```

**ISearchStrategy** interface methods:

| Method | Called | Purpose |
|--------|--------|---------|
| `initialize(session, tp)` | Once before file loop | Allocate pools, load/build index, pre-read precursors (FI_DB), read var-mod filter file (FASTA). |
| `openFiles(szDB, fpfasta, fpidx, fpdb, session)` | Once per file | Open DB file handles; set `session.bIdxNoFasta`. |
| `executeBatch(mstReader, firstScan, lastScan, analysisType, iPercentStart, iPercentEnd, tp, session)` | Once per batch | Preprocess + search + post-analysis for one spectrum batch; fills `session.queries`. |
| `closeFiles(fpfasta, fpidx)` | Once per file | Close file handles. |
| `finalize()` | Once after all files | Free memory pools and index arrays. |
| `isIndexBased()` | Any time | `true` for `FiStrategy`/`PiStrategy`. `Pipeline::run()` is the only consumer, and uses it solely to choose between the compact index-style progress line and the verbose FASTA-style per-file banners -- it carries no other semantics and must not be used to gate search behavior. |

**Concrete strategies:**

| Class | File | DB type | Notes |
|-------|------|---------|-------|
| `FiStrategy` | `search/FiStrategy.cpp` | `FI_DB` | Fused load+search path when `bPerformDatabaseSearch && !bMango && !bPerformSpecLibSearch`; legacy three-sweep otherwise. |
| `FastaStrategy` | `search/FastaStrategy.cpp` | `FASTA_DB` | Classic three-sweep (load -> allocate -> RunSearch -> PostAnalysis). |
| `PiStrategy` | `search/PiStrategy.cpp` | `PI_DB` | Same fused-vs-legacy split as `FiStrategy`, gated by the identical condition (`bPerformDatabaseSearch && !bMango && !bPerformSpecLibSearch`); legacy three-sweep against the plain peptide index otherwise. |

**AScore lifecycle:** `Pipeline::run()` -- not `DoSearch()` -- owns `SetAScoreOptions()` / `CreateAScoreDllInterface()` / `DeleteAScoreDllInterface()` for the batch path, called immediately after `_strategy->initialize()` succeeds and immediately after `_strategy->finalize()` runs. Historically this ordering was required because, for `FI_DB`, `FiStrategy::initialize()`'s call to `CometPeptideIndex::ReadPeptideIndex()` overwrote `g_staticParams.variableModParameters.varModList[]` from the `.idx` file's `VariableMod:` header, so `SetAScoreOptions()` had to run after that overwrite or it would configure AScore from stale/default mod values. As of Phase 0.5 (`docs/20260730_PI_reduction.md`) the `.idx` header no longer carries variable-mod settings at all -- `varModList[]` comes entirely from `comet.params`, stable well before this point -- so the ordering is no longer strictly required, but is left as-is since it's already correct. (The RTS path's `InitializeSingleSpectrumSearch()` has its own, separate AScore setup and was never affected by this.)

**`_pQueries` discipline (FASTA only):** `CometSearch::BinarySearchMass()` reads the query list through the `CometSearch` member `_pQueries` rather than a parameter; `CometSearch::DoSearch()` (the FASTA path) sets `_pQueries = &queries` at entry before any call into it. This is FASTA-specific -- the PI_DB path was refactored away from `_pQueries`: `CometSearch::SearchPeptideIndex(Query*, bool*, int)` and its `AnalyzePeptideIndex(Query*, const DBIndex&, bool*, sDBEntry*, int)` overload both take the `Query*` directly as a parameter and never touch `_pQueries` or `BinarySearchMass()`. Any new code path that calls into `BinarySearchMass()` still needs `_pQueries` assigned first; PI_DB code does not.

**IResultWriter** (`output/IResultWriter.h`) is the parallel output abstraction. Each format (`TxtWriter`, `PepXmlWriter`, `SqtWriter`, `PercolatorWriter`, `MzIdentMlWriter`) implements `open()`, `write()`, `close()`. `Pipeline` holds a `vector<unique_ptr<IResultWriter>>` and calls them around the batch loop. `close()` must be safe to call even if `open()` was never invoked or returned false: when one writer's `open()` fails, `Pipeline::run()` calls `close(false, false)` on every writer in the vector, including ones after the failed one.

---

## PepMassInfo / SpectrumInfoInternal

Small structs embedded in each `Query`.

```cpp
struct PepMassInfo          // core/Types.h
struct SpectrumInfoInternal // core/Types.h
```

`PepMassInfo` stores the calculated peptide mass (`dCalcPepMass`), the experimental MH+ mass (`dExpPepMass`), and the tolerance window in two forms: `dPeptideMassToleranceLow`/`dPeptideMassToleranceHigh` (absolute mass bounds) and `dPeptideMassToleranceMinus`/`dPeptideMassTolerancePlus` (bounds including isotope offsets, used for range checks), pre-computed for fast lookups.

`SpectrumInfoInternal` stores array size, `iHighestIon` (index of the highest-intensity bin, sizes the XCorr window in `CometPreprocess`'s `MakeCorrData` -- see commit `0c064a2e`), scan number, charge state (`usiChargeState`), `usiMaxFragCharge`, `dTotalIntensity`, retention time, Mango encoding, and the nativeID string from mzML files.

---

## sDBEntry

Passed through the FASTA search loop; holds data for a single protein from the database.

```cpp
typedef struct sDBEntry  // core/Types.h
```

| Field | Purpose |
|-------|---------|
| `strName` | Protein accession/description. |
| `strSeq` | Full protein amino acid sequence. |
| `lProteinFilePosition` | Byte offset into FASTA (used as the canonical protein identifier). |
| `vectorPeffMod` | PEFF modifications from the protein header. |
| `vectorPeffVariantSimple/Complex` | PEFF sequence variants. |
| `vectorPeffProcessed` | Processed/applied PEFF annotations (`vector<PeffProcessedStruct>`). |

---

## MassRange

```cpp
struct MassRange  // core/Params.h
extern MassRange g_massRange;
```

`dMinMass`/`dMaxMass` are set once at init (either the batch path's
`DoSearch()` or the RTS path's `InitializeSingleSpectrumSearch()`) as the
outer bounds from the configured peptide mass range. On the legacy
three-sweep batch paths (FASTA, and the PI_DB/FI_DB legacy fallback), the
shared `RunSearchAndPostAnalysis()` helper (`search/SearchUtils.cpp`) then
**re-narrows** them every batch from that batch's actual `SearchSession.queries`
(sorted by peptide mass: `queries.front()`'s tolerance-minus and `queries.back()`'s
tolerance-plus), so they track the current batch rather than the whole run's
range. The fused batch path (`FusedLoadAndSearchSpectra`) and the RTS path do
not do this per-batch/per-call re-narrowing.

**This asymmetry is provably benign, not a gap** (traced end-to-end for
`docs/20260728_CodeUpdates.md` item 6): `dMinMass`/`dMaxMass` are read for
candidate filtering *exclusively* by `SearchForPeptides()` and its
digestion-loop siblings (`WithinMassTolerance`, `MergeVarMods`,
`CompoundModSearch`), all reachable only through
`CometSearch::DoSearch(sDBEntry&, ...)` -- the FASTA-digestion search engine,
used for real `FASTA_DB` search and for one-time index building, never for
FI_DB/PI_DB *searching*. FI_DB's `SearchFragmentIndex()` and PI_DB's
`SearchPeptideIndex()` (used identically by the fused batch path, the legacy
FI_DB/PI_DB fallback, and RTS) never read `dMinMass`/`dMaxMass` at all -- PI_DB
does its own per-query binary search directly against
`pQuery->_pepMassInfo`, and FI_DB only reads the unrelated
`g_massRange.uiMaxFragmentArrayIndex`. So whether or not a code path
re-narrows these two fields has zero effect on FI_DB/PI_DB search results;
only `FastaStrategy` actually depends on the re-narrowed value, and it always
re-narrows (no fused/legacy split exists for FASTA_DB).
`usiMaxFragmentCharge` (not `iMaxFragmentCharge`) caps the fragment ion charge
loop and is the one field in this struct that genuinely is batch-only,
updated per-spectrum under `_maxChargeMutex` inside `CometPreprocess::PreprocessSpectrum`.
See `docs/GlobalVariables.md` for the full per-field thread-safety breakdown.

---

## Public API types (CometData.h)

These types cross the library boundary into `CometWrapper` and `RealtimeSearch`.

| Type | Purpose |
|------|---------|
| `CometScores` | XCorr, dSp, dCn, E-value, mass, matched/total ions, AScorePro -- returned per MS2 search hit. (MS1 spectral-library hits use the separate `CometScoresMS1` struct: dot product, RT, scan number.) |
| `Fragment` | Single fragment ion: mass, intensity, type, number, charge, neutral loss. |
| `VarMods` | User-facing mod definition (same as internal `VarMods`; shared header). |
| `EnzymeInfo` | Enzyme parameters surfaced to the wrapper layer. |
| `InputFileInfo` | Per-file input descriptor (type, scan range, file path). |
| `CometParam` / `TypedCometParam<T>` | Type-erased parameter container used to pass params through `ICometSearchManager::SetParam()`. |
