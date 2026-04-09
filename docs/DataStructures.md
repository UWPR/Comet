# Core Data Structures

Key types used throughout `CometSearch/`. All are defined in `CometDataInternal.h` unless noted. Types from `CometData.h` (the public API header) are marked accordingly.

---

## Query

The central per-spectrum data object. One `Query` is allocated for each spectrum/charge combination in a batch.

```cpp
struct Query  // CometDataInternal.h:861
```

**Scoring state:**

| Field | Purpose |
|-------|---------|
| `iXcorrHistogram[HISTO_SIZE]` | Histogram of XCorr scores for E-value estimation (152 bins). |
| `iHistogramCount` | Number of entries in the histogram. |
| `fPar[4]` | Fitted LMA regression parameters from `LinearRegression()`. |
| `siMaxXcorr` | Bin index of the histogram maximum. |
| `dLowestXcorrScore` / `dLowestDecoyXcorrScore` | Current minimum stored XCorr; gates whether a new hit is kept. |
| `iMatchPeptideCount` / `iDecoyMatchPeptideCount` | Number of results actually stored. |
| `_uliNumMatchedPeptides` | Total peptides scored (including those below cutoff). |

**Spectrum data (set by CometPreprocess):**

| Field | Purpose |
|-------|---------|
| `pfFastXcorrData[]` | Preprocessed intensity array for XCorr calculation. |
| `pfFastXcorrDataNL[]` | Same with NH₃/H₂O neutral loss contributions. |
| `pfSpScoreData[]` / `ppfSparseSpScoreData[][]` | Binned intensity for SP scoring. Sparse representation saves memory for large bin arrays. |
| `iFastXcorrDataSize` / `iSpScoreData` | Array sizes for the above. |
| `_pepMassInfo` | Experimental mass and tolerance window (see `PepMassInfo`). |
| `_spectrumInfoInternal` | Scan number, charge state, RT, array size (see `SpectrumInfoInternal`). |

**Results:**

| Field | Purpose |
|-------|---------|
| `_pResults` | Heap-allocated `Results[iNumStored]` array for target hits. |
| `_pDecoys` | Same for decoy hits (separate decoy mode only; `iDecoySearch == 2`). |
| `accessMutex` | Per-query mutex; guards `_pResults` updates in concurrent search threads. |

**Lifecycle:** allocated in `CometPreprocess`, freed in `Query::~Query()`. In batch mode, all `Query*` objects live in `g_pvQuery`. In the RTS thread-local path, each call owns its own heap `Query*` and frees it at the end of the call.

---

## Results

Holds one peptide hit. Each `Query` owns an array of `Results[iNumStored]`.

```cpp
struct Results  // CometDataInternal.h:194
```

| Field | Type | Purpose |
|-------|------|---------|
| `fXcorr` | `float` | Cross-correlation score. |
| `fScoreSp` | `float` | Preliminary SP score. |
| `dExpect` | `double` | E-value from LMA-fitted histogram. |
| `dPepMass` | `double` | Calculated peptide MH+ mass. |
| `iRankSp` / `iMatchedIons` / `iTotalIons` | `int` | SP rank and ion match counts. |
| `szPeptide[MAX_PEPTIDE_LEN]` | `char[]` | Peptide sequence (no flanking AAs). |
| `szPrevNextAA[2]` | `char[]` | `[0]` = preceding AA, `[1]` = following AA. |
| `piVarModSites[MAX_PEPTIDE_LEN_P2]` | `int[]` | Per-position variable mod encoding. Values 1–9 map to `varModList[0–8]`. Values ≥ `COMPOUNDMODS_OFFSET` (100) encode compound mods. Indices `iLenPeptide` and `iLenPeptide+1` hold N/C-terminal mod codes. |
| `pdVarModSites[MAX_PEPTIDE_LEN_P2]` | `double[]` | Mass delta at each modified position. |
| `lProteinFilePosition` | `comet_fileoffset_t` | File offset into the FASTA for the matched protein. |
| `pWhichProtein` | `vector<ProteinEntryStruct>` | All proteins sharing this peptide (sorted by file offset). |
| `pWhichDecoyProtein` | `vector<ProteinEntryStruct>` | Decoy proteins (concatenated search mode). |

---

## StaticParams

The global parameter aggregate. Fully populated before any search thread starts; treated as read-only during search.

```cpp
struct StaticParams  // CometDataInternal.h:602
extern StaticParams g_staticParams;
```

Contains nested sub-structs (all defined in `CometDataInternal.h`):

