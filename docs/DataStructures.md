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
| `bSparseFromPool` | `true` when the sparse child arrays belong to the RTS thread-local `RtsScratch` pool; the destructor must **not** `delete[]` them in this case. |
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

---

## StaticParams

The global parameter aggregate. Fully populated before any search thread starts; treated as read-only during search.

```cpp
struct StaticParams  // core/Params.h
extern StaticParams g_staticParams;
```

Contains nested sub-structs (all defined in `core/Params.h`):

| Sub-struct | Type | Key contents |
|------------|------|-------------|
| `options` | `Options` | ~40 integer/bool flags controlling output formats, decoy mode, charge limits, clipping, indexing, etc. |
| `tolerances` | `ToleranceParams` | Precursor tolerance + units, fragment bin size + offset, isotope error mode. |
| `massUtility` | `MassUtil` | `pdAAMassParent[128]` and `pdAAMassFragment[128]` look-up tables; mono vs. average flag. |
| `precalcMasses` | `PrecalcMasses` | Pre-computed `dNtermProton`, `dCtermOH2Proton`, `dOH2ProtonCtermNterm`, BIN'd H2O/NH3 values. |
| `staticModifications` | `StaticMod` | Per-AA static mod deltas (`pdStaticMods[128]`), peptide/protein terminal additions. |
| `variableModParameters` | `VarModParams` | `varModList[VMODS]` (9 slots), mod symbol codes, max-per-peptide limit, compound mod list. |
| `ionInformation` | `IonInfo` | Active ion series bitmask, water/ammonia loss flag, flanking peak mode. |
| `enzymeInformation` | `EnzymeInfo` | Search enzyme, sample enzyme, 2nd enzyme, allowed missed cleavages, offset directions. |
| `databaseInfo` | `DBInfo` | FASTA path; `iTotalNumProteins` and `uliTotAACount` updated during batch scan. |
| `dInverseBinWidth` / `dOneMinusBinOffset` | `double` | Used in the `BIN(mass)` macro on every fragment ion -- computed once to turn division into multiplication. |
| `tRealTimeStart` | `chrono::time_point` | Timestamp set at the start of each `DoSingleSpectrumSearch()` call; used to enforce `iMaxIndexRunTime`. |

---

## VarMods / VarModParams

```cpp
struct VarMods      // CometData.h   (one entry per mod slot)
struct VarModParams // core/Params.h (all mod config)
```

`VarModParams` contains:

| Field | Purpose |
|-------|---------|
| `varModList[VMODS]` | Array of 9 `VarMods` entries. `VMODS = 9`, indexed 0-8. |
| `cModCode[VMODS]` | Output symbol for each mod: `*`, `#`, `@`, `^`, `~`, `$`, `%`, `!`, `+`. |
| `bVarModSearch` | Set to `true` if any mod has a non-zero mass; gates the `WithVariableMods` code path. |
| `iMaxVarModPerPeptide` | Total modified residues allowed per peptide across all mods. |
| `iMaxPermutations` | Cap on permutation count in `WithVariableMods`. |
| `vdCompoundMasses` | `vector<double>` of masses from the compound mods file. |
| `iNumCompoundMasses` | `size_t` size of `vdCompoundMasses`. |

Each `VarMods` entry:

| Field | Purpose |
|-------|---------|
| `dVarModMass` | Mass delta (monoisotopic or average per `massUtility` setting). |
| `dNeutralLoss` | Fragment neutral loss mass for this mod. |
| `szVarModChar[MAX_VARMOD_AA]` | AAs this mod applies to (e.g. `"STY"`). |
| `iMaxNumVarModAAPerMod` / `iMinNumVarModAAPerMod` | Per-mod occurrence limits. |
| `iBinaryMod` | If 1, either all eligible residues in the peptide are modified or none. |
| `bRequireThisMod` | If true, only report peptides that carry this mod. |
| `iVarModTermDistance` / `iWhichTerm` | Terminal-distance constraint. |

---

## DBIndex

One entry in the peptide index (`g_pvDBIndex`), used during index generation and FASTA search. Sorted by peptide sequence and mass for deduplication.

```cpp
struct DBIndex  // core/Types.h
```

| Field | Type | Purpose |
|-------|------|---------|
| `sPeptide[MAX_PEPTIDE_LEN]` | `char[]` | Peptide amino acid sequence (null-terminated). |
| `cPrevAA` / `cNextAA` | `char` | Flanking residues (for enzyme termini check). |
| `pcVarModSites` | `vector<char>` | Variable mod encoding per position. Empty = unmodified; otherwise `[iLen+2]` chars using the same 0-9 scheme as `piVarModSites`. |
| `dPepMass` | `double` | MH+ mass; used as sort key within equal sequences. |
| `siVarModProteinFilter` | `unsigned short` | Bitwise filter derived from the protein filter file; `0` when not filtering. Initialized to `0`. |
| `lIndexProteinFilePosition` | `comet_fileoffset_t` | Index into `g_pvProteinsList` mapping to the list of protein file offsets. |

`DBIndex` provides `operator==` (sequence + mass + mod-sites) and `operator<` (sequence -> mass -> mod-sites -> protein position).

