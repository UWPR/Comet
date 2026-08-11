## Restoring self-describing `.idx` files (mods + search mode back in the header)

Date: 2026-08-11

### Background

PR #121 (`819171b7`, "Reduce PI memory ~1.6x and unify PI/FI on one .idx format")
did two things that this change partially reverts:

1. **Phase 0.5** dropped `VariableMod:`/`ProteinModList:`/`RequireVariableMod:` from
   the `.idx` header entirely. Variable-mod settings came solely from whatever was
   live in `comet.params` (batch) or RTS's `SetParam()` calls at search time, the
   same as a non-indexed FASTA search. Only `StaticMod:` stayed in the header.
2. **Phase 0** unified PI_DB and FI_DB onto one on-disk format/builder, which meant
   the file itself no longer implied a search mode. A new `index_search_type`
   comet.params key (and RTS CLI argument) became *required* to say which mode to
   run an existing `.idx` as.

Net effect: an `.idx` file alone was no longer sufficient to run a search against
it -- you also had to separately supply the right `variable_modNN`/
`require_variable_mod`/`protein_modslist_file` params (matching whatever the index
was built with) and the right `index_search_type`. Get any of those wrong and you'd
silently search with the wrong mods or the wrong algorithm.

### What changed here

Both pieces are reverted, but the PI_DB memory-reduction *storage layout* from the
same PR (`g_vRawPeptides` + on-the-fly `MaterializeOneEntry()` instead of the old
per-variant `DBIndex` table) is **kept** -- this change only touches the text
header and the PI_DB/FI_DB mode-selection wiring around it.

**`.idx` format bumped to v3** (magic string `Comet index database v3`; v1/v2 files
are rejected with a "rebuild it with -i or -j" message, same policy PR121 itself
used for its v1->v2 bump). New/restored header layout, written by
`CometPeptideIndex::WritePeptideIndex()` and parsed by
`CometPeptideIndex::ParsePeptideIndexHeader()`:

```
Comet index database v3.  Comet version <version>
IndexSearchType: peptide index          <- or "fragment ion index"
InputDB:  <path>
MassRange: ...
LengthRange: ...
MassType: ...
DecoySearch: ...
Enzyme: ...
Enzyme2: ...
NumPeptides: ...
StaticMod: ...
VariableMod: ...
ProteinModList: ...
RequireVariableMod: ...

<protein names / raw peptide table / proteins list / footer, unchanged>
```

- `IndexSearchType:` is new (not present pre-121 either, which used distinct magic
  strings/formats per mode instead). Human-readable value (`peptide index` /
  `fragment ion index`) rather than a numeric code, placed right after the magic
  line so it's cheap to peek. Set at build time from whichever of `-i`
  (`bCreateFragmentIndex`) / `-j` (`bCreatePeptideIndex`) triggered the build.
