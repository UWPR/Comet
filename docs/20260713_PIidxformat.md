# PI_DB index build: reusing FI_DB's peptide-generation and mod-permutation code

Status: PLANNING ONLY. No implementation yet.

## 1. Request

PI_DB's `.idx` build (`Comet.exe -D<fasta> -j`, `CometPeptideIndex::WritePeptideIndex()`) currently
generates its fully modified+unmodified peptide list by calling `CometSearch::RunSearch()` in
"build mode", which digests every protein through the legacy `DoSearch()` path and pushes one
`DBIndex` entry at a time (heap-allocated `pcVarModSites` per entry) under a global mutex into
`g_pvDBIndex`. This is the same code path used for ordinary FASTA_DB searches, not optimized for
bulk index construction.

FI_DB's `.idx` build (`-i`, `CometFragmentIndex::WriteFIPlainPeptideIndex()`) solves the same
"digest the whole proteome" problem far more cheaply: per-thread, per-length, in-memory, dedup'd,
no heap allocation per peptide instance, no lock contention.

Goal: make PI_DB's build reuse FI_DB's fast digestion and mod-permutation code so that:
1. The unmodified peptide list is generated once, per unique peptide (not per protein occurrence),
   using FI_DB's fast per-thread path.
2. Modifications are then permuted onto that deduplicated list.
3. The final combined (unmodified + all modified variants) list is sorted by mass and written to
   the PI_DB `.idx` file, exactly as today's `WritePeptideIndex()` writes it -- same on-disk format,
   same downstream `ReadPeptideIndexEntry()` reader, no format change.

## 2. Current PI_DB build path

`CometPeptideIndex::WritePeptideIndex()` sets `g_staticParams.options.bCreatePeptideIndex = true`,
calls `CometSearch::RunSearch(0, 0, tp, emptyQueries)`, which drives `DoSearch()` per protein. Inside
the shared digestion loop (`CometSearch.cpp:2749`, `while (iStartPos < iLenProtein)`), the branch at
line 2756:

```cpp
if ((g_staticParams.options.bCreatePeptideIndex && !g_staticParams.variableModParameters.iRequireVarMod)
   || g_staticParams.options.bCreateFragmentIndex)
```

gates entry into "index build mode" for this peptide window. Inside that, line 2768:

```cpp
if (g_staticParams.options.bFastPlainPeptideIdx && g_staticParams.options.bCreateFragmentIndex)
   { /* fast per-thread PepGenTuple path -- FI_DB only, see section 3 */ }
else
   { /* legacy path: lock g_pvDBIndexMutex, push one DBIndex, unlock -- PI_DB today */ }
```

PI_DB always takes the `else` (legacy) branch because `bFastPlainPeptideIdx` is only ever set
alongside `bCreateFragmentIndex`. Each `DBIndex` push happens once per protein occurrence (later
deduplicated by a final sort+unique pass in `WritePeptideIndex()`, which is where this session's
earlier I/L-aware dedup fix -- `ILAwarePeptideCompare` -- was added).

Full modification enumeration for PI_DB currently happens through the general-purpose
`VariableModSearch()` / `CompoundModSearch()` machinery inside this same digestion pass (the
existing engine that also serves ordinary FASTA_DB searches), which is why it already supports:
- Full `VMODS` = 15 variable-mod slots (`core/Constants.h:62`).
- Compound mods (`uiNumCompoundMasses` / J-residue combinatorics, `CometSearch.cpp:2741`,
  `:8394 CompoundModSearch()`).
- PEFF variant analysis is explicitly **excluded already** for `bCreatePeptideIndex`
  (`CometSearch.cpp:6825`: `if (*bDoPeffAnalysis && !g_staticParams.options.bCreatePeptideIndex)`),
  so PEFF is not a parity concern -- PI_DB's build already ignores PEFF variants today.

## 3. FI_DB's build path (the code being borrowed)

`CometFragmentIndex::WriteFIPlainPeptideIndex()` has two distinct phases:

### Phase A -- `GeneratePlainPeptideIndex(tp, slices)` (CometFragmentIndex.cpp:615-863)

Temporarily sets `bCreateFragmentIndex=true; bFastPlainPeptideIdx=true;` and calls the *same*
`RunSearch()` -> digestion loop, but takes the fast branch at line 2768. That branch:
- Packs each peptide window into a compact tuple: `PepGenTupleShort` (5-bit-packed `uint64_t`,
  length <= 12) or `PepGenTuple` (plain string, length >= 13) -- see `core/Types.h:301-413`.
