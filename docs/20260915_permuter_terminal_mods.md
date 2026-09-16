# Terminal variable modifications in the Modifications Permuter

Date: 2026-09-15
Branch: `ModificationsPermuter`
Status: **implemented** (Phases 0-4 landed 2026-09-15; sections 1-10 are the plan as agreed, section 11 records what was built and measured, including deviations)

## 1. Goal

Make N-terminal and C-terminal variable modifications first-class inputs to
`ModificationsPermuter`, so that the fragment index (FI_DB) and peptide index (PI_DB)
enumerate terminal mods the same way they enumerate residue mods, and add support for
*protein*-terminal variable modifications on every search path (plain FASTA, FI_DB,
PI_DB, RTS).

Motivation and history:

- Today the permuter is residue-only. Terminal mods are bolted on by the callers as a
  three-block nested loop (N-only, C-only, N x C) per residue permutation, duplicated
  verbatim in `CometFragmentIndex::AddFragmentsThreadProcRange()` and
  `CometPeptideIndex::EnumerateIndexPeptideMods()`.
- Terminal mods do not count toward `max_variable_mods_in_peptide` on the index path
  (they do on the plain-FASTA path). Protein-terminus eligibility (`iWhichTerm` /
  `iVarModTermDistance`) is unimplemented on the index path.
- The upstream permuter (vagisha/ModificationsPermuter, private, C++; local clone at
  `/mnt/c/Work/ModificationsPermuter`) added terminal-mod support across five squash-merged
  PRs on top of `1453a3c` (2023-01-06), the commit Comet's port corresponds to:
  #2 `4860e14` "Include terminal mods in modifications permutations" (2023-11-10),
  #3 `6e62e38` "Add modification rules" (2023-12-05),
  #4 `044f74b` "ModificationRule supports modifications at the protein termini" (2024-03-28),
  #5 `e1daeab` "Fix printing modified sequence for protein N/C term modifications" (2024-05-08),
  #6 `6b9f806` "Update ModificationRule and add tests" (2024-10-01, = master).
  Section 2b records how this plan reconciles with them. Per-PR diffs and exported trees
  are in the session scratchpad (`upstream/1453a3c_to_master.diff`, `pr2_*.diff` ...).
- `docs/20260714_modifications.md` section 7.3 deferred the permuter output-order rework
  until after this change; that dependency is unchanged.

## 2. Decisions (agreed 2026-09-15)