---

## PlainPeptideIndexStruct

Compact fixed-size tuple stored in the plain peptide index (`.idx` file) and loaded into `g_vRawPeptides` at runtime. Same core fields as `DBIndex` but without the `vector<char>` mod-site field (only unmodified peptides are stored here; modifications are layered on in `g_vFragmentPeptides`).

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

The external interface mirrors `vector<vector<comet_fileoffset_t>>`: `size()`, `empty()`, `clear()`, `reserve()`, `push_back(vector&&)`, `append_flat()`, `operator[](i)`, `at(i)`, range-for. `operator[](i)` returns a lightweight `Row` proxy (`ptr` + `n`) with `size()`, `operator[]`, `begin()`/`end()`. Only two internal heap allocations regardless of how many rows are stored (`m_flat`: all protein file offsets concatenated; `m_off`: `[N+1]` uint64 CSR offsets).

---

## SearchSession

Owns all mutable state for one batch search run. Created once at the top of `CometSearchManager::DoSearch()` and passed by reference through `Pipeline` to `ISearchStrategy` implementations.

```cpp
struct SearchSession  // search/SearchSession.h
```

| Field | Purpose |
|-------|---------|
| `params` | `const StaticParams&` -- read-only reference to `g_staticParams`. |
| `queries` | `vector<Query*>` -- per-batch MS2 query accumulator (replaces global `g_pvQuery` for the batch path). Protected by `queriesMutex`. |
| `ms1Queries` | `vector<QueryMS1*>` -- per-batch MS1 query accumulator (replaces global `g_pvQueryMS1`). |
| `queriesMutex` | `std::mutex` -- guards `queries` and `ms1Queries` during parallel spectrum loading. |
| `bPerformDatabaseSearch` | Replaces the former global `g_bPerformDatabaseSearch`. |
| `bPerformSpecLibSearch` | Replaces the former global `g_bPerformSpecLibSearch`. |
| `bIdxNoFasta` | Replaces the former global `g_bIdxNoFasta`. |
| `bPlainPeptideIndexRead` | Local copy of index-read state for this run. |
| `bSpecLibRead` | Local copy of speclib-read state for this run. |
| `status` | Per-run `CometStatus`; `g_cometStatus` remains as a global for the RTS path. |

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
| `isIndexBased()` | Any time | `true` for `FiStrategy`/`PiStrategy`; selects progress-message style in `Pipeline`. |

**Concrete strategies:**

| Class | File | DB type | Notes |
|-------|------|---------|-------|
| `FiStrategy` | `search/FiStrategy.cpp` | `FI_DB` | Fused load+search path when `bPerformDatabaseSearch && !bMango && !bPerformSpecLibSearch`; legacy three-sweep otherwise. |
| `FastaStrategy` | `search/FastaStrategy.cpp` | `FASTA_DB` | Classic three-sweep (load -> allocate -> RunSearch -> PostAnalysis). |
| `PiStrategy` | `search/PiStrategy.cpp` | `PI_DB` | Three-sweep like FASTA but against the plain peptide index; no Mango block. |

**IResultWriter** (`output/IResultWriter.h`) is the parallel output abstraction. Each format (`TxtWriter`, `PepXmlWriter`, `SqtWriter`, `PercolatorWriter`, `MzIdentMlWriter`) implements `open()`, `write()`, `close()`. `Pipeline` holds a `vector<unique_ptr<IResultWriter>>` and calls them around the batch loop.

---

## PepMassInfo / SpectrumInfoInternal

Small structs embedded in each `Query`.

```cpp
struct PepMassInfo          // core/Types.h
struct SpectrumInfoInternal // core/Types.h
```

`PepMassInfo` stores the experimental MH+ mass (`dExpPepMass`) and the +/- tolerance window (`dPeptideMassToleranceMinus` / `dPeptideMassTolerancePlus`) pre-computed for fast range checks.

`SpectrumInfoInternal` stores scan number, charge state, retention time, array size, Mango encoding, and the nativeID string from mzML files.

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

---

## MassRange

```cpp
struct MassRange  // CometDataInternal.h
extern MassRange g_massRange;
```

Computed once per spectrum batch from the lowest and highest precursor masses in `SearchSession.queries`. Search threads read `dMinMass` / `dMaxMass` for early-exit decisions in `SearchForPeptides`. `iMaxFragmentCharge` caps the fragment ion charge loop.

---

## Public API types (CometData.h)

These types cross the library boundary into `CometWrapper` and `RealtimeSearch`.

| Type | Purpose |
|------|---------|
| `Scores` | XCorr, dCn, E-value, mass, matched/total ions -- returned per search hit. |
| `Fragment` | Single fragment ion: mass, intensity, type, number, charge, neutral loss. |
| `VarMods` | User-facing mod definition (same as internal `VarMods`; shared header). |
| `EnzymeInfo` | Enzyme parameters surfaced to the wrapper layer. |
| `InputFileInfo` | Per-file input descriptor (type, scan range, file path). |
| `CometParam` / `TypedCometParam<T>` | Type-erased parameter container used to pass params through `ICometSearchManager::SetParam()`. |
