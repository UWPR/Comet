# Internal (pseudo-reverse) decoys for FI_DB searches -- design and plan

Status: **Plan reviewed 2026-09-14; all five Section 7 recommendations accepted.** The Section 4.5
RTS reporting fix was pulled forward and landed first (commit 94a854ef; it is a live bug for PI_DB
RTS with `decoy_search = 1`, independent of FI_DB). **Phases 1 and 2 implemented 2026-09-14**
(Section 6 has the measurements; Phase 1 = commit d208e7d1). FI_DB internal decoys are
functional in batch and RTS for `decoy_search = 1|2`. Phase 3 (guard rail, doc wording) and
Phase 4 (T34, T24 parity, benchmarks) remain. Branch: `FI_internaldecoys`.

## 1. Goal

`decoy_search = 1|2` currently generates a pseudo-reversed decoy for every peptide scored in
FASTA_DB and PI_DB searches, moving variable modifications along with their residue. FI_DB
searches ignore the setting entirely: `SearchFragmentIndex()` has no `iDecoySearch` branch and
its single `XcorrScoreI()` call hard-codes `bDecoyPep = false` (CometSearch.cpp:2168). The only
way to get decoys out of an FI_DB today is to index a target-decoy FASTA.

This plan adds Comet internal decoys to FI_DB with these properties:

- Every target peptide variant indexed gets exactly one decoy variant, pseudo-reversed with the
  same terminal-residue rule the PI_DB path uses (`iSearchEnzymeOffSet == 1`: last residue fixed,
  `ABCDEK -> EDCBAK`; otherwise first residue fixed, `ABCDEK -> AKEDCB`).
- Variable modifications move with their residue; N-/C-terminal variable mods stay on the terminus.
- The decoy references the **same `g_pvProteinsList` row** as its target. The existing reporting
  code prepends `decoy_prefix` to those protein names when the result sits in
  `pWhichDecoyProtein` (batch writers) -- one RTS-path gap is fixed in Phase 3.
- Decoys compete in the fragment-index pre-filter and the scoring cap on equal footing with
  targets (Section 2 explains why this is the only statistically sound option).
