# FRAGINDEX_MAX_COMBINATIONS and MAX_BITCOUNT: status and considerations

Status: **IMPLEMENTED.** Both caps have been raised (`MAX_BITCOUNT` to 50, effectively disabling
it; `FRAGINDEX_MAX_COMBINATIONS` to 10,000) -- see section 6. Companion to
docs/20260713_PIidxformat.md (the PI_DB-reuses-FI_DB's-peptide-generation project), which is
where these two caps were discovered to matter for PI_DB's new build path. Sections 1-5 below
describe the caps' original (pre-change) behavior and the investigation that led to the change;
section 6 covers what was actually implemented and its measured effect.

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
