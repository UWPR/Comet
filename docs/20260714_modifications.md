# FRAGINDEX_MAX_COMBINATIONS and MAX_BITCOUNT: status and considerations

Status: **IMPLEMENTED** (raising both ceilings, section 6) **+ PROPOSED** (section 7, not yet
implemented). Both caps have been raised (`MAX_BITCOUNT` to 50, effectively disabling it;
`FRAGINDEX_MAX_COMBINATIONS` to 10,000) -- see section 6. Companion to
docs/20260713_PIidxformat.md (the PI_DB-reuses-FI_DB's-peptide-generation project), which is
where these two caps were discovered to matter for PI_DB's new build path. Sections 1-5 below
describe the caps' original (pre-change) behavior and the investigation that led to the change;
section 6 covers what was actually implemented and its measured effect. Section 7 evaluates a
further behavior change -- generate modification combinations up to `FRAGINDEX_MAX_COMBINATIONS`
and truncate, instead of today's all-or-nothing skip described in section 2 -- and lays out an
implementation and testing plan for it. That change has not been made yet. Section 7.3 additionally
evaluates the complexity-ordering question for which combinations to keep when truncating; the
decision there (Option C, simplest-to-most-complex by total mod count with mod-type-breadth
tiebreak) is deferred until after a prerequisite change adds N-terminal/C-terminal modification
support to `ModificationsPermuter`.

## 1. What they are and where they live

Both are limits inside `ModificationsPermuter` (`CometSearch/CometModificationsPermuter.cpp`/`.h`),
the combinatorics engine that computes which variable-mod placements are valid for a given peptide.
It was written for FI_DB's fragment-ion-index build (`CometFragmentIndex::PermuteIndexPeptideMods`,
called from `WriteFIPlainPeptideIndex`) and, as of docs/20260713_PIidxformat.md's Phase B, is now
also called from PI_DB's build path (`CometPeptideIndex::MaterializeIndexPeptideMods`, via
`WritePeptideIndex`). Both callers share the exact same combinatorics code, so both are subject to
the exact same two caps.

| | `FRAGINDEX_MAX_COMBINATIONS` | `MAX_BITCOUNT` |
|---|---|---|
| Definition | `#define FRAGINDEX_MAX_COMBINATIONS 2000` (`core/Constants.h:34`) | `unsigned int MAX_BITCOUNT = 24;` (`CometModificationsPermuter.cpp:46`) |
| Kind | Compile-time constant (`#define`) | Runtime global variable (not even a `#define` -- could be changed at runtime, though nothing currently does) |
| What it bounds | Number of valid mod-placement combinations for a peptide's modifiable-sequence class | Number of candidate (modifiable) residue positions for a single mod type within a peptide |
| Gated by | `ModificationsPermuter::ignorePeptidesWithTooManyMods()` (`CometModificationsPermuter.cpp:71`), which returns `FRAGINDEX_KEEP_ALL_PEPTIDES == 1` -- i.e. both caps are only enforced when that flag is 1, which it always currently is (`core/Constants.h:36`) | Same gate |

Naming note: `FRAGINDEX_KEEP_ALL_PEPTIDES`'s comment says *"1 = consider up to
FRAGINDEX_MAX_COMBINATIONS of peptides; 0 = ignore all mods for peptide that exceed
FRAGINDEX_MAX_COMBINATIONS"* (`core/Constants.h:36`), and the gating function is named
`ignorePeptidesWithTooManyMods()`. Read literally, `FRAGINDEX_KEEP_ALL_PEPTIDES=1` sounds like it
should mean "don't drop anything," and a function named `ignorePeptidesWithTooManyMods()` returning
`true` sounds like it should mean "yes, drop over-limit peptides." In practice, per section 2 below,
setting `FRAGINDEX_KEEP_ALL_PEPTIDES=1` is what **enables** both caps (i.e. it's what causes
over-limit peptides to be silently dropped), and `FRAGINDEX_KEEP_ALL_PEPTIDES=0` would disable both
caps entirely (no over-limit checking at all -- untested; see section 4). This is confusing enough
that it's worth flagging for any future reader before they infer the wrong direction from either
name alone.

## 2. Verified behavior: both are all-or-nothing skips, not truncation

The `FRAGINDEX_MAX_COMBINATIONS` doc comment and variable naming (and this project's own
docs/20260713_PIidxformat.md, in an earlier draft) suggested it "truncates" a peptide's
combinations down to at most 2000. **This is incorrect.** Tracing the actual code and confirming
with a targeted test shows both caps are strict all-or-nothing thresholds: if a peptide's candidate
count or combination count for a given mod type exceeds either limit, that peptide's modifiable
class produces **zero** modified entries -- not a truncated subset up to the limit.

### Code trace