- `decoy_search = 0` output stays byte-identical to today (targets' postings unchanged).
- No `.idx` format change. The fragment index is built in memory from `g_vRawPeptides` on every
  load (`CometFragmentIndex::CreateFragmentIndex()`, both batch and RTS init), so decoy variants
  are derived at that point and never persisted.

## 2. Why decoys must be indexed, not reversed on the fly

PI_DB scores every mass-matched candidate, so `AnalyzePeptideIndex()` can simply reverse each
candidate after scoring it (CometSearch.cpp:2697-2996) and the decoy population samples the same
null as the targets.

FI_DB is different: a peptide is only scored if it (a) shares at least `iFragIndexMinIonsScore`
binned b/y fragments with the spectrum (posting-list walk, CometSearch.cpp:1596-1745) and (b)
ranks in the top `FRAGINDEX_MAX_NUMSCORED` (100) candidates by matched-fragment count. If we
reversed each *surviving target* and scored the decoy, the decoys would never have passed the
fragment pre-filter themselves. Their XCorr distribution would be systematically lower than that of
false targets (which did pass), so the target-decoy FDR estimate would be anti-conservative.

The decoy therefore has to be a first-class indexed variant: its own b/y fragments in
`g_iFragmentIndex`, its own entry in `g_fragmentPeptides`, counted and ranked exactly like a
target. This is precisely what already happens when a target-decoy FASTA is indexed, so search-time
behavior (candidate cap shared by targets and decoys, E-value histogram fed by both) is already
the accepted FI_DB semantics.

## 3. What the existing structures give us for free

| Fact | Consequence for decoys |
|---|---|
| Pseudo-reversal preserves residue composition and the mod set | Decoy precursor mass == target precursor mass, bit-for-bit (`ComputeIndexedPepMass()` sums the same residues/mods; only the order of *fragment* accumulation changes). So `vuiMassKey`, the `g_massRange` filter, the `g_bIndexPrecursors` presence filter, `CheckMassMatch()`, and the boundary-key exact recompute are all reusable unchanged. |
| `VariantArray` is 13 B/entry SoA: `vuiMassKey`, `vuiWhichPeptide`, `vuiModNumIdx`, `vucTermMods` | A decoy variant is `(same iWhichPeptide, same modNumIdx, same term mods) + 1 flag bit`. `vucTermMods` nibbles hold values 0..5 (`cNtermMod+1`, codes -1..4), so **bit 7 (0x80) is free** for the decoy flag. No per-entry growth. |
| `FragmentPeptidesStruct` (build staging) is 24 B with 5 B of tail padding | A `char cIsDecoy` fits in the padding; staging stays 24 B/entry. |
| `Results.lProteinFilePosition` / `ProteinEntryStruct.lWhichProtein` hold the `g_pvProteinsList` row | `StorePeptideI()` already routes `bDecoyPep` rows to `pWhichDecoyProtein` (CometSearch.cpp:9124-9127) with the *target's* row index -- exactly the "same protein list, prefix added at report time" behavior requested. |
| Batch protein-name helper (`CometMassSpecUtils.cpp:288-312`) walks `pWhichDecoyProtein` and every writer prefixes those names (`CometWriteTxt.cpp:445`, `CometWritePepXML.cpp:572/590`, `CometWriteSqt.cpp:350`, `CometWritePercolator.cpp:300`, mzIdentML) | Batch output needs **no writer changes**. |
| `XcorrScoreI()` already increments `_uliNumMatchedDecoyPeptides` and the XCorr histogram for `bDecoyPep` | E-value machinery unchanged. |
| `.idx` header `DecoySearch:` is read back into `g_staticParams.options.iDecoySearch` at load (CometPeptideIndex.cpp:1855) | The decoy mode is decided by the value the index was built with, same as PI_DB today. See Decision D3. |

## 4. Design

### 4.1 Data model (core/Types.h)

- `VariantArray`: add `static constexpr unsigned char DECOY_FLAG = 0x80;`,
  `bool IsDecoy(size_t i) const { return vucTermMods[i] & DECOY_FLAG; }`, and mask the flag out in
  `GetNtermMod()` (`(vucTermMods[i] >> 4) & 0x07`). `GetCtermMod()` is unaffected.
- `FragmentPeptidesStruct`: add `char cIsDecoy;` after `cCtermMod` (sizeof stays 24; add a
  `static_assert(sizeof(FragmentPeptidesStruct) == 24)` so the padding claim is checked).
- `GenerateFragmentIndex()` transcode: OR the flag into the term-mods byte when `s.cIsDecoy`.

Alternative considered: steal the top bit of `vuiWhichPeptide` (halves the raw-peptide ceiling
to 2^31 and touches the `CreateFragmentIndex()` range check). Rejected -- the term-mods byte has a
genuinely free bit. See Decision D1.

### 4.2 Shared pseudo-reverse helper (CometSearch.h/.cpp)

Factor the PI_DB reversal (CometSearch.cpp:2709-2735) into one static helper so the FI build,
the FI search, and PI_DB all use the same rule:

```cpp
// Pseudo-reverse szPeptide[0..iLen) into szDecoy (NUL-terminated), keeping the enzyme-side
// terminal residue fixed per g_staticParams.enzymeInformation.iSearchEnzymeOffSet, and
// permute the per-residue slot array the same way. piSites/piSitesDecoy are iLen+2 long
// (N-term at [iLen], C-term at [iLen+1]) and may be NULL when the caller has no mods.
static void PseudoReversePeptide(const char* szPeptide, int iLen, const int* piSites,
                                 char* szDecoy, int* piSitesDecoy);
```

`AnalyzePeptideIndex()` switches to it (pure refactor, output must stay byte-identical). The two
FASTA-path inline reversals (`SearchForPeptides()` at ~3660 and `CalcVarModIons()` at ~7972,
plus the compound-mod block at ~9461) use a flanking-residue-in-string layout and are **left
alone** (Decision D5).

### 4.3 Build time: emit a decoy variant per accepted target variant (CometFragmentIndex.cpp)

`AddFragments()` grows a `bool bDecoy` parameter and returns the accepted precursor mass
(`-1.0` when the variant is rejected by length/mass-range/precursor filters).

- **Count pass** (`AddFragmentsThreadProcRange()`): every existing `AddFragments(...)` call site
  (8 of them, one per mod-combination shape) becomes a call to a small local lambda
  `addVariant(modNumIdx, ctNterm, ctCterm)` that calls `AddFragments(bDecoy=false)`, and, if
  `g_staticParams.options.iDecoySearch != 0` and the returned mass is `>= 0`, calls
  `AddFragments(bDecoy=true, dKnownPepMass=<that mass>)`. The decoy therefore reuses the P2
  known-mass short-circuit and is never generated for a rejected target. Both entries are
  pushed to the thread-local staging vector target-first, so the concatenation order stays
  deterministic across thread counts (T22's 1-vs-8-thread guarantee).
- **Fill sub-passes**: unchanged loops; each staging entry carries its own `cIsDecoy`, passed
  through to `AddFragments(bDecoy = fp.cIsDecoy)`.
- **Inside `AddFragments()`**: the fragment-ladder loop currently walks `pModSeq` with two cursors
  (`j` forward, `k` reverse) on the forward sequence. For a decoy, first materialize the target's
  per-residue slot array (`piSlot[i]`, -1 = unmodified; the same `j`-walk `SearchFragmentIndex()`
  uses at CometSearch.cpp:1826-1840), call `PseudoReversePeptide()`, then run the ladder over
  `(szDecoy, piSlotDecoy)`. To keep one ladder loop, refactor it to read residue + slot from
  arrays for both cases; for targets the array is the identity materialization, and the order of
  floating-point additions per position (residue mass, then mod mass) is unchanged, so target
  postings are bit-identical. Terminal variable mods (`cNtermMod`/`cCtermMod`) are added to
  `dBion`/`dYion` before the loop exactly as today for both target and decoy.
- **Diagnostics**: the `- N total peptides, M FI entries` line reports the decoy variant count
  separately when `iDecoySearch != 0`.
- The `iTotal >= UINT_MAX` guard and the mass-key range check already cover the doubled count.

### 4.4 Search time: reconstruct and score decoy candidates (CometSearch.cpp, `SearchFragmentIndex()`)

After `rawView`/`szPeptide`/`piVarModSites` are materialized for a candidate variant
(CometSearch.cpp:1789-1857):

```cpp
const bool bDecoyPep = g_fragmentPeptides.IsDecoy(uiWhichVariant);
if (bDecoyPep)
   CometSearch::PseudoReversePeptide(szPeptide, iLenPeptide, piVarModSites, szPeptide, piVarModSites);  // in place via temps
```

Everything downstream (ladder build with NL tracking, `szProtein` flanking assembly from
`rawView.cPrevAA/cNextAA`, `dbe.lProteinFilePosition = rawView.lIndexProteinFilePosition`) is
then unchanged, and the `XcorrScoreI()` call passes `bDecoyPep` instead of `false`.

Two `XcorrScoreI()`/`StorePeptideI()` fixes so FI behaves like PI for decoys:

1. **FI gate uses the wrong floor for `decoy_search = 2`** (CometSearch.cpp:8600-8603): it
   compares against `pQuery->dLowestXcorrScore` even when the candidate is headed for
   `_pDecoys`. Mirror the PI branch (8616-8620): pick `dLowestDecoyXcorrScore` when
   `bDecoyPep && iDecoySearch == 2`.
2. **`CheckDuplicateI()` gate** (CometSearch.cpp:8665) currently returns early unless
   `PI_DB && iDecoySearch == 1`. Extend to `(PI_DB || FI_DB) && iDecoySearch == 1` so a
   self-palindromic decoy (e.g. `AGGAK`, whose pseudo-reverse is itself) merges into the target
   row's `pWhichDecoyProtein` instead of producing a second identical `_pResults` row. The scan
   is over at most `iNumStored` entries, so cost is negligible. Update the function's doc
   comment, which currently states FI_DB never sets `bDecoyPep`. (Decision D2.)

The stale comment at CometSearch.cpp:5005 (`StorePeptide()`'s FASTA/FI branch, "internal decoys
not supported yet") is updated.

### 4.5 Reporting

- **Batch**: one change (found in Phase 2 testing): `CometMassSpecUtils::GetProteinNameString()`'s
  indexed-DB branch has a legacy FI_DB fallback (CometMassSpecUtils.cpp:270) that resolves the
  row from `lProteinFilePosition` as *targets* whenever `pWhichProtein` is empty -- which is
  exactly the shape of an internal-decoy row -- and then the decoy walk added the prefixed
  copies, so every FI_DB decoy row listed each protein twice (`sp|X ; DECOY_sp|X`) and began
  with a target accession. Gated the fallback on `pWhichDecoyProtein.empty()`. PI_DB never
  took that branch, so its output is unchanged.
- **RTS** (`CometSearchManager::DoSingleSpectrumSearchMultiResults()`, CometSearchManager.cpp:2978-2996) -- **DONE 2026-09-14**, ahead of Phase 1:
  for indexed DBs it resolves names from `g_pvProteinsList[lProteinFilePosition]` and classifies
  each as target/decoy **by name prefix only**. An internal decoy's proteins are the target's, so
  every internal-decoy hit is reported as a target. This is a latent bug for PI_DB RTS with
  `decoy_search = 1` today and would hit FI_DB the same way. Fix: when
  `pOutput[i].pWhichDecoyProtein.size() > 0` (and `pWhichProtein` is empty), push every resolved
  name into `vProteinDecoys`; the existing loop at 3075-3082 already prepends the prefix only when
  it is not already present, so prefix-decoy proteins from a target-decoy index are not
  double-prefixed. Regression coverage: T22 `rts_pi` gained an internal-decoy sub-case (t19 index
  built with `decoy_search = 1`, synthetic target and decoy spectra; the decoy hit must come back
  as `AKS[79.9663]GFEDC` with a `DECOY_`-prefixed protein, the target control without). The
  sub-case is PI_DB-only until Phase 2 and is enabled for `rts_fi` then.
- RTS reports `_pResults` only (2931-2934), so `decoy_search = 2` yields no decoys in RTS output.
  Document that RTS should use `decoy_search = 1`; no code change.
- `RealtimeSearch/Search.cs:265` and `SearchMS1MS2.cs:867` set `decoy_search = 0` before init,
  but the `.idx` header value overrides it at load, so C# needs no change to *use* the feature;
  whether to expose it is a follow-up (Decision D3).

### 4.6 Guard rails

- If `iDecoySearch != 0` and any protein name in `g_pvProteinNameCache` starts with
  `szDecoyPrefix`, log a one-line warning at FI build: internal decoys on top of a target-decoy
  index double-count decoys. PI_DB has the same exposure today (its reversal block at 2697 is
  not gated on `!bDecoyPep`). Warn only; do not change behavior (Decision D4).

## 5. Cost

Reference full-scale FI_DB config from docs/20260827_PI_memory.md (124.9 M variants, 3.54e9
posting entries, ~13.8 GB of the ~19 GB is the posting list):

| Component | Today (targets only) | With internal decoys | Note |
|---|---|---|---|
| `g_fragmentPeptides` | 13 B x V | 13 B x 2V | +1.6 GB at 124.9 M variants |
| `g_iFragmentIndex` postings | 4 B x P | 4 B x 2P | dominant; +~14 GB at 3.54e9 postings |
| Staging (transient) | 24 B x V | 24 B x 2V | released page-wise during transcode as today |
| `g_vRawPeptides`, `g_pvProteinsList`, name cache | unchanged | unchanged | decoys share them |
| FI build time | count + 2 fill passes | ~2x each pass | build is O(variants x length) |
| Search time | -- | posting walk sees ~2x entries per peak; scoring still capped at 100/spectrum | expect well under 2x per spectrum; measure |

For comparison, the current workaround (index a target-decoy FASTA) pays the same 2x on variants
and postings **plus** 2x on raw peptides, protein-list rows, and the name cache. Internal decoys
are strictly cheaper than what FI_DB users do today to get decoys, and a tryptic human build is far
below the reference numbers above.

## 6. Implementation phases

Each phase ends with build + tests + a stop-and-report; no commits unless asked.

**Phase 0 -- Baseline capture (no code).** Record for the current binary: (a) `tests/unit/run_tests.py`
full pass; (b) T22 `rts_fi` output on the fixture; (c) batch FI_DB `.txt` output for
`decoy_search = 0` on the T24 big-data set plus `COMET_MEMREPORT` and wall-clock; (d) PI_DB
`decoy_search = 1` output on the same data (the reference decoy population for Phase 4's
comparisons).

**Phase 1 -- Data model + build-time decoy variants.** 4.1, 4.2 (helper + PI refactor), 4.3.
Acceptance: with `decoy_search = 0`, FI_DB batch and RTS outputs are byte-identical to Phase 0;
PI_DB `decoy_search = 1` output byte-identical after the helper refactor; with `decoy_search = 1`
the build log reports exactly one decoy variant per target variant and roughly 2x postings.

*DONE 2026-09-14.* Measured on `data/human.small.fasta` + `data/comet_phospho.params` (M ox +
STY phospho, trypsin), 197-spectrum `tests/rts_repro` fixture (also converted to `.ms2` for the
batch runs), before vs. after with the same source tree otherwise:

| Check | Result |
|---|---|
| FI_DB `decoy_search = 0`: `.idx`, RTS output, batch `.txt` body | byte-identical (only the output-path header line differs) |
| PI_DB `decoy_search = 1`: `.idx`, RTS output, batch `.txt` body (296 decoy rows) | byte-identical after the `PseudoReversePeptide()` refactor |
| FI_DB `decoy_search = 1` build | 5,444,902 decoy variants for 5,444,902 target variants (10.89M total); posting entries 1.532e8 -> 3.064e8 (2.00x); build 3 s -> 7 s |
| Unit suite (52 tests) + T22 `rts_fi`/`rts_pi` | see Phase 1 report in the conversation / commit message |

Note for Phase 2: an FI_DB built with `decoy_search != 0` is already searchable after Phase 1 but
NOT yet correct -- `SearchFragmentIndex()` still reconstructs every candidate from the raw (target)
sequence and passes `bDecoyPep = false`, so a decoy variant that survives the fragment pre-filter
is scored with the target's ladder and stored as a target. Do not use `decoy_search = 1|2` with
FI_DB until Phase 2 lands.

**Phase 2 -- Search-time scoring.** 4.4 including both gate fixes. Acceptance: FI_DB
`decoy_search = 1` on the T22 fixture produces `DECOY_`-prefixed hits; every reported decoy
sequence is the pseudo-reverse of a target sequence present in the fixture FASTA with mods at
mirrored positions (small Python check in the test); `decoy_search = 2` populates the separate
decoy output.

*DONE 2026-09-14* (plus the 4.5 batch-helper fix above). Same data as Phase 1:

| Check | Result |
|---|---|
| T22 `rts_fi` internal-decoy sub-case (guard removed) | passes: decoy spectrum -> `AKS[79.9663]GFEDC`, `DECOY_`-prefixed; control target unprefixed |
| FI_DB `decoy_search = 0` and PI_DB `decoy_search = 1`, all outputs | still byte-identical to the pre-Phase-1 baseline |
| FI_DB `decoy_search = 1`, batch, 197 spectra, top 3 | 87 rank-1 PSMs (67 without decoys), 36 rank-1 decoys; all 95 decoy rows un-reverse to a sequence present in the FASTA (I/L-aware) |
| FI_DB vs PI_DB `decoy_search = 1`, batch | same rank-1 modified peptide on 46/87 shared scans (the known ~51% FI-vs-PI agreement on this fixture, see T22's comment); target/decoy label agrees on every one; for the 34 (scan, charge, decoy sequence) keys both report, the phospho-isomer sets differ only by top-3 truncation |
| FI_DB `decoy_search = 2`, batch | target file has 0 `DECOY_` rows; `.decoy.txt` has 117 rows, all prefixed, all valid pseudo-reverses |
| FI_DB `decoy_search = 1`, RTS, 1 vs 8 threads | byte-identical; 36 decoy top-1 hits |
| Unit suite + T22 | 52/52, 2/2 |

**Phase 3 -- Reporting + guard rails.** 4.5 RTS fix already landed (enable its T22 sub-case for
`rts_fi` and verify), 4.6 warning, comment/doc cleanups (CometSearch.cpp:5005, `CheckDuplicateI()`
doc, CLAUDE.md "FI_DB searches currently don't support internal decoys" wording, the Key Globals
`g_fragmentPeptides` row).

**Phase 4 -- Tests, parity, performance.**
- New unit test (next free ID, T34): small fixture with (i) a palindromic tryptic peptide and
  (ii) an oxidizable-M peptide. Build FI_DB with `decoy_search = 1` and `= 2`; assert decoy
  presence, prefix, reversal correctness, mod mirroring (`PEPTM[15.9949]IDEK ->
  EDIM[15.9949]TPEPK`), palindrome merged (1 row, protein list carries both target and decoy
  names) for mode 1 and separate-list for mode 2. Also assert `decoy_search = 0` output is
  byte-identical between FI_DB before/after via the pinned baseline binary
  (`tests/regression/setup_baselines.py`), which doubles as a T18-style build-determinism check
  for the in-memory FI.
- T22 `rts_fi`: add a `decoy_search = 1` sub-case (decoy hits prefixed; 1-thread vs 8-thread
  byte-identical).
- T24 (`--bigdata`): add FI_DB `decoy_search = 1` vs PI_DB `decoy_search = 1` vs FASTA
  `decoy_search = 1`, PSMs at 1% FDR (`tools/qvalue.py`) within the existing 5% tolerance, and
  FI_DB internal-decoy vs FI_DB target-decoy-FASTA within 5%.
- Performance: `comet-benchmark` skill, batch and RTS Hz, `decoy_search = 0` vs `1`, plus
  `_check_timing()` against the baseline for the `= 0` case (must be within noise -- the target
  path only gains an array materialization per variant).

## 7. Decisions for review

- **D1 -- Decoy flag location.** Recommended: bit 7 of `vucTermMods` (free, zero growth, no
  addressing impact). Alternative: top bit of `vuiWhichPeptide`.
- **D2 -- Self-palindromic decoys.** Recommended: keep them and extend `CheckDuplicateI()` to
  FI_DB (PI_DB parity, T24 comparisons stay apples-to-apples). Alternative: skip at build time
  when `szDecoy == szPeptide` (slightly fewer decoys than PI_DB; simpler search path).
- **D3 -- Which `decoy_search` value wins for FI_DB.** Nothing is baked into the `.idx` for
  either PI_DB or FI_DB decoys, yet the header's `DecoySearch:` overrides the params value at load.
  Recommended: keep the current semantics for now (matches PI_DB, and the C# layer hard-codes
  `decoy_search = 0`), and open a follow-up to let the search-time value win for both indexed
  modes if you want RTS to toggle decoys without rebuilding.
- **D4 -- Internal decoys on a target-decoy index.** Recommended: warn only (Section 4.6).
  Alternative: skip decoy generation for raw peptides whose protein rows are all decoy-prefixed
  (needs `g_pvProteinNameCache` lookups per raw peptide at build; cheap but more code, and PI_DB
  would still differ).
- **D5 -- Scope of the reversal refactor.** Recommended: share the helper between PI_DB and
  FI_DB only; leave the three FASTA-path inline reversals as they are.

## 8. Files touched (expected)

| File | Change |
|---|---|
| `CometSearch/core/Types.h` | `VariantArray::DECOY_FLAG`/`IsDecoy()`, masked `GetNtermMod()`; `FragmentPeptidesStruct::cIsDecoy` + size static_assert |
| `CometSearch/CometFragmentIndex.h/.cpp` | `AddFragments()` `bDecoy` param + mass return; `addVariant` lambda in `AddFragmentsThreadProcRange()`; ladder loop over per-residue slot arrays; flag transcode; decoy count in build log; Section 4.6 warning |
| `CometSearch/CometSearch.h/.cpp` | `PseudoReversePeptide()`; `AnalyzePeptideIndex()` refactor; `SearchFragmentIndex()` decoy branch + `bDecoyPep` pass-through; `XcorrScoreI()` FI decoy floor; `CheckDuplicateI()` gate + doc; comment at 5005 |
| `CometSearch/CometSearchManager.cpp` | RTS protein classification by `pWhichDecoyProtein` |
| `tests/unit/run_tests.py`, `tests/unit/data/` | T34 fixture + test; T22/T24 sub-cases |
| `CLAUDE.md`, `docs/` | Feature wording; this document's status updates |