| # | Decision |
|---|---|
| D1 | **Model A**: termini are two extra positions in the permuter's bitmask, not a separate enumeration layer. Each permutation entry gains one N-term byte and one C-term byte. |
| D2 | Terminal mods **count toward `max_variable_mods_in_peptide`** on the index path (falls out of D1; matches the plain-FASTA path and the parameter's documentation). Per-mod `iMaxNumVarModAAPerMod` also counts a terminus occupied by that mod. |
| D3 | Protein-terminus eligibility is expressed by **new residue characters**: `^` = protein N-terminus only, `$` = protein C-terminus only. `n` / `c` keep meaning "any peptide terminus". Supported on the plain-FASTA path as well. |
| D4 | `variable_modNN` fields 5 (`term_distance`) and 6 (`which_term`) are **deprecated and ignored**, with warnings (section 5). A one-release bridge auto-translates the legacy protein-terminus idiom. The positive-distance residue restriction has no replacement. |
| D5 | Protein-terminus context is **folded into the permuter's terminal sentinel characters** (section 3.2). No per-peptide post-filter. The permuter stays context-free. |
| D6 | `.idx` header version bumps to **v5**. No new fields: scope travels in `szVarModChar`. v4 files are rejected with the existing loud rebuild message (no v4 read-compatibility). |
| D7 | Protein-terminus context = `cPrevAA == '-'` / `cNextAA == '-'` from the raw-peptide record. Clipped-Met peptides already record `'-'` (the digest runs the clipped pass on the shifted sequence), so no new per-peptide bit. |
| D8 | **Retire** the terminal nibbles in `VariantArray::vucTermMods` and the `cNtermMod` / `cCtermMod` chars in `FragmentPeptidesStruct`. The pool entry is the single source of truth for all mods on a variant. |
| D9 | Fix the adjacent latent gap: the FI_DB build never applies static `add_Nterm_protein` / `add_Cterm_protein` (PI_DB search does). Apply them with the same flank test, in the same accumulators. |

## 2b. Reconciliation with upstream PRs #2-#6

Verified against the clone (`master` = `6b9f806`):

| Upstream | This plan |
|---|---|
| Terminal mods are extra bit positions in the same mask: `Modifications::getModifiableSequence()` prefixes `'c'` then `'n'` sentinel chars to the modifiable sequence (they become the MSBs); `ModificationRule::getModifiedChars()` prepends `n`/`c` to the rule's residues; `getModBitmask()` matches them like residues. | **Confirms D1 / Model A.** Same mechanism. |
| Entry layout (`ModificationNumber::modifications`): `[0]` = N-term rule index, `[1]` = C-term rule index, `[2..]` = residues (sentinels stripped). Always +2, even with no terminal rules. | **Adopted** (section 3.3), with Comet's conditional stride: +2 only when `bVarTermModSearch`. |
| `MAX_GLOBAL_MODS` (=5) in `combine()` sums popcounts including the sentinel bits; a test comment cites Comet's `max_variable_mods_in_peptide` documentation as the reference semantics. | **Confirms D2.** Comet maps it to `iMaxVarModPerPeptide` (already enforced on the union popcount). |
| Protein context: `Peptide{sequence, nterm, cterm}` booleans (PR #4). Both peptide- and protein-terminal rules map to the same `'n'`/`'c'` sentinel, so dedup **loses** the context; PR #5 filters at print time with `rule->modifiesNterm(peptide)` and a code comment: "unique modifiable sequences ... do not track protein / peptide terminus information". | **Deliberate divergence, D5.** Comet folds the context into distinct sentinel chars so the dedup key carries it and no per-peptide filter is needed. Comet's context comes from the persisted flanks (D7), not booleans. |
| Dual rules (`ModificationRule("K", '*', NO_TERM, PEP_N_TERM)` -> chars `"nK"`): one mod on K and on the N-terminus. | Already expressible as `szVarModChar = "nK"`; per-mod cap covers the union (D2). |
| `residueTermini`: restrict a *residue* to a terminal position ("M only as first residue"). | This is the legacy `term_distance = 0` residue restriction, **deprecated by D4** with a warning. Not ported. |
| Per-rule `binary`, `minOccurrence`, `maxOccurrence` (PR #3/#6). | Not ported into the permuter. Comet's `iBinaryMod` is handled on the batch path; per-mod max already exists (`iMaxNumVarModAAPerMod`); min-occurrence has no Comet parameter. |
| Enumeration order unchanged by the PRs (subsets largest-first, last mod fastest, ascending masks); sentinels as MSBs mean "terminus unmodified" masks precede "terminus modified" within a mod. | Same engine in Comet; no reorder in this work (the deferred section 7.3 reorder of `docs/20260714_modifications.md` stays deferred). |

**Do not port** (upstream defects found in review):

- PR #6 changed `if (combinationCount != -1)` to `if (combinationCount > 0)` where `-1` is
  `ULLONG_MAX` (overflow sentinel); over-limit sequences are now enumerated instead of
  skipped. Comet's `FRAGINDEX_MAX_COMBINATIONS` clamp path is correct; keep it.
- `parseMods()` builds terminal rules with the 3-arg constructor, which since PR #6 sets
  `residueTermini` rather than `termini`; CLI terminal syntax produces dead rules. Irrelevant
  to Comet (params, not CLI), noted so nobody copies it.
- A rule with zero valid combinations coexisting with others reads `combinationsForModIdx[0]`
  of an empty array in the odometer (by inspection). Comet's Step 2 already truncates and
  Step 3 guards counts; re-verify with P9.

**Directly portable tests** (section 6.1 P11-P12): the `TestModifications_Combine` cases and
the `testModificationRules2` exact expected-output list (four overlapping K rules on
`MVTDSSDMK`, 15 entries). `testModificationRules` (51 entries) depends on binary and
min/max rules and a residue-terminus restriction, none of which Comet's permuter has, so it
is not portable as a whole; its terminal-mod rows are covered by P4-P7.

## 3. Design

### 3.1 Parameter semantics

`variable_modNN = <mass> <chars> <binary> <max_per_peptide> <deprecated> <deprecated> <required> <neutral_loss>`

| Char in field 2 | Meaning |
|---|---|
| `A`..`Z` | residue |
| `n` | peptide N-terminus (any peptide) |
| `c` | peptide C-terminus (any peptide) |
| `^` | protein N-terminus only (peptide with `cPrevAA == '-'`) |
| `$` | protein C-terminus only (peptide with `cNextAA == '-'`) |

`n^` is equivalent to `n` (a protein N-terminus is a peptide N-terminus). Both `n` and `^`
may appear in different slots with different masses (e.g. peptide-N-term carbamyl in one
slot, protein-N-term acetyl in another); they are distinct mods competing for one
position, resolved by the permuter's overlap rule exactly like two mods on one residue.

Per-slot flags derived at `InitializeStaticParams()` and at `.idx` header parse:
`bNtermMod` (`n` or `^`), `bCtermMod` (`c` or `$`), new `bProteinNtermOnly` (`^` present
and `n` absent), new `bProteinCtermOnly` (`$` present and `c` absent). The dead globals
`bVarProteinNTermMod` / `bVarProteinCTermMod` are repurposed to mean "some slot has `^`" /
"some slot has `$`".

### 3.2 Permuter data model (Model A + sentinels)

The modifiable sequence for a peptide becomes, when `bVarTermModSearch` is set (same
placement as upstream, both sentinels prefixed so the terminal bits are the two MSBs):

```
[N-sentinel][C-sentinel] <modifiable residues, as today>
```

Sentinels are internal, never persisted, chosen to collide with nothing in
`szVarModChar` or Comet's output grammar (proposal: `<` peptide-N, `{` protein-N,
`>` peptide-C, `}` protein-C -- final choice is an implementation detail confined to one
header constant block). The sentinel for a peptide is a two-way lookup on its flanks:

| `cPrevAA == '-'` | N-sentinel |  | `cNextAA == '-'` | C-sentinel |
|---|---|---|---|---|
| no | `<` |  | no | `>` |
| yes | `{` |  | yes | `}` |

`ALL_MODS` strings are translated before they reach the permuter: `n` -> `<{`, `^` -> `{`,
`c` -> `>}`, `$` -> `}`. `getModBitmask()` then needs no new logic. Because the sentinel is
part of the modifiable-sequence string, it is part of the dedup key: at most four context
classes per residue subsequence (194,673 unique sequences -> < 800,000 worst case on the
human phospho reference, still a few MB of `MOD_SEQS_POOL`).

Consequences inside the permuter:

- `combine()` counts terminal bits in the union popcount -> D2 for free. Two mods on one
  terminus -> overlap -> rejected -> "one N-term, one C-term per variant" stays structural.
- Step-2 per-mod filtering counts a mod's terminal bit against its `iMaxNumVarModAAPerMod`.
- A peptide with no modifiable residues but active terminal mods now has a real
  two-position modifiable sequence; the `PEPTIDE_MOD_SEQ_IDXS == -2` hook in
  `getModifiableSequences()` is removed (it becomes a normal `>= 0` index).
- `MAX_BITCOUNT` (50) and the 64-bit mask absorb two extra bits against
  `MAX_PEPTIDE_LEN` (50) without change.
- `initCombinations(maxPeptideLen, ...)` is called with `peptideLengthRange.iEnd + 2` when
  terminal search is on.

### 3.3 Pool entry layout

Fixed stride per modifiable sequence, as today, but `stride = iModSeqLen` where
`iModSeqLen` now includes the two sentinel positions when terminal search is on:

```
byte 0            : N-term slot (compacted ALL_MODS index, or -1)
byte 1            : C-term slot (compacted ALL_MODS index, or -1)
bytes 2..len-1    : residue slots, in modifiable-residue order (unchanged)
```

This is upstream's `ModificationNumber` layout. When `bVarTermModSearch` is false the layout
is exactly today's (no sentinel positions, no stride change): residue-only configurations
pay nothing (upstream always pays the +2). `GetModNumEntry()` arithmetic is unchanged. One
shared accessor replaces the retired nibble getters, and one constant
(`iTermSlotBytes = bVarTermModSearch ? 2 : 0`) is the residue offset for the five greedy
walkers, which iterate `pModSeq + iTermSlotBytes .. pModSeq + len - 1`.

Memory (human phospho reference, `docs/20260827_PI_memory.md`): pool is 42.2M entries /
498 MB today. Two bytes per entry = +84 MB (+17% of the pool, ~3-4% of index-resident
memory), and zero for residue-only configs. Entry *count* scales with the terminal
combinations exactly as the variant array already does (pool stays ~1/3 of the variant
array in every configuration). Check `MOD_NUM` (signed int) and `vuiModNumIdx` (uint32)
headroom on the largest configured index during Phase 4.

### 3.4 Callers: enumeration collapses

`AddFragmentsThreadProcRange()` and `EnumerateIndexPeptideMods()` become:

```
if (!iRequireVarMod) emit(peptide, unmodified)
for each entry in [MOD_SEQ_MOD_NUM_START, +CNT):
   if (PassesVarModProteinFilter(peptide, entry)) emit(peptide, entry)
```

The three terminal blocks and their duplicates are deleted. `AddFragments()` loses
`cNtermMod` / `cCtermMod`; it reads the two terminal bytes from the entry, translates them
via `TranslateVarModSlot()`, and folds the masses into `dBion` / `dYion` exactly where the
nibble masses are folded today (after decoy reversal of residue positions -- the
"terminal mods stay on their terminus" contract is preserved at the same point).
`ComputeIndexedPepMass()` reads the same bytes; its summation order stays bit-identical
between build and search (its header comment requires this).

D9: `AddFragments()` / `ComputeIndexedPepMass()` add `dAddNterminusProtein` when
`cPrevAA == '-'` and `dAddCterminusProtein` when `cNextAA == '-'`, matching
`SearchPeptideIndex()` / `AnalyzePeptideIndex()`.

### 3.5 Variant array and staging struct (D8)

- `FragmentPeptidesStruct`: drop `cNtermMod`, `cCtermMod`. Stays 24 bytes (alignment);
  update the `static_assert` comment. Fill passes obtain terminal slots from the entry.
- `VariantArray::vucTermMods` -> `vucFlags`; only `DECOY_FLAG` (bit 7) remains meaningful.
  `GetNtermMod()` / `GetCtermMod()` removed. Shrinking the byte to a bitset is out of
  scope (separate memopt).
- FI transcode (`CometFragmentIndex.cpp:462-467`) and PI transcode
  (`CometPeptideIndex.cpp:1202`) write only the decoy flag.
- Search-time consumers (`SearchFragmentIndex()` boundary resolution and candidate loop,
  `SearchPeptideIndex()` -> `MaterializeOneEntry()`) read terminal slots from the entry.
  `piVarModSites[iLen]` / `[iLen+1]` codes become `translated_slot + 1`, the same
  convention residues already use; the raw-slot `+1` special case disappears.

### 3.6 Plain-FASTA path (D3, D4)

- `HasVariableMod()`: replace the `iVarModTermDistance` / `iWhichTerm` decision table with
  flank/position tests: a `^` mod is eligible iff the peptide starts at protein position 0
  of the (possibly clipped) sequence; `$` iff it ends at the last residue. `n` / `c`
  unconditionally eligible.
- `VariableModSearch()` `piVarModCountsNC[]`: add 1 per eligible terminus using the same
  tests (replacing the `iVarModTermDistance < 0` branch and the distance branches).
- `MergeVarMods()`: unchanged in shape (already rejects two mods on one terminus).
- Static protein-terminal masses: already applied via `iStartPos == iFirstResiduePosition`;
  unchanged.

### 3.7 `.idx` header (D6)

- Version line: `Comet index database v5.`; `ParsePeptideIndexHeader()` literal updated,
  v4 rejected with the existing rebuild message.
- `VariableMod:` line unchanged in shape (five fields per slot). Reader adds `strchr` for
  `^` and `$` and sets the new per-slot flags alongside `bNtermMod` / `bCtermMod` /
  `bVarTermModSearch`.
- Fields 5/6 are not persisted (never were).

### 3.8 Output writers

- pepXML `<terminal_modification ... protein_terminus="Y|N">` can now be filled correctly
  from the per-slot protein flags; mzIdentML location likewise. Text output is unchanged
  (mass annotations at `n` / `c` positions as today).
- AScorePro still skips PSMs with any terminal variable mod (`CometPostAnalysis.cpp:262-278`,
  RTS mirrors). Unchanged by this plan; more terminal-mod PSMs => lower AScore coverage.
  Measured in T19/T20/T35, called out in the release notes.

## 4. Phases

Five phases, each a separate commit on `ModificationsPermuter`. Every phase's exit criteria
include the standing gate: Linux `make` and the Windows Release x64 solution build both finish with zero errors,
`tests/unit/run_tests.py` passes against both binaries, and `CometUnitTests` passes. Test IDs
refer to section 6. Phase order is fixed by dependencies: 0 is independent of the permuter;
1 changes the permuter without changing any index; 2 wires the index to it; 3 and 4 follow.

### Phase 0 -- parameters and plain-FASTA support for `^` / `$`; deprecate fields 5 and 6

**Goal.** Users can declare protein-terminal variable mods with `^` and `$` and get correct
plain-FASTA results; legacy `term_distance` / `which_term` usage is translated or warned per
section 5. **Not in scope:** the index path. FI_DB/PI_DB keep ignoring protein scope and
keep the `n`/`c` loops; a `^` mod on the index path behaves like `n` until Phase 2, and the
Phase 0 exit criteria pin that as "unchanged", not "correct".

**Steps** (in order):

1. `CometSearch/CometData.h` `struct VarMods`: add `bool bProteinNtermOnly`, `bool
   bProteinCtermOnly` (default false); document `iVarModTermDistance` / `iWhichTerm` as
   deprecated-and-ignored in the struct comment.
2. `CometSearch/CometSearchManager.cpp` `InitializeStaticParams()`: after the existing
   `'n'`/`'c'` scan, add the `'^'`/`'$'` scan setting `bNtermMod`/`bCtermMod`, the two new
   flags, `bVarTermModSearch`, and `bVarProteinNTermMod`/`bVarProteinCTermMod` (repurposed
   per section 3.1). Implement the section 5 case table here: bridge translation (`n`->`^`,
   `c`->`$` for distance 0 + which-term 0/1) and every warning, emitted once per slot via
   `logout`. Make sure the slot-merge/dedup of `szVarModChar` (`:1416-1451`) treats `^`/`$`
   like any other char.
3. `CometSearch/CometSearch.cpp` `HasVariableMod()`: replace the distance/which-term table
   with: `n`/`c` always eligible; `^` eligible iff `iStartPos == 0` of the (possibly clipped)
   protein sequence; `$` iff `iEndPos == _proteinInfo.iTmpProteinSeqLength - 1`.
4. `CometSearch.cpp` `VariableModSearch()` `piVarModCountsNC[]`: +1 per eligible terminus
   using the same three tests, replacing the `iVarModTermDistance < 0` branch and the
   distance branches. Leave `MergeVarMods()` alone (already rejects two mods on one
   terminus) but confirm `^`/`$` slots reach it through `bNtermMod`/`bCtermMod`.
5. `Comet.cpp` params template: add `^`/`$` to the field-2 description; label fields 5 and 6
   "deprecated, ignored"; keep the 8-field parser unchanged. `CometWriteParams.cpp`: same text.
6. `CometWritePepXML.cpp` / `CometWriteMzIdentML.cpp`: emit `protein_terminus="Y"` (pepXML)
   / the equivalent location for slots with `bProteinNtermOnly`/`bProteinCtermOnly`. Audit
   every writer that echoes `szVarModChar` into output for a place `^`/`$` would surprise
   (section 10 item).
7. Tests: add fixture `t37_protein_nterm.fasta` + `.ms2` (one protein whose N-terminal
   tryptic peptide also occurs internally in a second protein; two spectra: acetylated
   N-terminal form, unmodified internal form); add T37 (plain-FASTA) and T41 (deprecation
   table) to `run_tests.py`; extend `legacy_cases.build_params` if the mod-string helper
   rejects `^`/`$`.

**Exit criteria.**

- T37 passes on plain FASTA: `^` acetyl identifies the N-terminal spectrum with the acetyl,
  does not put an acetyl on the internal-peptide spectrum; the same params with `n` acetyl
  identifies both.
- T41 passes: legacy `n 0 3 0 0` params give results identical to `^` params and the log
  contains the bridge warning; `K 0 3 0 0` and `M 0 3 2 0` produce their respective warnings.
- All four T21 terminal-mod legacy cases pass **byte-identically** (their `n`/`c` mods use
  distance -1, so nothing changes for them).
- Index path provably unchanged: rebuild every `tests/unit/data/*.fasta.idx` and confirm
  `compare_idx.py` reports identical to the committed fixtures; T28 passes unmodified.
- Full unit suite green on both binaries.

**Deliverable.** One commit: "Protein-terminal variable mods (`^`, `$`) on the plain-FASTA
path; deprecate variable_mod term_distance / which_term". No index behavior change.

### Phase 1 -- permuter: terminal sentinel positions (index path still disabled)

**Goal.** `ModificationsPermuter` can emit terminal-aware permutations (Model A, section
3.2/3.3) and has its first unit-test suite; FI_DB/PI_DB keep producing byte-identical
indexes because they do not yet ask for the new behavior. **Not in scope:** any consumer
of the pool.

**Steps** (in order):

1. `CometModificationsPermuter.h`: new constant block for the four sentinel chars (final
   choice made here, section 10); `static const int TERM_SLOT_BYTES = 2`; declare
   `IGNORED_SEQ_CNT` `extern` (per `docs/20260714_modifications.md` 7.5).
2. New helper `TranslateModCharsForPermuter(const char* szVarModChar) -> string` mapping
   `n`->both N sentinels, `^`->protein-N sentinel, `c`->both C sentinels, `$`->protein-C
   sentinel, residues unchanged. Unit-tested by P3.
3. `getModifiableSequences()`: add a `bool bIncludeTermini` parameter. When true, prefix
   `[N-sentinel][C-sentinel]` chosen from the raw peptide's `cPrevAA`/`cNextAA` (section
   3.2 table) and treat a peptide with no modifiable residues as having a 2-char modifiable
   sequence; remove the `-2` sentinel (`PEPTIDE_MOD_SEQ_IDXS` is `-1` or a real index). When
   false, behave exactly as today.
4. `generateModifications()` / `combine()`: no algorithmic change; verify by P4-P8 that
   terminal bits flow through Step 1-3 and the `iMaxVarModPerPeptide` popcount. Add the
   zero-combination guard from section 2b (P9).
5. `initCombinations()` callers: width `peptideLengthRange.iEnd + TERM_SLOT_BYTES` when
   `bIncludeTermini`.
6. `CometFragmentIndex::PermuteIndexPeptideMods()`: pass `bIncludeTermini = false` (explicit,
   with a comment pointing at Phase 2) and translate `ALL_MODS` through step 2's helper
   (a no-op for residue-only strings, so the index is unchanged).
7. Tests: new `tests/unit/TestModificationsPermuter.cpp` with P1-P12; add it to
   `CometUnitTests.vcxproj` and the Linux unit-test build; golden file for P10 committed
   under `tests/unit/data/`.

**Exit criteria.**

- P1-P12 pass (P11/P12 are the ported upstream cases).
- Every `tests/unit/data/*.fasta.idx` rebuilt is `compare_idx.py`-identical to the
  committed fixture; T17, T18, T22, T28 pass unchanged (index path untouched).
- `IGNORED_SEQ_CNT` unchanged on the phospho reference build vs. Phase 0 binary
  (`COMET_MEMREPORT` + build log).

**Deliverable.** One commit: "ModificationsPermuter: terminal positions as bitmask
sentinels (disabled for index builds); first permuter unit tests".

### Phase 2 -- index path consumes terminal permutations; v5 header; D8, D9

**Goal.** FI_DB and PI_DB enumerate terminal mods through the permuter, protein scope is
honored at build time via the sentinels, terminal mods count toward the cap, the nibbles and
staging chars are gone, static protein-terminal masses are applied on FI_DB, and `.idx` is
v5. **Not in scope:** any change to enumeration *order* beyond what deleting the loops
implies; memory optimizations of the flags byte.

**Steps** (in order -- the index is inconsistent until 1-9 are all in, so they land as one
commit; the order is for review and for bisecting within the working tree):

1. `core/Types.h`: add the entry accessor (`ModEntryTermSlots(entry) -> {n, c}` reading
   bytes 0/1) and `g_iTermSlotBytes` (0 or 2, set once in `PermuteIndexPeptideMods()`).
   Update the layout comment above `MOD_NUMBERS_POOL`.
2. Convert the five greedy walkers to start at `pModSeq + g_iTermSlotBytes` /
   `mods + g_iTermSlotBytes`: `CometFragmentIndex.cpp` ladder (`:925-938`) and
   `ComputeIndexedPepMass()` (`:708-735`), `CometPeptideIndex.cpp` `tryPush` (`:658-677`) and
   `MaterializeOneEntry()` (`:903-938`), `CometSearch.cpp` FI search (`:1827-1841`). With
   `g_iTermSlotBytes == 0` this is a no-op; confirm fixtures still identical before step 3.
3. `AddFragments()` and `ComputeIndexedPepMass()`: drop `cNtermMod`/`cCtermMod` parameters;
   read terminal slots from the entry via the accessor, `TranslateVarModSlot()` them, fold
   masses into `dBion`/`dYion` at the existing point (after residue reversal for decoys).
   Add D9: `dAddNterminusProtein` when `cPrevAA == '-'`, `dAddCterminusProtein` when
   `cNextAA == '-'`, in both functions, same summation position, so build and search stay
   bit-identical.
4. `AddFragmentsThreadProcRange()`: delete the three terminal blocks and their per-permutation
   copies; the loop becomes section 3.4's shape. `FragmentPeptidesStruct`: remove the two
   chars; update the `static_assert` comment. Fill passes (`GenerateFragmentIndex()`
   `:366-375`, `:414-424`) read nothing terminal; transcode (`:462-467`) writes only
   `DECOY_FLAG`.
5. `VariantArray`: rename `vucTermMods` -> `vucFlags`; delete `GetNtermMod()`/`GetCtermMod()`;
   fix every compile error that surfaces (these are exactly the remaining nibble readers).
6. `CometPeptideIndex.cpp`: `EnumerateIndexPeptideMods()` loses its terminal blocks;
   `tryPush` counts sites from the entry; `MaterializeOneEntry()` sets
   `pcVarModSites.set(iLen, n+1)` / `set(iLen+1, c+1)` from translated entry slots; PI
   transcode (`:1202`) writes only flags.
7. `CometSearch.cpp`: `SearchFragmentIndex()` boundary resolution (`:1650`, `:1695`) and
   candidate loop (`:1783-1877`) use the accessor; terminal codes become
   `translated_slot + 1`. `SearchPeptideIndex()` / `AnalyzePeptideIndex()` likewise.
   `CometPostAnalysis.cpp`: verify no raw-slot assumption remains for terminal sites.
8. `.idx` header: writer emits `v5`; `ParsePeptideIndexHeader()` literal -> `v5`, plus
   `'^'`/`'$'` scans setting the protein flags; `CometSearch::InitializeMassesFromPeptideIndex()`
   and `CometSearchManager.cpp:1652` callers unchanged.
9. `PermuteIndexPeptideMods()`: pass `bIncludeTermini = bVarTermModSearch`; set
   `g_iTermSlotBytes` accordingly. Guard: if `bVarTermModSearch` and any terminal mod lives
   in a slot `>= FRAGINDEX_VMODS`, warn (existing limitation, now visible).
10. Fixtures: regenerate every `tests/unit/data/*.fasta.idx` (v5); freeze one pre-regen copy
    as `t43_v4.fasta.idx`. Add T38, T39, T40, T42, T43; add a `^`-acetyl params variant to
    `tests/rts_repro/` for T22.

**Exit criteria.**

- T38: `^` acetyl found on the N-terminal spectrum and absent on the internal spectrum, in
  both FI_DB and PI_DB; header reads `Comet index database v5` and shows `^` in `VariableMod:`.
- T39: cap 2 with STY + `n` acetyl does **not** identify the 2-phospho+acetyl spectrum on
  FI_DB, PI_DB or plain FASTA; cap 3 identifies it on all three.
- T40: internal-decoy PSMs carry the acetyl on the (unreversed) N-terminus; T34 counts hold.
- T42: FI_DB identifies the protein-N-terminal peptide with `add_Nterm_protein = 42.0106`
  and no variable mods, mass agreeing with plain FASTA and PI_DB to 1e-6.
- T43: a v4 file fails with the rebuild message.
- T28 passes after regen; T17/T18/T22 pass; T22's new `^` variant is 1-thread/8-thread
  byte-identical.
- Grep gate (section 9 item 7): no `cNtermMod`, `cCtermMod`, `GetNtermMod`, `GetCtermMod`
  in `CometSearch/`.
- Residue-only phospho reference: `COMET_MEMREPORT` pool size and `MOD_NUM` identical to
  Phase 1; `.txt` output byte-identical to the Phase 1 binary.

**Deliverable.** One commit: "FI_DB/PI_DB: terminal and protein-terminal variable mods via
ModificationsPermuter; count toward max_variable_mods_in_peptide; .idx v5; apply static
protein-terminal masses on FI_DB". Release-note paragraph drafted in the commit body.

### Phase 3 -- documentation and cleanup

**Goal.** Every place the repo describes the old model describes the new one. **Not in
scope:** code changes other than comment fixes.

**Steps.** `docs/DataStructures.md` (entry layout, `vucFlags`), `docs/GlobalVariables.md`
(`MOD_NUMBERS_POOL` stride, removed `-2`), `CLAUDE.md` Key Globals row for
`g_fragmentPeptides` (nibble text -> flags), `comet-codebase` skill, params documentation
for `^`/`$`, this document's "as built" appendix (deviations from plan, measured numbers),
`docs/20260714_modifications.md` 7.3 note that the prerequisite has landed.

**Exit criteria.** `grep -rn "nibble\|cNtermMod\|vucTermMods" docs/ CLAUDE.md .claude/`
returns only historical references marked as such; a reviewer can follow section 3 of this
document against the code with no stale line numbers in the doc's file table.

**Deliverable.** One commit: "Docs: terminal-mod permuter as built".

### Phase 4 -- validation at scale

**Goal.** Confirm no PSM-quality, runtime, or memory regression on real data, and confirm
the new parity that this work is supposed to create. **Not in scope:** fixing anything
found -- findings go back to Phase 2 as a follow-up commit.

**Steps.**

1. T23 / T24 / T24b as they exist (residue-only configs) against `v2026.02.2`
   (`--integration --bigdata`, Windows baseline `comet.win64.exe` for a Windows current
   binary, Linux for Linux).
2. New configs on the comet-debug3 data, current binary only (baseline lacks `^`): plain vs
   FI_DB vs PI_DB with `n` acetyl (cap 3) and with `^` acetyl (cap 3).
3. `COMET_MEMREPORT` on the phospho reference (`20260420-human-phosho/comet.params`) and on
   the `n`-acetyl config, Phase 0 binary vs Phase 2 binary.
4. Headroom check: on the largest index that builds on this machine, log `MOD_NUM` and
   `g_fragmentPeptides.size()` against `INT_MAX` / `UINT32_MAX`.

**Exit criteria** (thresholds inline; these are section 9 items 3-5):

- Step 1: PSM counts within 10% of baseline, same-version parity within 5%, every
  `_check_timing` ratio <= 1.25 (re-run once before calling a timing failure real).
- Step 2: plain / FI_DB / PI_DB within 5% of each other at 1% FDR for both configs. Today's
  code would fail the `^` config (FI_DB over-identifies); passing it is the point of D5.
- Step 3: residue-only pool bytes and `MOD_NUM` identical between binaries; `n`-acetyl pool
  <= 2.5x the residue-only pool (section 3.3 projection: 2x entries x 14.4/12.4 stride).
- Step 4: both counters below 50% of their type limit, or a documented plan to widen.

**Deliverable.** Numbers appended to this document's "as built" appendix; no code commit
unless a finding requires one.

## 5. Deprecation behavior (fields 5 and 6)

Implemented once in `InitializeStaticParams()`; batch and index build share it. Text goes
to `logout` at startup and is repeated in the index-build summary.

| Field 5, field 6 | Field 2 | Behavior |
|---|---|---|
| -1, any | any | silent (default) |
| 0, 0 | contains `n` | bridge: `n` -> `^`; warn naming the replacement |
| 0, 1 | contains `c` | bridge: `c` -> `$`; warn naming the replacement |
| 0, 2 or 0, 3 | contains `n`/`c` | no-op; warn "deprecated fields ignored" |
| 0, any | residues in field 2 | warn: legacy "residue at protein position 0" restriction dropped; now applies anywhere. For mixed `nK 0 3 0 0`, say both halves. |
| > 0, any | any | warn: distance constraint dropped |

Warnings, never errors. The bridge is removed after one release; the warning is
permanent (distance 0 stays non-default). The eight-field form keeps parsing.

## 6. Test matrix

Existing IDs run to T36; new harness tests start at T37. C++ permuter tests are new
`TEST_F` suites in `tests/unit/` (add a `TestModificationsPermuter.cpp` to
`CometUnitTests.vcxproj` and the Linux test make target).

### 6.1 C++ unit tests (first-ever permuter coverage)

| ID | What | Expected |
|---|---|---|
| P1 | `getModifiableSequences()` on 4 peptides sharing residues `CMK`, flanks (int,int), (-,int), (int,-), (-,-) | 4 distinct mod-seqs differing only in sentinels; residue part identical |
| P2 | Same with `bVarTermModSearch == false` | 1 mod-seq, no sentinels, stride == today (byte-identical pool to a pre-change fixture) |
| P3 | `ALL_MODS` translation: `n` -> `<{`, `^` -> `{`, `c` -> `>}`, `$` -> `}`, `n^` == `n` | exact strings |
| P4 | Entries for `<CMK>` with mods {M x1, n} , cap 2 | set == {M}, {n}, {n,M}; terminal byte at 0, residues at 1..3, byte 4 == -1 |
| P5 | Same with cap 1 | {M}, {n} only (D2) |
| P6 | Two N-term mods (`n` and `^`) on `{...}` peptide | never both on byte 0; on `<...>` peptide the `^` mod never appears |
| P7 | Both-termini single peptide `{K}` with `^` acetyl and `$` amide, cap 2 | {^}, {$}, {^,$}; cap 1 -> no {^,$} |
| P8 | Per-mod cap: `nK` max 1 on `<KK>` | {n}, {K1}, {K2}; never {n,K} |
| P9 | Overflow guards: 50-residue all-modifiable peptide + both termini, popcount 52 | rejected via `MAX_BITCOUNT` path, `IGNORED_SEQ_CNT` increments, no crash |
| P10 | Entry order for a fixed input is stable across two runs and matches a golden list | golden file in `tests/unit/data/` (this is what T18 determinism rests on) |
| P11 | **Ported from upstream `testModificationRules2`**: mods `M`, `K`, `K`, `K` (three distinct K mods), peptide `MVTDSSDMK`, no termini | exactly 15 entries in upstream's order: `{M,K1}` x3, `{M,K2}` x3, `{M,K3}` x3, `{M}` x3, `{K1}`, `{K2}`, `{K3}`; no entry with two mods on the same K |
| P12 | **Ported from upstream `TestModifications_Combine`** (with `[0]`=N, `[1]`=C layout): `MSMMK` masks {20,8,1} -> `{-1,-1,1,3,1,-1,2}`; `ncMSMMK` masks {64,32,20,8,1} -> rejected (6 > cap 5); `nAACLCFR` {128} -> `{4,-1,...}`; `cAACLCFR` {128} -> `{-1,5,...}`; `nMVEDASIK` {128,4,256} -> `{4,-1,1,-1,-1,-1,-1,3,-1,-1}`; `MSMMK` masks {20,22,1} -> rejected (overlap) | byte-exact entries; rejections return false and emit nothing |

### 6.2 Harness tests (`run_tests.py`)

| ID | Path | What | Assert |
|---|---|---|---|
| T21 (existing 4 term-mod cases) | plain FASTA | unchanged legacy behavior with `n`/`c` distance -1 | still pass byte-identically |
| T28 (existing) | FI_DB | C-term mod from `.idx` header | pass after fixture regen (D2 does not affect it: 1 mod) |
| T37 | plain FASTA | new fixture: protein with N-term peptide `A...K` + identical internal peptide; `^` acetyl | top hit for the N-term spectrum carries acetyl; internal-peptide spectrum does **not**; `n` acetyl variant of the params finds both |
| T38 | FI_DB + PI_DB | same fixture/params as T37 built into `.idx` | same two assertions in both index modes; `.idx` header says `v5`, `VariableMod:` shows `^` |
| T39 | FI_DB vs plain | `max_variable_mods_in_peptide = 2`, STY phospho + `n` acetyl, spectrum matching 2 phospho + acetyl | **not** identified on either path (D2); with cap 3 identified on both |
| T40 | FI_DB, internal decoys | T34 fixture + `^` acetyl | decoy PSMs keep acetyl on the N-terminus (reversed sequence, terminal site unchanged); target/decoy counts as T34 |
| T41 | params | legacy `n 0 3 0 0` params file | search results identical to `^` params; warning text present in log; `K 0 3 0 0` warns "restriction dropped"; `M 0 3 2 0` warns "distance dropped" |
| T42 | FI_DB | `add_Nterm_protein = 42.0106` static, no variable mods (D9) | N-term peptide identified with correct mass on FI_DB, PI_DB and plain; previously FI_DB missed it |
| T43 | v4 rejection | search against a committed v4 `.idx` (existing `t*.fasta.idx` fixtures before regen, kept as `t43_v4.fasta.idx`) | clean failure with rebuild message |
| T18 (existing) | determinism | two builds byte-identical | unchanged |
| T22 (existing) | RTS | 1-thread vs 8-thread byte-identical | unchanged; add a `^` acetyl variant of the rts_repro params |
| T17 (existing) | count stability | no-enzyme len 8-13 | unchanged (residue-only, stride unchanged) |

Fixtures to regenerate: every `tests/unit/data/*.fasta.idx` (v5 header). Fixtures to add:
`t37_protein_nterm.fasta` (+ `.ms2` with an N-terminal-acetyl spectrum and an internal
spectrum), `t39_cap.ms2`, `t43_v4.fasta.idx` (frozen copy of a current v4 file).

### 6.3 Integration / big data (Phase 4)

- T23/T24/T24b as today (residue-only): PSM parity and timing vs `v2026.02.2` unchanged.
- New configs (not gated on baseline parity, since the baseline lacks `^`): plain vs FI_DB
  vs PI_DB with `n` acetyl, and with `^` acetyl, each within 5% at 1% FDR of each other.
- `COMET_MEMREPORT` before/after for the phospho reference and for the `n`-acetyl config.

## 7. Files touched (expected)

| File | Phase | Change |
|---|---|---|
| `Comet.cpp` | 0 | params template text; keep 8-field parse |
| `CometSearch/CometSearchManager.cpp` | 0 | flag derivation, bridge, warnings |
| `CometSearch/CometData.h` | 0 | `VarMods`: `bProteinNtermOnly`, `bProteinCtermOnly` |
| `CometSearch/CometSearch.cpp` | 0, 2 | `HasVariableMod`, `VariableModSearch`; FI/PI search consumers |
| `CometSearch/CometWritePepXML.cpp`, `CometWriteMzIdentML.cpp` | 0 | `protein_terminus` |
| `CometSearch/CometModificationsPermuter.{h,cpp}` | 1 | sentinels, translation, width, `-2` removal |
| `CometSearch/core/Types.h` | 2 | `FragmentPeptidesStruct`, `VariantArray`, entry accessor |
| `CometSearch/CometFragmentIndex.{h,cpp}` | 2 | enumeration, `AddFragments`, `ComputeIndexedPepMass`, fills, transcode, D9 |
| `CometSearch/CometPeptideIndex.{h,cpp}` | 2 | enumeration, materialize, transcode, header v5 |
| `CometSearch/CometPostAnalysis.cpp` | 2 | read terminal sites via new convention (if any raw-slot assumption remains) |
| `tests/unit/TestModificationsPermuter.cpp` (new), `CometUnitTests.vcxproj`, Linux test target | 1 | P1-P10 |
| `tests/unit/run_tests.py`, `tests/unit/data/*` | 0-2 | T37-T43, fixture regen |
| `docs/*.md`, `CLAUDE.md`, `.claude/skills/comet-codebase` | 3 | as-built |

## 8. Risks and mitigations

- **Silent behavior change on the index path (D2).** Variants at the residue cap with a
  terminal mod disappear. Mitigation: release note; T39 pins the new semantics; users raise
  the cap by one.
- **Enumeration order changes posting lists.** Deleting the terminal loops reorders
  emission; the mass sort is not stable. T18/T22 only require self-consistency, which the
  new order provides; no byte-compat with pre-change `.idx` is claimed (v5 anyway).
- **Five walkers, one layout.** Any walker missing the `+1` offset silently mis-assigns
  mods. Mitigation: one shared accessor/iterator, P4 pins byte positions, T38 covers FI and
  PI, T22 covers RTS.
- **Pool growth on term-mod configs.** Proportional to the variant array (section 3.3);
  Phase 4 measures it. `MOD_NUM` / `vuiModNumIdx` limits checked there.
- **AScore coverage drop** where terminal-mod PSMs increase. Measured, documented, not
  fixed here.
- **Deprecation surprises.** Section 5 bridge + permanent warning; T41.
- **Upstream divergence.** Upstream keeps protein context out of the dedup key and filters
  at print time (section 2b). Comet deliberately diverges (D5) so the index never enumerates
  ineligible variants; semantics and tests are lifted, structure is not.

## 9. Acceptance criteria

1. All existing harness tests pass on Linux and Windows builds; `CometUnitTests` 100%.
2. P1-P10 and T37-T43 pass.
3. T24-style parity (plain vs FI_DB vs PI_DB) holds within 5% with `n` acetyl and with
   `^` acetyl configs.
4. No `_check_timing` regression on T23/T24 vs `v2026.02.2`.
5. `COMET_MEMREPORT`: residue-only phospho reference pool size unchanged; `n`-acetyl config
   pool within the section 3.3 projection.
6. Every `.idx` writes `v5`; a v4 file fails with the rebuild message.
7. No terminal-mod loop remains in `CometFragmentIndex.cpp` or `CometPeptideIndex.cpp`;
   `GetNtermMod` / `GetCtermMod` / `cNtermMod` / `cCtermMod` no longer exist.

## 10. Open items (historical)

- ~~Upstream source access~~ -- resolved 2026-09-15 via the local clone; see section 2b.
- **Sentinel character choice** (section 3.2): any four non-letter printable characters
  absent from `szVarModChar`'s grammar; decide at Phase 1.
- **Output formatting** of `^`/`$` mods in text/pepXML (Phase 0): confirm no writer echoes
  `szVarModChar` characters into user-visible strings where `^`/`$` would surprise.

## 11. As built (2026-09-15)

Phases 0-2 landed on `ModificationsPermuter` in three commits (`0ff2805b`, `3a821bb7`,
`73de1373`); this section records where the implementation deviated from, or decided
things left open by, sections 1-9.

**Decided during implementation**

- **Sentinel characters** (3.2): `<` peptide N-term, `{` protein N-term, `>` peptide C-term,
  `}` protein C-term (`ModificationsPermuter::TERM_*`). Both sentinels are always prefixed
  when terminal search is on, so the entry layout is `[N][C][residues...]` (upstream's), and
  `combine()` needed no change.
- **Phase 1 bisectability** (4): landed with `bIncludeTermini = false` in the index driver,
  as planned; index identity was proven against the Phase 0 binary (81/81 `.idx` files
  across three mod configs, identical phospho-reference permutation ledger).
- **Shared raw-peptide rows -- option C plus static-mass row split (2026-09-15/16).** The
  raw-peptide table normally holds one row per unique sequence, so a peptide that is
  protein-terminal in one protein and internal in another shares a row. Two mechanisms keep
  that exact:
  - *Mass.* A shared row's stored mass must be correct for every occurrence, and it is not
    when `add_Nterm_protein`/`add_Cterm_protein` is non-zero (D9 folds those statics into the
    stored mass for `-` flanks). The digest and merge therefore key rows on
    `PepRowSplitClass(cPrev, cNext, add_Nterm_protein, add_Cterm_protein)` (`core/Types.h`):
    occurrences whose protein-terminal static differs land in separate rows with separate
    masses; when both protein-terminal statics are zero (the common case) the class is always
    0 and the table is one row per sequence as before. The union-with-mass-adjustment step
    from the first option C commit remains as a safety net; with the split in place it never
    changes a stored mass.
  - *Attribution.* Every protein occurrence in `ProteinsListCSR` carries three context bits
    (`PROT_NTERM_HERE` 0x01, `PROT_CTERM_HERE` 0x02, `PROT_BOTH_TERM_HERE` 0x04 = this one
    occurrence is the whole protein; `PepOccurrenceContext()`; one byte per occurrence,
    persisted in the v5 protein-list section). A protein that holds the peptide more than once
    keeps one reference whose byte is the OR of its copies, so within-protein dedup keys
    include the full context and a sequence repeated at both termini of one protein records
    N|C -- but not 0x04, which the OR never fabricates. The bits are used twice through one
    rule (`ProteinsListCSR::flagsSatisfy()`): at build time an entry whose terminal slots are
    protein-scoped is emitted only if some occurrence has the matching bits -- when `^` and
    `$` are both set, an occurrence carrying 0x04 -- and at output time (`GetProteinNameString()`, the mzIdentML per-PSM
    list, the RTS result path) a PSM's protein list is filtered to the occurrences that
    support its protein-scoped terminal mods.

  Result: FI_DB/PI_DB attribute `^`/`$` PSMs exactly as the plain-FASTA path does (T46), a
  peptide N-terminal in one protein and C-terminal in another no longer carries both, and a
  shared peptide never scores with a protein-terminal static it does not carry in that
  protein (T42 pins the attribution set against plain FASTA; T50 pins the on-disk context
  bytes). Cost: one byte per protein occurrence (~5 MB on the phospho reference), one
  `hasContext()` scan per protein-scoped entry at build time, and extra rows only when a
  protein-terminal static is configured. Consumers doing protein inference on N-terminal
  acetylation are the reason this was worth doing now, while v5 was still unreleased.
- **Index-format compatibility story** (D6, tightened with option C). Three cases, each with
  a test: a **v4** file fails at the header literal with the rebuild message (T43); a **v5**
  file whose protein-list rows lack the per-occurrence context bytes -- the layout written by
  this branch between the v5 bump and option C, never released -- fails deterministically
  because `ReadPeptideIndex()` now requires the protein-list walk to end exactly at the
  footer (T48 forges such a file from a fixture); a current **v5** file round-trips through
  build, write, reload and search with exact shared-peptide attribution (T46). Old binaries
  refuse v5 by the header literal. There is no partial-acceptance path: every section is
  bounded by the footer pointers and the reader validates counts, offsets and the section
  end before anything is used.
- **Indexed mzIdentML protein references (pre-existing, fixed 2026-09-16).** The `.mzid`
  writer's tmp file carried `g_pvProteinsList` row values for FI_DB/PI_DB searches, and its
  second pass seeked the `.idx` by those values as if they were FASTA byte offsets -- since
  rows hold name-section ordinals after a load, every indexed `.mzid` came out with garbage
  accessions ("Comet", "omet", ...). `PrintTmpPSM()` now walks every target and decoy bucket
  (filtered by the PSM's protein-scoped terminal context, with the same FI_DB fallback as
  `GetProteinNameString()`), and `ResolveTmpProteinName()` resolves an indexed reference
  through `g_pvProteinNameCache`. T47 part 3 checks FI_DB/PI_DB internal-decoy `.mzid` output
  end to end (resolvable evidence, real accessions, exact `^` attribution).
- **FI_DB search-time ladder (D9 completed, 2026-09-16).** D9 put the static protein-terminal
  masses into the FI_DB build's fragment ladders and into `ComputeIndexedPepMass()`, but
  `SearchFragmentIndex()`'s own b/y ladder -- the one XCorr scores -- still started from the
  peptide-terminal constants plus the variable terminal mods only. A candidate with a `-` flank
  and a non-zero `add_Nterm_protein`/`add_Cterm_protein` was therefore retrieved from the right
  bins but scored against a shifted ladder (T42's PSM: xcorr 3.46 on FI_DB vs 5.36 on plain FASTA
  and PI_DB). The flank statics are now added there too, from the raw-peptide row's flanks, so
  they apply to internal decoys of the variant as well. T42 asserts xcorr equality across the
  three paths for both termini.
- **mzIdentML `SearchModification` for `^`/`$`** declares the variable mass only. The pre-branch
  writer folded `add_Nterm_protein`/`add_Cterm_protein` into that `massDelta`; since the static is
  its own `fixedMod="true"` block and the per-PSM `<Modification>` carries the variable mass,
  that double-counted. pepXML's `<terminal_modification mass=...>` keeps the total terminal mass
  (its `massdiff` is the variable mass), which is that format's convention.
- **Plain-FASTA bug found by T42**: `MergeVarMods()` rebuilt the precursor mass from scratch
  adding only `dAddCterminusProtein`; variable-mod peptides at the protein N-terminus with
  `add_Nterm_protein != 0` were reported (and mass-checked) short by that amount. Fixed in
  Phase 2 alongside D9.
- **`AddFragments()` hardening check** now expects the flank statics in the stored mass;
  before D9 any `add_Nterm_protein` above 10 Da (acetyl included) would have warned on every
  protein-N-terminal peptide.
- **Phase 0 deprecation bridge** also handles `term_distance < -1` (the undocumented `-2`
  "not on the C-terminal residue" special case) with a warning; that check is removed.
- **Zero-combination guard** (2b): a mod whose per-mod max is 0 is dropped in Step 1.
- **`MAX_BITCOUNT`** widened by `TERM_SLOT_BYTES`; `initBinomialCoefficients()` width too.

**Tests as built**

- P1-P13 in `tests/unit/TestModificationsPermuter.cpp` (P10 is a run-twice pool identity
  check rather than a golden file; P11 pins exact order). `std::max`/`std::min` must be
  parenthesized there because `windows.h` defines `max`/`min`.
- T37, T41 (Phase 0); T38, T39, T40, T42, T43 (Phase 2); `t22_rts_{fi,pi}_protterm`
  (integration-gated RTS determinism with a `^` acetyl) instead of extending T22 in place.
- Review follow-ups: T42 covers both `add_Nterm_protein` and `add_Cterm_protein`; T44
  (big-data n/^/$ parity, `--integration --bigdata`), T45 (mixed terminal codes on every
  path), T46 (shared-peptide attribution), T47 (pepXML/mzIdentML terminal annotations,
  single declarations for mixed slots, indexed internal-decoy `.mzid` evidence), T48
  (forged flags-less v5 rejected), T49 (deprecation-bridge edge
  cases: legacy `term_distance`/`which_term` values that do and do not map to `^`/`$`), T50
  (per-occurrence context bytes read back from the `.idx`, including a sequence repeated at
  both termini of one protein and a shared peptide across proteins), T51 (`print_ascorepro_score`
  with `^`/`$` configured: AScorePro is handed only the residue letters of each mod, a pure
  protein-terminal mod registers nothing, and results match with AScorePro on and off).
- Both CI workflows run `CometUnitTests` and the harness (`run_tests.py` without
  `--integration`) after the build.
- T40 uses the T37 fixture with `decoy_search = 1` rather than the T34 fixture.
- Committed `.idx` fixtures regenerated as v5; `tests/unit/data/t43_v4.fasta.idx` is a
  frozen v4 copy for T43.

**Measured**

- Phospho reference (current `20260420-human-phosho/comet.params`, M x3 + STY x3, 8 threads):
  194,673 modifiable sequences, 72,881,595 permutation entries, `MOD_NUMBERS_POOL` 866.8 MB
  -- larger than section 3.3's 498 MB figure because the reference params have grown since
  the memory doc; identical between the Phase 0 and Phase 1 binaries.
- Run counts: `CometUnitTests` 71 (59 + 12); `run_tests.py` 65 on both Linux and Windows.

**Phase 4 -- validation at scale (2026-09-15, Linux build, 8 threads, this machine)**

Full `--integration --bigdata` run: **73 passed, 0 failed** (58 unit + T17, T18, T22 x4,
T23, T24, T24b, T44, and the Phase 0-2 additions). Against the `v2026.02.2` Linux release:

| Check | Result |
|---|---|
| T23 internal-decoy / target-decoy PSMs at 1% FDR | 17,660 / 17,660; baseline 17,701 / 17,660 |
| T23 search time ratio (current / baseline) | 1.006 / 0.987 |
| T24 plain / FI_DB / PI_DB PSMs at 1% FDR | 17,660 / 17,736 / 17,660; baseline identical |
| T24 time ratios: plain search, FI build, FI search, PI build, PI search | 0.987, 1.001, 0.560, 0.985, 0.547 |
| T24b internal decoys plain / FI_DB / PI_DB | 17,701 / 17,717 / 17,701 |
| T44 `n` acetyl (cap 3) plain / FI_DB / PI_DB | 17,654 / 17,734 / 17,654 (ratios 1.005 / 1.000) |
| T44 `^` acetyl (cap 3) plain / FI_DB / PI_DB | 17,770 / 17,831 / 17,770 (ratios 1.003 / 1.000) |
| T44 `$` amidation (cap 3) plain / FI_DB / PI_DB | 17,632 / 17,720 / 17,632 (ratios 1.005 / 1.000); added with option C, unchanged by it for `n`/`^` |
| T22 RTS 1-vs-8 threads, FI_DB and PI_DB, with and without decoys | byte-identical, 197 spectra |
| T22b RTS with `^` acetyl, FI_DB / PI_DB | byte-identical; 1 / 2 of 197 results carry the acetyl |
| T18 two builds of human.small.fasta | byte-identical |

The `^` acetyl config is the one that would have failed before Phase 2 (FI_DB/PI_DB ignored
protein scope); it now agrees with plain FASTA to 0.3%.

`COMET_MEMREPORT` on the phospho reference (`human.canonical.target-decoy.fasta`, M x3 +
STY x3, cap 5, `scan_range 1 300` so only the index load and a short search are timed),
Phase 0 binary (v4 `.idx`) vs. Phase 2 binary (v5 `.idx`):

| Config | Binary | Modifiable seqs | Pool entries | `MOD_NUMBERS_POOL` | FI variants | Peak RSS |
|---|---|---|---|---|---|---|
| residue-only | Phase 0 | 194,673 | 72,881,595 | 866.8 MB | 1.675e8 | 24.1 GB |
| residue-only | Phase 2 | 194,673 | 72,881,595 | 866.8 MB | 1.675e8 | 24.1 GB |
| + `n` acetyl | Phase 0 | 194,673 | 72,881,595 | 866.8 MB | 3.317e8 | 45.9 GB |
| + `n` acetyl | Phase 2 | 210,462 | 137,630,817 | 1,858.1 MB | 3.198e8 | 45.2 GB |

- Residue-only: identical ledger and variant count, as required.
- `n` acetyl: pool 2.14x the residue-only pool (projection was <= 2.5x); the sentinel
  contexts add 8% unique modifiable sequences; the variant count drops 3.6% because
  combinations at the residue cap plus the acetyl now exceed `max_variable_mods_in_peptide`
  (D2), and peak RSS is 0.7 GB lower. Every peptide now has a modifiable sequence
  (5.007e6 vs 4.457e6 "modifiable peptides").
- Headroom on the largest config here: `MOD_NUM` 137.6M of `INT_MAX` (6.4%);
  FI variants 3.2e8 of `UINT32_MAX` (7.4%). Both well under the 50% threshold.

Windows (VS 2026 build) cross-check: T44 gives the identical PSM counts -- `n` acetyl
17,654 / 17,734 / 17,654 and `^` acetyl 17,770 / 17,831 / 17,770 for plain / FI_DB / PI_DB --
and T23 / T24 / T24b pass with the same counts as on Linux (index builds ~11 s, searches
6-9 s on this machine).

**Acceptance criteria (section 9): all seven met.**