- `VariableMod:`/`ProteinModList:`/`RequireVariableMod:` are restored verbatim from
  the pre-Phase-0.5 FI_DB header format (the fuller of the two pre-unification
  formats, since PI_DB now reuses FI_DB's peptide-generation path anyway). Parsed
  back into `g_staticParams.variableModParameters`, **overwriting** whatever
  comet.params/RTS supplied -- the same override precedent `StaticMod:` already
  established for static mods.

**`index_search_type` param is kept, but narrowed in scope.** Once an `.idx` file
exists, its own `IndexSearchType:` line is authoritative -- no parameter is
consulted. `index_search_type` is now consulted *only* when a `database_name`
pointing at a not-yet-existent `.idx` needs to be auto-built from a FASTA sitting
alongside it (both batch's "specified .idx is missing, FASTA exists" path in
`ValidateSequenceDatabaseFile()`, and RTS's equivalent auto-build path) -- there's
nothing in a file that doesn't exist yet to read a mode from, so this is the one
place a parameter is still unavoidable. This mirrors how `-i`/`-j` picked a build
format pre-121; RTS didn't have this ambiguity before PR121 since it could only
ever reach FI_DB.

**Dispatch changes in `CometSearchManager.cpp`.** Three sites previously derived
`g_staticParams.iDbType` from `index_search_type` unconditionally; all three now
split on whether the `.idx` already exists:
- exists -> `CometPeptideIndex::ParsePeptideIndexHeader()` (peeked early, before
  deciding which of `ReadPeptideIndex()`/`CreateFragmentIndex()` to call) sets
  `iDbType` from the file's own `IndexSearchType:` line.
- missing (about to auto-build) -> `index_search_type` picks the format to build,
  unchanged from PR121's behavior.

### What was deliberately *not* reverted

- The PI_DB raw-peptide-table + on-the-fly-materialization storage layout (the
  ~1.6x memory win) -- kept as-is.
- Mod-permutation tables (`MOD_SEQS`/`MOD_NUMBERS`/etc.) are still regenerated
  fresh each session from `g_vRawPeptides` + whichever variable mods are now
  active (sourced from the header instead of live params, but the *regeneration*
  itself, rather than persisting the permutation tables to disk, is unchanged from
  Phase 0.5 -- see `docs/20260730_PI_reduction.md`).
- `-i`/`-j` still call the same `WritePeptideIndex()` builder and produce the same
  binary layout either way; only the `IndexSearchType:` header value differs
  between them now.

### A pre-existing bug found (and fixed) along the way

Verifying this change end-to-end (T19/T20, and legacy T21 cases) hit an
intermittent, layout-sensitive corruption: `g_staticParams.inputFile.szFileName`
sometimes came back truncated/empty after `UpdateInputFile()` returned, causing
`FiStrategy::openFiles()`'s second `.idx` open (for protein lookups) to fail with a
garbled path, or the search to silently process 0 spectra. Reproduced identically
on unmodified `master`, so unrelated to this change -- root-caused via
AddressSanitizer/UBSan to `AllocateResultsMem()` (`CometSearch/search/SearchUtils.cpp`):
`new Results[iNumStored]` doesn't value-initialize, and the per-slot manual reset
loop that follows it missed several scalar fields (`bClippedM`, `cHasVariableMod`,
`fDeltaCn`, `fLastDeltaCn`, `usiRankXcorr`, `lProteinFilePosition`, `lWhichProtein`,
`cPrevAA`, `cNextAA`) -- reading one of those (UBSan caught `bClippedM` specifically,
"load of value 190, which is not a valid value for type 'bool'") before it was
ever explicitly written is undefined behavior, and manifested as unrelated-looking
corruption elsewhere depending on what garbage happened to be on the heap. Fixed by
adding the missing fields to the existing reset loop (both `_pResults[]` and
`_pDecoys[]`). This took the unit suite from 17/42 passing to 42/42 (T19-T21's
"cannot read .idx file" / "no spectra searched" failures were this bug, not
anything specific to legacy-case configuration as first suspected).

### A gap the restoration reintroduces, and where it bit

Neither the restored header format nor the pre-121 one it's modeled on ever persisted
per-mod **count** constraints (`iMaxNumVarModAAPerMod`, `max_variable_mods_in_peptide`,
`iVarModTermDistance`, `iWhichTerm`) -- only identity (`VariableMod:`'s chars/mass/NL) and
the global require-mod bitmask (`RequireVariableMod:`). Those count constraints still come
from live params/`SetParam()` on every run, same as always. `ParsePeptideIndexHeader()`
overwrites `varModList[].szVarModChar`/`dVarModMass`/`dNeutralLoss(2)` per slot but leaves
`iMaxNumVarModAAPerMod` etc. exactly as whatever the caller set them to -- so a caller that
hardcodes a single mod's full `VarMods` struct (rather than reading real params) can end up
with the *identity* of slot N coming from the `.idx` while the *max-count* for that same
slot N is still whatever the caller's hardcoded, unrelated value was.

This bit `tests/rts_repro/rts_repro.cpp` directly: it hardcoded a single phospho-S mod
(matching T19/T20's simple fixture) and reused it for T22's second check too, which searches
a `human.small.fasta`-built `.idx` with two real active mods (M oxidation + STY phospho,
`data/comet_phospho.params`). Post-restoration, the header correctly activated both mods for
that `.idx`, but the driver's single-mod, `max_variable_mods_in_peptide=1` config was now
inconsistent with what the index needed -- `EnumerateIndexPeptideMods()`'s own combination-
count self-check caught it (`"Unexpected combination count"`, then a hard `VarModSites::
MAX_SITES` error) rather than silently mis-scoring. Fixed by widening the driver's
hardcoded config to match `comet_phospho.params` exactly (both mod slots, `max=4`) --
verified safe for the simpler ground-truth fixture too (extra permitted-but-inactive slots
and a higher per-peptide cap don't change which candidate scores best for an unambiguous
8-residue peptide with one phospho-acceptor).

`RealtimeSearch/SearchMS1MS2.cs` has the same structural gap (hardcoded `VarModsWrapper`
calls, one mod slot active at mass 0 i.e. effectively disabled, a second commented out) but
it predates this change -- it already carries a `TODO(user): extend this to optionally load
these ... settings from an actual comet.params file` comment, and Phase 0.5 had the identical
requirement (RTS needing its own correct explicit mod config, independent of the `.idx`).
Not touched here since it's demo/example code with an existing, acknowledged TODO, not a
regression this change introduces -- flagged for whoever picks that TODO up next.

### v4 addendum (same day): mod *count* limits also persisted

The gap above -- `iMaxNumVarModAAPerMod`/`max_variable_mods_in_peptide` never being part
of the header, only mod *identity* -- was closed the same day, going one step past a pure
revert of #121. Checked against `v2025.03.0` (the true pre-121 baseline, both its
now-retired separate PI_DB and FI_DB formats) first: neither ever persisted these two
fields either -- `CometFragmentIndex::PermuteIndexPeptideMods()`/
`CometModificationsPermuter` always read them from whatever was live in `g_staticParams`
at search time, from `comet.params`/`SetParam()`, for the entire history of this codebase
up to and including that tag. So this is a genuine new capability, not a restoration.

Format bumped to **v4** (`Comet index database v4`) since the header shape changed again:

- `VariableMod:` gained a 5th `:`-delimited field per slot, `iMaxNumVarModAAPerMod`:
  `S:79.966331:0.000000:0.000000:2` (chars:mass:NL1:NL2:maxPerMod).
- New `MaxVariableModsInPeptide: N` line, right after `RequireVariableMod:` and now the
  last populated header line (loop terminator).

Both parsed into `g_staticParams.variableModParameters` by `ParsePeptideIndexHeader()`,
overwriting live params -- same precedent as everything else in the header. Confirmed via
`CometFragmentIndex.cpp`/`CometModificationsPermuter.cpp` that both fields are already
fully consumed by the FI/PI enumeration path (`vMaxNumVarModsPerMod` built straight from
`varModList[i].iMaxNumVarModAAPerMod`; the global cap enforced as a hard bit-count check)
-- no enumeration-side changes needed, purely a persist-and-restore addition.

`iVarModTermDistance`/`iWhichTerm` (peptide/protein N/C-term mod restriction) remain
**unsupported for FI_DB/PI_DB** and are not part of the header -- confirmed zero references
to either field in `CometFragmentIndex.cpp`, `CometModificationsPermuter.cpp`, or
`CometPeptideIndex.cpp`, at this tag or `v2025.03.0`. Only the plain-FASTA search path
(`CometSearch.cpp`) has ever enforced them.

### Fixture/test fallout

- `tests/unit/compare_idx.py` is unaffected -- it derives section boundaries from
  the footer pointers, not header content, so a longer header doesn't need any
  changes there.
- `tests/unit/data/*.idx` committed fixtures (t1-t16 etc.) needed rebuilding under
  the v4 format (v3 initially, then v4 same day).
- T19/T20 were inverted back from their Phase-0.5 form: build with the real
  phospho-S mod, search with `variable_mod01` left blank -- proving the header
  wins, the same direction these tests had pre-121.
