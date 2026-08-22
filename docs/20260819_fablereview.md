# Deep Analysis: Bugs and Performance Opportunities (2026-08-19)

Six parallel deep reviews were run over the codebase, one per subsystem: core
search/scoring (`CometSearch.cpp`), index build/read (`CometFragmentIndex`,
`CometPeptideIndex`, `CometPredictedMask`), spectrum preprocessing
(`CometPreprocess`), post-analysis and utilities (`CometPostAnalysis`,
`CometMassSpecUtils`, `CombinatoricsUtils`, `CometModificationsPermuter`,
`CometAlignment`, `CometSpecLib`), manager/output/CLI (`Comet.cpp`,
`CometSearchManager`, all writers, `search/` + `output/` layers), and a
dedicated threading/RTS concurrency audit. Every finding below was verified by
reading the code and its callers (static analysis; no binary was built in this
session). Findings independently confirmed by more than one review are marked
**[xN]**. Line numbers are as of commit `ff3fc268` (branch `carafe`).

Already-fixed issues (T25-T28 area: mod-slot sentinel, `dMaxNL` break,
`ExportPeptideIndexVariants` mass-range init) were excluded; several findings
below are siblings of those bug shapes.

---

## 0. Master-branch triage (added 2026-08-19)

The analysis above was performed on branch `carafe` at commit `ff3fc268`.
`carafe` was branched from `master`'s current HEAD (`6edec914` is the exact
merge-base, and `master` has not advanced past it), so `carafe` is strictly
*ahead* of `master` — it never merged back. This section re-verifies every
finding below against `master`'s actual current code before anyone starts
fixing things here.

**Most reviewed files are byte-identical between `master` and
`carafe@ff3fc268`** (confirmed via `sha1sum` diffing every file `carafe`
touches vs. every other file in the repo): `CometSearch.cpp`,
`CometPreprocess.cpp`/`.h`, `CometPostAnalysis.cpp`, `CometSearch.h`,
`CometModificationsPermuter.cpp`, `CombinatoricsUtils.cpp`/`.h`,
`CometSpecLib.cpp`, `CometMassSpecUtils.cpp`, `CometData.h`, `core/Types.h`,
`search/Pipeline.cpp`, `search/FiStrategy.cpp`, `output/*` (writers,
`MzIdentMlWriter.h`), `CometWrapper/*`, `threading/*`. **Every finding sited
only in those files transfers to master unchanged, with the exact same line
numbers.** That covers B1-B4, B6-B8, B13, C2, C4, C7, C9, C11, most of C12,
most of section 3, and most of section 4 (P3, P4, P6, P7, P9, P10, and half
of P11) — no further action needed on those beyond what's already written
below.

Four files differ, because `carafe` added a predicted-fragment-mask feature
and a `-x` peptide-index-variant export feature on top of them:
`Comet.cpp`, `CometSearch/CometSearchManager.cpp`/`.h`,
`CometSearch/CometFragmentIndex.cpp`, `CometSearch/CometPeptideIndex.cpp`/
`.h`. `CometPredictedMask.cpp`/`.h` are new files that exist **only** on
`carafe` — findings that depend on them do not apply to master at all.
Every finding touching one of these five files was individually re-checked
against master's real current line numbers (not recomputed by arithmetic —
actually grepped/read). Master's `CometPeptideIndex.cpp` in particular is
already well ahead of the reviewed carafe snapshot in its own right: PR #125
("Restore index files...") hardened the `.idx` v4 header parser (bounds-checked
`%31s`/`sscanf`-return-count validation) on master's line, and commit
`1fff1db2` ("Phase 0.5") changed `g_vDBIndexVariants` to regenerate per
session instead of persisting — both landed on master before `carafe` branched,
so they're visible in both, but they mean some of the exact code shapes the
review describes have moved or hardened.