- Dedups **within each protein** via a per-thread hash set (`_seenShort` / `_seenLong`), with
  I/L canonicalization already folded into the dedup key (`PackPeptide(..., bIL)` / manual `L->I`
  folding).
- Pushes into per-length, per-thread buffers (`g_vvvPepGenShort[len][thread]`,
  `g_vvvPepGenLong[len][thread]`) -- no lock, no per-peptide heap allocation.
- **Silently skips any window containing `*`** (stop codon) via `memchr(...)`.
- **Does not special-case `J` residues.** `PackPeptide` can encode `J` as an extended 5-bit code
  (`core/Types.h:312-316`, codes 21-26 for B/J/O/U/X/Z), so a peptide containing `J` is packed and
  stored like any other peptide, but `CompoundModSearch()` is never called anywhere in
  `CometFragmentIndex.cpp` (confirmed: zero references). **FI_DB does not enumerate compound-mod
  mass variants at all today** -- a J-containing peptide's mass reflects only whatever
  `dCalcPepMass` computed inline (no compound-mass combinatorics).

After digestion, `GeneratePlainPeptideIndex` does a parallel per-length sort+dedup+protein-merge
(across all threads this time), producing globally unique peptides in `g_pvDBIndex` (`DBIndex`
elements) / `g_pvProteinsList` (flat CSR), then a parallel per-length-slice mass sort.

Doc comment on the function: *"Populate g_pvDBIndex and g_pvProteinsList from the FASTA database
using the fast per-thread PepGenTuple path... Post-conditions: g_pvDBIndex: unique peptides, sorted
by mass...; g_pvProteinsList: ... one row per unique peptide."*

**This is the exact same pair of globals (`g_pvDBIndex`, `g_pvProteinsList`) that PI_DB's own build
already populates and writes today** -- meaning Phase A is a near-direct drop-in for PI_DB's
"unmodified peptide list" step. (Compound mods are not a gap here -- see section 6.)

### Phase B -- `PermuteIndexPeptideMods()` + write (CometFragmentIndex.cpp:87-147, :1078+)

After the protein-list section of the file is written, `PermuteIndexPeptideMods(g_vRawPeptides)`
computes mod-placement **combinatorics metadata** -- not a fully enumerated list:
- Builds `ALL_MODS` / `vMaxNumVarModsPerMod` from `varModList[0 .. FRAGINDEX_VMODS)`, i.e. **only the
  first 5 of PI_DB's 15 variable-mod slots** (`FRAGINDEX_VMODS = 5`, `core/Constants.h:59`, comment:
  *"only parse first five variable mods for fragment ion index searches"*).
- Calls `ModificationsPermuter::initCombinations`, `getModifiableSequences` (fills `MOD_SEQS`,
  `PEPTIDE_MOD_SEQ_IDXS`), `getModificationCombinations` (fills `MOD_NUMBERS`, an array of
  `ModificationNumber{ modStringLen, char* modifications }`, plus `MOD_SEQ_MOD_NUM_START` /
  `MOD_SEQ_MOD_NUM_CNT` ranges).
- Combination count per modifiable sequence is capped by `FRAGINDEX_MAX_COMBINATIONS` (2000,
  `core/Constants.h:34`) and `FRAGINDEX_MAX_MODS_PER_MOD`.

`WriteFIPlainPeptideIndex()` then writes `MOD_SEQS` / `MOD_NUMBERS` / `MOD_SEQ_MOD_NUM_START` /
`MOD_SEQ_MOD_NUM_CNT` / `PEPTIDE_MOD_SEQ_IDXS` to the plain-peptide `.idx` file as **compact
combinatorial tables** -- never a materialized list of (peptide, specific mod placement) pairs.

**The actual enumeration -- walking every valid mod combination per peptide -- happens only at
search-time load**, inside `GenerateFragmentIndex()` -> `AddFragmentsThreadProc()` ->
`AddFragments()` (CometFragmentIndex.cpp:247-597), which:
- Adds the unmodified variant (unless `iRequireVarMod`).
- Adds n/c-term-only mod combos if `bVarTermModSearch`.
- For `modNumIdx` in `[MOD_SEQ_MOD_NUM_START[seqIdx], +MOD_SEQ_MOD_NUM_CNT[seqIdx])`, calls
  `AddFragments(..., modNumIdx, ...)`, which reconstructs the peptide string, computes
  `dCalcPepMass` (residue masses + `MOD_NUMBERS[modNumIdx].modifications[j]` wherever it applies +
  n/c-term mod mass), validates against `g_massRange`, and -- **only during the `bCountOnly=1`
  sizing pass** -- pushes a lightweight `FragmentPeptidesStruct{ iWhichPeptide, modNumIdx, dPepMass,
  cNtermMod, cCtermMod }` (12-16 bytes, no explicit per-position mod array, no full peptide string)
  into `g_vFragmentPeptides`. The rest of `AddFragments` computes b/y fragment ion bins, which is
  fragment-index-specific and irrelevant here.