| Sub-struct | Type | Key contents |
|------------|------|-------------|
| `options` | `Options` | ~40 integer/bool flags controlling output formats, decoy mode, charge limits, clipping, indexing, etc. |
| `tolerances` | `ToleranceParams` | Precursor tolerance + units, fragment bin size + offset, isotope error mode. |
| `massUtility` | `MassUtil` | `pdAAMassParent[128]` and `pdAAMassFragment[128]` look-up tables; mono vs. average flag. |
| `precalcMasses` | `PrecalcMasses` | Pre-computed `dNtermProton`, `dCtermOH2Proton`, `dOH2ProtonCtermNterm`, BIN'd H₂O/NH₃ values. |
| `staticModifications` | `StaticMod` | Per-AA static mod deltas (`pdStaticMods[128]`), peptide/protein terminal additions. |
| `variableModParameters` | `VarModParams` | `varModList[VMODS]` (9 slots), mod symbol codes, max-per-peptide limit, compound mod list. |
| `ionInformation` | `IonInfo` | Active ion series bitmask, water/ammonia loss flag, flanking peak mode. |
| `enzymeInformation` | `EnzymeInfo` | Search enzyme, sample enzyme, 2nd enzyme, allowed missed cleavages, offset directions. |
| `databaseInfo` | `DBInfo` | FASTA path; `iTotalNumProteins` and `uliTotAACount` updated during batch scan. |
| `dInverseBinWidth` / `dOneMinusBinOffset` | `double` | Used in the `BIN(mass)` macro on every fragment ion — computed once to turn division into multiplication. |
| `tRealTimeStart` | `chrono::time_point` | Timestamp set at the start of each `DoSingleSpectrumSearch()` call; used to enforce `iMaxIndexRunTime`. |

---

## VarMods / VarModParams

```cpp
struct VarMods  // CometData.h:218   (one entry per mod slot)
struct VarModParams  // CometDataInternal.h:472   (all mod config)
```

`VarModParams` contains:

| Field | Purpose |
|-------|---------|
| `varModList[VMODS]` | Array of 9 `VarMods` entries. `VMODS = 9`, indexed 0–8. |
| `cModCode[VMODS]` | Output symbol for each mod: `*`, `#`, `@`, `^`, `~`, `$`, `%`, `!`, `+`. |
| `bVarModSearch` | Set to `true` if any mod has a non-zero mass; gates the `WithVariableMods` code path. |
| `iMaxVarModPerPeptide` | Total modified residues allowed per peptide across all mods. |
| `iMaxPermutations` | Cap on permutation count in `WithVariableMods`. |
| `vdCompoundMasses` | `vector<double>` of masses from the compound mods file (compoundmods branch). |
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

One entry in the peptide index (`g_pvDBIndex`), sorted by mass for binary-search lookup.

```cpp
struct DBIndex  // CometDataInternal.h:377
```

| Field | Purpose |
|-------|---------|
| `szPeptide[MAX_PEPTIDE_LEN]` | Peptide amino acid sequence. |
| `szPrevNextAA[2]` | Flanking residues (for enzyme termini check in index search). |
| `pcVarModSites[MAX_PEPTIDE_LEN_P2]` | Compact mod-site encoding (0–9; same scheme as `piVarModSites`). |
| `dPepMass` | MH+ mass; the sort key. |
| `lIndexProteinFilePosition` | Index into `g_pvProteinsList` mapping to a list of protein file offsets. |

---

## PepMassInfo / SpectrumInfoInternal

Small structs embedded in each `Query`.

```cpp
struct PepMassInfo  // CometDataInternal.h:219
```
Stores the experimental MH+ mass (`dExpPepMass`) and the ± tolerance window (`dPeptideMassToleranceMinus` / `dPeptideMassTolerancePlus`) pre-computed for fast range checks.

```cpp
struct SpectrumInfoInternal  // CometDataInternal.h:228
```
Stores scan number, charge state, retention time, array size, and the nativeID string from mzML files.

---

## sDBEntry

Passed through the FASTA search loop; holds data for a single protein from the database.

```cpp
typedef struct sDBEntry  // CometDataInternal.h:348
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
struct MassRange  // CometDataInternal.h:243
extern MassRange g_massRange;
```

Computed once per spectrum batch from the lowest and highest precursor masses in `g_pvQuery`. Search threads read `dMinMass` / `dMaxMass` for early-exit decisions in `SearchForPeptides`. `iMaxFragmentCharge` caps the fragment ion charge loop.

---

## Public API types (CometData.h)

These types cross the library boundary into `CometWrapper` and `RealtimeSearch`.

| Type | Purpose |
|------|---------|
| `Scores` | XCorr, dCn, E-value, mass, matched/total ions — returned per search hit. |
| `Fragment` | Single fragment ion: mass, intensity, type, number, charge, neutral loss. |
| `VarMods` | User-facing mod definition (same as internal `VarMods`; shared header). |
| `EnzymeInfo` | Enzyme parameters surfaced to the wrapper layer. |
| `InputFileInfo` | Per-file input descriptor (type, scan range, file path). |
| `CometParam` / `TypedCometParam<T>` | Type-erased parameter container used to pass params through `ICometSearchManager::SetParam()`. |