| ID | Verdict | Master location | Notes |
|----|---------|------------------|-------|
| B1 | ✅ **FIXED** (2026-08-19) | `CometSearch.cpp:7535` | Confirmed byte-identical; decoy `break` still there. Fix applied: deleted the `break;`, mirroring the target/decoy-y branches which have none. |
| B2 | ✅ **FIXED** (2026-08-19) | `CometSearch.cpp:1668` | Confirmed byte-identical. Fix applied: `if (i > iStartPos)` → `if (i > 0)`. |
| B3 | ✅ **FIXED** (2026-08-19) | `CometSearch.cpp:5783` | Confirmed byte-identical. Fix applied: `int iSum8 = i9 + i8;` → `int iSum8 = iSum9 + i8;`, restoring the `iSumN = iSum(N+1) + iN` pattern. |
| B4 | ✅ **FIXED** (2026-08-19) | `CometModificationsPermuter.cpp:428` | Confirmed byte-identical. **Fix applied:** replaced the O(n²) all-pairs loop with a single O(n) running-union pass — `if ((combinedBitmasks & bitmasks[j]) != 0) return false; combinedBitmasks \|= bitmasks[j];` — which detects the same pairwise-overlap conflicts (any two masks sharing a bit shows up when the later one is OR'd against the accumulated union of the earlier ones) while leaving `combinedBitmasks` holding the union of *every* mask instead of just the last pair. **Verified with a targeted differential test**, not just the existing suite: built an FI_DB with 3 single-residue variable mods (S/T/Y) and `max_variable_mods_in_peptide = 2`, instrumented `combine()` with a temporary trace. Pre-fix, the 3-mod-type combination (`modNumCount=3`) computed `combinedBitmasks=3` (binary `011`, only the last pair's bits) → `bitCount=2` → incorrectly passed the cap check. Post-fix, the same combination computes `combinedBitmasks=7` (`111`, all three) → `bitCount=3` → correctly rejected. Pairwise (`modNumCount=2`) and singleton (`modNumCount=1`) combinations were unaffected in both runs. Full unit suite (42/42) also still passes. |
| B5 | ✅ **FIXED (2026-08-21)** | `CometPeptideIndex.cpp` reset loop and `VariableMod:` parse in `ParsePeptideIndexHeader()` | Master's header parser is the hardened v4 parser (PR #125), but it still only restored `szVarModChar`/mass/`bVarModSearch`/`bUseFragmentNeutralLoss` from the `.idx` header — it never re-derived `bNtermMod`/`bCtermMod`/`bVarTermModSearch`, which were set once from *search-time* params before the header overwrote `szVarModChar`. **Fix applied:** the reset loop now also zeroes `bNtermMod`/`bCtermMod`/`bVarTermModSearch`, and the `VariableMod:` parse now derives all three from each slot's restored `szVarModChar` (`'n'`/`'c'` membership), mirroring `CometSearchManager.cpp`'s search-time derivation exactly. `bVarProteinNTermMod`/`bVarProteinCTermMod` were investigated and deliberately **not** added to the header restore: confirmed via full-codebase grep that these two flags have **zero readers anywhere** — not just unsupported by FI_DB/PI_DB (per the existing `iWhichTerm`/`iVarModTermDistance` exclusion below) but never consulted by any code path at all, so there is nothing for a header entry to feed. Verified via a differential test: built an FI_DB/`.idx` with a real c-term mod baked in, searched it with `variable_mod01`/`02` left blank — pre-fix the correct c-term-modified peptide never got enumerated (1281 vs. 2581 total peptides) and a wrong candidate ranked #1; post-fix the c-term-modified peptide is correctly enumerated and ranks #1. Full clean build + 42/42 unit suite passed. |
| B6 | ✅ **FIXED** (2026-08-19) | `CometSearch.cpp:3139-3192` etc. | Confirmed byte-identical. **Fix applied** (see the B6 write-up below for full before/after code and reasoning): (1) `SearchForPeptides()`'s and `CalcVarModIons()`'s target-ladder precursor-NL zero/fill loops now bound on `min(g_staticParams.options.iMaxPrecursorCharge, MAX_PRECURSOR_CHARGE-1)` instead of the first-matching query's own charge, so every charge a later query might read is freshly zeroed/filled, not left stale from a prior peptide; (2) found and fixed the identical bug in `CalcVarModIons()`'s *decoy* branch (not explicitly cited by the original review, but same cache-once-reuse-per-query shape); (3) `CompoundModSearch()` — which the review flagged as never touching the array at all — now zeroes+fills it properly for both target and decoy, closing a real read-garbage risk, not just a missing feature; (4) `SearchFragmentIndex()` (FI_DB) — which only `memset`-zeroed but never filled — now fills it per-candidate using the same formula as the PI_DB path, so `precursor_NL_ions` finally has an effect in FI_DB searches. Deliberately did **not** touch `MAX_PRECURSOR_CHARGE`-boundary behavior itself (charge-9 array indexing) — that's C8, a separate open finding; the clamp to `MAX_PRECURSOR_CHARGE-1` keeps this fix from widening C8's blast radius. Verified: full clean `make` build (no new warnings), 42/42 unit tests, and a functional smoke test — a plain-FASTA search with `precursor_NL_ions` configured and two spectra (charge 2 and charge 4) matching the same candidate peptide in one batch (the exact scenario that triggers the bug) — completed cleanly with sane, charge-appropriate scores for both. |
| B7 | ✅ **FIXED** (2026-08-19) | `CometSearch.cpp:6983` (was cited as `6953`, the binary search one line above) | Confirmed byte-identical. **Fix applied:** the seek-back `while` loop compared `dCalcPepMass` (pre-PEFF) while the binary search immediately above it keyed on `dTmpCalcPepMass` (post-PEFF); changed the comparison to `dTmpCalcPepMass`, matching the already-correct pattern in `WithinMassTolerancePeff()` (line 3743) which uses the same combined mass consistently in both steps. |
| B8 | ✅ **FIXED** (2026-08-19) | Target loop bound `CometSearch.cpp:5290` (was cited as `5260`, the length/mass pre-check a few lines above the actual loop); decoy stray write `CometSearch.cpp:5198` | Confirmed byte-identical. **Fix applied, both sub-bugs:** (1) target loop widened from `ii <= usiLenPeptide` to `ii < usiLenPeptide + 2`, matching the decoy branch's already-correct bound, so two IDs differing only in C-term mod state are no longer merged; (2) decoy branch's `pQuery->_pDecoys[i].pdVarModSites[i] = 0.0;` (using the *stored-entry* index `i` as a *position* index into an unrelated `double[MAX_PEPTIDE_LEN_P2]` array — meaningless at best, an OOB write past that array once `num_results` exceeds its size at worst) replaced with the missing comparison `if (piVarModSites[ii] != 0) { bIsDuplicate = 0; break; }`, mirroring how the target branch already folds the `iVal == 0` case into its `>= 0` comparison correctly. |
| B9 | ✅ **FIXED (guard, 2026-08-21)** | `CometSearchManager.cpp:3336` and `:3359` (both `setSymbol(i + 1 + '0')` call sites — the review only cited one), `CometPostAnalysis.cpp:920` unchanged | Same bug at two call sites, not one. Given the 1-5 design intent (scope note from Jimmy, 2026-08-19: AScorePro was only ever intended for `variable_mod01`-`variable_mod05`, the modifications limit for FI), the review's framing ("corrupts mod sites for `variable_mod10`-`15`") describes a config that was never a supported combination in the first place — AScorePro + slots beyond 5 isn't a regression to fix so much as an unsupported configuration that should fail loudly instead of silently. **Fix applied:** `InitializeStaticParams()` now rejects (via `g_cometStatus`/`logerr`, `return false`) any search with `print_ascorepro_score != 0` (covers both a specific slot 1-5 and `-1`/"all mods") alongside an active `variable_mod06`-`15`. Verified with a 4-case differential test. The slot-15/`'?'` PEFF-placeholder collision is a separate concern, unaffected by this guard, and remains open. |
| B10 | ✅ **FIXED** (2026-08-19) | `add_U_selenocysteine` defined `Comet.cpp:441`, printed `Comet.cpp:1127`; actually-read name `add_U_user_amino_acid` at `CometSearchManager.cpp:812`; writer mismatches `CometWritePepXML.cpp:217,230` / `CometWriteMzIdentML.cpp:609,622` (unchanged files, confirmed identical); `speclib_ms_level` template text `Comet.cpp:938`, parser registration `Comet.cpp:404`, typo'd consumer `spectraL_library_ms_level` at `CometSearchManager.cpp:421` | All sub-findings confirmed present verbatim, just at new line numbers in the two changed files. **Fix applied:** `add_U_user_amino_acid` is the legacy/dead name — `add_U_selenocysteine` is the one actually registered by the CLI and printed in the params template, so `CometSearchManager.cpp:812` was changed to read `add_U_selenocysteine` (not the reverse). Found the same bug shape on the `O` slot while in there: the writers queried `add_O_ornithine`, which isn't registered anywhere — the real, live param is `add_O_pyrrolysine` (`Comet.cpp:435`) — so both writer call sites were corrected too (`CometWritePepXML.cpp:217`, `CometWriteMzIdentML.cpp:609`). Also fixed the `speclib_ms_level` three-way mismatch by standardizing on the template's existing name, `spectral_library_ms_level`: renamed the parser registration at `Comet.cpp:404` and fixed the `CometSearchManager.cpp:421` typo (`spectraL_...` → `spectral_...`) to match. |
| B11 | ✅ **FIXED** (2026-08-19) | Sentinel init now `Comet.cpp:644-651` (on `enzymeInformation.*` directly); checks unchanged at `Comet.cpp:~699-720` | Confirmed: `szSearchEnzymeName`/`szSearchEnzyme2Name`/`szSampleEnzymeName` were local arrays sentinel-initialized to `"-"`, but the parse loop and later `strcmp` checks both operate on `enzymeInformation.*` members instead, whose `EnzymeInfo` default constructor (`CometData.h`, unchanged) never sets that sentinel — checks could never fire. **Fix applied:** deleted the three unused local arrays and sentinel-initialize `enzymeInformation.szSearchEnzymeName`/`.szSearchEnzyme2Name`/`.szSampleEnzymeName` directly instead, so the existing `strcmp(..., "-")` checks now actually test what got written (or didn't) by the parse loop. Verified: `search_enzyme_number = 99` now exits 1 with "Error - search_enzyme_number 99 is missing definition in params file."; `search_enzyme_number = 1` (a valid entry) still runs with no false-positive error. |
| B12 | ✅ **FIXED** (2026-08-19) | `CometFragmentIndex.cpp:806-816` (long path, representative assign at 813), `:874-885` (short path, at 885) | Confirmed: dedup kept only the smallest-file-offset occurrence's `siVarModProteinFilter`. **Fix applied (both the long and short paths):** replaced `dbi.siVarModProteinFilter = rep.siVarModProteinFilter` (the representative-only assignment) with an `unsigned short siVarModFilterUnion` accumulated via `\|=` over every entry in the dedup run (reset to 0 after each flush), then assigned to `dbi.siVarModProteinFilter` — mirroring how the per-protein occurrence list (`prot`/`prots_flat`) already accumulates across the whole run rather than keeping only the representative's. A peptide shared between a listed and an unlisted protein now keeps every mod bit any of its occurrences allows. |
| B13 | ✅ **FIXED** (2026-08-19) — all 7 sub-issues | `CometSearch.cpp` / `CometPostAnalysis.cpp` various | Confirmed byte-identical. **All seven sub-findings fixed** — see the B13 write-up below for the full per-item detail (root cause, fix, and location for each). |
| C1 | ✅ **FIXED** (2026-08-19) | alloc `CometSearchManager.cpp:1562` (position matches original); read `CometFragmentIndex.cpp:541` (`AddFragments()`'s `g_bIndexPrecursors[BIN(dCalcPepMass)]`, not line 574 as in the carafe review) | Both halves confirmed present on master. Fix applied: allocation changed to `BIN(dPeptideMassHigh) + 1` and the init loop to `<=`, giving the array a valid slot at the inclusive top bin `ReadPrecursors`/`AddFragments` can reach. |
| C2 | ✅ **FIXED** (2026-08-19) | `CometPreprocess.cpp:1228-1237` | Confirmed byte-identical. **Fix applied:** `AllocateMemory()`'s three batch-pool arrays (`ppdTmpRawDataArr`/`ppdTmpFastXcorrDataArr`/`ppdTmpCorrelationDataArr`) now allocate `iArraySizeGlobal + iXcorrProcessingOffset` doubles instead of exactly `iArraySizeGlobal`, matching the RTS thread-local pool's already-correct `iSize + iXcorrPad` pattern; `Preprocess()`'s per-spectrum memset was widened to cover the same padded region so it doesn't leave stale data from a prior spectrum in the new pad bytes. Confirmed via code reading that the RTS pool was already safe and that the one other under-allocated site (`PreprocessSingleSpectrumCore()`'s non-thread-local-pool branch) is unreachable dead code (`PreprocessSingleSpectrum()`, its only caller, itself has no callers anywhere — matches Section 3's separate dead-code finding), so no fix was needed there. |
| C3 | ✅ **FIXED** (2026-08-19) | `CometSearchManager.cpp:2488` (`DoSingleSpectrumSearchMultiResults` entry gate) | Confirmed: same missing-clamp shape. **Fix applied:** clamped `iArraySize` directly to `iArraySizeGlobal` at all three sites where it's computed from `(dExpPepMass + dCushion) * dInverseBinWidth` (`CometPreprocess.cpp`'s `PreprocessSingleSpectrumCore()`, batch `Preprocess()`, and the fused preprocessing path) — this fixes both the `isEqual(dPeptideMassLow, 0.0)` bypass in the batch/fused mass-range gates *and* the RTS single-spectrum path, which has no mass-range gate of its own at all (confirmed by reading it) and was equally exposed to an out-of-configured-range caller-supplied precursor. |
| C4 | ✅ **FIXED** (2026-08-19) | `CometPreprocess.cpp:2053-2061` | Confirmed byte-identical. Fix applied: added the missing `if (!bUseThreadLocalPool)` guard around the first catch block's five `delete[]`s, matching its two sibling catches. |
| C5 | **partially ✅ FIXED (sizing bug only) 2026-08-19; MS2 NULL-deref left as-is by request** | `CometSearchManager.cpp:1609` (`g_vulSpecLibPrecursorIndex.resize(BINPREC(...) + 1)`); `CometSpecLib.cpp:935` etc. unchanged | **Jimmy's context (2026-08-19): MS2 spectral-library search isn't an implemented feature.** Investigated and confirmed: the batch `RunSpecLibSearch()`/`StoreSpecLib()` code is reachable (`DoSearch()` sets `g_bPerformSpecLibSearch` whenever `spectral_library_name` is set, no MS-level branch) and does NULL-deref on the first scored library entry (`_pSpecLibResults` confirmed never allocated anywhere) — real but unfinished scaffolding, not a regression; no `tests/unit/` coverage exercises `spectral_library_name` at all. The real, working spectral-library feature is MS1-only (`SearchMS1Library`/`RunMS1Search(QueryMS1*,...)`), reached only through the RTS/wrapper path — a `TODO(batch-MS1)` comment in `SearchUtils.cpp` confirms even that was never wired into batch `DoSearch()`. **Fix applied, sizing bug only (explicitly requested; the MS2 NULL-deref is untouched):** `g_vulSpecLibPrecursorIndex.resize(BINPREC(dPeptideMassHigh))` → `+ 1`, since `SetSpecLibPrecursorIndex()` clamps its fill range to `iMaxBin` inclusive and runs for *every* library entry during `LoadSpecLib()` regardless of MS level — the `std::out_of_range` this threw could hit even an MS1-only run, before any MS1/MS2 split. **Verified with a differential test**: crafted an `.msp` entry whose tolerance window reaches `dPeptideMassHigh`'s bin exactly; pre-fix this aborted immediately with `terminate called after throwing an instance of 'std::out_of_range' ... __n (which is 999) >= this->size() (which is 999)` (`LoadSpecLib()` runs before any spectra are even loaded, so the crash preempts the whole search); post-fix the same run completes cleanly through to "Search end". Full unit suite (42/42) also still passes. The unclamped *read* side and the MS2 `StoreSpecLib()` NULL-deref remain — both are scoped to the unfinished MS2 path and were left alone per this decision. |
| C6 | ✅ **FIXED** (2026-08-19) | `CometFragmentIndex.cpp:590-633` (reverse scan; unguarded read at `:626`, `k` decremented at `:632`) | Explicitly checked whether any carafe commit (`0de7c729`, "Fix AddFragments() early-exit break unsound against NL-shifted insertions") already fixes this — it doesn't; that commit changes an unrelated early-exit `break` condition, not the `k`-index guard. C6's bug predates all of `carafe` and is master's own. The "hardened pattern" comparison target moved to `CometPeptideIndex.cpp:634` (`MaterializeOneEntry()`'s forward-only guard) — note that function has no reverse scan, so it's an idiom reference, not a literal twin. Fix applied: changed `if (sPeptide[iPosReverse] == modSeq[k])` to `if (k >= 0 && sPeptide[iPosReverse] == modSeq[k])`. |
| C7 | ✅ **FIXED** (2026-08-19) | `CombinatoricsUtils.cpp:29-56` (`int` Pascal triangle), call site `CometModificationsPermuter.cpp:749` (`initBinomialCoefficients(peptideLengthRange.iEnd, MAX_K_VAL)`) | Confirmed byte-identical; `MAX_BITCOUNT`/default `peptide_length_range.iEnd` = 50 already on master. **Fix applied:** `BINOM_COEF`, `nChooseK()`, and `getCombinationCount()` now build/return `long long` instead of `int` (the Pascal's-triangle construction itself no longer overflows for any n up to 50/k up to 10), and the one exploitable call site (`CometModificationsPermuter.cpp`'s `generateModifications()`) clamps the `long long` result to a small `int` sentinel (`FRAGINDEX_MAX_COMBINATIONS + 1`) whenever it exceeds the cap, before assigning into the `int combinationCount` the existing `> FRAGINDEX_MAX_COMBINATIONS` guard checks — so a peptide with ≥44 modifiable residues of one type (with a user-configured `iMaxNumVarModAAPerMod` ≥10) now correctly gets skipped via that guard instead of the guard being bypassed by a wrapped-negative value that later crashed `new unsigned long long[negative]`. Confirmed `initCombinations()`'s own `nChooseK()` call (real-array-sizing use, not just a threshold check) stays safe unclamped: its `maxMods` is architecturally capped at `FRAGINDEX_MAX_MODS_PER_MOD = 5`, so `nChooseK(50, 5)` never approaches the overflow range in the first place — clamping there would have been actively wrong (under-allocating a real combinations array). |
| C8 | ✅ **FIXED** (2026-08-19) | clamp `CometSearchManager.cpp:1086-1087` (`max_precursor_charge` clamped to `MAX_PRECURSOR_CHARGE`, allowing charge 9); array dims (`CometData.h`), fill/read loops (`CometSearch.cpp`), RTS unclamped charge (`CometPreprocess.cpp:1591`) all unchanged files | Confirmed present. **Fix applied:** dimensioned every `uiBinnedPrecursorNL`/`uiBinnedPrecursorNLDecoy`-shaped array (member arrays in `CometSearch.h`, the pool-backed reinterpret-casts, the local stack array in `SearchFragmentIndex()`, and the `XcorrScore()`/`XcorrScoreI()` parameter/pointer types) at `MAX_PRECURSOR_CHARGE + 1` instead of `MAX_PRECURSOR_CHARGE`, matching the sibling `_uiBinnedIonMasses` arrays' existing `MAX_FRAGMENT_CHARGE + 1` convention exactly — charge 9 (which `max_precursor_charge`'s own clamp explicitly allows) is now a valid index instead of one past the end. Also removed the `MAX_PRECURSOR_CHARGE - 1` defensive clamps added while fixing B6/B13 (those existed specifically to avoid writing index 9 before this fix; they're unnecessary now and were artificially preventing charge-9 queries from ever getting `precursor_NL_ions` fill coverage). Separately clamped the RTS single-spectrum path's caller-supplied `iPrecursorCharge` to `[1, MAX_PRECURSOR_CHARGE]` before it's stored into `usiChargeState` (`CometPreprocess.cpp`'s `PreprocessSingleSpectrumCore()`), since that path — unlike the batch/fused charge-guessing loops — had no bound on it at all. Verified with a functional test: a charge-9 query with `precursor_NL_ions` enabled completes cleanly with a correct ID and plausible score. |
| C9 | ✅ **FIXED** (2026-08-19) | `search/Pipeline.cpp:287`, `output/MzIdentMlWriter.h:66,127` | Confirmed byte-identical. Fix applied: moved the writer-close loop before `_strategy->closeFiles(fpfasta, fpidx)` so `MzIdentMlWriter::close()`'s deferred merge no longer reads through an already-closed `FILE*`. |
| C10 | ✅ **FIXED** (4 of 5 sub-issues; 1 doesn't apply — see below), 2026-08-19 | `strcpy(szParamVal, pStr+1)` `Comet.cpp:547`; `mass_offsets` strtok-inside-if `Comet.cpp:494-513` (advance at `:508` only reached on `sscanf` success); `precursor_NL_ions` uninitialized `dMass` `Comet.cpp:515-532`; `strncpy(pInputFile->szFileName,...)` `Comet.cpp:754`; `-F`/`-L` stack-garbage `IntRange` `Comet.cpp:148-172`; `sprintf` chain into `szMod[512]` `CometSearchManager.cpp:1371-1462` | All sub-findings individually re-read and confirmed structurally identical, just shifted, **except the `-F`/`-L` one — see below.** **Fixes applied:** (1) `strcpy(szParamVal, pStr+1)` → bounded `strncpy` + explicit null-terminate (`szParamVal` is 512 bytes, the source line can be up to `SIZE_BUF`=8192); (2) `mass_offsets`' `tok = strtok(NULL, ...)` moved outside the `if (sscanf(...)==1)` so a non-numeric token can no longer stall the loop forever; (3) `precursor_NL_ions`'s `sscanf` return value is now checked before using `dMass`, matching `mass_offsets`; (4) `ParseCmdLine()`'s `strncpy(pInputFile->szFileName, cmd, i)` now clamps `i` to `SIZE_FILE-1` first (was bounded only by the raw argv's own length); (5) the `sprintf` chain into `g_staticParams.szMod` (8 call sites) replaced with a new bounded `AppendSzMod()` helper (`vsnprintf` into the buffer's actual remaining space) so a fully-populated 15-variable-mod + multi-static-mod config truncates safely instead of overflowing the fixed 512-byte global buffer. **The `-F`/`-L` `IntRange iScanRange` sub-item does not apply as described**: `IntRange` has a default constructor that zero-initializes both fields (`CometData.h`), so there is no "stack garbage" here — and `ProcessCmdLine()`'s two-pass design (options applied once before `LoadParameters()`, then reapplied after) means the post-`LoadParameters()` pass always re-derives the final `scan_range` from the by-then-correctly-loaded base regardless of what the pre-`LoadParameters()` pass temporarily set. No fix made; left as an open question for whoever wrote the original finding, in case a mechanism was intended that isn't visible from reading the code alone. Verified with a functional test: a malformed `mass_offsets = 10.0 garbageTOKEN 20.0` completes in 0.1s (no hang). |
| C11 | ✅ **FIXED** (2026-08-19) | `CometSearch.cpp:4157-4214`, `CometSearch.h:139,143` | Confirmed byte-identical. **Fix applied:** `CometSearch`'s constructor now zero-initializes `_proteinInfo.pszProteinSeq = nullptr` and `_proteinInfo.iAllocatedProtSeqLength = 0` (the `ProteinInfo` struct itself has no constructor), so `TranslateNA2AA()`'s `realloc(_proteinInfo.pszProteinSeq, ...)` always starts from a well-defined `nullptr` (equivalent to `malloc`) rather than whatever a freshly-`new`'d (and, for the one-`CometSearch`-per-protein nucleotide-search path, potentially recycled-heap-block) instance's memory happened to contain. The destructor now also `free(_proteinInfo.pszProteinSeq)`s it, closing the per-protein leak (`free(nullptr)` is a safe no-op for every non-nucleotide search, which never allocates this buffer at all). |
| C12 | ✅ **FIXED 2026-08-19** — all 8 applicable sub-items (9th, Carafe mask, N/A) | See the Section 2 write-up below for the full per-item detail, now marked FIXED | Audited 2026-08-19 (8 of 9 sub-items confirmed still open on master; the PR #125 `.idx` hardening only ever touched `VariableMod:`/`RequireVariableMod:` parsing, not `Enzyme:`/`Enzyme2:` or the count-`fread`/section-size/writer-error-checking issues), then fixed the same day at your "fix all" request. Verified with a clean `make` build and the full 42/42 unit suite. |
| C13 | **not applicable — already fixed on master** | `CometSearch/search/FiStrategy.cpp:169-188` | Surprising: master's `finalize()` already nulls `g_bIndexPrecursors`/`g_iFragmentIndex`/`g_iFragmentIndexOffset` and resets `g_bPlainPeptideIndexRead = false` after freeing them, with a comment explicitly citing the "second `DoSearch()` in the same process" scenario this finding warns about. This file is unchanged between master and carafe, so the original review's claim appears to have been already-stale even for the commit it was written against — worth a note back to whoever ran the original review, but no fix needed here. |
| Section 3: split init mutexes | applies unchanged (renumbered) | `CometSearchManager.cpp:2166/2173` and `:2421/2426` | Confirmed: two distinct `static std::mutex` locals, same shape. |
| Section 3: flags published before init completes | applies unchanged (renumbered) | `g_bPeptideIndexRead`/`g_bPlainPeptideIndexRead` set `CometPeptideIndex.cpp:291-292`, end of `ReadPeptideIndex()` (76-295); FI build happens after in the caller (`CometSearchManager.cpp:2268-2275`) | Confirmed. |
| Section 3: `_vFragmentPeptidesMutex` never locked | applies unchanged (renumbered) | Declared/init/destroy `CometFragmentIndex.cpp:41/181/256`; stale comment `:558`; unguarded `push_back` `:565` | Confirmed — no `LockMutex`/`UnlockMutex` call anywhere in the file. |
| Section 3: `ValidateSequenceDatabaseFile` suffix inverted | applies unchanged | `CometSearchManager.cpp:179` (unaffected — precedes all carafe insertions) | Confirmed present; still guards `g_bIdxNoFasta`, which the new `ExportPeptideIndexVariants()` call path (carafe-only) doesn't change the readership of. |
| Section 3: `strtok`/`localtime` non-reentrant | location update only | `ParsePeptideIndexHeader`'s `strtok` now at `CometPeptideIndex.cpp:1206,1212` | Low priority; renumbered only. |
| Section 3: 8-byte binary I/O assumes 64-bit | location stale, not re-verified | was `CometPeptideIndex.cpp:190,205,1064-1071` | File substantially changed (now uses explicit `uint64_t`/`clSizeCometFileOffset` in the areas spot-checked); low-priority `static_assert` suggestion still reasonable in principle but exact line refs need a fresh look before acting. |
| Section 3: other items (dead code, `RtsScratch` leak, `SetParam` leak, `CometWrapper` finalizer, `InitPrecomputedDecoyBins`) | applies unchanged | as originally written | All in unchanged files. |
| P1 | applies unchanged (renumbered) | `AddFragmentsThreadProc(bool, ThreadPool* /*tp*/)` `CometFragmentIndex.cpp:275-311` (param explicitly unused, param name itself is the tell); count call `:194`; fill loop `:244-249` | Confirmed still fully single-threaded. |
| P2 | applies unchanged (renumbered) | `sPeptide` copy `CometFragmentIndex.cpp:428`; `modSeq` copy `:433/447`; mass recompute `:469-492`; `fp.dPepMass` available but unused at call site `:247` | Confirmed. |
| P3, P4, P6, P7, P9, P10 | applies unchanged | as originally written | All in unchanged files (`CometSearch.cpp`, `CometPreprocess.cpp`/`.h`, `output/*`, `threading/*`, `CometWrapper/*`). |
| P5 | applies unchanged (renumbered) | RTS FASTA re-open per spectrum: `CometSearchManager.cpp:2728` (`fopen(...databaseInfo.szDatabase, "rb")`, `FASTA_DB` branch only) | Confirmed master already has `g_pvProteinNameCache` (declared line 70) serving FI_DB/PI_DB names with no per-spectrum I/O — the finding's ask (extend that cache to the RTS FASTA path) is still open, scoped down to just the FASTA_DB case. |
| P8 | applies unchanged (renumbered) | `CometSearchManager.cpp:1327` (`for (int ii=i+1; ii<VMODS-1; ...)`) | Confirmed. |
| P11 | **applies, modified** | `g_vFragmentPeptides` sort (no `.reserve()`) `CometFragmentIndex.cpp:222-227`; second sort moved to `CometPeptideIndex.cpp:745` inside `GenerateVariantArray()` | The `CometPeptideIndex.cpp` sort target changed: post-Phase-0.5 (`1fff1db2`), `g_vDBIndexVariants` regenerates every session rather than once at `.idx`-build time, and unlike `g_vFragmentPeptides` it **is** `.reserve()`d (line 706) before its sort — so the "no reserve" complaint applies only to the FI-build side now, not both sort sites as originally framed. |

**Net effect on the "Suggested fix order" section below:** every item listed
in step 1-3 there still applies to master with the line-number corrections
in the table above, **except C13, which should be dropped** (already fixed),
and **B5, which needs its fix description adjusted** to "also derive/reset
`bNtermMod`/`bCtermMod`/`bVarTermModSearch`/`bVarProteinNTermMod`/
`bVarProteinCTermMod` in the `VariableMod:` parse and reset loop" rather than
"derive... in the parse" generically — master's parse already restores
everything else correctly. C12's `.idx`-reader/writer sub-claims should be
re-audited fresh against master's current `CometPeptideIndex.cpp` before
inclusion in any fix batch, since part of that file's hardening already
landed independently.

**Fix status (updated 2026-08-19):** all seven "suggested fix order" step 1
one-liners have been implemented on `master` and are marked ✅ **FIXED** in
the table above: **B1, B2, B3, C4, C9, C1, C6**. Touched files:
`CometSearch.cpp`, `CometPreprocess.cpp`, `CometSearchManager.cpp`,
`CometFragmentIndex.cpp`, `search/Pipeline.cpp`. **B10** has also been fixed
(param-name plumbing, not a step-1 one-liner but small/self-contained):
`add_U_selenocysteine` is now the name `CometSearchManager.cpp` actually
reads (the legacy `add_U_user_amino_acid` read was removed, not kept as an
alias); the same wrong-name bug on the `O` writer path
(`add_O_ornithine` → `add_O_pyrrolysine`) and the `speclib_ms_level` /
`spectraL_library_ms_level` / `spectral_library_ms_level` three-way mismatch
(standardized on `spectral_library_ms_level`, the template's existing name)
were fixed in the same pass. Touched files for B10: `Comet.cpp`,
`CometSearchManager.cpp`, `CometWritePepXML.cpp`, `CometWriteMzIdentML.cpp`.
Verified with a full clean `make` build (zero new warnings/errors) and the
fast unit suite (`tests/unit/run_tests.py`, 42/42 passed) after each round of
changes. **B4** has also been fixed (`CometModificationsPermuter.cpp`):
replaced the O(n²) all-pairs bitmask-overlap loop with an O(n) running-union
pass that both fixes the assignment-not-accumulation bug and collapses the
loop, exactly as the review's suggested fix described. This one was
additionally confirmed with a targeted differential test (not just the
existing suite) — see the B4 row above for the before/after trace values.
**B6** has also been fixed (`CometSearch.cpp`) — the precursor-NL binned
array's zero/fill loops used a per-query charge bound instead of a
session-wide one, went untouched entirely in `CompoundModSearch()`, and were
never filled (only zeroed) in `SearchFragmentIndex()`; all four affected call
sites (plus a fifth, the `CalcVarModIons()` decoy branch, found while fixing
this and sharing the identical bug shape) are now fixed. See the B6 row above
and its Section 1 write-up for the full site-by-site detail and the
functional smoke test used to verify it. **B7 and B8** (both PEFF,
`CometSearch.cpp`) have also been fixed: B7 was a one-variable mass-compare
fix in `MergeVarMods()`'s walk-back; B8 was two sub-bugs in
`CheckDuplicate()` (a loop bound omitting the C-term mod slot, and a stray
OOB-capable write replaced with the comparison it should have been). Neither
has a dedicated PEFF fixture in the test suite to differentially verify at
runtime, so both rely on direct comparison against already-correct sibling
code in the same file (see their Section 1 entries) plus the full build/test
pass. **B9 was not fixed** — per a scope clarification that AScorePro was
only ever intended for `variable_mod01`-`05`, the original "shared alphabet
for all 15 slots" fix no longer fits; see the B9 row/write-up for the
narrower guard now proposed instead. **B11, B12, and all seven B13
sub-findings have also been fixed**: B11 (`Comet.cpp`) sentinel-initializes
the actual `enzymeInformation.*` struct members the missing-enzyme-definition
checks test, instead of unused local arrays of the same name — verified
`search_enzyme_number = 99` now errors and a valid number doesn't
false-positive. B12 (`CometFragmentIndex.cpp`) ORs `siVarModProteinFilter`
across a whole dedup run instead of keeping only the representative
occurrence's mask, in both the long and short peptide-length paths. B13's
seven independent scoring-consistency defects (`CometSearch.cpp` /
`CometPostAnalysis.cpp`) are each fixed at their own site — see the B13
write-up in Section 1 for the full list (missing FI `dMinimumXcorr` gate,
RTS/batch MS1 dot-product start-index mismatch, decoy sp_rank pool-size
mismatch, `LinearRegression` subset mismatch, `&&`/`||` enzyme-termini
mismatch, an undersized/uninitialized tie-break dummy array, and a
tie-break loop-bound mismatch). **C2, C3, C7, C8, C10 (4 of 5 sub-issues),
and C11 have also been fixed**: C2 pads the batch XCorr pool arrays by
`iXcorrProcessingOffset`, matching the RTS pool; C3 clamps `iArraySize` to
`iArraySizeGlobal` at all three computation sites, closing both the
`peptide_mass_low = 0.0` bypass and the RTS path's total lack of a
mass-range gate; C7 widens the binomial-coefficient machinery to `long long`
and clamps the one exploitable call site to a safe sentinel; C8 resizes
every precursor-NL array to `MAX_PRECURSOR_CHARGE + 1` (matching
`_uiBinnedIonMasses`'s existing `+1` convention) and clamps the RTS path's
unbounded input charge; C10 fixes 4 of its 5 sub-issues (bounded `strcpy`,
the `mass_offsets` hang, `precursor_NL_ions`'s uninitialized read, the
`ParseCmdLine` overflow, and the `szMod` `sprintf` chain) — the 5th
(`-F`/`-L` "stack garbage") turned out not to apply, since `IntRange` already
zero-initializes and the two-pass CLI design already reconciles correctly;
C11 zero-initializes and now frees `_proteinInfo.pszProteinSeq`. See each
finding's Section 1 entry for full detail and how each was verified (several
got a targeted functional test beyond the unit suite — charge-9 precursor,
malformed `mass_offsets`, etc.). **C5's sizing bug has also been fixed** on
your explicit direction to scope it to just that (the MS2 `StoreSpecLib()`
NULL-deref is unfinished-feature territory, not a regression, per your
context that MS2 spectral-library search isn't implemented) —
`g_vulSpecLibPrecursorIndex` now sized `+1`, verified with a differential
test showing the exact pre-fix `std::out_of_range` abort (during library
*loading*, before any spectra are read) versus a clean post-fix run.
**C12's entire corrupt/truncated-file robustness cluster has also been
fixed** (all 8 applicable sub-items; the 9th, the predicted-fragment mask,
was already N/A on master): `.idx` reader now checks every `fread` return
value and bounds `tNumRaw`/`tNumProteinEntries`/`tNumProteins` against a
sane entry-size floor before `reserve()`/`resize()`, adds a per-entry
`p + fieldSize > pEnd` bounds check in the raw-peptide parse loop, and the
footer monotonicity guard now requires a full 8-byte gap (not just ≥ 1)
so the section-size subtraction can no longer underflow
(`CometPeptideIndex.cpp`); `Enzyme:`/`Enzyme2:` sscanf formats are now
bounded (`%47s`/`%19s`, matching `VariableMod:`'s existing pattern); the
writer now checks `ferror(fptr)` before its final `fclose` and both known
failure paths (`fclose`+`remove()`) clean up the partial file instead of
leaking a handle/0-byte `.idx`. The PEFF header parser's three sub-bugs are
fixed: the closing-paren scan now also breaks on `'\0'`, the realloc guard
is `>=` instead of `>` at all three occurrences, and a failed `fgets` now
returns a proper error instead of silently underflowing `strlen()-1` on an
empty string (`CometSearch.cpp`). The MSP loader's three sub-bugs are
fixed: the `"Name:"`-line EOF loop now checks for `EOF` instead of spinning
forever, the bare-`"Name:"` OOB `sscanf` now checks `strlen() > 6` first,
and `SpecLibStruct`'s previously-uninitialized `fRTime`/
`fScaleMinInten`/`fScaleMaxInten`/`pfUnitVector`/`uiArraySizeMS1` are now
explicitly zeroed (`CometSpecLib.cpp`). `GetProteinNameString`'s two
`strlen()-1` underflow sites are now guarded with `strlen() > 0`
(`CometMassSpecUtils.cpp`). The mzIdentML merge path's `exit(1)` on
tmp-file-open failure is now `return false` (reported via
`g_cometStatus`/`logerr` instead of killing the host process), the entire
per-line field-parse loop is now wrapped in a `try/catch` that reports and
returns `false` on a malformed numeric field instead of letting
`stoi`/`stod`/`stof`/`.at()` throw uncaught, and the unconditional trailing
`</SpectrumIdentificationResult>` close tag is now skipped when `vMzid` is
empty (no matching open tag was ever written) (`CometWriteMzIdentML.cpp`).
Unescaped XML attribute values (file paths, instrument model/manufacturer
strings, database paths) are now run through the existing
`CometMassSpecUtils::EscapeString()` at every site the finding named, in
both `CometWritePepXML.cpp` and `CometWriteMzIdentML.cpp`. The `Query`
destructor's OOM null-deref is fixed by guarding each cleanup loop on the
owning pointer-array itself being non-null, not just `iFastXcorrDataSize`/
`iSpScoreData` (`core/Types.h`) — this covers all four allocation call
sites in `CometPreprocess.cpp` uniformly rather than requiring an
allocation-order fix at each one individually. The MS1 preprocessing
empty-scan underflow (`.at(iNumPeaks - 1)` on an empty spectrum) is now
guarded in both `PreprocessThreadProcMS1` (the batch/library-build path)
and `PreprocessMS1SingleSpectrumThreadLocal` (the live RTS single-spectrum
path — same bug shape, not originally cited by line number but fixed for
consistency since it's the production code path); the zero-magnitude NaN
division is guarded in `PreprocessThreadProcMS1` (`CometPreprocess.cpp`).
Verified with a full clean `make` build (zero new warnings) and the full
unit suite (42/42 passed). This first batch (B1-B4, B6-B8, B10-B13, C1-C12)
was committed to the `fablereview` branch and pushed
(`5cc3be9b`).

**Fix status (updated 2026-08-21):** **B5** has also been fixed
(`CometPeptideIndex.cpp`) — see its Section 1 entry above for the full
before/after differential-test detail. While auditing what else belongs in
the self-describing `.idx` header (the question that led to fixing B5),
two more gaps were found and fixed the same day, **B14** and **B15** (new
findings, not in the original 2026-08-19 review — see their Section 1
write-ups above for full detail): B14 persists `decoy_prefix`/
`num_enzyme_termini`/`allowed_missed_cleavage`/`clip_nterm_methionine` in
the header (`CometPeptideIndex.cpp`); B15 fixes a separate, more severe,
pre-existing bug found while verifying B14 — `AnalyzePeptideIndex()`'s
PI_DB decoy-by-protein-name classification read a map
(`g_pvProteinNames`) that's only ever populated at index *build* time and
is empty during search, so every PI_DB decoy was silently scored as a
target regardless of `decoy_prefix` correctness; fixed by reading
`g_pvProteinNameCache` instead (`CometSearch.cpp`, `core/Types.h`). B5/B14/
B15 were verified with full clean builds, the 42-test unit suite, targeted
differential tests, and (for B15) the full `--integration` suite against
real data (48/48, including T23's PI_DB target-decoy parity check at 1%
FDR against the `v2025.03.0` baseline). Committed to `fablereview` and
pushed (`194ef3e7`).

**B9 and all of Section 3 have also been fixed (2026-08-21)** — see B9's
and Section 3's own entries for full per-item detail. B9's narrower guard
(loud error if `print_ascorepro_score` is active alongside any
`variable_mod06`-`15`) is implemented and verified with a 4-case
differential test; the PEFF-placeholder collision remains open, unaffected
by this guard. All 10 Section 3 items are fixed: split init mutexes unified
under one lock; the premature-flag-publish race closed with a new
`g_bPeptideIndexFullyInitialized` flag; the `RtsScratch` fallback-allocation
leak and the batch NaN transient (the latter fixed by deleting two
dead-code functions that carried it); `SetParam`/`~CometSearchManager`'s
pointer-container leaks; `CometSearchManagerWrapper`'s missing C++/CLI
finalizer, verified with a full `MSBuild.exe Comet.sln` build (via WSL
interop) producing `MSToolkit.lib`/`CometSearch.lib`/`CometWrapper.dll`/
`RealtimeSearch.exe`/`Comet.exe` with zero errors — an initial attempt hit an
unrelated, separate build gap in `MSToolkit.vcxproj` (its `Reference`/
`HintPath` items for the two ThermoFisher assemblies weren't being translated
into `/FU` compiler switches by a command-line `MSBuild.exe` invocation, even
though both DLLs are present on disk at the exact expected path), fixed by
setting `ForcedUsingFiles` explicitly on `RAWReader.cpp`'s `ClCompile` item
(see that file's own comment for detail); the inverted
`ValidateSequenceDatabaseFile` suffix check; the
entire dead-code cluster (several functions/types deleted outright --
`CometCheckForUpdates`, `PreprocessSingleSpectrum`/`PreprocessMS1SingleSpectrum`,
`FragmentIndexReader`, `Threading::BeginThread`+`Semaphore` and friends,
`NormalizeDoubleToChar` -- plus `GetProteinSequence`, which turned out to be
live rather than dead, and `_vFragmentPeptidesMutex`, which now actually
locks); non-reentrant `strtok`/`localtime` (via `strtok_r`/a new
`comet_localtime()` wrapper); `InitPrecomputedDecoyBins`'s staleness (a
fingerprint check + `assert`); and the 8-byte-I/O 64-bit assumption (a
`static_assert`). Verified with a full clean build, the 42-test unit suite,
and a full `--integration` run (T17/T18/T19/T20/T22/T23/T24, 8/8 passed).
Not yet committed. Everything else in this triage (C5's MS2-NULL-deref
half, and the performance items) remains open.

---

## 1. Highest-priority bugs — wrong results in common configurations

### B1. Decoy fragment ladder aborts at the first NL-modified residue (critical)
`CometSearch/CometSearch.cpp:7535` (`CalcVarModIons`). A bare `break;` in the
decoy b-ion branch exits the entire decoy ladder loop (`for (i =
iDecoyStartPos; i < iDecoyEndPos; ++i)`, line 7510) the first time a decoy
residue carries a fragment-neutral-loss variable mod. `_pdAAforwardDecoy` /
`_pdAAreverseDecoy` stay unfilled for all remaining positions and retain stale
values from the previous peptide scored by that `CometSearch` instance; lines
7584/7657 bin those garbage masses into `_uiBinnedIonMassesDecoy`. Every
phospho + `decoy_search` run corrupts the decoy XCorr distribution — which
directly distorts FDR. `iPositionNLY` is also never set, so decoy y-NL ions
are dropped. The target ladder (7144-7227) and the decoy y-branch (7551-7564)
have no such break. Present since the original import — long-standing
upstream, but exactly the sibling shape of the recently fixed NL bugs.
**Fix:** delete the `break` (mirror the target branch).

### B2. `SearchFragmentIndex` NL running counts use stale `iStartPos` (high)
`CometSearch/CometSearch.cpp:1668` (guard `if (i > iStartPos)`; `iStartPos`
mutated at 1815-1823 based on the *previous* candidate's `cPrevAA`). The loop
runs in peptide-local coordinates (`i = 0..iLenMinus1`), so the guard must be
`i > 0`. For the 2nd and later of up to 100 FI candidates per query, when the
previous candidate had a non-`'-'` preceding residue, the cumulative-NL copy
at `i == 1` is skipped; if the current candidate's residue 0 carries the NL
mod, all b-series NL peaks are silently dropped (symmetrically for y). FI
phospho scores become candidate-order-dependent. The batch analog at 7152 is
correct only because its loop runs in protein coordinates.

### B3. Mods 10-15 escape `max_variable_mods_in_peptide` in FASTA search (high)
`CometSearch/CometSearch.cpp:5783`: `int iSum8 = i9 + i8;` — every other
nesting level accumulates `iSumN = iSum(N+1) + iN`. In the original 9-mod code
`i9` was the outermost loop so this was correct; commit `d36c8d79` (15-mod
extension) added the `i15..i10` outer loops and `iSum9 = iSum10 + i9` but did
not update this line. Combinations using `variable_mod10`-`15` alongside mods
1-8 exceed the cap unpruned and unrejected (no downstream total-cap recheck
exists) — illegal peptides are scored and reported, plus combinatorial
slowdown. Clear mechanical regression.

### B4. `combine()` total-mods check sees only the last two bitmasks — FI/PI over-enumeration (high)
`CometSearch/CometModificationsPermuter.cpp:428`: `combinedBitmasks =
bitmasks[j] | bitmasks[k];` inside the double loop is an assignment, not an
accumulation, so the popcount check at 440-443 tests only
`bitmasks[n-2]|bitmasks[n-1]`. Example: 3 mod types x 2 sites,
`max_variable_mods_in_peptide = 4` emits a 6-mod variant; with one mod type
the check sees 0 bits. No consumer of `MOD_NUMBERS` re-applies the cap
(verified: `CometFragmentIndex.cpp:397/477`, `CometPeptideIndex.cpp:413`).
Indexes are inflated and FI/PI PSMs can violate the configured cap,
inconsistent with FASTA behavior (which has the *different* B3 bug — the two
paths disagree). **Fix:** `combinedBitmasks |= bitmasks[j]` accumulated over
all masks (also collapses the O(n^2) pair loop to O(n)).

### B5. `.idx` header restore never sets terminal-mod flags — terminal variable mods silently dropped (high) — ✅ FIXED 2026-08-21
`CometSearch/CometPeptideIndex.cpp:1180-1193` (reset loop) and 1323-1374
(`VariableMod:` parse). The header parse declares itself authoritative for mod
config but `bNtermMod`/`bCtermMod`/`bVarTermModSearch` are only ever derived
from *search-time* params in `CometSearchManager.cpp:1383-1399`. Searching an
`.idx` built with `variable_mod01 = 42.010565 n ...` from a params file with
no variable mods (the exact use case docs/20260811_restore_idx_header_mods.md
advertises) leaves `bVarTermModSearch` false — every n/c-term-modified variant
is skipped by `AddFragmentsThreadProc()` and `EnumerateIndexPeptideMods()`
with no error. Conversely, stale flags from mismatched search-time params
create phantom terminal-mod variants with the wrong slot's mass. Also skews
`ComputeVarModConfigString()` (CometPredictedMask.cpp:175-177), so the Carafe
mask guard can spuriously reject or falsely accept (carafe-only; drops out
for master). **Fix:** derive all three flags from the header's `szVarModChar`
in the parse and reset them in the reset loop.

**Fix (2026-08-21):** the reset loop (`ParsePeptideIndexHeader()`, top of the
function) now zeroes `varModList[x].bNtermMod`/`bCtermMod` per slot and
`bVarTermModSearch` globally, alongside the fields it already reset. The
`VariableMod:` parse now checks each restored slot's `szVarModChar` for
`'n'`/`'c'` and sets `bNtermMod`/`bCtermMod`/`bVarTermModSearch` accordingly —
the identical derivation `CometSearchManager.cpp:1399-1411` already performs
from live params, just applied to the header-restored identity instead.
`bVarProteinNTermMod`/`bVarProteinCTermMod` were investigated separately and
deliberately **excluded** from the restore: a full-codebase grep found they
have **zero readers anywhere** (not written-then-read by anything, not even
FI_DB/PI_DB) — they're set in `CometSearchManager.cpp` and never consulted
again, so unlike `bNtermMod`/`bCtermMod`/`bVarTermModSearch` there is nothing
downstream for a header entry to feed. This is consistent with (and confirms)
the existing note above that `iWhichTerm`/`iVarModTermDistance` — the field
these two flags summarize — are unsupported by FI_DB/PI_DB.

Verified with a differential test since no existing fixture exercises this:
built an FI_DB `.idx` from the legacy `ctermmod` fixture (`epgc_9entry.fasta`,
a real c-term variable mod baked in at build time), then searched it with
`variable_mod01`/`02` left blank at search time. **Pre-fix** (temporarily
reverted): the FI build enumerated only 1281 total peptides (no c-term-mod
variants at all — never generated), and the top-ranked hit was a *different,
wrong* peptide (`YFDSFGDLSSASAIMGNPK`, unmodified C-term) at the same
coincidental mass. **Post-fix:** 2581 total peptides enumerated (including
the c-term variants), and the correct c-term-modified peptide
(`YFDSFGDLSSASAIMGNP` + `128.094963_c`) now ranks #1, matching the same
assertion the existing `t21_ctermmod` legacy test uses. Full clean `make`
build (zero new warnings) and the 42-test unit suite passed both before and
after.

### B6. Precursor-NL binned arrays: stale across queries; never filled in FI and CompoundModSearch (medium) — ✅ FIXED 2026-08-19
`CometSearch/CometSearch.cpp:3139-3192` / 7295-7308 / 7378-7393 fill
`_uiBinnedPrecursorNL` only for the first matching query's charge (gated on
`bFirstTimeThroughLoopForPeptide`), but `XcorrScore` (4449) reads up to *each*
query's own charge — higher-charge entries retain bins from a previous
peptide, adding spurious intensity. `CompoundModSearch` (8699-9019) never
fills the array at all, and `SearchFragmentIndex` zeroes but never fills it
(1742-1743), so `precursor_NL_ions` contributes nothing in FI_DB searches —
worth documenting if intentional. The PI path refills per candidate and is
fine.

**Fix (2026-08-19):** the root cause is a write/read bound mismatch — the
zero+fill loops ran `for (ctCharge = <the query that happened to trigger
bFirstTimeThroughLoopForPeptide>->charge; ...)` while every subsequent
matching query's `XcorrScore()` call reads up to *its own* charge, which can
be higher. Fixed by computing a session-wide bound once,
`iPrecursorNLMaxCharge = min(g_staticParams.options.iMaxPrecursorCharge,
MAX_PRECURSOR_CHARGE - 1)`, and using it as the loop bound everywhere the
array is populated (mirroring how the sibling `_uiBinnedIonMasses` fill
already loops `1..g_massRange.usiMaxFragmentCharge`, a global bound, rather
than a per-query one). The `-1` clamp deliberately avoids touching index
`MAX_PRECURSOR_CHARGE` (9) at all — that boundary is C8's separate,
still-open off-by-one; this fix doesn't widen C8's trigger window by writing
that index unconditionally.

Four call sites needed the fix, one of which (the `CalcVarModIons()` decoy
branch) the original review didn't cite but has the identical bug shape:
1. `SearchForPeptides()` target ladder (the review's `3139-3192`) — fixed.
2. `CalcVarModIons()` target ladder (the review's `7295-7308`/`7378-7393`) —
   fixed.
3. `CalcVarModIons()` **decoy** ladder — same `bFirstTimeThroughLoopForPeptide`
   caching as the target branch, same bug, not explicitly named in the
   original write-up. Fixed with its own locally-scoped
   `iPrecursorNLMaxChargeDecoy` (the target fix's variable is out of scope by
   the time the decoy block runs). Note: `SearchForPeptides()`'s *own* decoy
   branch does **not** have this bug — unlike `CalcVarModIons()`, it's not
   gated by `bFirstTimeThroughLoopForPeptide` at all and fully rebuilds its
   ladder (inefficiently — see P7) for every matching query, so it's always
   using the correct, current query's own charge already.
4. `CompoundModSearch()` — confirmed it also caches its ladder once per
   candidate (`bFirstTime`, mirroring the other two functions) and never
   touched `_uiBinnedPrecursorNL`/`_uiBinnedPrecursorNLDecoy` at all before
   calling the shared `XcorrScore()`, which unconditionally reads them
   whenever `precursor_NL_ions` is configured — a real read-garbage bug
   (whatever peptide, from *any* search path, last wrote those slots), not
   just a missing feature. Implemented the real fill (same formula, using
   this function's own `dModMass`), for both target and decoy, rather than
   just zeroing it out.
5. `SearchFragmentIndex()` (FI_DB): confirmed this function processes one
   query at a time with no cross-query caching, so no global-bound clamp is
   needed here — added a fill loop bound directly by `pQuery`'s own charge
   (matching `AnalyzePeptideIndex()`'s already-correct PI_DB pattern) right
   after the existing `memset`-to-zero, so `precursor_NL_ions` now actually
   contributes to FI_DB scoring instead of silently doing nothing.

Verified via a full clean `make` build, the 42-test unit suite, and a
functional smoke test: a plain-FASTA search with `precursor_NL_ions`
configured and two spectra (charge 2, charge 4) matching one candidate
peptide in a single batch — the exact multi-charge-per-peptide shape the bug
requires — produced correct IDs at both charges with no crash.

### B7. PEFF: `MergeVarMods` walk-back compares the wrong mass (medium) — ✅ FIXED 2026-08-19
`CometSearch/CometSearch.cpp:6953` — the binary search at 6949 uses
`dTmpCalcPepMass` (peptide + PEFF delta) but the seek-back loop compares
`dCalcPepMass` (without it). Negative-mass PEFF mods (amidation -0.98) stop
the walk-back early: queries between the true first match and the stop point
are never scored — missed IDs. Compare the correct pattern in
`WithinMassTolerancePeff` (3713).

**Fix (2026-08-19):** changed the seek-back comparison from `dCalcPepMass` to
`dTmpCalcPepMass` (master line 6983), so both the binary search and the
walk-back key on the same PEFF-adjusted mass, matching
`WithinMassTolerancePeff()`'s already-correct pattern (line 3743) exactly.
One-variable fix. Verified via full clean `make` build (no new warnings) and
the 42-test unit suite; no dedicated PEFF fixture exists in the current test
suite to differentially exercise this (PEFF requires a PEFF-annotated FASTA
+ OBO file), so this one relies on the direct side-by-side comparison against
the already-correct sibling function rather than a runtime differential test.

### B8. PEFF: `CheckDuplicate` target loop misses the C-term mod slot; decoy branch has a stray OOB write (medium) — ✅ FIXED 2026-08-19
Target: `CometSearch/CometSearch.cpp:5260` loops `ii <= usiLenPeptide` (omits
`len+1`, the C-term slot; decoy branch at 5141 is correct) — two IDs differing
only in C-term mod state are merged as duplicates. Decoy: 5167-5168 `else
pQuery->_pDecoys[i].pdVarModSites[i] = 0.0;` writes into a *stored* decoy
using the stored-entry index `i` as a position index (should be `ii`, and a
check function shouldn't write at all), and fails to test `piVarModSites[ii]
!= 0` so modified-vs-unmodified mismatches merge. With `num_results > 53`,
`pdVarModSites[i]` writes out of bounds past the array (`double[53]`).

**Fix (2026-08-19), both sub-bugs:**
1. Target loop (master line 5290) widened from `ii <= usiLenPeptide` to
   `ii < usiLenPeptide + 2`, matching the decoy branch's bound exactly (both
   now cover indices `0..len+1` inclusive, i.e. every residue plus both
   termini).
2. Decoy branch's stray write (master line 5198) replaced with the missing
   comparison: `if (piVarModSites[ii] != 0) { bIsDuplicate = 0; break; }`.
   Confirmed `pdVarModSites` (`CometSearch.h:392`'s `double[MAX_PEPTIDE_LEN_P2]`)
   is a real, actively-read array (output writers, E-value mass recompute) —
   not dead code — so the OOB-write risk was real, not just a logic bug.
   Mirrors how the sibling *target* branch a few lines below already folds
   the `iVal == 0` case correctly into its `>= 0` comparison; the decoy
   branch's separate `else` for `iVal == 0` was the one place that diverged.

Verified via full clean `make` build (no new warnings) and the 42-test unit
suite; same PEFF-fixture caveat as B7 applies — no dedicated PEFF test
exists to differentially exercise this at runtime, so verification relies on
direct code comparison against the already-correct sibling branches/functions
identified above.

### B9. AScorePro corrupts mod sites for `variable_mod10`-`15` and PEFF placeholders (high when AScorePro enabled) — guard ✅ FIXED 2026-08-21
Registration uses one char per slot: `pepMod.setSymbol(i + 1 + '0')`
(`CometSearchManager.cpp:3399`), but the sequence handed to AScorePro embeds
`std::to_string(piVarModSites[i])` (`CometPostAnalysis.cpp:920`) — `"10"` is
parsed by AScorePro as symbol `'1'` (variable_mod01's mass) plus dropped
`'0'`. On the rewrite path (score >= cutoff, 968-981) the site is silently
reassigned to slot 1 with slot 1's mass. Slot 15's symbol `'?'` also collides
with Comet's PEFF placeholder (924), and PEFF-modded sites are unconditionally
erased by the memset at 941-942. **Fix:** one shared single-char alphabet on
both sides (e.g. `'1'..'9','A'..'F'`), or restrict handoff to slots 1-9 with a
loud warning.

**Scope note from Jimmy (2026-08-19):** AScorePro was only ever intended to
be applied for `variable_mod01`-`variable_mod05` — the modifications limit
for FI. This reframes the finding: AScorePro + `variable_mod06`-`15` was
never a supported combination to begin with, so "corrupts mod sites for
`variable_mod10`-`15`" describes misuse of an unsupported config rather than
a regression in previously-working behavior. The `'?'`/PEFF-placeholder
collision (924, 941-942) is unaffected by this scoping and remains a real
concern on its own — **still open**, not addressed by this fix.

**Fix (2026-08-21):** implemented the narrower guard, not the original
"shared alphabet across all 15 slots" suggestion (which no longer fits given
the 1-5 scope): `InitializeStaticParams()` (`CometSearchManager.cpp`, right
after the existing per-slot variable-mod-derivation loop) now checks, whenever
`print_ascorepro_score != 0` (covers both a specific slot 1-5 and `-1` /
"localize all variable mods" — both are documented, valid values, and both
reach the same sequence-embedding code that breaks on a two-digit slot
number), whether any of `variable_mod06`-`15` (`varModList[5..14]`) is active.
If so, returns `false` with a clear error via `g_cometStatus`/`logerr` instead
of silently corrupting mod sites — mirrors the existing
`g_cometStatus.SetStatus`/`logerr`/`return false` pattern already used
throughout `InitializeStaticParams()`. Verified with a 4-case differential
test (slot-1-only + inactive mod06 → passes; slot-1 + active mod06 →
rejected; `-1` + active mod06 → rejected; disabled (`0`) + active mod06 →
passes, guard correctly inactive) plus a full clean build and the 42-test
unit suite.

### B10. Parameter names that silently do nothing
- `add_U_selenocysteine` (`Comet.cpp:475`, printed by `-p` at 1164) is never
  read — `InitializeStaticParams` reads only `add_U_user_amino_acid`
  (`CometSearchManager.cpp:815`), which the CLI never sets. U-containing
  peptides search unmodified with no warning. Relatedly, pepXML/mzIdentML
  headers query the wrong names `add_O_ornithine` / `add_U_user_amino_acid`
  (`CometWritePepXML.cpp:217,230`, `CometWriteMzIdentML.cpp:609,622`) so those
  static mods are omitted from output metadata even when applied.
- `speclib_ms_level` has a three-way name mismatch: template prints
  `spectral_library_ms_level` (`Comet.cpp:975`), parser registers
  `speclib_ms_level` (438), consumer reads `spectraL_library_ms_level`
  (`CometSearchManager.cpp:421`). `iSpecLibMSLevel` always keeps its default.

### B11. `search_enzyme_number` validation is dead code (high)
`Comet.cpp:683-685` initializes *local* `"-"` sentinel arrays, but the parse
loop writes directly into `enzymeInformation.*`, whose default-constructed
values are `""`/`"Cut_everywhere"` (`CometData.h:345-365`) — the `strcmp(...,
"-")` checks at 733-755 can never fire. `search_enzyme_number = 99` silently
runs with an empty break-AA enzyme (and `bNoEnzymeSelected=0`) instead of
erroring — empty/garbage results.

### B12. FI/PI dedup drops `siVarModProteinFilter` bits from non-representative proteins (medium)
`CometSearch/CometFragmentIndex.cpp:1004` (long path), 1076 (short path). With
`protein_modslist_file` active, a peptide shared between a listed and an
unlisted protein keeps only the representative occurrence's per-protein mask
(smallest file offset). If that is the unlisted protein, every mod variant of
the shared peptide is silently never enumerated, and results depend on FASTA
ordering. **Fix:** OR the mask across the dedup run.

### B13. Scoring-consistency defects (low-medium, worth a pass) — ✅ ALL 7 FIXED 2026-08-19
Seven independent sub-findings, each fixed individually. Verified together via
a full clean `make` build (no new warnings) and the 42-test unit suite; a
few also got a targeted functional check (noted per item below).

1. **`XcorrScoreI` FI branch stores zero/negative-XCorr candidates: no
   `dMinimumXcorr` gate** (`CometSearch.cpp:8042-8065`; PI branch at 8076 and
   FASTA at 4508 have it). **Fix:** added `dXcorr >= g_staticParams.options.dMinimumXcorr &&`
   to the FI_DB branch's condition (master line ~8124), matching the PI_DB
   branch two lines below it and the FASTA path. `StorePeptideI()` itself has
   no internal gate (unlike `StorePeptide()`, which double-checks), so this
   caller-side condition was the only place this could have been enforced.
2. **RTS vs batch MS1 scores diverge**: RTS dot product started at `j = 0`
   (`CometSearch.cpp:1102`) while batch starts at `BINPREC(dMS1MinMass)`
   (2588/2601). **Fix:** changed the RTS loop's start index from `0` to
   `BINPREC(g_staticParams.options.dMS1MinMass)`, matching `SearchMS1Library()`'s
   batch path exactly.
3. **`AnalyzeSP` ranks targets over `iNumStored` but decoys over
   `iNumPeptideOutputLines`** (`CometPostAnalysis.cpp:387-388` vs 463-464) —
   decoy sp_rank systematically compressed (bias for Percolator features).
   **Fix:** changed the decoy branch's cap from `iNumPeptideOutputLines` to
   `iNumStored`, matching the target branch.
4. **`LinearRegression` computes means and sums-of-squares over different
   point subsets** (`piHistogram[i] > 0` at 1243 vs `pdCumulative[i] > 0` at
   1261) — small silent E-value skew on sparse histograms. **Fix:** changed
   the sum-of-squares loop's condition from `pdCumulative[i] > 0` to
   `piHistogram[i] > 0`, matching the means loop. (`pdCumulative[i]` holds
   `log10` of the raw cumulative count by this point, which is `0` — not
   `>0` — whenever that count is exactly `1`, wrongly excluding otherwise-valid
   points from the sum-of-squares that the means calculation had included.)
5. **`CheckEnzymeStartTermini`/`CheckEnzymeEndTermini` gate on `&&` where
   `CheckEnzymeTermini` uses `||`** (`CometSearch.cpp:3839,3866` vs 3731);
   since enzyme2 defaults to Cut_everywhere they return true unconditionally,
   disabling PEFF variant flank-cleavage validation in single-enzyme
   searches. **Fix:** changed both `&&` to `||`. Confirmed both functions'
   only two call sites (master lines ~2991, ~3033) are inside PEFF-variant
   handling specifically, so the blast radius is scoped to PEFF + variants,
   not general search.
6. **Tie-break nondeterminism**: `StorePeptide`'s incoming-preference loops
   read `piVarModSites[0..len+1]` even when the caller passed the 4-element
   dummy from the unmodified path (`CometSearch.cpp:2650`; reads 4657-4668,
   4922-4933) — stack OOB read decides ties by garbage. **Fix:** resized the
   dummy from `int piVarModSites[4]` to `int piVarModSites[MAX_PEPTIDE_LEN_P2] = {0}`
   at its one declaration site (`SearchForPeptides()`, master line ~2673),
   matching every other real `piVarModSites` declaration's size elsewhere in
   the file and zero-initializing it so the tie-break loop reads well-defined
   "no mod here" values instead of uninitialized stack memory for any peptide
   longer than 2 residues.
7. **`SortFnSp` tie-break bound uses `peptideLengthRange.iEnd` while
   `SortFnXcorr`/`SortFnMod` use `usiLenPeptide + 2`**
   (`CometPostAnalysis.cpp:1050`) — max-length terminal-mod ties order
   nondeterministically. **Fix:** changed `SortFnSp`'s loop bound from
   `g_staticParams.options.peptideLengthRange.iEnd` (the search's configured
   max length, unrelated to this specific peptide) to `a.usiLenPeptide + 2`
   (this peptide's own length), matching its two sibling sort functions.

### B14. `.idx` header never persisted `decoy_prefix`/enzyme-digestion params — high, PI_DB decoy split at risk — ✅ FIXED 2026-08-21 (found 2026-08-21, not in the original 2026-08-19 review)
Follow-up question asked after B5 landed: "what parameters are stored/read
from the `.idx` header, and is anything missing?" Auditing every header line
`WritePeptideIndex()`/`ParsePeptideIndexHeader()` round-trip
(`CometSearch/CometPeptideIndex.cpp`) found two gaps, one severe:

- **`decoy_prefix` was never in the header at all**, unlike every other
  build-time-baked identity (`Enzyme:`, `StaticMod:`, `VariableMod:`).
  `AnalyzePeptideIndex()` (PI_DB) classifies a candidate as target vs. decoy
  purely by matching its *stored* protein name against
  `g_staticParams.szDecoyPrefix` — read live from `comet.params`/`SetParam()`
  at search time, defaulting to `"DECOY_"` if unset. A PI_DB target-decoy
  `.idx` built from a FASTA using a different convention (e.g. `rev_`), then
  searched later without re-supplying the matching `decoy_prefix`, silently
  misclassifies every real decoy as a target — corrupting the whole
  target/decoy split with no error, directly distorting FDR. Same bug shape
  as B5, landing on FDR correctness instead of missing mods.
- **`num_enzyme_termini`/`allowed_missed_cleavage`/`clip_nterm_methionine`**
  are also absent. Lower severity: they only affect digestion, already baked
  into the raw-peptide table at build time, so the actual peptide IDs are
  unaffected — but the pepXML/mzIdentML writers read them **live** for
  `search_summary`/`SpectrumIdentificationProtocol` enzyme metadata
  (`CometWritePepXML.cpp:203-207`, `CometWriteMzIdentML.cpp:1011-1066`), so a
  semi-tryptic `.idx` searched later with default `num_enzyme_termini=2`
  reports the wrong enzyme specificity in the output XML.

(`iWhichTerm`/`iVarModTermDistance` remain the one deliberate, documented
non-gap — confirmed zero references in FI_DB/PI_DB, per B5 above.)

**Fix (2026-08-21):** added `DecoyPrefix:`, `NumEnzymeTermini:`,
`AllowedMissedCleavage:`, and `ClipNtermMethionine:` lines to the v4 header,
written in `WritePeptideIndex()` and restored in `ParsePeptideIndexHeader()`,
overriding live params — same precedent as every other header field. All
four are optional on read (no `bFound.../error-if-missing` gate, matching
`DecoySearch:`/`Enzyme:`/`Enzyme2:`'s existing precedent), so `.idx` files
built before this change still load, just without this restore. Verified via
debug instrumentation that `g_staticParams.szDecoyPrefix` is correctly
restored at the exact point `AnalyzePeptideIndex()` uses it, and via a full
clean build + 42/42 unit suite (both before and after, confirming backward
compatibility with pre-existing `.idx` fixtures lacking these lines).

### B15. PI_DB decoy-by-protein-name check reads the wrong map — every PI_DB decoy silently misclassified as a target (critical) — ✅ FIXED 2026-08-21 (found 2026-08-21 while verifying B14, not in the original 2026-08-19 review)
`CometSearch.cpp:2148-2166` (`AnalyzePeptideIndex()`). The decoy-classification
lookup (`bDecoyPep`, described in B14) read `g_pvProteinNames`
(`map<long long, IndexProteinStruct>`) — but that map is populated **only**
while *building* a `.idx` (the digestion path in `CometSearch.cpp:942` and
`WritePeptideIndex()`'s own lookup while writing). Reading an existing `.idx`
back for a search (`ReadPeptideIndex()`) never touches `g_pvProteinNames` at
all — it populates a completely different structure,
`g_pvProteinNameCache` (`unordered_map<comet_fileoffset_t, string>`,
`CometPeptideIndex.cpp:298-317`), for exactly this kind of protein-name
lookup. Confirmed via debug instrumentation: `g_pvProteinNames.size() == 0`
at the point `AnalyzePeptideIndex()` runs during a normal batch/RTS search of
an existing `.idx`. Net effect: `bDecoyPep` was **unconditionally false** for
every PI_DB search of an already-built index with `decoy_search` enabled,
regardless of whether `decoy_prefix` matched (i.e. B14's fix alone would have
had no visible effect without this) — every real decoy candidate from a
target-decoy PI_DB `.idx` silently scored and stored as a target, corrupting
the target/decoy split (and therefore FDR) for that entire search mode, with
no error or warning. Present since PI_DB's decoy-by-name mechanism was
written; independent of and pre-dating B14/decoy_prefix persistence.

**Fix (2026-08-21):** changed the lookup from `g_pvProteinNames.find(...)` to
`g_pvProteinNameCache.find(...)` (and `.szProt` field access to `.c_str()`,
since the cache's value is a plain `string`), matching the identical pattern
`CometSearchManager.cpp:2846` already uses for search-time protein-name
resolution. Added the missing `extern` declaration for `g_pvProteinNameCache`
(`core/Types.h`) so `CometSearch.cpp` can see it — it was previously visible
only within `CometSearchManager.cpp`/`CometPeptideIndex.cpp`. Verified with a
targeted differential test: a 2-protein FASTA (one decoy-prefixed), PI_DB
with `decoy_search=2` (separate target/decoy output files) — pre-fix, the
real decoy candidate (a clean, high-scoring match) landed in the *target*
output file; post-fix it correctly lands in the *decoy* output file and the
target file is empty. Additionally verified against real data: the full
`--integration` suite, including T23's PI_DB target-decoy-vs-internal-decoy
parity check at 1% FDR (~17,660 PSMs) against a 177MB real mzXML and the
`v2025.03.0` baseline, passed both before this fix (the mechanism was already
non-functional, so B15 doesn't regress T23's prior pass) and after (still
agrees with baseline within tolerance) — 48/48 tests total.

---

## 2. Crashes and memory-safety bugs

### C1. `g_bIndexPrecursors` off-by-one heap write/read **[x3]** (high) — ✅ FIXED 2026-08-19 (found already applied 2026-08-21; doc status was stale)
Allocation `malloc(BIN(dPeptideMassHigh) * sizeof(bool))`
(`CometSearchManager.cpp:1569`) gives valid indices `0..BIN(high)-1`, but
`ReadPrecursors` clamps `iEnd` to `iMaxBin = BIN(dPeptideMassHigh)`
*inclusive* and writes `g_bIndexPrecursors[x]` for `x <= iEnd`
(`CometPreprocess.cpp:381, 656-660` and every isotope-offset block:
668-675, 709-716, 741-775) — a 1-byte heap OOB **write** whenever a
precursor+tolerance window reaches the top of the mass range (routine in real
FI_DB runs with `fragindex_skipreadprecursors = 0`). The read side
(`CometFragmentIndex.cpp:574`) has the matching one-past read, whose garbage
byte nondeterministically includes/excludes top-bin peptides from the index.
Found independently by three reviews. **Fix:** allocate `BIN(high) + 1`
(matching the `uiMaxFragmentArrayIndex = BIN(...)+1` pattern at
`CometSearchManager.cpp:1510`) and initialize the extra slot.

**Status (confirmed 2026-08-21):** already fixed, in the same `5cc3be9b`
commit as B1-B13/C2/C3/C7/C8/C10-C12 — this item's header was simply never
updated to say so. Current code (`CometSearchManager.cpp:1630-1638`):
allocation is `(BIN(dPeptideMassHigh) + 1) * sizeof(bool)`, the error-path
`printf` and the initialization loop (`x <= BIN(...)`) both cover the extra
slot too. Covered going forward by the existing FI_DB-path integration tests
(T22-T24); no new fixture added for this pass.

### C2. Batch XCorr loop reads up to 75 doubles past the pooled arrays (high) — ✅ FIXED 2026-08-19
`CometPreprocess.cpp:1228-1237` reads `pdTmpCorrelationData[i]` up to
`iArraySize + iXcorrProcessingOffset - 1` (offset default 75), but the batch
pool arrays are allocated exactly `iArraySizeGlobal` with no pad
(`AllocateMemory()` at 3043/3063/3083). With default params (`dMS1MaxMass
3000 < dPeptideMassHigh 5000`, `dMS1BinSize == dFragmentBinSize`),
`iArraySize == iArraySizeGlobal` for a precursor at `peptide_mass_high`, so
any MH+ within ~75 bins of the top reads OOB heap; the garbage feeds
`dSum`/`dMinXcorrInten`, nondeterministically perturbing the last bins of the
XCorr background. The RTS `RtsScratch` pool allocates the same buffers with
`+ iXcorrPad` explicitly (117-120) — the batch pool was never given the pad.
**Fix:** allocate `iArraySizeGlobal + iXcorrProcessingOffset` doubles.

### C3. `peptide_mass_low = 0.0` disables the high-mass gate — heap overflow in `LoadIons()` (high, config-gated) — ✅ FIXED 2026-08-19
Gate at `CometPreprocess.cpp:2576-2580` (batch) and 3482-3486 (fused):
`isEqual(dPeptideMassLow, 0.0) || (in range)` skips the `<= high` check
entirely, so a 6,000 Da precursor yields `iArraySize > iArraySizeGlobal` and
`LoadIons()` (2865-2866) writes ~1,000 doubles past the pool allocation
(sliding-window and SpScore loops likewise). The public RTS core
`PreprocessSingleSpectrumCore` has no clamp of its own (only the shipped
entry gates at `CometSearchManager.cpp:2551`). **Fix:** clamp `iArraySize` to
`iArraySizeGlobal` where computed (1644, 2606, 3508).

### C4. RTS OOM handler frees the thread-local pool — use-after-free + double free **[x2]** (high impact, OOM-only trigger) — ✅ FIXED 2026-08-19 (found already applied 2026-08-21; doc status was stale)
`CometPreprocess.cpp:2053-2061`: the first of three `bad_alloc` catches in
`PreprocessSingleSpectrumCore` unconditionally `delete[]`s the five buffers
that, on the RTS path, alias `g_rtsScratch` members. `iAllocSize` is
unchanged, so `EnsureInitialized()` won't re-allocate: the next spectrum on
that thread writes freed memory, and thread exit double-frees. The two
sibling catches (2088-2095, 2125-2132) have the correct
`if (!bUseThreadLocalPool)` guard — this one is missing it. One-line fix.

**Status (confirmed 2026-08-21):** already fixed, in the same `5cc3be9b`
commit — the missing guard was added, so all three `bad_alloc` catches in
`PreprocessSingleSpectrumCore` (now at `CometPreprocess.cpp:2117, 2153, 2190`
after the later Section-3 dead-code deletions shifted line numbers) are
consistently gated on `if (!bUseThreadLocalPool)`. The 2026-08-21 Section 3
pass separately found and fixed a related-but-distinct leak in the same
struct — `AllocSparseChild()`'s rare pool-exhausted fallback block was never
freed (`vFallbackSparseChildren`, committed `31b3575a`) — which is a
different bug from this one, not a duplicate. No new fixture added for this
pass; an OOM-triggered path is impractical to exercise in the unit suite.

### C5. Spectral-library MS2 search segfaults on its first stored hit (critical for that path) — re-scoped 2026-08-19; sizing bug ✅ FIXED, MS2 NULL-deref left as-is by request
`CometSpecLib.cpp:935` reads `pQuery->_pSpecLibResults[0].fSpecLibScore`, but
the only assignment to `_pSpecLibResults` anywhere is `= NULL` in the `Query`
constructor (`core/Types.h:872`). Path is reachable: `speclib` param + `.msp`
sets `g_bPerformSpecLibSearch` (`CometSearchManager.cpp:2026`) →
`RunSpecLibSearch` → `StoreSpecLib` (`CometSearch.cpp:1021-1024`);
`fLowestSpecLibScore` starts at -999.9, so essentially the first scored
library entry dereferences NULL. The batch MS2 speclib path has apparently
never run end-to-end. Companion bugs: the speclib precursor index is sized
`BINPREC(dPeptideMassHigh)` (`CometSearchManager.cpp:1588`) but written
through index `iMaxBin` inclusive (`CometSpecLib.cpp:734, 785-789` —
`std::out_of_range` via `.at()`) and read unclamped
(`CometSearch.cpp:1011-1015` — UB), same off-by-one family as C1.

**Re-scoped (2026-08-19):** Jimmy noted MS2 spectral-library search isn't an
implemented feature. A background investigation confirmed and refined this:

- **The NULL-deref is real and reachable, but describes unfinished
  scaffolding, not a regression.** `DoSearch()` sets `g_bPerformSpecLibSearch`
  whenever `spectral_library_name` is set — there's no MS-level branch at
  all — and this unconditionally leads to `RunSpecLibSearch()` →
  `StoreSpecLib()`, whose first line dereferences `_pSpecLibResults[0]`,
  confirmed never allocated anywhere (`new SpecLibResults`: zero hits in the
  whole tree). No test in `tests/unit/` exercises `spectral_library_name` at
  all, consistent with this having never been finished or run end-to-end.
  The real, working spectral-library feature is MS1-only
  (`SearchMS1Library`/`RunMS1Search(QueryMS1*,...)`), reached exclusively via
  the RTS/wrapper path — a `TODO(batch-MS1)` comment in `SearchUtils.cpp`
  confirms even that working MS1 feature was never wired into batch
  `DoSearch()`, so there's no working batch alternative to confuse this with.
- **The precursor-index sizing bug is separate and still matters regardless
  of the MS2 question.** `SetSpecLibPrecursorIndex()` runs unconditionally
  for *every* library entry during `LoadSpecLib()`, independent of MS level
  — so the `std::out_of_range` write-side half of this bug can throw during
  library *loading* even for someone using the real, working MS1 feature,
  before any MS1/MS2 split ever happens. Only the unclamped *read* side is
  MS2-only (inside `RunSpecLibSearch`), so only that half is scoped to the
  unfinished path.

**Fix applied (2026-08-19), sizing bug only — explicitly scoped this way by
request:** `g_vulSpecLibPrecursorIndex.resize(BINPREC(dPeptideMassHigh))` →
`resize(BINPREC(dPeptideMassHigh) + 1)` (`CometSearchManager.cpp:1609`),
matching the `+1` convention used elsewhere (e.g.
`uiMaxFragmentArrayIndex = BIN(dFragIndexMaxMass) + 1`). The MS2
`StoreSpecLib()` NULL-deref and the unclamped read side are both left
untouched, since they're scoped to the unfinished MS2 path.

**Verified with a differential test**, since no existing unit test exercises
`spectral_library_name`: crafted a minimal `.msp` entry whose precursor
tolerance window reaches exactly `dPeptideMassHigh`'s bin. Pre-fix, running
a search with this library aborted immediately —
`terminate called after throwing an instance of 'std::out_of_range' ...
vector::_M_range_check: __n (which is 999) >= this->size() (which is 999)`
— before any spectra were even loaded (`LoadSpecLib()` runs near the top of
`DoSearch()`, well before the "Load spectra" step), confirming the crash
preempts the entire search, not just the MS2 scoring path. Post-fix, the
identical run completes cleanly through "Search end" with no crash. Full
unit suite (42/42) also still passes.

### C6. `AddFragments()` reverse mod scan runs `k` to -1 — OOB reads in the FI-build hot loop (high) — ✅ FIXED 2026-08-19 (found already applied 2026-08-21; doc status was stale)
`CometFragmentIndex.cpp:731-748`: the reverse scan visits positions
`iEndPos..1`; once every `modSeq` entry is consumed, `k` hits -1 and each
remaining iteration evaluates `modSeq[(size_t)-1]` (UB read). Fires on
essentially every modified peptide whose first modifiable residue is at index
>= 2. Usually benign under SSO, but for heap-allocated `modSeq` (>15
modifiable residues) a chance byte match then reads `mods[-1]` and can add a
wrong mod mass to the y-ion ladder. Direct sibling of the fixed T25 sentinel
bug. **Fix:** `if (k >= 0 && ...)` (mirroring `MaterializeOneEntry()`'s
guard at `CometPeptideIndex.cpp:650`).

**Status (confirmed 2026-08-21):** already fixed, in the same `5cc3be9b`
commit. Current code (`CometFragmentIndex.cpp`, `AddFragments()` reverse
scan): `if (k >= 0 && sPeptide[iPosReverse] == modSeq[k])` — guard present.
No new fixture added for this pass.

### C7. Binomial-coefficient int overflow at default peptide length (high, opened by MAX_BITCOUNT 24→50) — ✅ FIXED 2026-08-19
`CombinatoricsUtils.cpp:52`: Pascal's triangle in `int` with
`initBinomialCoefficients(50, 10)` — C(44,10) = 2,481,256,778 > INT_MAX; rows
44-50 overflow (UB, wraps negative) on every FI/PI build at default
`peptide_length_range`. A peptide with >= 44 modifiable residues of one type
makes `getCombinationCount` return negative, which *passes* the
`> FRAGINDEX_MAX_COMBINATIONS` guards
(`CometModificationsPermuter.cpp:519-525`) and reaches
`new unsigned long long[negative]` (557) → `std::bad_array_new_length`. The
old `MAX_BITCOUNT=24` cap made this unreachable. `nChooseK` (196-204) also
overflows its intermediate for `k > 10`. **Fix:** build in `int64_t` and
clamp above `FRAGINDEX_MAX_COMBINATIONS` to a sentinel so the guards fire.

### C8. Precursor-NL arrays indexed one past `MAX_PRECURSOR_CHARGE` (high, config-gated; RTS unclamped) — ✅ FIXED 2026-08-19
`uiBinnedPrecursorNL` is `[MAX_PRECURSOR_NL_SIZE][MAX_PRECURSOR_CHARGE]` =
`[5][9]` (`CometData.h:30-31`), but every fill/read loop runs `for (ctCharge =
usiChargeState; ctCharge >= 1; --ctCharge)` and indexes `[ctNL][ctCharge]` —
index 9 at charge 9, which the param clamp explicitly allows
(`CometSearchManager.cpp:1089-1090`). Member-array writes corrupt the
adjacent decoy array; pool-backed writes (`CometSearch.cpp:2251-2263`,
2318-2330, 2536-2548) are a heap overflow. Worse: the RTS single-spectrum
path stores the caller's charge with **no clamp at all**
(`CometPreprocess.cpp:1591`), so a C# caller passing charge >= 10 drives
arbitrary OOB writes in `AnalyzePeptideIndex` and OOB reads in `XcorrScoreI`.
**Fix:** dimension `MAX_PRECURSOR_CHARGE + 1` or clamp the loops, and clamp
the RTS input charge.

### C9. Pipeline regression: db `FILE*` closed before the mzIdentML merge reads it (high) — ✅ FIXED 2026-08-19 (found already applied 2026-08-21; doc status was stale)
`search/Pipeline.cpp:287` calls `_strategy->closeFiles(fpfasta, fpidx)`
*before* the writer-close loop at 290-294; `MzIdentMlWriter::close()`
(`output/MzIdentMlWriter.h:66,127`) then runs the deferred merge on `_fpdb` —
an alias of the just-closed handle — which `ParseTmpFile` unconditionally
seeks/reads (`CometWriteMzIdentML.cpp:315,342,458,504,1303` →
`CometMassSpecUtils::GetProteinName`). Every `output_mzidentmlfile = 1` run
does stdio on a freed `FILE*` — crash or garbage protein names. Regression
from the legacy ordering (merge ran before `fclose` in
`45cfa421~1:CometSearchManager.cpp`). **Fix:** close writers before
`closeFiles` (one-line move).

**Status (confirmed 2026-08-21):** already fixed, in the same `5cc3be9b`
commit. Current code (`search/Pipeline.cpp:290-296`): the writer-close loop
now runs before `_strategy->closeFiles(fpfasta, fpidx)`, with a comment
explaining why the order matters. No new fixture added for this pass.

### C10. Param-file parsing: stack smash and hang — ✅ FIXED 2026-08-19 (4 of 5 sub-issues; see Section 0/table for the one that doesn't apply)
- `Comet.cpp:581`: `strcpy(szParamVal, pStr + 1)` copies up to ~8 KB of
  params line into `szParamVal[512]` — stack buffer overflow on any long
  value (a `database_name` path alone can exceed it; paths are `SIZE_FILE` =
  4096 elsewhere).
- `Comet.cpp:536-545`: in `mass_offsets`, `strtok(NULL, ...)` is inside
  `if (sscanf(tok, "%lf", ...) == 1)` — a non-numeric token never advances
  `tok`: infinite loop at 100% CPU at startup. The sibling
  `precursor_NL_ions` handler (557-564) advances correctly but uses `dMass`
  uninitialized when `sscanf` fails.
- `Comet.cpp:788` (`ParseCmdLine`): `strncpy(pInputFile->szFileName, cmd, i)`
  bounded only by `strlen(argv[k])` — argv can exceed `SIZE_FILE`.
- `Comet.cpp:178-196`: `-F`/`-L` uses `IntRange iScanRange` contents when
  `GetParamValue` fails (first pass runs before `LoadParameters`) — stack
  garbage can survive into the scan filter.
- `CometSearchManager.cpp:1374-1476`: unbounded `sprintf` chain into
  `szMod[512]` can overflow with a fully-populated 15-varmod + many-static-mod
  config.

### C11. Nucleotide search: `realloc` on an uninitialized pointer + leak (medium) — ✅ FIXED 2026-08-19
`CometSearch/CometSearch.cpp:4157-4214` (`TranslateNA2AA`);
`_proteinInfo.pszProteinSeq` / `iAllocatedProtSeqLength` (`CometSearch.h:139,143`)
are never initialized by any constructor. `SearchThreadProc` heap-allocates a
fresh `CometSearch` per protein (1266); on recycled heap blocks the members
are garbage → `realloc(garbage)` is UB. Works today only because fresh pages
are zeroed. The buffer is also never freed (once per protein per NA search).

### C12. Corrupt/truncated-file robustness cluster (medium, several sites) — audited 2026-08-19, FIXED 2026-08-19
Nine sub-items in the original write-up. Full re-audit against master's
current code, item by item (line numbers below are master's real current
ones as of the audit, not the stale `carafe`-branch numbers this finding
originally cited). All 8 applicable sub-items were fixed in a single pass
after the audit, at your "fix all" request; each is now marked ✅ FIXED
below with what changed.

1. **`.idx` reader: unchecked/unbounded counts — ✅ FIXED.**
   `tNumRaw` (`CometPeptideIndex.cpp:142-146`): `fread` return value
   discarded (`(void)fread`), then used unbounded in
   `g_vRawPeptides.reserve((size_t)tNumRaw)` *before* the section-size-
   validated buffered read at line 150 — a corrupt/huge `tNumRaw` throws an
   uncaught `std::length_error`/`bad_alloc` straight out of `reserve()`. The
   raw-peptide parse loop (`:156-175`) then walks a `const char* p` through
   `vBuf` for `tNumRaw` iterations with no bound check of `p` against
   `vBuf.data()+tSectionSize` — a genuine heap-buffer-overread if `tNumRaw`
   doesn't match what the section actually holds (the `iLen` sanity check at
   `:162` only bounds one field, not the running pointer). `tNumProteinEntries`
   (`:180-181`) and per-loop `tNumProteins` (`:195-200`) have the identical
   unchecked-`fread`-into-unbounded-`reserve`/`resize` shape. **Section-size
   underflow is real**: `:148`'s
   `tSectionSize = (size_t)(clProteinsFilePos - clPeptidesFilePos) - sizeof(uint64_t)`
   only has a monotonicity guard at `:125-137` (gap ≥ 1, not gap ≥ 8) — a
   1-7 byte footer gap underflows this `size_t` subtraction to near-`SIZE_MAX`,
   directly feeding `vector<char> vBuf(tSectionSize)` at `:149` — this
   contradicts a comment at `:116-124` in the same function claiming later
   subtraction is "provably safe." None of `ReadPeptideIndex()`'s callers
   (`CometSearchManager.cpp:2296`/`:2363`) wrap it in `try/catch`, so any of
   these exceptions crash the process uncaught.
   **Fix:** `fread` return values for `tNumRaw`, `tNumProteinEntries`,
   per-loop `tNumProteins`, and the bulk `vFlatProteinOffsets` read are now
   all checked; `tNumRaw` and `tNumProteinEntries` are bounds-checked
   against a minimum-entry-size floor before `reserve()`; the raw-peptide
   parse loop now bounds every field read against a new `pEnd` pointer
   (`p + fieldSize > pEnd`); the footer monotonicity guard now requires
   `clPeptidesFilePos + sizeof(uint64_t) <= clProteinsFilePos` (a full
   8-byte gap) instead of just `>= `, closing the section-size underflow.
2. **`Enzyme:`/`Enzyme2:` header parse — ✅ FIXED.**
   `ParsePeptideIndexHeader():1368,1376`:
   `sscanf(szBuf, "Enzyme: %s [%d %s %s]", szSearchEnzymeName, ...)` — plain
   `%s`, unlike `VariableMod:`'s already-hardened `%31s` (`:1257`). Buffers
   are `char[ENZYME_NAME_LEN]` (48) and `char[MAX_ENZYME_AA]` (20,
   `CometData.h:331-338`) — a global buffer overflow from an oversized
   file-supplied token. Fix shape: `%47s [%d %19s %19s]`, mirroring the
   `VariableMod:` pattern exactly.
   **Fix:** both sscanf format strings changed to `%47s [%d %19s %19s]`,
   exactly as proposed.
3. **`.idx` writer: no error checking — ✅ FIXED.** Every
   `fwrite` in `WritePeptideIndex()` (`:955,964,968-974,982,987,1009,1021-1022`)
   is fire-and-forget — no return-value check, no `ferror(fptr)` anywhere
   before `fclose(fptr)` at `:1024`; disk-full mid-write silently reports
   success. The digestion-failure leak is also confirmed:
   `fptr = fopen(...)` at `:788`, and `GeneratePlainPeptideIndex()`'s failure
   path at `:823-829` returns `false` with no `fclose(fptr)` and no file
   deletion — a leaked handle plus a 0-byte `.idx` left on disk (the protein-
   lookup failure path at `:1000-1007` at least `fclose`s, just doesn't
   delete the partial file — a lesser variant of the same class). Fix shape:
   check `fwrite` byte counts / `ferror` after the write section; on any
   failure (including the `:823` branch) `fclose()` + `remove()` the partial
   output file before returning `false`.
   **Fix:** the digestion-failure path now `fclose()`s and `remove()`s the
   partial file before returning `false`; the protein-lookup failure path's
   existing `fclose()` now also gets a `remove()`; a final `ferror(fptr)`
   check right before the closing `fclose()` reports a "disk full?" error
   and cleans up the partial file if the write section failed silently.
4. **Carafe mask (`CometPredictedMask.cpp:337`) — NOT APPLICABLE.** This
   file is carafe-only and doesn't exist on master at all.
5. **PEFF header parse — ✅ FIXED, all three sub-bugs, in
   `CometSearch.cpp` (unchanged file).** The `iLen > iLenAllocMods` realloc
   guard (should be `>=`) occurs identically **three times**, not once:
   `:550`, `:683`, `:794` — each followed a few lines later by
   `szMods[iLen]='\0'` (`:568` for the first), a one-byte heap overflow when
   `iLen == iLenAllocMods` exactly. The closing-paren scan
   (`:536-546`, and the mirrored blocks near `:670-687`/`:781-798`) checks
   for `' '`/`'\r'`/`'\n'`/`'('`/`')'` but never `'\0'` — runs past the
   string's null terminator into adjacent heap memory if no proper closing
   delimiter appears. `szPeffLine[strlen(szPeffLine)-1]` at `:505`: if the
   `fgets` two lines above (`:501`) fails, the `// throw error` comment is a
   no-op (doesn't return or set an error) and `szPeffLine` stays the empty
   string set at `:500` — `strlen()-1` on an empty string underflows
   (`size_t` wraps to `SIZE_MAX`), a wild OOB read via unsigned wraparound.
   **Fix:** the realloc guard is now `iLen >= iLenAllocMods` at all three
   sites; the closing-paren scan now also breaks on `*pStr2 == '\0'` at all
   three occurrences; the failed-`fgets` branch now reports the error via
   `g_cometStatus`/`logerr` and `return false`s (with `fclose(fp)`) instead
   of falling through to the underflowing `strlen()-1`.
6. **MSP library loader — ✅ FIXED, all sub-bugs, in
   `CometSpecLib.cpp` (unchanged file).** `ReadSpecLibMSP()`'s
   `struct SpecLibStruct pTmp;` (`:400`) only explicitly sets 4 of the
   struct's 11 members (`strName`, `iLibEntry`, `iSpecLibCharge`,
   `dSpecLibMW` — confirmed against the struct definition,
   `core/Types.h:541-554`); `fRTime`, `fScaleMinInten`, `fScaleMaxInten`,
   `pfUnitVector`, `uiArraySizeMS1` are left as uninitialized garbage on this
   plain (no-constructor) struct — a garbage `pfUnitVector` pushed into
   `g_vSpecLib` defeats any `!= nullptr` null-guard downstream, risking a
   wild-pointer deref. `:388-393`: `while (cChar != '\n') cChar = getc(fp);`
   has no EOF check — `getc()` returns `EOF` forever at end of file, hanging
   if a `"Name:"` line never reaches a newline before EOF. `:398`:
   `sscanf(szBuf + 6, "%s", szTmp)` reads past a bare 5-character `"Name:"`
   line with no length check first.
   **Fix:** the EOF loop is now `int iChar; do { iChar = getc(fp); } while
   (iChar != '\n' && iChar != EOF);`; the bare-`"Name:"` sscanf is now
   guarded by `if (strlen(szBuf) > 6) sscanf(...); else szTmp[0] = '\0';`;
   `pTmp.fRTime`/`fScaleMinInten`/`fScaleMaxInten`/`pfUnitVector`/
   `uiArraySizeMS1` are now all explicitly zeroed/nulled alongside the 4
   fields the code already initialized.
7. **`GetProteinNameString` — ✅ FIXED, both sites, in
   `CometMassSpecUtils.cpp` (unchanged file).** `:358` and `:393`:
   `szProteinName[strlen(szProteinName) - 1]` — if `szProteinName` is an
   empty string after `fgets`/`fscanf` (empty FASTA description, or
   `fscanf` failing and leaving the buffer untouched), `strlen()-1`
   underflows to a huge `size_t`, an OOB read (and, since this is the target
   of an assignment in the surrounding `while` loop, a potential OOB *write*
   too).
   **Fix:** both sites now loop `while (strlen(szProteinName) > 0 &&
   (szProteinName[strlen(szProteinName)-1] == '\n' ||
   szProteinName[strlen(szProteinName)-1] == '\r'))` before indexing.
8. **mzIdentML merge — ✅ FIXED, all three sub-bugs, in
   `CometWriteMzIdentML.cpp` (unchanged file).** `:136`: `exit(1)` directly
   inside library code on tmp-file-open failure — kills the entire hosting
   process, including an embedding RTS host, instead of returning an error
   to the caller. `:156-223` (and `:455`, `:501`): many `std::stoi`/
   `std::stod` calls on fields parsed from the tmp file, no `try/catch`
   anywhere — throw uncaught (crashing the process) on a truncated/corrupted
   tmp file. `WriteSpectrumIdentificationList()` (`:1245` on): the closing
   `</SpectrumIdentificationResult>` tag is written unconditionally after the
   result loop (`:1364`) regardless of whether the loop ran at all — for a
   zero-PSM run, `vMzid` is empty, the loop never executes (so no opening
   tag is ever written), and this unconditional closing tag becomes a
   genuinely mismatched/invalid piece of XML.
   **Fix:** `:136`'s `exit(1)` is now `return false` (reported via
   `g_cometStatus.SetStatus`/`logerr`); the entire per-line parse body of
   `ParseTmpFile()` (from the tmp-file-open check through its final `return
   true`) is now wrapped in one `try/catch (const std::exception&)` that
   reports the error and returns `false` instead of letting a malformed
   field's `stoi`/`stod`/`stof`/`.at()` throw uncaught — this covers all the
   field-parsing sites cited (`:156-223`) plus the `stol`/`stoi` calls in
   the later `:455`/`:501`-area protein-offset parsing loops in the same
   function; `WriteSpectrumIdentificationList()`'s trailing close tag is now
   wrapped in `if (!(*vMzid).empty()) { ... }` so it's only written when an
   opening tag actually preceded it.
9. **Unescaped XML attributes — ✅ FIXED, all cited sites, in
   both files (unchanged).** `CometWritePepXML.cpp`: `:137`
   (`summary_xml="%s.pep.xml"`), `:139` (`base_name`), `:141` (`msModel`),
   `:160` (`raw_data`), `:183` (`local_path`) all `fprintf` raw C strings
   (paths, instrument model strings) directly into XML attribute values with
   no call to `CometMassSpecUtils::EscapeString` (which exists and is
   already used elsewhere in the same file, e.g. `:459,550,559,568,577,599,615`
   — so this isn't a missing capability, just inconsistent application).
   `CometWriteMzIdentML.cpp`: `:562` (`userParam value`, an arbitrary
   user-configured param string), `:1139`/`:1175` (`location`, database/input
   file paths), `:1158` (`userParam name`) — same pattern. A path containing
   `&` (plausible on Windows) produces invalid XML wherever this is hit.
   **Fix:** every cited site in both files now builds a `std::string`, runs
   it through `CometMassSpecUtils::EscapeString()`, and prints the escaped
   copy — matching the pattern already used elsewhere in each file. In
   `CometWriteMzIdentML.cpp`, `WriteInputs()`'s `SearchDatabase location`,
   `DatabaseName userParam`, and `SpectraData location` attributes are all
   covered the same way.
10. **`Query` destructor null-derefs during OOM cleanup — ✅ FIXED,
    in `CometPreprocess.cpp`/`core/Types.h` (both unchanged).**
    `pScoring->iFastXcorrDataSize`/`iSpScoreData` are assigned
    (`CometPreprocess.cpp:1289,1480,2065,2144`) *before* the corresponding
    `new float*[...]` allocation is even attempted a few lines later; if that
    allocation throws `bad_alloc` (caught, function returns `false`, pointer
    stays whatever it was — likely still null), the `Query` destructor
    (`core/Types.h:877-921`) iterates `for (i=0; i<iFastXcorrDataSize; ++i)`
    over `ppfSparseFastXcorrData[i]`/`ppfSparseFastXcorrDataNL[i]`
    unconditionally — a null-pointer dereference if the caller deletes the
    `Query` after the failed preprocessing call.
    **Fix:** each of the three cleanup loops in the destructor now also
    requires the owning pointer array itself (`ppfSparseSpScoreData`,
    `ppfSparseFastXcorrData`, `ppfSparseFastXcorrDataNL`) to be non-null
    before indexing into it, in addition to the existing `!bSparseFromPool`
    check — this handles the failure uniformly regardless of which of the
    four `CometPreprocess.cpp` allocation call sites triggered it, rather
    than requiring an allocation-order fix at each site individually.
11. **MS1 library preprocessing — ✅ FIXED, both sub-bugs, in
    `CometPreprocess.cpp` (unchanged file).** `:1108`:
    `mstSpectrum.at(pTmp.iNumPeaks - 1).mz` where `iNumPeaks` is `unsigned
    int` — for an empty scan (`iNumPeaks == 0`, reachable since
    `minimum_peaks = 0` is an accepted config value), `iNumPeaks - 1`
    underflows to a huge unsigned value passed to `.at()`. `:1137-1148`: no
    guard against `dMagnitude == 0.0` before `pdTmpFastXcorrData[i] =
    pdTmpRawData[i] / dMagnitude` (`:1144`) — if every peak in the spectrum
    falls outside `[dMS1MinMass, dMS1MaxMass]`, `pdTmpRawData[]` stays
    all-zero and this divides `0.0/0.0`, filling the library's unit vector
    with NaN for every bin, silently, with no downstream guard on this
    producer side (unlike the query-side computation, which does guard
    against this).
    **Fix:** `PreprocessThreadProcMS1` now returns early (after `delete
    pPreprocessThreadDataMS1`, releasing the pool slot via its destructor)
    if `pTmp.iNumPeaks <= 0`, before the `.at(iNumPeaks - 1)` call; the
    `dMagnitude == 0.0` case is now guarded so `pdTmpFastXcorrData[i]` stays
    at its already-zeroed value instead of computing `0.0/0.0`. The same
    `iNumPeaks - 1` underflow, found while fixing this, also existed in
    `PreprocessMS1SingleSpectrumThreadLocal` — the live RTS single-spectrum
    path documented in `CLAUDE.md` (not originally cited by line number,
    since the finding's audit only walked the batch/library-build function,
    but the identical bug shape) — and is now guarded there too with an
    early `return nullptr` for `iNumPeaks <= 0`, matching that function's
    existing nullptr-on-empty-spectrum convention. (Its non-thread-local
    twin, `PreprocessMS1SingleSpectrum`, has the same unguarded pattern but
    has no callers anywhere in the repo — confirmed dead code — so it was
    left as-is rather than fixing unreachable code out of scope.)

**Bottom line**: every sub-item except the carafe-only mask one was
confirmed still open on the 2026-08-19 audit (several — the section-size
underflow, the 3x-repeated PEFF realloc guard, the double MSP/mzIdentML bug
clusters — turned out to have more instances or sharper edges than the
original one-line summary suggested), then all 8 were fixed the same day
at your "fix all" request. Verified with a clean `make` build (zero new
warnings) and the full 42/42 unit suite.

### C13. Operational traps (medium/low) — ✅ FIXED 2026-08-21
- `-F` without `-L`: the skip-to-start loop can't advance when scan `iFirstScan`
  isn't MS/MS (`CometPreprocess.cpp:858`; `iLastScan` still 0 per the code's
  own comment) — search silently ends with zero spectra. `ReadPrecursors`
  handles the same case correctly via `iFileLastScan` (430). The fused-path
  copy (3832) inherits it.
  **Fix:** all 4 sites (`CometPreprocess.cpp:457, 890, 3843, 4045` post-fix)
  now bound the advance loop on `iFileLastScan`, not `iLastScan`; only the
  first was already correct and served as the reference pattern for the
  other 3.
- Preprocessing pool "wait" spins 240 s while *holding*
  `g_preprocessMemoryPoolMutex` — slot release needs that mutex, so the wait
  can never succeed; on timeout the spectrum is silently dropped and
  `pPreprocessThreadData` leaks (`CometPreprocess.cpp:1000-1027`, MS1 variant
  1056-1084). Unreachable today only because pool size == thread count.
  **Fix:** `PreprocessThreadProc`/`PreprocessThreadProcMS1` now lock the
  mutex only around each individual poll (with a 1ms sleep between failed
  polls) instead of holding it for the whole spin, so a slot-releasing
  destructor on another thread can actually acquire it; the timeout path now
  also `delete`s the leaked `PreprocessThreadData`/`PreprocessThreadDataMS1`.
- Manager reuse: `staticParamsInitializationComplete` is one-shot
  (`CometSearchManager.cpp:1613`) so post-run `SetParam`s are silently
  ignored, and `FiStrategy::finalize()` frees `g_bIndexPrecursors`
  (`search/FiStrategy.cpp:174,182`) that only `InitializeStaticParams` ever
  allocates — a second FI_DB search in the same process null-derefs.
  **Fix:** `InitializeStaticParams()`'s early-return path now re-allocates
  `g_bIndexPrecursors` if it's `NULL` (i.e. freed by a prior
  `FiStrategy::finalize()`) before returning, using whatever param values
  are already live in `g_staticParams` — scoped narrowly to the null-deref;
  the broader "post-run `SetParam`s are silently ignored" limitation of this
  one-shot design is unchanged and out of scope for this fix.

**Verified 2026-08-21:** full clean rebuild, 52/52 unit tests, and a full
`--integration` run (T17/T18/T19/T20/T22/T23/T24, 57/57 passed on that run;
one single-sample `_check_timing` flake on T24's FI_DB index-build time in
an earlier run reproduced as a pass on re-run — machine noise, not a
regression from these changes, none of which touch the index-build path).
No new fixture added for these 3 items — the first two require host-
specific -F/-L CLI conditions and OOM-adjacent thread-pool exhaustion that
aren't practical to reproduce deterministically in the unit suite; the third
(manager reuse) would need a new test harness entry point that issues two
successive `DoSearch()` calls on one `CometSearchManager`, which the current
`run_tests.py` (subprocess-per-invocation) can't exercise.

---

## 3. Latent / API-hardening issues (no live trigger on shipped paths)

**All items in this section were fixed 2026-08-21.** Verified collectively via
a full clean `make` build (zero new warnings) and the 42-test unit suite,
plus a full `--integration` run (T17/T18/T19/T20/T22/T23/T24, 8/8 passed,
including T23's decoy-mode parity and T24's plain-FASTA/FI_DB/PI_DB parity
against real bigdata) — none of these latent/concurrency items regressed
anything live. `CometSearchManagerWrapper`'s C++/CLI finalizer was verified
with a full `MSBuild.exe Comet.sln` build (via WSL interop), producing
`MSToolkit.lib`, `CometSearch.lib`, `CometWrapper.dll`, `RealtimeSearch.exe`,
and `Comet.exe` with zero errors. Getting there required also fixing an
unrelated, pre-existing gap this surfaced: `MSToolkit.vcxproj`'s `Reference`/
`HintPath` items for the two ThermoFisher managed assemblies (needed by
`RAWReader.cpp`'s `/clr` compilation) weren't being translated into `/FU`
compiler switches by a command-line `MSBuild.exe` invocation — confirmed via
a diagnostic-verbosity build showing only `mscorlib.dll`'s `/FU` on the
actual `CL.exe` command line, with both ThermoFisher DLLs present on disk at
their exact `HintPath` but never passed to the compiler. Fixed by setting
`ForcedUsingFiles` explicitly on `RAWReader.cpp`'s `ClCompile` item (see that
file's own comment in `MSToolkit.vcxproj` for full detail) rather than
relying on the `Reference` items to auto-generate it.

- ✅ **Split init mutexes — FIXED.** `InitializeSingleSpectrumSearch` and
  `InitializeSingleSpectrumMS1Search` use different mutexes
  (`CometSearchManager.cpp:2229/2236` vs 2484/2489) but both run
  `InitializeStaticParams()` (guarded by a plain non-atomic bool) and both
  `fillPool()` on the same `ThreadPool` (no internal lock; `ThreadPool.h:72-90`).
  Concurrent MS1+MS2 init from two Tasks — which the API permits — races on
  all of `g_staticParams` and can use-after-free the pool. The shipped C#
  driver inits sequentially, so latent. **Fix:** both functions now lock a
  single shared `g_initSingleSpectrumMutex` (file-scope `static std::mutex` in
  `CometSearchManager.cpp`, replacing each function's own separate
  function-local static mutex) instead of one mutex + atomic flag each — this
  fully serializes the two init paths against each other, closing both the
  `InitializeStaticParams()` race and the shared-`ThreadPool::fillPool()` race
  at once.
- ✅ **Flags published before init completes — FIXED.** `g_bPeptideIndexRead` /
  `g_bPlainPeptideIndexRead` are set at the end of `ReadPeptideIndex()`
  (`CometPeptideIndex.cpp:300-301`) but mass tables and the FI build happen
  *after* (`CometSearch.cpp:136-162`, `CometSearchManager.cpp:2338`); the
  lock-free fast paths (`CometSearch.cpp:119-120,175`) trust them. Safe today
  only because shipped entry points gate behind the init mutex. **Fix:** left
  `ReadPeptideIndex()`'s own flags untouched (its other 4 call sites are all
  already safe, either single-threaded-by-construction or already
  redundantly re-setting the flag at the correct later point) and instead
  added a new, narrowly-scoped atomic `g_bPeptideIndexFullyInitialized`, set
  only once `EnsurePeptideIndexLoaded()` (`CometSearch.cpp` — the one genuine
  multi-RTS-thread-reachable lazy-load path) has *also* finished mass-init and
  AScorePro-interface setup, not just the raw `.idx` read; that function's
  unlocked fast-path check and its re-check-under-lock both now gate on this
  new flag instead. `InitializeSingleSpectrumSearch()`'s PI_DB branch also
  sets the new flag at the same point it already sets the old one, so
  subsequent per-query `EnsurePeptideIndexLoaded()` calls still take the fast
  path after normal one-time init (without this, every RTS query would have
  taken the locked slow path forever).
- ✅ **`RtsScratch::AllocSparseChild()` heap fallback leak — FIXED.** The
  Query frees nothing when `bSparseFromPool` (`CometPreprocess.cpp:149-154`,
  `core/Types.h:880-887`) — currently unreachable by capacity math, but a
  landmine if the margin shrinks. **Fix:** `RtsScratch` now tracks fallback
  allocations in a `vector<float*> vFallbackSparseChildren`, freed at the
  start of the *next* `ResetForNewSpectrum()` call (safe: by then the
  previous spectrum's Query has finished scoring and been destroyed) and
  before `EnsureInitialized()`/`~RtsScratch()` free or resize the pool.
  **Same for the batch empty-spectrum NaN transient** — this turned out to be
  the unguarded `dMagnitude`-divide inside `PreprocessMS1SingleSpectrum()`,
  which (along with its sibling `PreprocessSingleSpectrum()`) had zero
  callers anywhere in the repo or any `.vcxproj`/Makefile; both were deleted
  outright rather than fixed in place, closing this and overlapping with the
  dead-code item below (`PreprocessMS1SingleSpectrumThreadLocal`, the actually
  -used thread-local sibling, already had this guard from the earlier C12 fix
  batch).
- ✅ **`SetParam`/`~CometSearchManager` leaks — FIXED.** All 10
  `SetParam` overloads leaked the replaced `CometParam*` on every override
  (`_mapStaticParams.erase(name)` drops the map entry but never `delete`s the
  value it pointed to), and `~CometSearchManager` cleared
  `_mapStaticParams`/`g_pvInputFiles` — both containers *of pointers* — via
  `.clear()` alone, which destroys the pointers, not what they point to
  (`CometSearchManager.cpp:388-390, 1658-1887`) — real for long-lived RTS
  hosts that re-set params. **Fix:** all 10 `SetParam` overloads now `delete
  ret.first->second` before overwriting it (using the iterator `insert()`
  already returns on failure, instead of erase+reinsert); the destructor now
  loops over both containers deleting each pointer before `.clear()`.
  Confirmed `InputFileInfo*`/`CometParam*` are exclusively owned by
  `CometSearchManager` (only ever `new`'d in `Comet.cpp`/`SetParam()`, never
  deleted anywhere else), so no double-free risk.
- ✅ **`CometSearchManagerWrapper` missing finalizer — FIXED.** Had a
  destructor but no C++/CLI finalizer (`CometWrapper/CometWrapper.h:36`) —
  native manager leaks if the C# host never calls Dispose. **Fix:** added
  `!CometSearchManagerWrapper()` (the finalizer) holding the actual native
  cleanup (`ReleaseCometSearchManager()` + freeing `_pvInputFilesList`); the
  destructor now just delegates to it via `this->!CometSearchManagerWrapper();`
  — the standard C++/CLI IDisposable pattern, where the compiler-generated
  `Dispose()` still runs cleanup deterministically and suppresses the
  finalizer, but the GC-invoked finalizer now also runs cleanup if `Dispose()`
  is never called.
- ✅ **`ValidateSequenceDatabaseFile` inverted suffix test — FIXED.**
  (`CometSearchManager.cpp:179`, `strcmp` truthiness) — currently guards a
  dead global (`g_bIdxNoFasta` has no readers) but truncates the wrong names
  if ever consumed. **Fix:** negated the `strcmp` (`!strcmp(...)`) so the
  `.idx`-extension strip actually runs when the suffix *is* `.idx`, instead of
  every time it *isn't*. Inert today (confirmed `sTmpDB` feeds nothing but the
  dead `g_bIdxNoFasta`), so zero behavior change on any current path.
- ✅ **Dead code with teeth — all items resolved.**
  `CometCheckForUpdates`/`CheckForUpdates` (no callers anywhere, not even
  compiled into the Linux Makefile or the Windows `.vcxproj`; `exit()` on
  network error, 128-byte stack strcpy from plain-HTTP response, `[-1]`
  index) — **deleted** (`CometCheckForUpdates.cpp`/`.h` removed entirely, not
  referenced by any build). `PreprocessSingleSpectrum`/
  `PreprocessMS1SingleSpectrum` (no callers; the MS1 one uses pool slot 0 with
  no lock) — **deleted** (see the `RtsScratch` item above). `FragmentIndexReader`
  (never instantiated; would snapshot dangling globals) — **deleted**
  (header-only, zero instantiations anywhere; removed from
  `CometSearch.cpp`'s includes and both the Makefile and `.vcxproj`).
  `Threading::BeginThread` + the lossy binary `Semaphore` — **deleted**, along
  with `ThreadSleep`/`InitSemaphore`/`WaitSemaphore`/`SignalSemaphore`/
  `DestroySemaphore` (all zero-caller) and the now-unused `_threadId` member;
  `Semaphore`/`ThreadProc` type definitions removed from
  `OSSpecificThreading.h` too (confirmed unused elsewhere; `Mutex`/`ThreadId`
  usage there is untouched since those stay live). `GetProteinSequence`
  (mutates "read-only" `g_staticParams` counters) — turned out to be **live**,
  not dead (called from `CometWriteMzIdentML.cpp:334`) — **fixed** by removing
  the two `g_staticParams.databaseInfo.uliTotAACount++` increments, which were
  copy-pasted from the real one-time digestion loop in `CometSearch.cpp` but
  re-ran every time this function fetched a protein sequence for mzIdentML
  output, silently inflating the count SQT's `DBSeqLength` header line reports
  whenever both output formats were enabled together. `NormalizeDoubleToChar`
  (clamps to +128 which wraps to -128) — zero callers — **deleted**.
  `_vFragmentPeptidesMutex` never locked despite a comment saying it's needed
  above an unlocked `push_back` (`CometFragmentIndex.cpp`) — this one is *not*
  dead (the `push_back` runs on every FI build), just currently
  single-threaded by construction (P1's still-open finding: the FI build pass
  ignores its `ThreadPool` entirely) — **fixed** by actually locking
  `_vFragmentPeptidesMutex` around the check-then-push, a no-op today but
  correct and ready for whenever P1 gets addressed and this pass is
  genuinely parallelized.
- ✅ **Non-reentrant libc — FIXED.** `strtok` in `ParsePeptideIndexHeader`
  (`CometPeptideIndex.cpp:1290-1296`), `localtime` in three writers.
  **Fix:** `Common.h` now `#define strtok_r strtok_s` on `_MSC_VER` (same
  precedent MSToolkit already uses) and adds a `comet_localtime()` inline
  wrapper (POSIX `localtime_r`/MSVC `localtime_s` have incompatible
  signatures — argument order and return type both differ — so this couldn't
  be a plain `#define`; it normalizes both behind one signature with a
  `thread_local struct tm` buffer, a drop-in replacement for `localtime(&t)`).
  The `strtok` call site now uses `strtok_r` with an explicit `saveptr`; all
  4 `localtime(&tTime)` call sites (`CometWritePepXML.cpp`,
  `CometWriteMzIdentML.cpp` x2, `CometWriteSqt.cpp`) now call
  `comet_localtime(&tTime)`.
- ✅ **`InitPrecomputedDecoyBins` staleness — FIXED.** `std::call_once` per
  process but RTS supports re-initialization with new params — stale
  ion-series/bin-width table would silently produce wrong E-values.
  **Fix:** captures a fingerprint (ion-series count, max fragment charge, bin
  width, bin offset) when the table is built, and `GenerateXcorrDecoys()`
  (the only caller) now `assert()`s the fingerprint still matches live params
  on every call, not just the first — `std::call_once` itself can only
  express "run exactly once ever," so the staleness check has to live
  outside it. Confirmed `NDEBUG` isn't defined anywhere in this build, so the
  assert is live in the shipped Release configuration, not compiled out.
- ✅ **8-byte binary I/O 64-bit assumption — FIXED.** `.idx` binary I/O reads/
  writes `size_t` objects using `clSizeCometFileOffset`
  (`== sizeof(comet_fileoffset_t)`, always 8) as the byte count for both
  `fread`/`fwrite` (`CometPeptideIndex.cpp:190,205,1064-1071`) — only correct
  if `sizeof(size_t)` is also 8. **Fix:** added
  `static_assert(sizeof(size_t) == sizeof(comet_fileoffset_t), ...)` right
  next to `clSizeCometFileOffset`'s definition (`CometFragmentIndex.cpp`) —
  fails the build outright on a hypothetical 32-bit target instead of
  silently overreading/overwriting past these variables at runtime.

**Concurrency audit bottom line:** the RTS thread-local contract (caller-owned
`Query*`/`QueryMS1*`, read-only indexes, mutexed `g_cometStatus`/aligner) is
genuinely upheld on every live path traced end-to-end from C# through the
wrapper to the thread-local C++ functions. The items above were the complete
list of exceptions/latencies found, and all are now fixed.

---

## 4. Performance opportunities (ranked by expected payoff)

### P1. Parallelize the fragment-index build passes (largest win: ~4-6x FI build)
`CometFragmentIndex.cpp:202-203` (count), 252-257 (fill), 308-449: despite the
ThreadPool being plumbed through, `AddFragmentsThreadProc(bool, ThreadPool*
/*tp*/)` ignores the pool and loops over *all* of `g_vRawPeptides` on the
calling thread; both `wait_on_threads()` calls are no-ops and the comments
describe a threaded design that no longer exists. Count and fill are each
O(variants x peptide length) — the dominant cost of a large FI build — and
embarrassingly parallel (count: per-thread bin arrays merged at the end;
fill: partition `g_vFragmentPeptides` with per-partition pre-counts as write
cursors).

### P2. `AddFragments()` per-call churn (double-digit % of both build passes) — ✅ FIXED 2026-08-21
`CometFragmentIndex.cpp:461` (heap-backed `string sPeptide` copy per call),
480 (`modSeq` string copy per call), 502-525 (full residue-by-residue
precursor-mass recompute in the fill pass even though `fp.dPepMass` already
holds it from the count pass). Called twice per variant, potentially 10^8+
times on whole-proteome builds. Use `const&`/`const char*` and pass the known
mass into the fill pass.

**Fix applied:** `sPeptide` replaced with a `const char* pszPeptide` read
directly from the raw-peptide entry's `szPeptide` char[]; `modSeq` replaced
with a `const string&` bound to the existing `MOD_SEQS` entry (or a shared
static empty string) instead of copying it. Added a `dKnownPepMass`
parameter (default `-1.0`, sentinel for "not yet known"); the fill pass's
single call site (`GenerateFragmentIndex()`) now passes `fp.dPepMass`
through, so `AddFragments()` skips the entire O(peptide length) mass
recompute and its hardening check on that second call — both already ran
once during the count pass for this exact variant. The 8 count-pass call
sites (inside `AddFragmentsThreadProc()`) are untouched, since that's the
one place the mass genuinely isn't known yet.

### P3. Per-candidate full-array memsets in FI/PI search (direct RTS Hz win) — ✅ FIXED 2026-08-21
`CometSearch.cpp:1740-1741` (`SearchFragmentIndex`), 2180 and 2472
(`AnalyzePeptideIndex`): each scored candidate memsets the whole
`pbDuplFragment` (`iArraySizeGlobal` bools — hundreds of KB at small bins)
and, in FI, the full ~142 KB `uiBinnedIonMasses`. `AnalyzePeptideIndex` does
the full memset *and* a per-bin first-pass clear (2198-2263) — one is
redundant. Up to `FRAGINDEX_MAX_NUMSCORED = 100` candidates per query → tens
of MB of avoidable memset per spectrum. Adopt the FASTA path's memset-free
two-pass clear (3116-3152); preserve the `iFoundVariableMod = 2` side effect
at 2238.

**Fix applied:** `SearchFragmentIndex()` now clears `pbDuplFragment` via a
fine-grained pass mirroring the set pass's own loop nest (same technique the
FASTA path already used), instead of a full-array memset; `uiBinnedIonMasses`
kept its existing tight per-row memset. `AnalyzePeptideIndex()`'s target-peptide
path already had this fine-grained clear -- removed its now-redundant full
memset. Its **decoy** path had no fine-grained clear at all (relied entirely
on the full memset) -- added one, mirroring the target path's pattern.

**Verified 2026-08-21:** full clean rebuild, 52/52 unit tests, and a full
`--integration` run (T17-T24, 58/58 passed both times P2/P3 were tested,
including an exact FI_DB PSM-count match against the `v2026.02.2` baseline,
ratio 1.000) -- confirming both fixes are results-identical, with a real,
consistent speedup: FI_DB index build 0.895x, PI_DB index build 0.794x,
FI_DB search 0.878x, PI_DB search 0.749x the baseline's wall-clock time.

### P4. Spectrum deep copies and per-charge redundant preprocessing — ✅ FIXED 2026-08-21 (copy-elimination half only)
`CometPreprocess.h:216-229`: `Preprocess()` takes `Spectrum` by value and
`LoadIons()` copies it again — two full peak-vector copies per charge state
(4 per spectrum for a 2+/3+ guess) in batch, fused, and MS1 paths. Pass
`const Spectrum&` and make one mutable copy per spectrum only when needed
(the `sortIntensity()` case). Separately, the whole binning/windowing
pipeline repeats per charge state (the comment at 2622-2624 already notes
it); hoisting charge-independent work nearly halves preprocessing for
charge-guessed spectra.

**Fix applied (copy-elimination only):** `Preprocess()`/`LoadIons()` now take
`Spectrum&` (not `const Spectrum&` — MSToolkit's `Spectrum` class has no
`const`-qualified accessors at all, e.g. `size()`/`at()`/`getMZ()`, so a
`const&` parameter can't call any of them without modifying that
third-party-adapted header; a plain non-const reference achieves the same
copy-elimination without touching MSToolkit). `LoadIons()`'s conditional
`sortIntensity()` now makes exactly one local copy only when that branch
actually triggers (FI/PI mode with more peaks than
`fragindex_numpeaks`), reading through a `Spectrum*` pointing at whichever
one (the caller's reference, or the freshly-sorted local copy) is current,
instead of unconditionally copying on every call regardless of whether
sorting is even needed.

**Not attempted — "hoist charge-independent work":** investigated and found
substantially harder than it first reads. Almost the entire downstream
pipeline (bin cutoffs, `iArraySize`, the `dExpPepMass+50` peak window) is
keyed off the *guessed* neutral mass, which is genuinely different per
charge guess — not superficially redundant work, a real per-charge
recomputation. Left open; flagged here rather than forcing a risky
restructure.

### P5. Protein-name resolution I/O (multi-x on the output phase) — ✅ FIXED 2026-08-21 (2 of 3 sub-issues)
Three compounding issues: (a) each enabled writer independently calls
`GetProteinNameString` per PSM (`CometWriteTxt.cpp:423`,
`CometWritePepXML.cpp:535`, `CometWritePercolator.cpp:132`,
`CometWriteSqt.cpp:330`) — 4x redundant random I/O with several formats
enabled; (b) each call does per-protein `fseek`+`fscanf`; (c) the `.idx`
name-cache build does random-access `fseek`+256-byte reads under a 32 MB
stdio buffer that each seek discards (`CometPeptideIndex.cpp:94, 219-238`) —
one sequential read of the contiguous name section is strictly better.
Resolve names once per result batch into a map keyed on the protein offset
and hand it to all writers; extend the cache to the RTS FASTA path, which
currently re-opens the db per spectrum (`CometSearchManager.cpp:2789-2798`).

**Fix applied (a+b, indexed-DB mode only):** discovered `g_pvProteinNameCache`
(the same process-wide, in-memory, offset->name cache the RTS path already
uses) is a ready-made drop-in for exactly this -- `GetProteinNameString()`'s
3 indexed-DB (FI_DB/PI_DB) read blocks now resolve through a shared
`resolveIndexedProteinName` lambda that looks up the cache (falling back to
the original `fseek`+`fgets`/`fscanf` only on a cache miss, which shouldn't
normally happen once an index is loaded) instead of doing its own
`fseek`+`fgets`/`fscanf` per protein per call. This eliminates both (a) and
(b) simultaneously for indexed-DB batch search -- every enabled writer now
reads from the same in-memory cache instead of each independently hitting
the file. Verified all 4 writers (txt/sqt/pep.xml/pin) resolve the correct
name with a manual PI_DB smoke test with all 5 output formats enabled at
once.

**Fix applied (c):** `.idx` name-cache build (`CometPeptideIndex.cpp`) now
collects the distinct offsets actually referenced, sorts them, and does ONE
sequential `fread` spanning `[min, max + WIDTH_REFERENCE)` instead of one
`fseek`+`fread` per protein -- safe because `WritePeptideIndex()` writes
every protein's name back-to-back in one contiguous section (verified
against its write-loop directly).

**Not attempted:** extending the cache to the RTS FASTA-path (batch-search
writers only; RTS's inline resolution logic in `CometSearchManager.cpp` is a
separate code path not touched here).

### P6. `sDBEntry` copied by value 2-4x per protein, with redundant re-sorts — ✅ FIXED 2026-08-21
`CometSearch.cpp:1276, 2633, 3481` plus the `SearchThreadData` copy
(`CometSearch.h:45-47`): full sequence + PEFF vectors deep-copied per call
layer; `SearchForPeptides` runs up to 3x per protein and re-sorts the
already-sorted PEFF vectors each time (2674-2681). Pass `const sDBEntry&`.

**Fix applied:** `DoSearch()`/`SearchForPeptides()`/`SearchForVariants()` all
take `sDBEntry&` now (not `const&` -- `SearchForPeptides()` sorts
`dbe.vectorPeffMod`/`vectorPeffVariantSimple`/`vectorPeffVariantComplex` in
place, and `SearchForVariants()` forwards its own `dbe` into
`SearchForPeptides()`, so neither can be const). `SearchThreadData`'s own
copy (necessary -- it's heap-allocated and handed to a thread pool, so it
needs an owned copy independent of the caller's loop variable) is
unaffected; every *subsequent* by-value hop on top of that is eliminated.
The 3 PEFF-vector sorts are now guarded with `is_sorted()` first, so the
second `SearchForPeptides()` call for the N-term-Met-clipped variant (same
`dbe`, already sorted by the first call) pays an O(n) check instead of
another O(n log n) sort.

### P7. Decoy work redone per matching query — ✅ FIXED 2026-08-21
`CometSearch.cpp:3202-3345`: for unmodified peptides the target ion set is
computed once per peptide but the entire decoy construction + ladder + two
binning passes re-run for every matching query; `CalcVarModIons` already
gates its decoy build on `bFirstTimeThroughLoopForPeptide` (7399). Matters
for wide-tolerance/DIA-style searches with decoys. (Interacts with B6 if
fixed.)

**Fix applied:** the decoy ladder (`szDecoyPeptide`,
`_pdAAforwardDecoy`/`_pdAAreverseDecoy`, `_uiBinnedIonMassesDecoy`,
`_uiBinnedPrecursorNLDecoy`) is now built once per peptide, gated by
`bFirstTimeThroughLoopForPeptide` exactly like the target ladder just above
it -- `szDecoyPeptide` moved from a per-iteration stack declaration inside
the `while` loop to a per-peptide one outside it, so it stays valid across
every matching query after the first. Also had to fix the decoy's two
precursor-NL binning loops, which bounded on
`_pQueries->at(iWhichQuery)->_spectrumInfoInternal.usiChargeState` (the
*current* query's own charge) instead of the global max charge the target
side's analogous loops already use (`iPrecursorNLMaxCharge =
g_staticParams.options.iMaxPrecursorCharge`) -- without this, a peptide's
first-matching query at a lower charge would leave higher-charge slots
unfilled for every later query that reuses the now-cached ladder.

**Caught and fixed during verification, not shipped:** the first version of
this fix flipped `bFirstTimeThroughLoopForPeptide` to `false` in its
original location (right after the target block, before the decoy block's
own gate check) -- meaning the decoy gate always saw `false` and never
built its ladder at all, `XcorrScore()` scoring every decoy candidate
against uninitialized/stale leftover data. Caught by `t21_autodecoy`
(migrated legacy case, `decoy_search=1`) failing with a completely wrong
peptide/protein reported. Fixed by moving the flip to after *both* the
target and decoy one-time blocks, but still outside the
`if (iDecoySearch)` conditional (a non-decoy search must still flip it, or
the target block's own preexisting one-time-per-peptide optimization
regresses). Full 52-test unit suite passes after the fix, including
`t21_autodecoy`.

### P8. Mod-permuter subset enumeration — 2 of 3 sub-items ✅ FIXED 2026-08-21
`CometModificationsPermuter.cpp:574-589`: step 2 scans all of
`ALL_COMBINATIONS` (~2.4M entries for a 50-residue modifiable string) per mod
per unique sequence; direct subset enumeration of `modBitmask`
(`sub = (sub - 1) & mask`) is exactly `2^bitCount` iterations and removes the
`ALL_COMBINATIONS` dependence for this step. **Still open** — this is the
largest sub-item and the one genuine algorithmic rewrite here; not attempted
this pass.

Also: `initCombinations` leaks every intermediate merge buffer (~70 MB per
index build at defaults; 162-200), and the variable-mod merge loop never
considers slot 15 (`CometSearchManager.cpp:1330`, `ii < VMODS-1`). **Both
fixed 2026-08-21:**
- `initCombinations`: the previous `allCombos` buffer (the first iteration's
  `combos` array, or a prior iteration's `temp`) is now `delete[]`d right
  before being superseded by the new merge result, instead of being silently
  overwritten and leaked on every iteration but the last.
- Merge loop: `for (int ii=i+1; ii<VMODS-1; ++ii)` → `ii<VMODS` — slot 15
  (`variable_mod15`) is now eligible to be detected as a duplicate of an
  earlier slot and merged in, same as every other slot. Verified
  results-preserving (not just non-crashing) with a differential test: ran
  the same phospho+NL search once with the mod in slot 1 only and once with
  an identical duplicate additionally placed in slot 15, and diffed the
  `.txt` output — byte-identical (peptide, score, mod position all match),
  confirming the merge is transparent to search results as intended.

Full 52-test unit suite still passes after both fixes.

### P9. Preprocessing micro-wins — 1 of 3 sub-items ✅ FIXED 2026-08-21
Batch memsets 3 x `iArraySizeGlobal` doubles per charge state
(`CometPreprocess.cpp:1197-1200`) though only `[0, iArraySize + offset)` is
touched — the RTS `iZeroBound` logic (1691-1728) already implements the tight
version (~6 MB less traffic per charge state at `fragment_bin_tol 0.02`).
**Still open.** RTS FI top-N peak selection full-sorts all peaks
(1802-1808) — `std::nth_element` + small sort. **Still open.**

MS1 batch memsets three full arrays and never touches one of them
(1094-1097; the `// FIX` comment already flags it). **Fixed 2026-08-21:**
`pdTmpCorrelationData` is fetched from the pool and `memset`, but
`PreprocessThreadProcMS1` never reads it afterward (only `pdTmpRawData` and
`pdTmpFastXcorrData` feed the unit-vector computation) — removed both the
fetch and the memset. Full 52-test unit suite still passes.

### P10. Concurrency scaling ceilings (RTS)
- `SearchMemoryPool::acquireSlot/releaseSlot` global mutex+CV taken twice per
  spectrum (`threading/SearchMemoryPool.cpp:101-116`) — measured flat ~4-5M
  ops/s from 8-512 threads (docs/20260618_mutexserialization.md); the doc's
  fused-batch pre-assigned-slot pattern is the known fix.
- `pQuery->accessMutex` locked per scored candidate
  (`CometSearch.cpp:4468/4527, 8009/8088`) — uncontended for RTS (Query is
  thread-owned) but still an atomic RMW pair x hundreds-thousands of
  candidates per spectrum that batch needs and RTS pays for nothing.
- `ThreadPool::wait_for_available_thread()` is a 1-5 ms sleep poll whose
  predicate ignores queued tasks (`ThreadPool.h:105-144`) — batch-only.
- Wrapper: peak arrays pinned for the entire ms-scale native call and a fresh
  managed object graph per call (`CometWrapper/CometWrapper.cpp:140-189`);
  `matchingFragments` is built even when ignored.
- Footprints: `thread_local RtsScratch` retains ~1-15 MB per thread for
  thread lifetime (worth documenting for C# hosts with big Task pools);
  `SearchFragmentIndex` keeps a ~143 KB frame on the stack (1455-1456) — the
  PI path already moved the same array into the pool.

### P11. Output-phase mechanics
Writers issue 25-40 `fprintf` calls per result line — build each line with
one `snprintf` (as SqtWriter does) or `setvbuf` 1 MB. The mzIdentML merge
materializes the tmp file three ways before sort/unique
(`CometWriteMzIdentML.cpp:120-294`) — ~3x tmp-size memory high-water; two
streaming passes bound it. The two remaining single-threaded
`std::sort`s over up to 10^8 24-byte structs (`CometFragmentIndex.cpp:230`,
`CometPeptideIndex.cpp:829`) were left behind when the per-length slice sorts
were parallelized; `g_vFragmentPeptides` also grows with no `reserve`
(transient ~1.5x spike at realloc on the build's largest allocation).

---

## 5. Suggested fix order

Small, high-payoff, low-risk first:

1. **One-liners with wrong-result or crash impact — ✅ DONE (2026-08-19, see
   Section 0 for per-item detail; committed `5cc3be9b`):** B1 (delete the
   decoy `break`), B2 (`i > 0`), B3 (`iSum8 = iSum9 + i8`), C4 (add the
   missing pool guard), C9 (swap Pipeline close order), C1 (`+1` on the
   `g_bIndexPrecursors` allocation), C6 (`k >= 0` guard).
2. **Localized logic fixes:** B4, C2, C7, C8, C10 all ✅ **DONE** 2026-08-19
   (committed `5cc3be9b`); **B5 ✅ DONE 2026-08-21** (derive terminal flags
   from the header; committed `194ef3e7`) — see its Section 1 entry; **B9 ✅
   DONE 2026-08-21** (loud-error guard, not the original shared-alphabet fix
   — see B9's note on scope; committed `31b3575a`); C5's MS2 NULL-deref half
   (the sizing half is done — see step 3) still open.
3. **Param plumbing:** B10, B11, B12 all ✅ **DONE** 2026-08-19 (committed
   `5cc3be9b`); the C12 robustness cluster is also ✅ **DONE** 2026-08-19
   (audited, then fixed the same day — see Section 0/2, committed
   `5cc3be9b`); C5's speclib index sizing (only) also ✅ **DONE** 2026-08-19.
4. **`.idx` header self-description gaps — ✅ DONE 2026-08-21 (new findings,
   not in the original review; committed `194ef3e7`):** B14 (persist
   `decoy_prefix`/`num_enzyme_termini`/`allowed_missed_cleavage`/
   `clip_nterm_methionine` in the header) and B15 (PI_DB decoy
   classification read the wrong map, `g_pvProteinNames` instead of
   `g_pvProteinNameCache` — found while verifying B14; independently
   severe on its own). See their Section 1 write-ups above.
5. **Section 3 (latent/concurrency/dead-code) — ✅ DONE 2026-08-21, all 10
   items (committed `31b3575a`):** see Section 3's own entries for full
   per-item detail.
6. **C13 (operational traps) — ✅ DONE 2026-08-21, all 3 items:** see its
   Section 2 entry for full per-item detail. Only C5's MS2 NULL-deref half
   (left as-is by request — unfinished/unused feature path) remains open in
   Section 2.
7. **Perf, small mechanical sub-items — ✅ DONE 2026-08-21:** P9's unused
   MS1 memset (removed) and P8's `initCombinations` leak + merge-loop slot-15
   off-by-one (both fixed) — see their Section 4 entries. P11's missing
   `g_vFragmentPeptides.reserve()` was scoped for this same pass but turned
   out to need an accurate pre-pass upper-bound estimate (the real
   contributing paths are the nested unmodified/modified/n-term/c-term/combo
   branches in `AddFragmentsThreadProc()`, not a single count) rather than a
   one-line addition — **deferred, not done**.
8. **P2 and P3 — ✅ DONE 2026-08-21:** `AddFragments()` per-call churn (heap
   copies + redundant fill-pass mass recompute) and the FI/PI per-candidate
   full-array memsets — see their Section 4 entries for fix detail and
   verification. Both confirmed results-identical against the `v2026.02.2`
   baseline with a real, consistent speedup (FI_DB/PI_DB index build and
   search wall-clock all faster than baseline).
9. **Perf, in payoff order:** P1 → P4 → P5 → P6 → P7 (P11's `.reserve()` item
   folds in here too). **Still fully open.**

Suggested regression tests (T25-style crafted fixtures) — ✅ **DONE 2026-08-21**,
all 8 implemented as committed fixtures `T26`-`T33` in `tests/unit/run_tests.py`
(10 registered test functions; T26 and T27 each split into two):

- **T26** (`t26_b1_fasta_decoy`, `t26_b2_fi_nl_order`): phospho + `decoy_search = 2`
  FASTA-path decoy ladder and FI NL running-count order (B1/B2).
- **T27** (`t27_modcap_fasta`, `t27_modcap_fi`): 3-mod-type cap violation on
  variable-mod slots 10-15, FASTA and FI paths (B3/B4).
- **T28** (`t28_idx_cterm_mod`): `.idx`-only search with a C-term variable mod,
  confirming the header-restore path sets the terminal-mod flags (B5).
- **T29** (`t29_decoyprefix`): target-decoy PI_DB `.idx` search with the correct
  `decoy_prefix`, confirming the decoy row is classified correctly and the
  internally-generated on-the-fly reversed decoy doesn't leak into rank-1
  output at `num_output_lines = 1` (B14/B15).
- **T30** (`t30_mass_boundary`): precursor at the `digest_mass_range` upper
  boundary, exercising the C1/C2 array-sizing paths.
- **T31** (`t31_speclib_sizing`): a minimal `.msp` speclib MS2 run (C5 sizing).
- **T32** (`t32_bad_enzyme_number`): `search_enzyme_number = 99` must error
  (B11).
- **T33** (`t33_param_robustness`): a params file with a 600-char value and a
  malformed `mass_offsets` must error, not crash/hang (C10).

All 10 pass individually and as part of the full 52-test suite
(`python3 tests/unit/run_tests.py --comet <path>`), with regression-catching
power spot-verified for T26-B2 and T31 by temporarily reverting the
underlying fix and confirming the test fails. An ASan/UBSan CI leg over the
existing T21/T23 suites would still catch roughly half of section 2
automatically — that remains open.