This is re-run **every time the FI_DB `.idx` is loaded for a search** (confirmed by this session's
own benchmark output: `- generate fragment ion index`, `- sort peptides by mass`, `- populate
index` print on every RTS/batch startup, costing ~1-2 s for a ~3.7M-peptide plain index). That cost
is acceptable for FI_DB because the expensive part (FASTA digestion + protein-level dedup) already
happened once at `-i` build time; combination-walking over an already-deduplicated peptide list is
cheap and its output (fragment bins) can't be persisted compactly anyway.

## 4. The core architectural mismatch

FI_DB **defers** full (peptide x mod-combination) enumeration to search-time load and never
persists it. PI_DB's `.idx` format **requires** each entry to carry an explicit, fully materialized
mod-site encoding (`cNumMods` + `(position, whichMod)` pairs, written once at build time) because
PI_DB's search path does no on-the-fly re-permutation -- it just reads entries and mass-searches
them.

So this cannot be "call FI_DB's functions instead of DoSearch()". It is:

- **Phase A is reusable close to as-is**: `GeneratePlainPeptideIndex()` already produces exactly the
  globals PI_DB needs for its unmodified-peptide step. Needs: relaxing the gate at
  `CometSearch.cpp:2768` so the fast path also fires for `bCreatePeptideIndex` (not just
  `bCreateFragmentIndex`). No compound-mod fallback is needed (see section 6) -- confirmed the
  legacy path doesn't actually produce compound-mod index entries either.
- **Phase B's combinatorics generation is reusable**: `ModificationsPermuter` /
  `PermuteIndexPeptideMods`'s table-building is legitimate, reusable machinery for "what are the
  valid mod placements on this modifiable substring" -- capped at `FRAGINDEX_VMODS = 5`, which is
  the accepted scope for PI_DB's new build path (decided in section 6).
- **Phase B's terminal action needs new code**: nothing in `CometFragmentIndex.cpp` materializes a
  full modified-peptide entry with an explicit `pcVarModSites` array -- `AddFragments()` builds
  fragment bins, not `DBIndex`-style entries. A new function is needed, structurally mirroring
  `AddFragmentsThreadProc()`'s combination-walking loop, that for each `(iWhichPeptide, modNumIdx)`
  pair:
  1. Computes `dCalcPepMass` (same logic as `AddFragments()`, minus the b/y ion ladder).
  2. Translates `MOD_NUMBERS[modNumIdx].modifications` / `MOD_SEQS[seqIdx]` into PI_DB's explicit
     per-position `pcVarModSites` encoding (position index + which-mod-slot byte per modified
     residue).
  3. Appends a `DBIndex`-equivalent entry (peptide index reference, mass, mod sites) to a list.
  4. After all peptides x combinations are walked, sort the combined (unmodified + modified) list
     by mass and hand it to the existing `WritePeptideIndex()` file-writing tail (unchanged) --
     same on-disk format, same `ReadPeptideIndexEntry()`.

## 5. Feature-parity gaps -- assessed

