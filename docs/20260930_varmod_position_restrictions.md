# variable_mod position restrictions on every search path (2026-09-30)

Branch `terminalmods`, for release 2026.03 rev. 0 (v2026.03.0; the v2026.02.3 tag was withdrawn). Restores the `variable_modNN` fifth and
sixth fields (`term_distance`, `which_term`) that `docs/20260915_permuter_terminal_mods.md`
(#130, decision D4) deprecated, and makes the fragment-ion (FI) and peptide (PI) index paths
honor them for the first time. The motivating case is N-terminal pyroglutamate
(`-17.026549 Q 0 1 0 2 0 0.0`): v2026.02.2 applied it only to a peptide-N-terminal Q on the
plain-FASTA path and to every Q on FI/PI; the #130 build applied it to every Q everywhere.

## 1. Semantics

`variable_modNN = <mass> <residues> <binary> <max> <term_distance> <which_term> <required> <NL>`

| term_distance | Meaning |
|---|---|
| -1 | no restriction (default) |
| -2 | not on the peptide's C-terminal residue |
| d >= 0 | residue (or `n`/`c` terminus) within d residues of the terminus named by which_term |

which_term: 0 = protein N, 1 = protein C, 2 = peptide N, 3 = peptide C. Terminal codes in the
residue string are unchanged from #130: `n`/`c` = any peptide terminus, `^`/`$` = protein
N-/C-terminus only.

`InitializeStaticParams()` still rewrites the one idiom with an exact terminal-code
equivalent -- `n` with `0 0`, `c` with `0 1` -- to `^` / `$` (now silently), and resets the
fields to `-1 0` when the slot holds nothing else; a slot that also lists residues (e.g.
`nK 0 3 0 0`) keeps the fields, which restrict K to protein position 0 as in v2026.02.2. The
slot-merge step again compares the two fields, so a restricted slot is never folded into an
unrestricted one of the same mass.

## 2. Plain-FASTA path: v2026.02.2 restored

The 89 lines #130 removed from `CountVarMods()`, `SubtractVarMods()`, `HasVariableMod()`,
`VariableModSearch()` and `MergeVarMods()` are restored verbatim, with 2.2's
`bNtermMod`/`bCtermMod` tests replaced by the `^`/`$`-aware `VarModNtermAllowed()` /
`VarModCtermAllowed()`. One 2.2 defect is fixed on the way: `MergeVarMods()` consumed a
terminal site whenever the slot was an n/c-term mod, while `VariableModSearch()` counted it
only when the distance rule admitted it, so a distance-restricted terminal mod could shift
every later site assignment of that slot. Both now use the same `VarModNtermCounted()` /
`VarModCtermCounted()` tests (which keep 2.2's exact conditions, including that a
which_term 3 distance never admits an n-term mod and a which_term 0 distance tests a c-term
mod against the peptide start).

## 3. FI/PI path: position classes in the permuter

`CometFragmentIndex::PermuteIndexPeptideMods()` passes each compacted mod's rule
(`ModPositionRule`, `CometModificationsPermuter.h`) to
`ModificationsPermuter::getModifiableSequences()`. When any rule is restricted, every
modifiable-sequence position (terminal sentinels included) gets a class byte -- bit m set
when mod m may take that position (`getPositionClass()`) -- and the class bytes become part
of the dedup key, so two peptides sharing modifiable residues (`QAQK`, `AQQK` -> `QQ`) but
not eligibility get distinct permutation sets. `generateModifications()` ANDs each
restricted mod's position bitmask with its class bits. The pool keeps plain residue letters,
so every consumer (fragment ladders, `ComputeIndexedPepMass()`, `MaterializeOneEntry()`,
`SearchFragmentIndex()`) is unchanged; the class pool is build-time only and released after
`getModificationCombinations()`.

| Rule | FI/PI |
|---|---|
| peptide N/C (which_term 2/3), any d; -2 | exact |
| protein N/C (which_term 0/1), d = 0 | exact (the `-` flank marks the protein terminus) |
| protein N/C, d > 0 | partial: only peptides at that protein terminus are admitted (the index has no position-in-protein); `PermuteIndexPeptideMods()` warns |

As with `^`/`$`, a peptide shared by several proteins is one index row whose flanks are
OR'd across its occurrences ("protein-terminal in any protein"), so protein-terminus rules
use that union on the index paths while the FASTA path evaluates each protein separately.

## 4. .idx format (stays v5)

Each `VariableMod:` slot is now `chars:mass:NL1:NL2:max:term_distance:which_term`. The
reader also accepts the 5-field form of indexes built before this change and treats those
slots as unrestricted -- exactly how they were built. v5 had no lasting public release (the
v2026.02.3 release that introduced it was withdrawn the same day), so the version stays; an older v5 binary reading a new file would take
the first five fields and silently search without restrictions.

## 5. AScorePro

AScorePro knows a mod only as residues + mass and could relocalize a restricted mod onto a
forbidden residue (e.g. pyroglutamate onto an internal Q). `AScoreOptions` gains an optional
`std::function<bool(const Peptide&)>` filter; `AScoreCalculator` skips a generated
peptidoform the filter rejects before scoring, so it is neither the top peptide nor a
site-scoring alternative. `CometPostAnalysis::CalculateAScorePro()` sets it (on a per-call
copy of the shared `g_AScoreOptions`) only when a slot AScorePro sees is restricted, using
`getPositionClass()`; a mod at the position Comet itself placed it is always allowed, which
covers FASTA protein-distance rules a PSM cannot re-derive. The reported MOB and site scores
are therefore computed among allowed peptidoforms; when the original is the only one, it
keeps its own MOB score and a 5000.0 site score.

Build note: neither the AScorePro nor the CometSearch Makefile tracks header dependencies.
Changing `AScoreOptions.h` needs `cd AScorePro && make clean` plus `make cclean` (and
`rm Comet.o`); a partial rebuild mixes class layouts and crashes in `free()` inside
`AScoreDllInterface::CalculateScoreWithOptions()`.

## 6. Also in this branch

T18 (index build determinism) regression from #130: within-protein dedup keys gained the
protein-terminus context, so copies of a peptide repeated in one protein (UBC's ubiquitin
repeats) both reached the dedup merge, tied on every sort key and arrived in thread order;
the representative's mass differed in its last bits. Both dedup comparators now break ties
on every field the representative contributes.

## 7. Tests

T41 (`t41_termmod_fields`, was `t41_termmod_deprecation`) and T49 rewritten for the restored
semantics; T54 (pyroglutamate on all three paths, `.idx` header, partial-rule warning) and
T55 (AScorePro filter) added; `PermuterTest` P14-P17 cover the position classes.

## 8. Known limitations

- Internal decoys reverse the peptide and move each mod with its residue, so a decoy's
  pyroglutamate lands on an internal residue. Target and decoy stay mass-matched; v2026.02.2's
  FASTA path behaved the same way.
- Protein-terminus distance rules with d > 0 are partial on FI/PI (section 3).