`ModificationsPermuter::generateModifications()` (`CometModificationsPermuter.cpp:478-529`, "Step
1") is called once per unique modifiable-sequence (the ordered list of a peptide's candidate
residues across all active mod types). It sets the "no mods" sentinel up front:

```cpp
*ret_modNumStart = -1;
*ret_modNumCount = 0;
```

then loops over each active mod type (`m`, e.g. first M-oxidation, then STY-phospho). For each mod
type with at least one candidate residue in the peptide:

```cpp
if (ignorePeptidesWithTooManyMods() && bitCount > MAX_BITCOUNT)      // :509
{
   IGNORED_SEQ_CNT++;
   return;                                                            // whole function exits
}

int combinationCount = CombinatoricsUtils::getCombinationCount(int(bitCount), vMaxNumVarModsPerMod[m]);  // nCk + ... + nC1

if (ignorePeptidesWithTooManyMods() && combinationCount > FRAGINDEX_MAX_COMBINATIONS)   // :517
{
   IGNORED_SEQ_CNT++;
   return;                                                            // whole function exits
}
```

Both are bare `return` statements inside a `void` function whose only output (`*ret_modNumStart`,
`*ret_modNumCount`) was already set to the "nothing generated" sentinel before the loop began. A
`return` here does not `continue` to the next mod type or degrade gracefully -- it exits
`generateModifications()` immediately, abandoning **every** mod type for this peptide's modifiable
sequence, including ones that individually would have been well under either limit. The caller
(`CometFragmentIndex::AddFragmentsThreadProc` and, since Phase B,
`CometPeptideIndex::MaterializeIndexPeptideMods`) sees `MOD_SEQ_MOD_NUM_START[modSeqIdx] == -1` and
skips modified-variant generation for that peptide entirely (`CometFragmentIndex.cpp:311`,
`CometPeptideIndex.cpp:355`); only the unmodified variant (generated independently, upstream of this
code) survives.

A third enforcement point, `ModificationsPermuter::getTotalCombinationCount()`
(`CometModificationsPermuter.cpp:386-405`), checks the **combined** combinatorics across all
subsets of active mod types (e.g. "M alone," "STY alone," "M and STY together") after Step 1 has
already passed for each mod type individually -- it also returns a sentinel (`-1`) that causes the
entire generation step to be skipped (`CometModificationsPermuter.cpp:616-618`) if either any
individual subset's product, or the grand total across all subsets, exceeds
`FRAGINDEX_MAX_COMBINATIONS`. Same all-or-nothing outcome, just checking a different (combined)
count.

There is a truncation-*looking* code path later in `generateModifications()`
(`CometModificationsPermuter.cpp:550-551`: `combinationsForModArrLen = FRAGINDEX_KEEP_ALL_PEPTIDES
&& calculatedCombinationsCount > FRAGINDEX_MAX_COMBINATIONS ? FRAGINDEX_MAX_COMBINATIONS :
calculatedCombinationsCount;`) and a matching break inside the combination-generation loop
(`:684-687`: `if (FRAGINDEX_KEEP_ALL_PEPTIDES && totalModNumCount + modNumCalculated >=
FRAGINDEX_MAX_COMBINATIONS) break;`). **Both appear to be structurally unreachable** given the
guards above: by the time either runs, the code has already established (via the Step 1 and
`getTotalCombinationCount` checks) that the relevant count does not exceed
`FRAGINDEX_MAX_COMBINATIONS`, so the truncation branch of the ternary and the break condition can
never actually trigger. This is corroborated by the defensive `cout << "ERROR: calculated
combination count exceeds FRAGINDEX_MAX_COMBINATIONS..."` right above the ternary
(`CometModificationsPermuter.cpp:541-547`), which was never observed to print during any test in
this investigation (and structurally cannot, for the same reason). Not fully proven dead by
execution tracing/coverage tooling -- this is a reading-based conclusion -- but consistent with
every empirical result gathered.

### Empirical confirmation

Built a tiny synthetic FASTA (three all-serine "proteins" of length 20, 23, and 30, no enzyme,
length range 20-30, so no-enzyme sliding windows over these homopolymers produce exactly one
distinct peptide per length 20-30 -- 11 unique unmodified peptides) with a single active mod
(`variable_mod01 = 79.966331 STY 0 3 -1 0 0 0.0`, i.e. STY phospho, max 3 per peptide, default
`FRAGINDEX_MAX_COMBINATIONS=2000` and `MAX_BITCOUNT=24`). For a peptide of length `L` (all S, so
`L` candidate sites), the theoretical combination count at max-3-mods is `C(L,1)+C(L,2)+C(L,3)`:

| L | candidates | C(L,1)+C(L,2)+C(L,3) | vs. `MAX_BITCOUNT`=24 | vs. `FRAGINDEX_MAX_COMBINATIONS`=2000 | combos generated (predicted, all-or-nothing) |
|---|---|---|---|---|---|
| 20 | 20 | 1,350 | under | under | 1,350 |
| 21 | 21 | 1,561 | under | under | 1,561 |
| 22 | 22 | 1,793 | under | under | 1,793 |
| 23 | 23 | 2,047 | under | **over** | 0 |
| 24 | 24 | 2,324 | under (not `>`24) | **over** | 0 |
| 25-30 | 25-30 | 2,625-5,425 | **over** | over | 0 |

Predicted total peptides = 11 unmodified + (1,350+1,561+1,793) modified = **4,715**. Actual
`comet.exe -j` output: `created: cap.fasta.idx (4715 peptides)`. Exact match, confirming: (a) both
caps are strict all-or-nothing thresholds with no partial/truncated middle ground, and (b) the
`MAX_BITCOUNT` boundary is `bitCount > 24` (so exactly 24 candidates is fine; 25 is not).

## 3. Why this matters differently for FI_DB vs. PI_DB

FI_DB's fragment index is rebuilt from the compact "plain peptide index" file (which stores only
unmodified peptides plus the combinatorics *tables*, not enumerated modified peptides -- see
docs/20260713_PIidxformat.md section 3) every time a search process starts. Silently dropping a
pathologically-modifiable peptide's fragment-ion contributions there is a deliberate, documented
size/speed trade-off for an index that gets thrown away and regenerated on every load; a peptide
lost to either cap simply never contributes fragment bins, with no persistent record.

PI_DB's `.idx` file is the opposite design: everything is enumerated once at build time and the
file *is* the search index, read back verbatim. A peptide silently dropped by either cap at build
time is **permanently unfindable** by any future search against that `.idx` file, however heavily
modified the real (spiked-in, phosphorylated, etc.) form of that peptide might be in a real sample --
there is no regeneration step where it could reappear. This is the reason
docs/20260713_PIidxformat.md section 5/8 calls the inherited caps a genuine functional gap for PI_DB
specifically, not merely an FI_DB quirk PI_DB happens to inherit.

## 4. What raising the caps does and doesn't fix (measured on human.small.fasta)

Full methodology and evidence trail in docs/20260713_PIidxformat.md section 8, item 4/5
"CORRECTION" subsection; summarized here for this doc's specific focus.

Baseline (`FRAGINDEX_MAX_COMBINATIONS=2000`, `MAX_BITCOUNT=24`, both defaults): PI_DB build of
`human.small.fasta` with M-oxidation + STY-phospho variable mods (trypsin, 2 missed cleavages,
length 8-50) produced 10,516,541 peptide+mod-state entries vs. 10,869,334 from the legacy
(pre-Phase-B) `VariableModSearch`-based build, which has no equivalent cap at all -- a gap of
352,793 entries (~3.2%), concentrated in just 284 distinct peptide sequences (long, S/T-rich,
low-complexity/RS-domain-like sequences).

Raising only `FRAGINDEX_MAX_COMBINATIONS` to 1,000,000 (temporary experiment, reverted;
`MAX_BITCOUNT` left at 24): closed 97.7% of the gap (352,793 -> 15,232 residual). The residual traced
to exactly two peptides (plus their missed-cleavage variants) with 30-31 S/T/Y candidate sites --
still over `MAX_BITCOUNT=24` regardless of how high `FRAGINDEX_MAX_COMBINATIONS` is set, so they
still produced zero modified entries. Once those four entries are set aside, the remainder of the
apparent mismatch was a cosmetic I/L-representative-selection difference (same mass-identical
peptide under `equal_I_and_L=1`, different literal I/L spelling chosen as canonical -- not a lost
peptide).

**Conclusion: the two caps must be addressed together.** Raising `FRAGINDEX_MAX_COMBINATIONS` alone
fixes peptides with a moderately high candidate count (here, up to ~30 candidates depending on which
combination-count check they hit first) but leaves any peptide with more than 24 candidate sites for
a single mod type silently empty, no matter how generous `FRAGINDEX_MAX_COMBINATIONS` is set. Only
`MAX_BITCOUNT` blocks those.

## 5. Open considerations that were weighed before implementing (see section 6 for what was chosen)