| Gap | Today (PI_DB, `DoSearch()` path) | FI_DB code being borrowed | Resolution |
|---|---|---|---|
| Variable mod slots | Full `VMODS` = 15 | `PermuteIndexPeptideMods` only reads `varModList[0..FRAGINDEX_VMODS)` = first 5 | **Accepted**: cap PI_DB's new build path at 5 variable-mod slots too, matching FI_DB (decided in section 6, question 1) |
| Compound mods (J residues) | `CompoundModSearch()` is *called* during digestion (`CometSearch.cpp:3298`, `:8394`), but its only output path (`CometSearch.cpp:8481`, `while (iWhichQuery < _pQueries->size())`) requires a non-empty query list -- index builds call `RunSearch()` with an empty query list, so it never executes. **Verified: zero compound-mod entries reach `g_pvDBIndex` in the current PI_DB build.** | Zero references to `CompoundModSearch`/J-handling anywhere in `CometFragmentIndex.cpp` | **No gap** -- FI_DB's fast path already matches PI_DB's actual current (non-)behavior for J-containing peptides. Confirmed in section 6, question 2. |
| PEFF variants | Already excluded for `bCreatePeptideIndex` today (`CometSearch.cpp:6825`) | Also not handled in FI_DB path | No gap -- already consistent |
| `iRequireVarMod` (require_variable_mod=1) | When set, PI_DB's digestion loop **skips the shared index-build branch entirely** (`!iRequireVarMod` in the line-2756 gate) -- unmodified peptides are presumably never stored, only fully-required-mod variants via a different path | `AddFragmentsThreadProc` explicitly checks `iRequireVarMod` before adding the bare unmodified variant, and still adds it to `g_vRawPeptides`/enumeration unconditionally upstream | **Needs new logic** (section 6, question 3): the new build path generates the unmodified peptide via Phase A unconditionally (no `iRequireVarMod` gate exists in `GeneratePlainPeptideIndex`), then must suppress emitting the unmodified-only entry (and any peptide with zero modifiable sites) at Phase B time when `iRequireVarMod` is set |
| Max combinations per peptide | No documented cap in the legacy path (bounded only by `VariableModSearch`'s own recursion limits) | Capped at `FRAGINDEX_MAX_COMBINATIONS` = 2000 per modifiable sequence | Possible behavior change for peptides with many modifiable residues + many mod types -- likely fine in practice but should be validated during implementation, not assumed |

## 6. Open questions -- answered

1. **Variable-mod slot count**: **Decided -- cap at 5, matching FI_DB.** The new PI_DB build path
   will reuse `PermuteIndexPeptideMods`/`ModificationsPermuter` unchanged, limited to the first
   `FRAGINDEX_VMODS` (5) variable-mod slots, same as FI_DB. Any PI_DB build configured with more
   than 5 active variable-mod slots will need a validation check (out of scope question: should
   this be a hard error, or a warning that mods 6-15 are ignored for the index build -- flag as a
   follow-up decision when implementation starts).

2. **Compound mods**: **Confirmed not actually supported in PI_DB's build today -- verified in
   code, not just inferred.** `CompoundModSearch()` (`CometSearch.cpp:8394`) is called during index
   digestion (`CometSearch.cpp:3298`, gated only by `bProteinHasJ && iJcount > 0`, with no
   `bCreatePeptideIndex` check), but its *only* output mechanism is the loop at
   `CometSearch.cpp:8481`: `while (iWhichQuery < (int)_pQueries->size())`, which drives
   `CheckMassMatch()` -> `XcorrScore()`/`StorePeptide()` against real query spectra. Index building
   calls `RunSearch(0, 0, tp, emptyQueries)` with an **empty** query list, so `_pQueries->size() ==
   0` and this loop never executes. Unlike the main digestion loop, which has an explicit
   `g_pvDBIndex.push_back(sEntry)` branch for index-build mode
   (`CometSearch.cpp:2829-2866`), `CompoundModSearch()` has **no equivalent branch** -- there is no
   code path by which a compound-mod (J-residue) peptide variant can ever reach `g_pvDBIndex`
   during a PI_DB build. Compound mods are therefore **not a real feature-parity gap**: FI_DB's fast
   path (which also never calls `CompoundModSearch`) already matches PI_DB's actual current
   behavior for J-containing peptides (base mass only, no compound variants). No fallback/hybrid
   logic is needed for this.

3. **`iRequireVarMod` semantics**: still needs explicit handling in the new Phase B enumeration
   function -- generate the unmodified peptide via Phase A as usual, but at Phase B, exclude the
   unmodified-only entry AND drop any peptide with zero valid mod combinations entirely from the
   final output when `require_variable_mod = 1`, matching today's effective behavior.

4. **.idx format**: no format change -- same `WritePeptideIndex()` write tail, same
   `ReadPeptideIndexEntry()` reader.

## 7. Expected benefit (qualitative -- needs benchmarking once implemented)

- Digestion cost drops from "per protein occurrence, one mutex-guarded heap-allocated `DBIndex`
  push" to "per-thread, per-length, lock-free, in unique-peptide space" -- same class of win already
  measured for FI_DB's own `-i` build.
- Mod permutation is applied once per **unique deduplicated peptide** instead of once per **protein
  occurrence of a peptide**, which is a substantial additional win for proteomes with high sequence
  redundancy (paralogs, isoforms, multi-copy genes) since today's legacy path re-permutes mods
  separately for every occurrence before the final dedup pass.
- Deduplication (with I/L canonicalization) now happens **before** mod permutation, at the raw-
  peptide level, using FI_DB's already-correct `PackPeptide(..., bIL)` / canonical-string dedup. This
  makes the `ILAwarePeptideCompare` fix added earlier this session (item #8 in
  `docs/20260713_PIopt.md`) effectively redundant for peptide-level dedup post-conversion (dedup
  would already be I/L-correct before mods are ever applied) -- worth keeping as a defensive second
  layer, but no longer load-bearing for correctness.

## 8. Proposed phased plan

1. **Phase A wiring -- DONE.** No gate change was actually needed at `CometSearch.cpp:2768`:
   `GeneratePlainPeptideIndex()` unconditionally forces `bCreateFragmentIndex = true` for the
   duration of its internal `RunSearch()` call, so the existing gates at `CometSearch.cpp:2756` and
   `:2768` already take the fast path regardless of the caller's actual `bCreatePeptideIndex`/
   `iRequireVarMod` state -- that part of the original plan was based on an incomplete read of the
   gating logic. The one real fix was `GeneratePlainPeptideIndex()`'s flag save/restore: it
   hardcoded `iDbType = FI_DB` on exit, which is only correct for its current FI_DB caller (both
   FI_DB's and PI_DB's build entry points actually start with `iDbType == FASTA_DB`, verified via
   `CometSearchManager.cpp:2004` and `:2011`). Changed to save/restore the caller's actual
   `iDbType`/`bCreateFragmentIndex` values instead of hardcoding.

   Added a temporary, opt-in (env var `COMET_VALIDATE_FAST_PI_GEN`) validation block inside
   `CometPeptideIndex::WritePeptideIndex()` (`CometPeptideIndex.cpp`, after the legacy `RunSearch()`
   call) that swaps `g_pvDBIndex`/`g_pvProteinsList`/`g_pvProteinNames` aside, calls
   `GeneratePlainPeptideIndex()` from the real PI_DB build context, logs a count comparison, checks
   `iDbType` was restored, then swaps the legacy results back -- so production `-j` output is
   byte-for-byte unchanged unless the env var is set. Verified against three fixtures:
   - `t1_basic.fasta` (no protein duplication): fast=6, legacy raw=6 -- matches T1's known-correct
     count.
   - `t16_crosspath.fasta` (2 identical proteins, so every peptide has 2 raw occurrences): fast=21
     (already deduped), legacy raw=42 (pre-dedup, one entry per protein occurrence) -- expected
     mismatch by design; the final **written** count for both paths is 21, matching T16's expected
     count.
   - `t5_enzyme.fasta` (real trypsin digestion): fast=5, legacy raw=5 -- matches.

   No `iDbType`/flag-restore warnings were logged in any run. Full unit suite
   (`tests/unit/run_tests.py`, 19 tests including T20's real `-j` PI_DB batch-search regression)
   passes unchanged, confirming the validation hook has zero effect on production behavior when the
   env var is unset (the default for every existing test and real invocation).

2. **Phase B combinatorics**: reuse `PermuteIndexPeptideMods`/`ModificationsPermuter` as-is, capped
   at `FRAGINDEX_VMODS = 5` (decided -- no generalization needed).
3. **New Phase B enumeration function**: write the new function (working name
   `MaterializeIndexPeptideMods` or similar) that walks `MOD_SEQ_MOD_NUM_START/CNT` per peptide
   (mirroring `AddFragmentsThreadProc`'s loop structure) and produces full `DBIndex`-equivalent
   entries with explicit `pcVarModSites`, honoring `iRequireVarMod` (section 5).
4. **Wire into `WritePeptideIndex()`**: replace the `RunSearch()`-based generation call with Phase A
   + Phase B, keep the existing sort-by-mass + file-write tail unchanged.
5. **Validation**: run the full unit suite (`tests/unit/run_tests.py --integration`), plus
   `compare_idx.py` against the legacy-path output for at least one test FASTA with variable mods
   configured (>5 slots configured should be flagged, not silently truncated -- add a build-time
   warning/error if `iRequireVarMod` or active var mods exceed `FRAGINDEX_VMODS`), and one with
   `require_variable_mod=1`. Confirm peptide counts and mass sums match (or differ only in the
   documented, expected way -- e.g. I/L canonicalization) between old and new build paths.