1. **Scope of the fix**: change both constants only for the PI_DB build path (leaving FI_DB's
   values untouched, since FI_DB's size/speed trade-off reasoning in section 3 above still applies
   there), or change them globally (simpler, but changes FI_DB's fragment-index size/build-time
   trade-off too, which was not requested and hasn't been evaluated). Given both are currently
   process-global constants/variables read by shared code with no caller-specific parameterization,
   a PI_DB-only change would require either a new parameter threaded through
   `ModificationsPermuter`'s call chain, or a global variable toggled by the caller before/after
   invoking `PermuteIndexPeptideMods` (mirroring how `GeneratePlainPeptideIndex()` already
   temporarily toggles `bCreateFragmentIndex`/`iDbType` around its `RunSearch()` call -- see
   docs/20260713_PIidxformat.md section 8 item 1).
2. **How high to raise them, or remove entirely**: `MAX_BITCOUNT`'s ceiling is really a bitmask-width
   concern in `getModBitmask()`/`ALL_COMBINATIONS` (`unsigned long long`, 64 bits) more than a
   deliberately chosen number -- the comment beside it (`CometModificationsPermuter.cpp:42-44`)
   references `FRAGINDEX_MAX_COMBINATIONS (65,534)`, a value that does not match the current
   `#define` (2000) anywhere in the codebase; this looks like a stale comment from an earlier version
   of the constant and should not be relied on when picking a new `MAX_BITCOUNT`. Any value up to
   63 is bit-width-safe; the real constraint is how large `ALL_COMBINATION_CNT`
   (`ModificationsPermuter::initCombinations`, sized by peptide length and `iMaxNumVariableMods`)
   and the combinatorial explosion of `getTotalCombinationCount` get for realistic long,
   low-complexity peptides -- this needs empirical memory/time profiling on a full human proteome
   build (this document's tests used `human.small.fasta`, a small subset) before picking a number,
   not just a mass-produced worst case.
3. **Silent vs. logged truncation**: whatever numeric ceiling is chosen, `IGNORED_SEQ_CNT` already
   exists as a running counter of dropped modifiable-sequence classes
   (`CometModificationsPermuter.cpp:49`) but does not appear to be surfaced to the user anywhere in
   the current build-time logging (`WritePeptideIndex()`/`WriteFIPlainPeptideIndex()`'s status
   output) -- not verified further in this session. For PI_DB specifically, given section 3's
   "permanently unfindable" consequence, logging `IGNORED_SEQ_CNT` (and ideally which peptides) at
   the end of a `-j` build seems worth doing regardless of where the numeric ceilings end up, so a
   user builds an index that's known-exhaustive-except-for-N-flagged-peptides rather than
   silently-incomplete.
4. **`FRAGINDEX_KEEP_ALL_PEPTIDES=0` is untested**: per the constant's own comment, setting it to 0
   would presumably disable enforcement of both caps entirely (`ignorePeptidesWithTooManyMods()`
   returns `false`, so every `&&`-gated check in section 2 short-circuits false and no `return`/`-1`
   ever fires) -- functionally equivalent to option (a) "skip the cap entirely" from the decision
   list in docs/20260713_PIidxformat.md section 8, and possibly simpler to implement (flip one flag
   around the PI_DB build call, matching the existing temporary-flag-toggle pattern) than raising
   both numeric ceilings to arbitrarily large values. Not tested in this session -- flagging as a
   candidate approach worth trying before independently reinventing an "unbounded" mode via very
   large constants (which is what this session's experiment did, and which still leaves the
   individual-mod-type early-`return` bare-`return` structure fragile against any future integer
   overflow for extreme worst cases, however unlikely in practice).

## 6. What was implemented

Chose option (b)-adjacent from item 2 above: raise both ceilings rather than remove enforcement
entirely (`FRAGINDEX_KEEP_ALL_PEPTIDES=0`, item 4, was not used), and change them globally rather
than scoping to PI_DB only (item 1) -- both FI_DB and PI_DB now get the same, more permissive
limits, since they share this code and no caller-specific parameterization was added.

- **`MAX_BITCOUNT`**: `CometModificationsPermuter.cpp:46`, changed `24` -> `50`. Since
  `peptideLengthRange.iEnd` is hard-clamped to `MAX_PEPTIDE_LEN - 1 = 50` everywhere in the
  codebase (`CometSearchManager.cpp:1001`), no real peptide's candidate-residue count for a single
  mod type can ever exceed 50 -- this value **removes the cap's practical effect entirely** rather
  than merely raising it. Verified at the exact boundary (a 50-residue, all-serine synthetic
  peptide, single STY mod) before implementing: predicted and actual peptide counts matched exactly
  (1,001), confirming no off-by-one, overflow, or other issue at the true maximum. The stale
  comment referencing an unrelated `FRAGINDEX_MAX_COMBINATIONS (65,534)` value (see item 2 above)
  was replaced with an explanation of the new value's derivation.
- **`FRAGINDEX_MAX_COMBINATIONS`**: `core/Constants.h:34`, changed `2000` -> `10000`. Chosen as a
  substantial but still-bounded raise (per section 5 item 2's caution about full human-proteome
  memory/time profiling before removing this ceiling entirely) rather than an attempt to disable it
  outright the way `MAX_BITCOUNT` effectively was.

### Validation

Full unit + integration suite (`tests/unit/run_tests.py --integration`, 21 tests: 19 unit + T17
count-range + T18 determinism) passes with both new values, including T18's byte-identical
two-build-determinism check -- the higher ceilings don't introduce nondeterminism.

Re-ran the `human.small.fasta` M+STY-mod legacy-vs-new comparison (same methodology as
docs/20260713_PIidxformat.md section 8: `git worktree` build of the pre-Phase-B commit `05c01bdb`
as "legacy" (no caps of any kind), trypsin + 2 missed cleavages + M-oxidation + STY-phospho,
length 8-50) with both new values in place simultaneously (previous sessions had only ever raised
one cap at a time). Result:

| Config | Peptide count | vs. legacy (10,869,334) |
|---|---|---|
| Defaults (`FRAGINDEX_MAX_COMBINATIONS=2000`, `MAX_BITCOUNT=24`) | 10,516,541 | -352,793 (96.8% match) |
| `FRAGINDEX_MAX_COMBINATIONS=1,000,000` only (`MAX_BITCOUNT=24` unchanged, prior experiment) | 10,854,102 | -15,232 (99.86% match) |
| **Both raised (implemented): `FRAGINDEX_MAX_COMBINATIONS=10000`, `MAX_BITCOUNT=50`** | **10,860,586** | **-8,748 (99.92% match)** |

A detailed per-peptide diff of this final residual (mirroring the methodology used for the earlier
15,232 and 352,793 residuals) was started but not completed in this session -- the process ran
substantially longer than the equivalent diffs earlier in this investigation (>5 minutes without
producing output, likely resource contention from the volume of large builds/comparisons run this
session) and was killed rather than left blocking further work. Based on the categories already
established for the smaller residuals, the expected composition of the remaining 8,748 is: (a) a
cosmetic, zero-net-effect component from I/L-representative-selection differences (same
mass-identical peptide under `equal_I_and_L=1`, different literal spelling chosen as canonical --
contributes equally to "legacy-only" and "new-only" counts, cancels in the net total, so does not
by itself explain a net residual), and (b) a small number of exceptionally candidate-dense
peptides whose combination count still exceeds even 10,000 (e.g. a peptide with ~45+ S/T/Y
candidates at `max_mods=3` could reach `C(45,3)=14,190`, over the new ceiling). This attribution is
inferred from the established pattern, not independently re-confirmed for this specific residual --
flagged as unfinished verification, not a claim of certainty.

## 7. Proposed: truncate at `FRAGINDEX_MAX_COMBINATIONS` instead of all-or-nothing skip (not yet implemented)

Request: instead of section 2's verified behavior (a peptide's modifiable class produces **zero**
modified entries once its combination count exceeds `FRAGINDEX_MAX_COMBINATIONS`), generate
combinations up to `FRAGINDEX_MAX_COMBINATIONS` and then stop -- keep a partial, capped set of
modified entries rather than none.

### 7.1 Key finding: the truncation logic already exists and is dead code

Section 2 already identified this: `generateModifications()` (`CometModificationsPermuter.cpp`)
contains a truncation-shaped code path that section 2 called "structurally unreachable" --

- `:556` -- `combinationsForModArrLen` is already computed as
  `min(calculatedCombinationsCount, FRAGINDEX_MAX_COMBINATIONS)` per mod type.
- `:574-589` -- the per-mod-type combination-gathering loop already breaks once
  `combinationsFound >= FRAGINDEX_MAX_COMBINATIONS` (`:576-577`).
- `:691-694` and `:706-707` -- the Step 3 combine loop already breaks once the running total
  (`totalModNumCount`) reaches `FRAGINDEX_MAX_COMBINATIONS`, both inside a single combination-set's
  inner loop and across combination sets.

All of this is currently unreachable because two earlier gates abort the **entire** peptide first:

1. The Step 1 per-mod-type early `return` at `:513-517` (`bitCount > MAX_BITCOUNT`) and `:521-525`
   (`combinationCount > FRAGINDEX_MAX_COMBINATIONS`), which exit `generateModifications()` before
   any combination is generated.
2. `getTotalCombinationCount()` (`:390-409`) returning its `-1` sentinel whenever any mod-type
   subset's product, or the grand total, exceeds `FRAGINDEX_MAX_COMBINATIONS` -- consumed at `:622`
   (`if (combinationCount != -1)`), which routes straight to the `else` branch (`:719-721`,
   `IGNORED_SEQ_CNT++`) and generates nothing.

So implementing truncation is mostly about **removing these two abort gates**, not writing new
truncation logic -- the code downstream of them was already built to cap output at
`FRAGINDEX_MAX_COMBINATIONS`; it just never gets the chance to run today.

### 7.2 Proposed code changes

**A. Step 1, `FRAGINDEX_MAX_COMBINATIONS` gate only (`:521-525`)** -- remove the early `return`.
Let `combinationCount` (already computed at `:519` via `getCombinationCount()`, uncapped) flow into
`combinationCounts` as-is; `:556`'s existing `min(...)` ternary then caps the per-mod-type array
correctly with no further change needed there.

Leave the `MAX_BITCOUNT` gate (`:513-517`) untouched. `MAX_BITCOUNT` bounds the number of candidate
*residues*, not the number of *combinations* -- there is no natural "truncate to N candidates"
semantics without arbitrarily discarding specific residues from consideration, which is a different
and more invasive feature than what's requested here. Per section 6, `MAX_BITCOUNT=50` is already
peptide-length-ceiling-safe (`MAX_PEPTIDE_LEN - 1 = 50`), so this gate is already a practical no-op
for any legal peptide and costs nothing to leave as an all-or-nothing safety net. Flag this
asymmetry explicitly in a code comment so a future reader doesn't assume truncation applies
uniformly to both caps.

**B. `getTotalCombinationCount()` (`:390-409`)** -- remove the `-1` abort sentinel; replace with
saturating arithmetic so it never causes a full-peptide skip, while still avoiding `int` overflow
on the per-subset product:

```cpp
// per-subset product loop, :397-405
combos *= combinationCounts.at(s);
if (combos > FRAGINDEX_MAX_COMBINATIONS)
   combos = FRAGINDEX_MAX_COMBINATIONS;   // clamp instead of aborting; precision beyond the cap isn't needed
```

and drop the `allCombos > FRAGINDEX_MAX_COMBINATIONS ? -1 : allCombos` ternary at `:408` in favor of
returning `allCombos` directly (each of the at-most `2^(active mod count) - 1` subsets now
contributes at most `FRAGINDEX_MAX_COMBINATIONS`, so the sum stays well within `int` range for any
realistic number of active mod types). Since this function can no longer return `-1`, the
`if (combinationCount != -1)` branch at `:622` and its `else { IGNORED_SEQ_CNT++; }` at `:719-721`
become dead and should be removed (or repurposed -- see item D).

**C. Remove or repurpose the `"ERROR: calculated combination count exceeds FRAGINDEX_MAX_COMBINATIONS..."`
cout at `:545-550`.** Section 2 noted this line was never observed to print because it was
structurally unreachable under the current all-or-nothing behavior. After change A, a per-mod-type
`calculatedCombinationsCount > FRAGINDEX_MAX_COMBINATIONS` becomes the **expected** trigger for
truncation, not a bug -- printing it as `"ERROR"` would mislabel normal, by-design behavior. Replace
with the counter in item D instead of a console message.

**D. Add a new counter distinguishing "truncated" from "fully dropped".** `IGNORED_SEQ_CNT`
(`:53`) currently means "this modifiable class produced zero entries" (still a real outcome after
this change, via the `MAX_BITCOUNT` gate). Add e.g. `int TRUNCATED_SEQ_CNT = 0;` alongside it,
incremented exactly once per modifiable-sequence class where the Step 3 break (`:691-694` or
`:706-707`) actually fired, or any per-mod-type array was capped at `:556` -- i.e., once per class
that produced a **partial, not full**, set of modified entries. This distinction matters more after
this change than before: a partial result has a nonzero entry count and looks superficially
complete, unlike today's unambiguous zero, so it needs its own signal. Declare both as `extern int`
in `CometModificationsPermuter.h` (currently neither is declared there; `IGNORED_SEQ_CNT` is a
file-local global with no other reader anywhere in the codebase today -- confirmed by grep -- which
is exactly the "not surfaced to logging" gap section 5 item 3 already flagged).

**E. Surface both counters in build-completion status output** (`WriteFIPlainPeptideIndex()` in
`CometFragmentIndex.cpp`, `WritePeptideIndex()` in `CometPeptideIndex.cpp`), e.g. appended to the
existing `" - write peptides/proteins to file"` status line area: `"N sequences had modifications
truncated at FRAGINDEX_MAX_COMBINATIONS=10000; M sequences had all modifications dropped
(MAX_BITCOUNT)"`. This was already recommended in section 5 item 3 for the all-or-nothing case; it
becomes more important once truncation can silently produce incomplete-but-nonzero results.

### 7.3 Selection-bias caveat -- enumeration order and complexity-ordering options

The combinations that survive truncation are whichever ones are enumerated **first**, not a random
or representative sample. Two independent orderings compose to produce today's traversal order:

- **Position-subset table** (`ALL_COMBINATIONS`, built once in `initCombinations()`, `:133-223`) --
  a peptide-independent lookup table. For subset sizes `k = maxMods` down to `1`, it generates every
  k-subset of positions as a bitmask, then merges each size-group into one array sorted by
  **ascending numeric bitmask value** (`:161`'s merge step). `getModBitmask()` maps sequence
  position 0 (the N-terminal-most candidate) to the *highest* bit (`:342`, `len - i - 1`), so
  ascending bitmask order has nothing to do with modification count -- a 3-mod combination clustered
  toward the C-terminus can sort before a 1-mod combination near the N-terminus, purely as an
  artifact of the bit-packing scheme, not a deliberate choice.
- **Mod-type-subset order** (`getCombinationSets(modCount)`, `:353-380`) -- decides which *mod
  types* to combine for a given peptide. The loop runs `i = modCount` down to `1` (`:356`), so it
  enumerates the **largest** mod-type-combined subsets first (e.g. "M-oxidation + STY-phospho
  together" before "M-oxidation alone" or "STY-phospho alone").
- **Net traversal** (Step 3, `:616-708`): outer loop = mod-type-subset, largest-combined-first;
  inner loop = cross product of each member mod type's own bitmask-ascending position-subset list.
  Neither axis is ordered by total modification count -- that's an unordered side effect of the two
  composing. If the cap lands mid-enumeration, single-mod-type entries can be crowded out by
  combined-mod-type entries (or vice versa), and surviving entries skew toward C-terminal-clustered
  placements over N-terminal ones -- not an unbiased sample of the true combinatorial space.

To fix this, "complexity" needs a definition. Three options, in increasing order of how closely they
match "fewest total modifications = simplest":

**Option A -- total modification count only.** Complexity = total number of modified residues across
all mod types, regardless of type. 1 phospho is simpler than 2 oxidations, which is simpler than
1 oxidation + 1 phospho (also count 2 -- a tie, unresolved by this option alone). Most directly
search-relevant: multiply-modified peptides get combinatorially rarer with each added mod in real
samples, independent of which mod types are involved, so this ordering is most likely to keep the
most-plausible combinations under the cap.
- Cost: largest of the three. Mod-type-subset and position-subset are separate axes today, generated
  independently; sorting by total count requires unifying them -- either generate everything then
  sort by popcount before truncating (defeats truncation's point of avoiding full generation), or
  restructure Step 3 to walk popcount level-by-level directly (a real rewrite of the nested-loop
  structure, not a parameter tweak).

**Option B -- mod-type breadth first, then within-type count.** Complexity = number of distinct mod
types involved first (single-type combos rank above any multi-type combo, regardless of total count
-- so 3 oxidations alone would rank as "simpler" than 1 oxidation + 1 phospho, even though the latter
has a lower total count). Implementation: reverse `getCombinationSets()`'s loop bound (`i = 1` up to
`modCount` instead of `modCount` down to `1`), plus sort each mod type's own position-subset list by
popcount (not raw bitmask value) before the C-terminal-bias artifact applies.
- Cost: smallest -- a one-line loop-bound flip plus a local `sort()` of the already-materialized
  per-mod-type lists in `combinationsForAllMods`, no structural rewrite.
- Downside: only approximates "simplest first" -- can rank a 3-mod single-type combination ahead of
  a 2-mod mixed-type one, which reads as backwards if "complex" is meant to track total change to the
  peptide.

**Option C -- total count primary, mod-type breadth as tiebreak.** Complexity = total modification
count first (Option A's key); among combinations tied on total count, prefer fewer distinct mod types
(homogeneous beats heterogeneous at the same count -- 2 oxidations ranks above 1 oxidation +
1 phospho). The "purest" match to "simplest to most complex," and resolves Option A's ties in a
principled way.
- Cost: same order of effort as Option A -- it's Option A's rewrite with an extra comparator field on
  ties, not meaningfully harder once the two axes are already being unified.

**Decision: Option C**, deferred. This work will be taken up after a prerequisite change to
`ModificationsPermuter` to support N-terminal and C-terminal modifications, since that change is
also expected to touch the combinatorics/enumeration structure this reorder depends on -- doing the
complexity-ordering rewrite first would mean redoing part of it once terminal-mod support lands.
Until then, section 7's truncation behavior (7.1/7.2) can still be implemented and will use today's
enumeration order (mod-type-subset descending, position-subset bitmask-ascending) as-is; the bias
described above applies to that interim state and should be called out as a known limitation if
truncation ships before Option C does.

Whichever option is eventually implemented, it must stay deterministic (see 7.6 item 4) -- reservoir
sampling or any random subset selection is explicitly out of scope unless seeded and tested for
determinism.

### 7.4 Scope: FI_DB vs. PI_DB, global vs. caller-specific

Per section 6, both caps and this combinatorics code are shared, ungated by caller, with no
caller-specific parameterization currently threaded through. Absent new plumbing (section 5 item 1's
option of a toggle mirroring `GeneratePlainPeptideIndex()`'s temporary `bCreateFragmentIndex`/
`iDbType` pattern), this truncation change would apply globally to both FI_DB and PI_DB builds, same
as the section 6 raise. Not proposing caller-specific scoping here unless requested -- consistent
with the choice already made in section 6.

### 7.5 Files touched

- `CometSearch/CometModificationsPermuter.cpp` -- `generateModifications()` (~`:513-556`),
  `getTotalCombinationCount()` (~`:390-409`), new `TRUNCATED_SEQ_CNT` counter (~`:53`), optional
  `getCombinationSets()` reorder (~`:356`).
- `CometSearch/CometModificationsPermuter.h` -- declare `IGNORED_SEQ_CNT` and `TRUNCATED_SEQ_CNT` as
  `extern int` so build-status code in other files can read them.
- `CometSearch/CometFragmentIndex.cpp` (`WriteFIPlainPeptideIndex()`), `CometSearch/CometPeptideIndex.cpp`
  (`WritePeptideIndex()`) -- surface both counters in build-completion status output.

### 7.6 Testing plan

1. **Formalize section 2's synthetic test.** Commit the all-serine FASTA (3 proteins, lengths 20/23/
   30, no enzyme, length range 20-30, single STY-phospho mod max 3) as a new unit test (e.g. `t19`)
   in `tests/unit/data/` -- it already isolates the `FRAGINDEX_MAX_COMBINATIONS` boundary precisely.
   Recompute expected peptide counts under truncation: the current all-or-nothing total (4,715, from
   section 2) assumed zero contribution from the length 23-30 peptides; the truncated version
   instead contributes up to `FRAGINDEX_MAX_COMBINATIONS` modified entries for each over-cap
   peptide. Recommend generating the new expected count empirically from a post-implementation
   reference build (rather than hand-deriving it) and locking that in as the regression baseline,
   using `compare_idx.py` plus a manual entry-count check as in section 2's methodology.
2. **Off-by-one boundary check.** Add cases exactly at `combinationCount == FRAGINDEX_MAX_COMBINATIONS`
   (no truncation should occur) and `== FRAGINDEX_MAX_COMBINATIONS + 1` (exactly one combination
   dropped), mirroring the care section 2 already took for `MAX_BITCOUNT`'s `>` vs. `>=` boundary
   (L=24 vs. L=25 in that table).
3. **Unmodified peptide still present.** Assert the unmodified variant is generated regardless of
   truncation (already true and unaffected by this change, but worth a regression guard given how
   central it is to the answer given for the original all-or-nothing question).
4. **Determinism (T18-style).** Two independent builds of the same input must still produce
   byte-identical `.idx` output after this change -- truncation must select the same subset every
   run. Run T18 unmodified after the change to confirm the new control flow (removed early returns,
   clamped arithmetic) introduced no incidental nondeterminism.
5. **Full regression suite.** `tests/unit/run_tests.py --integration` (T1-T18) must continue passing.
   T17's count-range window will likely need re-derivation: PI_DB peptide counts against
   `human.small.fasta` should shift upward since some of the section 6 residual (8,748 entries) came
   from peptides now getting partial rather than zero modified entries. Recompute and update the
   T17 acceptable range after the change lands rather than reusing the current
   `[8,800,000, 9,100,000]` window unchanged.
6. **Repeat the section 6 legacy comparison.** Rebuild `human.small.fasta` with M-oxidation +
   STY-phospho, trypsin, 2 missed cleavages, length 8-50; compare against both (a) the no-cap legacy
   build (`05c01bdb`) and (b) the pre-truncation all-or-nothing build (current HEAD) to quantify how
   much of the residual 8,748-entry gap this closes. Expect: peptides that were previously fully
   dropped for exceeding `FRAGINDEX_MAX_COMBINATIONS` (but under `MAX_BITCOUNT=50`) now contribute
   exactly `FRAGINDEX_MAX_COMBINATIONS` entries each, not their true (larger) count -- the gap versus
   legacy narrows but does not fully close for those specific peptides. That is expected and by
   design, not a bug to chase further.
7. **Counter validation.** Verify `TRUNCATED_SEQ_CNT` increments exactly once per modifiable-sequence
   class that hit the cap (not once per over-cap mod type within a class), and that it appears in
   build stdout; spot-check against the synthetic test's known-truncated peptides (the length 23-30
   rows in section 2's table).
8. **Build-time impact.** Peptides that previously exited `generateModifications()` almost instantly
   at the Step 1 early return now proceed through the full Step 2/Step 3 generation (up to
   `FRAGINDEX_MAX_COMBINATIONS` entries each) instead. Rerun the `human.small.fasta` build-time
   comparison from sections 4/6 to quantify the delta before this lands on a full human proteome
   build, where low-complexity S/T/Y-rich regions (section 4) are exactly the peptides most likely to
   hit the cap.
9. **When Option C (7.3) is implemented** (deferred until after N-/C-terminal mod support lands),
   add a small targeted synthetic case with 2 active mod types where the cap lands between
   equal-total-count combinations of different mod-type breadth (e.g. 2 oxidations vs. 1 oxidation +
   1 phospho, both total count 2), to confirm the homogeneous (fewer-mod-type) combination is kept
   over the heterogeneous one at the tie, and that lower total-count combinations are kept over
   higher ones generally.
