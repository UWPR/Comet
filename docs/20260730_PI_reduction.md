# PI_DB memory reduction: splitting DBIndex into a raw-peptide table + compact per-variant record

Status: Phases 1-3 (Section 4), Phase 0 (Section 4, "unify the PI_DB/FI_DB on-disk format"), and
Phase 0.5 (Section 4, "drop persisted mod-permutation tables and compact variant array") are all
**IMPLEMENTED (2026-07-31).** PI_DB and FI_DB share one `.idx` file, one builder, one reader, and
dispatch on the `index_search_type` comet.params key (RTS: a `RealtimeSearch.exe` CLI argument)
instead of sniffing the file's own header. The `.idx` file persists only what can't be redone
without re-digesting the FASTA (enzyme, static mods, decoy mode, peptide mass/length range) plus
the raw unmodified peptide table; `MOD_NUMBERS`/`MOD_SEQS`/etc. and the modified-peptide variant
array are regenerated once per search session from live `comet.params`, for both search modes.

**Validation.** Full unit/integration suite passes on both a fresh Linux `make` build and a fresh
Windows MSBuild build (Clean + Build, working around the known `zconf.h`/`unistd.h` cross-platform
gotcha triggered by the preceding Linux build -- see `docs/CometCodingStyleGuidelines.md`/CLAUDE.md):
40/40 default tests, T17/T18 (real 8.9M-peptide no-enzyme build + determinism), T19/T20 (AScore +
PI_DB regression, rewritten for Phase 0.5 -- see below), and T22 (`t22_rts_fi`/`t22_rts_pi`, real
RTS single-spectrum search via `tests/rts_repro/`, both ground-truth localization and 1-vs-8-thread
determinism over 197 real spectra) -- all identical pass results and, where compared directly
(T19/T20's AScore score), byte-identical output between platforms.

A real production gap surfaced during this validation, not caught by the test suite until the fix
was already in progress: **RTS had no source for variable mods at all once the `.idx` header
stopped carrying them.** `RealtimeSearch/SearchMS1MS2.cs` never loads a `comet.params` file, and
its `SetParam("variable_mod01", ...)` calls lived only inside the "build a brand-new `.idx`" branch
-- for a run against an *already-existing* `.idx` (the common case), variable mods came exclusively
from the now-removed header line. Fixed by moving that `SetParam` block so it runs unconditionally,
on every run, not just index auto-build (per explicit user direction: extend RTS's existing
CLI/`SetParam`-based mechanism, matching how `index_search_type` was added, rather than giving RTS
a `comet.params`-file-loading path -- the user is separately planning to extend the example program
with optional `comet.params` support of its own). `tests/rts_repro/rts_repro.cpp` -- the Linux test
driver that exercises the same `ICometSearchManager` API `RealtimeSearch.exe` calls through
`CometWrapper.dll` -- got the equivalent fix (explicit `variable_mod01` + a new `index_search_type`
CLI argument, both previously implicit/absent) so T22 continues to test real behavior.

T19/T20 were inverted to match the new architecture: they now build the index with a *blank*
variable mod (proving build-time mod settings are irrelevant, Phase 0.5's whole point) and search
with the real phospho-S mod in search-time params (the only source left), rather than the old
build-real/search-blank pattern that specifically proved the header-override behavior Phase 0.5
removed.

## 1. Background

A same-day investigation (`20260730-phospho-v2026020-vs-v2026021` RTS PI-vs-FI benchmark, wide-mod
config: M/STY variable mods, up to 3 per residue type, 3 total, mass 700-5000, length 7-50, against
`human.canonical.target-decoy.fasta`) measured RTS peak process memory of **11.7 GB for PI_DB** vs.
**20.2 GB for FI_DB**, both against the same 1.249e8-modified-peptide search space. The gap was
smaller than expected given that FI_DB should contain a superset of PI_DB's information (peptides,
mods, masses) plus the fragment-ion posting list on top.

To isolate index-only memory from spectra/thread/results overhead, a temporary `MEMPROBE`
instrumentation line was added to `RealtimeSearch/SearchMS1MS2.cs` right after
`InitializeSingleSpectrumSearch()` returns (before any spectra are preloaded or search threads
start), and both index formats were rebuilt fresh from `20260420-human-phosho/comet.params` and
re-measured in isolation:

| | Index-loaded memory (measured, 1 thread) | Full-run RTS peak (original benchmark) |
|---|---|---|
| PI_DB | **10.48 GB** | 11.7 GB |
| FI_DB | **19.0 GB** | 20.2 GB |

So essentially all of the PI-vs-FI gap *is* the index itself; the ~1.2 GB difference between the
index-only reading and the full-run peak is consistent between both backends (preloaded spectra +
thread scratch + results).

### Why the gap isn't bigger: per-entry struct cost

`CometSearch/core/Types.h`:

- **PI_DB**: one flat `vector<DBIndex>` (`core/Types.h:331`), **88 bytes/entry** (8 `lIndexProteinFilePosition`
  + 8 `dPepMass` + 2 `siVarModProteinFilter` + 1 `cPrevAA` + 1 `cNextAA` + 17 `VarModSites` + 51
  `sPeptide[MAX_PEPTIDE_LEN]`, no padding at either end). 1.249e8 peptides x 88B = 10.24 GiB
  predicted, vs. 10.48 GB measured -- near-exact, since `ReadPeptideIndex()`
  (`CometPeptideIndex.cpp:159-179`) is a straight sequential read into one contiguous,
  pre-`reserve()`d vector.
- **FI_DB**: `g_vRawPeptides` (`PlainPeptideIndexStruct`, 72B x 4.659e6 unique raw peptides, sequence
  text stored *once*) + `g_vFragmentPeptides` (`FragmentPeptidesStruct`, 24B x 1.249e8 modified
  variants -- just `dPepMass` + a 4-byte raw-peptide index + a 4-byte mod-permutation index + 2
  terminal-mod bytes, **no duplicated sequence text or explicit mod-site array**) + `g_iFragmentIndex`
  (the fragment-ion posting list, 4B x 3.700e9 entries = 13.78 GiB) = 0.31 + 2.79 + 13.78 = 16.9 GiB
  predicted, vs. 19.0 GB measured (remainder is `MOD_SEQS`/`MOD_NUMBERS` mod-permutation scaffolding
  plus normal allocator overhead).

**The two backends are paying for different things that happen to land in the same order of
magnitude for this dataset.** PI_DB pays once per modified peptide for an 88-byte record that mostly
*duplicates* the same peptide's full sequence text and protein/flank info across every one of its
mod-site variants. FI_DB avoids that duplication (24B/entry) but then pays for a fragment-ion
posting list PI_DB has no equivalent of at all. This document is about fixing PI_DB's side of that
trade: the 88-byte record is bloated by duplication that FI_DB already solved for its own use case,
and PI_DB's search algorithm has no structural need for a posting list, so PI_DB can adopt FI_DB's
compact-record trick without taking on FI_DB's added cost.

## 2. The trick FI_DB already uses (and why PI_DB can borrow it directly)

The pieces PI_DB needs already exist in the codebase, built for FI_DB, and are *already computed as
an intermediate step inside PI_DB's own current build path* before being thrown away:

- **`PlainPeptideIndexStruct`** (`core/Types.h:501`) -- one row per unique raw (unmodified) peptide:
  protein file position, unmodified mass, `siVarModProteinFilter`, flanking AAs, sequence text.
  Exactly the data that's duplicated across every mod-variant of a peptide in today's `DBIndex`.
- **`FragmentPeptidesStruct`** (`core/Types.h:521`) -- one row per modified-peptide variant: modified
  mass, a `iWhichPeptide` index into the raw-peptide table, a `modNumIdx` index into `MOD_NUMBERS`
  (the pre-computed mod-site-combination table), plus `cNtermMod`/`cCtermMod`. 24 bytes, no sequence
  text, no explicit per-site array.
- **`MaterializeIndexPeptideMods()`** (`CometPeptideIndex.cpp:260-468`) -- already contains, in its
  `buildEntry` lambda (lines 274-341), the *exact* logic to reconstruct a full `DBIndex` (sequence,
  `VarModSites`, mass, flank AAs, protein position) from `(iWhichPeptide, modNumIdx, cNtermMod,
  cCtermMod)` plus `g_vRawPeptides`/`MOD_NUMBERS`/`MOD_SEQS`/`PEPTIDE_MOD_SEQ_IDXS`. This function is
  currently called *once, eagerly, for all 1.249e8 entries* during `WritePeptideIndex()`
  (`CometPeptideIndex.cpp:539-540`) to materialize the bloated on-disk/in-memory format. **This is
  the reconstruction function the new design needs -- it just needs to run per-candidate at search
  time instead of once for every peptide at build time.**
- **Today's PI_DB build already computes `g_vRawPeptides` and calls `PermuteIndexPeptideMods()`**
  (`CometPeptideIndex.cpp:520-537`, see `docs/20260713_PIidxformat.md`) as Phase A/B of building the
  index, then **discards** `g_vRawPeptides` (`CometPeptideIndex.cpp:557`,
  `vector<PlainPeptideIndexStruct>().swap(g_vRawPeptides)`) once `MaterializeIndexPeptideMods()` has
  eagerly expanded everything. All the compact-representation machinery already runs on every PI_DB
  build; it's just being used to produce a bulkier result than necessary and then thrown away.
- **The search-time scoring path is already representation-agnostic.** `SearchPeptideIndex(Query*,
  ...)` (`CometSearch.cpp:1890-1936`) is the *only* call site that consumes `g_pvDBIndex` during a
  search (both RTS's per-spectrum path and batch PI_DB's multi-query path funnel into this one
  function -- `CometSearch.cpp:204`, `:224`, `:1870`). It does a binary search on `dPepMass`, then
  calls `AnalyzePeptideIndex(pQuery, g_pvDBIndex[i], ...)` (`CometSearch.cpp:1926`) once per
  mass-window survivor -- typically tens to low hundreds of candidates per query spectrum, not
  millions. `AnalyzePeptideIndex` (`CometSearch.cpp:1941-2010+`) only reads `sDBI.sPeptide`,
  `sDBI.pcVarModSites`, `sDBI.cPrevAA/cNextAA`, `sDBI.dPepMass`,
  `sDBI.lIndexProteinFilePosition` -- exactly what `buildEntry` already knows how to produce.
  FI_DB's own scoring path (`XcorrScoreI`, `CometSearch.cpp:7822`, shared between both `DbType`s)
  already takes primitive parameters (`piVarModSites`, `iLenPeptide`, `szProteinSeq`, ...) rather
  than a `DBIndex&`, and FI_DB's caller already reconstructs those per-candidate from
  `FragmentPeptidesStruct` + `PlainPeptideIndexStruct` via `FragmentIndexReader`
  (`CometFragmentIndexReader.h:46-54`) -- i.e. **FI_DB already proves per-candidate reconstruction at
  scoring time is fast enough in production** (FI_DB runs at thousands of Hz in the same benchmark).

Net: PI_DB reusing FI_DB's compact-record pattern is not a new technique for this codebase, it's
applying a pattern that already exists and is already load-bearing for FI_DB, to the one place
(PI_DB's own index) that still uses the old bloated approach.

## 3. Target design

Replace `vector<DBIndex> g_pvDBIndex` (88B/entry) with:

- `g_vRawPeptides` (reused as-is, `PlainPeptideIndexStruct`, 72B x ~4.66M unique peptides)
- `g_vFragmentPeptides` (reused as-is, `FragmentPeptidesStruct`, 24B x 1.249e8 variants) -- **per
  the resolution of Open Question 3 (Section 8), this is literally the same array FI_DB already
  builds, not a PI_DB-specific lookalike.** PI_DB and FI_DB share one `.idx` file and one in-memory
  raw-peptide/variant representation; the only thing PI_DB's search mode skips is building the
  fragment-ion posting list (`g_iFragmentIndex`/`g_iFragmentIndexOffset`) on top of it, since PI_DB's
  binary-search-on-mass algorithm never needed one. See Section 4, Phase 0 for the dispatch
  mechanism this requires.

Projected memory: 4.659e6 x 72B (raw table) + 1.249e8 x 24B (compact variants) = 0.31 + 2.79 =
**~3.1 GB**, down from the current 10.24 GiB structural cost -- roughly a **3.3x reduction**.

**IMPLEMENTED AND MEASURED (2026-07-30).** Phases 1-3 (Section 4) are implemented; Phase 0
(PI_DB/FI_DB format unification) is not yet done -- PI_DB currently has its own v2 `.idx` format
and its own `g_vDBIndexVariants` array, structurally identical to but not literally sharing
FI_DB's `g_vFragmentPeptides`. Measured on the same full-scale benchmark as Section 1 (same
`MEMPROBE` methodology, same 1.249e8-variant index, `20260420-human-phosho/comet.params`):

| | Predicted | Measured |
|---|---|---|
| PI_DB index-load memory | ~3.1 GB | **6.58 GB** (down from 10.48 GB pre-change -- 1.6x reduction) |
| PI_DB `-j` build time | not modeled | **46s** (down from 3m30s -- skips the old eager full-enumeration pass) |
| PI_DB `-j` build peak memory | not modeled | **7.6 GB** (down from 22.8 GB) |

The 3.1 GB prediction only accounted for `g_vRawPeptides` + the compact variant array. It missed
a real cost: `MOD_NUMBERS`/`MOD_SEQS`/`PEPTIDE_MOD_SEQ_IDXS` (built by
`PermuteIndexPeptideMods()`) must now stay resident for PI_DB's entire search session, not just
during index build, since `MaterializeOneEntry()` references them per candidate. FI_DB already
pays this same cost (it showed up as FI_DB's own ~2 GB predicted-vs-measured gap in Section 1);
PI_DB now pays it too, for the same reason. `ModificationNumber` (`core/Types.h:699`) is the
likely dominant piece of that gap -- its `char* modifications` field is a separate heap
allocation per entry (not inline), and `MOD_NUMBERS` can have many entries for a heavily-modified
search space; the per-allocation overhead across that many small allocations adds up. **Follow-up
opportunity, not yet done:** giving `ModificationNumber` an inline/compact encoding (the same
pattern `VarModSites` already used to replace a heap-allocated `vector<char>`, per the comment at
`core/Types.h:278-289`) would likely recover a meaningful share of this gap for both FI_DB and
PI_DB.

Correctness: validated byte-for-byte identical `.txt` output (all columns, all PSM rows) between
a pre-change and post-change build on real data (`comet-debug3`'s `human.fasta` +
`20170103_HelaQC_01.mzXML`, M-oxidation up to 3 mods, 572 real spectra, 430 PSM rows, decoy search
on). Full existing unit/integration suite (40 tests, including T20's PI_DB batch regression test
and T17/T18's real 8.9M-peptide FI_DB-plain-index builds) passes.

### Rejected alternative: dedup after loading today's on-disk format, no format change

Considered: keep `WritePeptideIndex()`/`ReadPeptideIndexEntry()`'s on-disk format exactly as today
(fully materialized `DBIndex` records), but do a one-time post-load grouping pass in
`ReadPeptideIndex()` that repacks the loaded records into a `{raw-peptide-index, dPepMass,
pcVarModSites}` compact struct (~32B/entry: dedup only `lIndexProteinFilePosition` /
`siVarModProteinFilter` / `cPrevAA` / `cNextAA` / `sPeptide` into a raw-peptide table by grouping,
keep `pcVarModSites` per-entry rather than re-deriving a `modNumIdx`).

This is lower-risk (no on-disk format change, no dependency on rebuilding `MOD_SEQS`/`MOD_NUMBERS`
at load time, no build-side changes at all) and still gets ~2.5x reduction (10.24 GiB -> ~4.0 GiB:
4.66M x ~64B raw table + 1.249e8 x 32B compact record). But it does nothing for build time or disk
I/O (still writes/reads the full 88B/entry file), and it's strictly dominated by the target design
once the target design's on-disk format change is accepted -- the target design needs no grouping
pass (Phase A/B of the build already knows the raw-peptide/variant split before it's thrown away)
and additionally shrinks the `.idx` file itself and speeds up `-j` index build (skips the
`MaterializeIndexPeptideMods()` full-enumeration pass entirely, matching FI_DB's much faster `-i`
build: 15s vs. today's PI_DB's 3m30s in the reference build). Documented here as the fallback if the
on-disk format change in Section 4 turns out to be unacceptable for some reason (e.g. a hard
requirement to keep old `.idx` files readable).

**Update (resolving Open Question 3, Section 8):** the target design goes further than "PI_DB gets
its own new compact format" -- PI_DB and FI_DB share a single `.idx` file. See Section 4, Phase 0.

## 4. Implementation plan (target design)

### Phase 0 -- unify the PI_DB/FI_DB on-disk format and add explicit search-mode selection

**IMPLEMENTED (2026-07-30), with two changes from the plan below, both found necessary during
implementation:**

1. **Mod-permutation tables (`MOD_SEQS`/`MOD_NUMBERS`/`MOD_SEQ_MOD_NUM_START`/
   `MOD_SEQ_MOD_NUM_CNT`/`PEPTIDE_MOD_SEQ_IDXS`) are persisted directly in the `.idx` file and
   read back as-is, not recomputed at load time.** Investigating FI_DB's pre-unification plain-index
   format (`WriteFIPlainPeptideIndex()`/`ReadPlainPeptideIndex()`, both since retired) found it
   already did this -- an earlier assumption in this doc's Section 8 (Open Question 2 resolution:
   "recompute, don't reserialize... the same cost FI_DB already pays") was wrong; FI_DB reserializes,
   it never recomputes at load. Reusing FI_DB's proven approach also retroactively fixed a real
   latent bug in this doc's own Phases 1-3 implementation: recomputing via `PermuteIndexPeptideMods()`
   at load time (as originally implemented) required several mod-related fields --
   `iMaxNumVarModAAPerMod` per mod slot, the overall `max_variable_mods_in_peptide` cap, and
   `peptideLengthRange` -- to be byte-for-byte reproduced from the search-time environment, none of
   which any earlier `.idx` header version actually persisted. If search-time `comet.params` ever
   set different values for these than build time, `modNumIdx` lookups would silently resolve to the
   wrong mod combination. Persisting the tables directly removes this class of bug entirely, for
   both search modes.
   **Superseded 2026-07-31 -- see the Phase 0.5 section below.** This decision to persist rather than
   recompute is reversed there, but for a different reason than the bug being worked around here: the
   bug above only exists because a *separately persisted* compact variant array referenced the
   recomputed table by index. Phase 0.5 removes that referencing structure entirely rather than
   re-adding recompute alongside it, so the bug class doesn't reappear.
2. **`ProteinModList:`/`RequireVariableMod:` header lines** (previously FI_DB-only) are now written
   and parsed for both modes, since `CometPeptideIndex::ParsePeptideIndexHeader()` became the one
   shared header parser.

The rest of this section is the original plan, which still describes the mechanism accurately.
Section 8 covers the resolved open questions.

**Validation.** Full unit/integration suite passes (40/40 default tests, T17/T18 real 8.9M-peptide
builds). Real-data check: built one unified index (`comet-debug3`'s `human.fasta`, M-oxidation,
572 real spectra), searched it as both PI_DB (`index_search_type=0`) and FI_DB (`index_search_type=1`)
from the exact same file, and diffed each mode's `.txt` output against its own pre-unification
baseline (built with the pre-Phase-0 binary, PI_DB via its old separate v2 format, FI_DB via its old
separate plain-index format) -- **byte-identical in both modes.** PI_DB and FI_DB legitimately
disagree with *each other* on this dataset (432 vs. 65 PSM rows) but that gap is pre-existing and
reproduced identically pre- and post-unification, not something this change introduced.

Two bugs surfaced and fixed during implementation, both the same class of mistake: an
`strncmp(buf, "literal string", N)` magic-string check where `N` didn't match the literal's actual
length (`"Comet index database"` is 20 characters, not the 21 first written; same one-off error
made twice, independently, at two different call sites). Both now use `sizeof("literal") - 1`
instead of a hardcoded length, which can't drift out of sync with the string. Caught by T19/T20
failing immediately -- worth remembering as a reason to prefer `sizeof(...)-1` over a manually
counted length for this pattern going forward.

Separately, `index_search_type` needed registering in **two** independent places to actually take
effect from a `comet.params` file: `CometSearchManager`'s internal `SetParam`/`GetParamValue` map
(added in Task 12) and `Comet.cpp`'s `LoadParameters()` key-to-parser-lambda table (a separate,
explicit allow-list the batch comet.params text-file reader uses -- unrecognized keys in a
`comet.params` file are silently ignored, not forwarded). RTS doesn't have this second gate; its
`SetParam` calls from `RealtimeSearch/SearchMS1MS2.cs` go straight to the shared map.

Per Section 8's resolution of Open Question 3, PI_DB and FI_DB are to share one `.idx` file rather
than each having its own format. This removes today's implicit PI-vs-FI dispatch and replaces it
with an explicit one:

- **Today's dispatch is auto-detected from the file itself.** `CometSearchManager.cpp:1490-1550`
  opens the `.idx` file, reads its first line, and sets `g_staticParams.iDbType` based on which of
  two magic strings it finds (`"Comet peptide index"` at line 1518 vs. `"Comet fragment ion index"`
  at line 1532) -- this is only possible today because PI_DB and FI_DB write distinct headers into
  distinct on-disk formats (`CometPeptideIndex.cpp:614` vs. `CometFragmentIndex.cpp:960`). Once both
  backends read the *same* file, this header can no longer imply which search mode to run.
- **New mechanism: an explicit search-mode parameter**, e.g. a new `comet.params` key (working name
  `index_search_type = 0`, 0=peptide-index-style mass search / 1=fragment-ion-index-style search,
  naming TBD) read alongside `database_name`. `CometSearchManager.cpp:1518-1540`'s two branches
  collapse into one: keep the header check only as a "this is a valid Comet index file" sanity
  check, and set `g_staticParams.iDbType` from the new parameter instead of from which magic string
  matched.
- **Batch search** already loads `comet.params` for every run, so this is a normal new param, no
  extra plumbing.
- **RTS does not load `comet.params` at all today.** `RealtimeSearch/SearchMS1MS2.cs`'s `Main()` (the
  code path actually used by `RealtimeSearch.exe [query] [MS1ref] [database.idx] [threads]
  [ascorepro]`) never reads a params file -- every other search setting is either baked into the
  `.idx` file at build time or defaulted on the C++ side, and `Main()` only calls
  `globalSearchMgr.SetParam(...)` for a handful of explicit overrides (e.g. `ms1_mass_range` at line
  186). Since a shared index can no longer imply search mode, and RTS has no params file to read it
  from, this needs a new positional CLI argument (mirroring how `[ascorepro]` was already added as an
  RTS-specific bolt-on rather than sourced from `comet.params`), forwarded via a new
  `globalSearchMgr.SetParam("index_search_type", ...)` call in `Main()`.
- **Build-time collapse**: today's separate `-j` (PI_DB, `WritePeptideIndex()`) and `-i` (FI_DB,
  `WriteFIPlainPeptideIndex()`) command-line build modes collapse into one build (Phase 1 below),
  since there is only one on-disk artifact to produce now. The fragment-ion posting list itself is
  *not* part of that shared on-disk artifact -- both today's FI_DB RTS startup and this design's
  FI-mode search build `g_iFragmentIndex`/`g_iFragmentIndexOffset` at load time from the shared
  plain-peptide/variant data (see Phase 2); PI-mode search simply skips that step.

### Phase 1 -- build/write (`CometPeptideIndex::WritePeptideIndex`, `CometPeptideIndex.cpp:471+`)

- After Phase A/B already produce `g_vRawPeptides` + `PermuteIndexPeptideMods()`'s tables
  (`CometPeptideIndex.cpp:520-537`), **stop calling `MaterializeIndexPeptideMods()`**
  (`CometPeptideIndex.cpp:539-546`). Instead, enumerate the same `(iWhichPeptide, modNumIdx,
  cNtermMod, cCtermMod)` tuples `MaterializeIndexPeptideMods()`'s outer loop already walks
  (lines 343-454) but only compute `dPepMass` (needed for the mass-sort) and mass-range filtering
  per tuple -- skip building `pcVarModSites`/`sPeptide`/flank copies entirely, push a
  `FragmentPeptidesStruct`-shaped record instead of a `DBIndex`.
- Sort the compact array by `dPepMass` (replaces `CometMassSpecUtils::DBICompareByMass` sort at
  `CometPeptideIndex.cpp:589`).
- **Dedup (resolving Open Question 1, Section 8): drop the sequence+mod-state+protein-position dedup
  pass entirely** (`CometPeptideIndex.cpp:585-586`). It's a no-op given how index creation changes
  under this design -- Phase A already guarantees unique raw peptides and Phase B's enumeration is
  already non-duplicating (the existing comment at `CometPeptideIndex.cpp:582-584` already called
  this "a defensive no-op in the common case" even for today's legacy-build-path-derived input; it
  has no remaining purpose once the legacy per-protein `RunSearch()` build path is fully out of the
  picture). No dedup step needed in the new compact-array build.
- On-disk layout: this is no longer a PI_DB-specific format -- it's the single shared format from
  Phase 0. Write `g_vRawPeptides` (reuse `CometFragmentIndex`'s plain-index writer if it's already
  factored out, otherwise mirror its format, i.e. converge with `WriteFIPlainPeptideIndex()`,
  `CometFragmentIndex.cpp:891+`) plus the sorted compact variant array, replacing both today's PI_DB
  per-entry `ReadPeptideIndexEntry`-format block (`CometPeptideIndex.cpp:35-41`) and today's FI_DB
  plain-index format as two separate things. **Bump the file's header/version string** (today's
  checks are `"Comet peptide index database"` at `CometPeptideIndex.cpp:68` and `"Comet fragment ion
  index plain peptides"` at `CometFragmentIndex.cpp:960` -- both retired in favor of one new shared
  magic string) so old-format files of either kind are rejected with a clear "rebuild your index"
  error rather than silently misread.

### Phase 2 -- read/load (`CometPeptideIndex::ReadPeptideIndex`, `CometPeptideIndex.cpp:43-222`)

- Read `g_vRawPeptides` and the compact variant array directly (both flat, pre-`reserve()`d reads,
  same pattern as today's `g_pvDBIndex` read at lines 159-179). Given Phase 0's format unification,
  this read path converges with (or is shared with) whatever loads FI_DB's plain index today.
- **Recompute, don't reserialize (resolving Open Question 2, Section 8): call
  `PermuteIndexPeptideMods()` at load time** to rebuild `MOD_SEQS`/`MOD_NUMBERS`/
  `PEPTIDE_MOD_SEQ_IDXS` from `g_vRawPeptides` + the current run's `comet.params` mod settings,
  rather than persisting those tables in the `.idx` file. This is the same cost FI_DB already pays
  today (~20s of its ~60s "generate fragment ion index" step at RTS startup, per the 20260729 RTS
  benchmark notes) and is accepted as-is for the shared format -- no new serialization format to
  design or keep in sync with `PermuteIndexPeptideMods()`'s logic.
- `g_staticParams.iDbType` is set from the Phase 0 search-mode parameter (`PI_DB` or `FI_DB`), not
  hardcoded -- both modes share this same load step; FI_DB additionally builds the fragment-ion
  posting list from the now-loaded variant array afterward, PI_DB does not.
- `g_bPeptideIndexRead = true` as today.

### Phase 3 -- search (`CometSearch::SearchPeptideIndex(Query*, ...)`, `CometSearch.cpp:1890-1936`)

- Factor `MaterializeIndexPeptideMods()`'s `buildEntry` lambda (`CometPeptideIndex.cpp:274-341`) out
  into a standalone single-entry function, e.g.
  `CometPeptideIndex::MaterializeOneEntry(size_t iWhichPeptide, int modNumIdx, char cNtermMod, char
  cCtermMod, DBIndex& out)`, callable from both `WritePeptideIndex()` (if any residual eager-build
  path still needs it) and `CometSearch.cpp`.
- Binary search (`CometSearch.cpp:1904-1911`) runs against the compact array's `dPepMass` instead of
  `g_pvDBIndex[iMid].dPepMass` -- identical algorithm, smaller array to search.
- For each mass-window survivor (`CometSearch.cpp:1916-1926`), call `MaterializeOneEntry()` to build
  a stack-local `DBIndex` on the fly, then call `AnalyzePeptideIndex(pQuery, dbiLocal, ...)`
  unchanged. **No changes needed inside `AnalyzePeptideIndex` or any scoring code** -- this is the
  same shape FI_DB's `FragmentIndexReader` pattern already establishes.

### Phase 4 -- cleanup

- Remove `DBIndex`'s `sPeptide[51]`/`VarModSites` bulk fields from the persistent in-memory array
  entirely (they only exist transiently now, one stack-local instance per scored candidate).
- Legacy per-protein `g_pvDBIndex.push_back()` build sites (`CometSearch.cpp:2858`, `:6975`) are the
  *old* `RunSearch()`-driven build path already superseded by Phase A/B
  (`docs/20260713_PIidxformat.md`) -- confirm these are dead code on the current build path before
  touching them; out of scope for this change if so, but worth a follow-up cleanup note.

### Phase 0 follow-up -- RTS auto-build regressions found and fixed (2026-07-30)

Actually exercising `RealtimeSearch.exe` against a *missing* `.idx` (the "auto-build from the
underlying FASTA if the requested index doesn't exist yet" path) surfaced two real bugs, both in
`CometSearchManager.cpp`/`CometPeptideIndex.cpp`, neither caught by the batch-only validation this
doc originally reported:

1. **`CometPeptideIndex::WritePeptideIndex()` didn't handle a `database_name` that already ends in
   `.idx`.** RTS's auto-build path passes the not-yet-existent `.idx` path directly as
   `database_name` (not a `.fasta` path with `.idx` appended, the way batch `-i`/`-j` usage always
   does), so `WritePeptideIndex()`'s original `sprintf(szIndexFile, "%s.idx", database_name)` produced
   a doubly-suffixed path, and `GeneratePlainPeptideIndex()` tried to open the (nonexistent) `.idx`
   file itself as the FASTA to digest. This exact case was handled correctly by the retired
   `WriteFIPlainPeptideIndex()` (strip the `.idx` suffix in place before digesting, restore it
   afterward) but that handling was lost when the function was consolidated into
   `WritePeptideIndex()`. Fixed by porting the same strip/restore logic over.
2. **RTS could reach `iDbType == PI_DB` with a missing `.idx` but nothing would build it.**
   `ValidateSequenceDatabaseFile()`'s "requested `.idx` missing, underlying FASTA exists" branch
   unconditionally set only `bCreateFragmentIndex = true` (a holdover from when FI_DB was the only
   mode RTS could auto-build, since PI_DB mode was previously unreachable without an
   already-existing PI-formatted `.idx` to sniff -- there was no way to *request* PI_DB before
   `index_search_type` existed). `InitializeSingleSpectrumSearch()`'s PI_DB branch, in turn, went
   straight to `ReadPeptideIndex()` with no build call at all. Fixed on both ends: the "missing"
   branch now sets `bCreatePeptideIndex` + `iDbType = PI_DB` when `index_search_type == 0` (mirroring
   the FI_DB case), and the PI_DB branch in `InitializeSingleSpectrumSearch()` gained a
   `WritePeptideIndex()` call (+ the matching `CometSearch::AllocateMemory()` re-allocation) mirroring
   the FI_DB branch's existing `CreateFragmentIndex()` call.

Both fixes validated by re-running the auto-build scenario for both modes (fresh `.idx`, real Hela
`.raw` data, real phospho/oxidation-modified peptides scored correctly in the output) and by a full
unit/integration suite re-run (40/40 + T17/T18) after each fix. Also validated in this same pass:
a full Linux `make` build (previously never attempted -- separate toolchain from the Windows MSBuild
path all earlier validation in this doc used) compiles cleanly with zero errors, and the resulting
Linux binary passes the identical 40/40 + T17/T18 suite with byte-identical peptide counts to
Windows.

### Phase 0.5 -- drop persisted mod-permutation tables and compact variant array; full regen per session (planned, 2026-07-31)

**Motivating question.** If modified peptides are expanded from `g_vRawPeptides` at the start of
every search session anyway, does `VariableMod:` (and `ProteinModList:`/`RequireVariableMod:`) still
need to live in the `.idx` header at all, or can that generation step just read `comet.params`
directly, the same way a non-indexed FASTA search already does?

**Why Phase 0 answered "yes, it must stay in the header."** Under Phase 0's design, generation is
only *half* re-run at search time: `MOD_NUMBERS`/`MOD_SEQS`/`PEPTIDE_MOD_SEQ_IDXS` and the full
per-variant array (mass + `modNumIdx` + terminal mod codes) are built once at index-build time and
persisted; search time only re-derives a per-candidate `DBIndex` (`MaterializeOneEntry()`) from
those frozen tables. `modNumIdx` is a foreign-key-like reference into a *specific* `MOD_NUMBERS`
table, built from a *specific* `varModList[]` order/composition. If the live `varModList[]` at
search time (whether restored from `comet.params` or anything else) doesn't match what built the
frozen tables, `modNumIdx` lookups and `MaterializeOneEntry()`'s independently-recomputed mass
resolve against the wrong data -- candidates get selected against one mass and scored against
another, or a compacted mod-slot index resolves to the wrong residue. `VariableMod:` exists purely
to pin the live value to match the frozen one; nothing else does.

**Phase 0.5: remove the frozen half instead of pinning against it.** Persist only what genuinely
can't be redone without re-digesting the FASTA -- enzyme, static mods, decoy mode, peptide mass
range, peptide length range (these determine which raw peptides physically exist in
`g_vRawPeptides`, so they must stay fixed at build time). Everything downstream of
`g_vRawPeptides` -- `MOD_NUMBERS`/`MOD_SEQS`/`PEPTIDE_MOD_SEQ_IDXS` (`PermuteIndexPeptideMods()`) and
the full mass-sorted variant array -- is generated fresh in memory once per search session, for both
PI_DB and FI_DB, directly from whatever `comet.params` says at that moment. Nothing referencing the
old, frozen shape is ever persisted, so there is nothing for a mismatched `varModList[]` to
disagree with. This is not the same design as the recompute approach the Phase 0 IMPLEMENTED note
above rejected: that one *also* persisted a compact variant array whose `modNumIdx` entries assumed
the recomputed table would come out byte-identical to build time. Phase 0.5 persists no such array,
so there's no reference that can go stale regardless of what `comet.params` says.

**Session lifecycle (2026-07-31, confirmed).** Regeneration happens exactly once per session:
batch search regenerates once at process start and holds the result for the run; RTS regenerates
once inside `InitializeSingleSpectrumSearch()` and holds it until
`CometSearchManager::FinalizeSingleSpectrumSearch()` tears it down. There is no mechanism for
`comet.params` to change mid-session, so there is no invalidation/regeneration-trigger logic to
design and no concurrency hazard between a live search thread and a param mutation -- a session's
in-memory mod state is immutable for its entire lifetime by construction. A new session with
different mod settings always starts from a clean regeneration.

**Cost -- confirmed acceptable.** Regeneration cost is `PermuteIndexPeptideMods()` (rebuild
`MOD_NUMBERS`/`MOD_SEQS`) plus enumerating and mass-sorting the full variant array, i.e. the same
work Phase 1's `WritePeptideIndex()` already does at build time, now repeated once per session
instead of stored on disk. This is the same order of cost FI_DB's RTS startup already pays today
for the `MOD_NUMBERS` half alone (~20s of its ~60s "generate fragment ion index" step at 8.9M-peptide
scale, Section 4 Phase 0's numbers) -- confirmed acceptable for both search modes, including RTS.
No new memory cost: the regenerated array's in-memory shape is identical to Phase 0's persisted
version (`FragmentPeptidesStruct`, 24B/variant); only its source changes, from disk read to
in-memory build. A useful side effect: since both modes now call the same regeneration step, the
`g_vDBIndexVariants`/`g_vFragmentPeptides` split noted as a follow-up in `docs/GlobalVariables.md`
(two structurally-identical globals because PI_DB/FI_DB didn't yet share a build/dispatch path for
the array) can be collapsed into one shared array as part of this work.

**On-disk format impact.** The `.idx` file shrinks back down: header (`MassType:`/`StaticMod:`/
`Enzyme:`/`Enzyme2:`/`DecoySearch:`/peptide mass range/peptide length range only --
`VariableMod:`/`ProteinModList:`/`RequireVariableMod:` are removed) + protein names + raw peptide
table (`g_vRawPeptides`) + proteins list. The permutation-table section (`.clPermutationsFilePos`)
and the persisted compact variant array section (`.clVariantsFilePos`), both added in Phase 0's
commit (`022da11f`), are removed for both search modes -- the footer goes from 4 pointers back to 2
(`clPeptidesFilePos`/`clProteinsFilePos`). Existing Phase-0-format `.idx` files fail the version
check by design and require a rebuild, same as every prior format change in this document.

**Reproducibility note.** Previously the `.idx` file alone fully determined the search space (mods
included). After this change, `.idx` + `comet.params`'s mod settings jointly determine it -- the
same way a non-indexed FASTA search has always worked, and consistent with the premise that
motivated this change: variable mods are a per-search choice, not a property of the index.

## 5. Compatibility

This changes the on-disk `.idx` file format for **both** PI_DB and FI_DB -- per Section 8's
resolution of Open Question 3, they converge onto one shared format rather than PI_DB alone getting
a new format. Existing `.idx` files built with older Comet versions (either kind) will fail the
header/version check (by design, per Phase 0/1) and require a rebuild with the new unified build
step. The `-i`/`-j` build-flag distinction goes away at the same time (Phase 0) since there is only
one build path left to invoke. A new `comet.params` key, `index_search_type` (and, for RTS, a new `RealtimeSearch.exe` CLI
argument, also `index_search_type`), selects PI-style vs. FI-style search against a given index.
Omitting it (unset, `-1`) defaults to `FI_DB`, matching the pre-unification default for the
"ambiguous `.idx` specified" case (`CometSearchManager.cpp`'s `ValidateSequenceDatabaseFile()` and
the dispatch blocks in `InitializeStaticParams()`/`InitializeSingleSpectrumSearch()` -- see Section
4, Phase 0 above for the current call sites).

**Phase 0.5 changes this again.** The header shrinks to `MassType:`/`StaticMod:`/`Enzyme:`/
`Enzyme2:`/`DecoySearch:`/peptide mass range/peptide length range -- `VariableMod:`/
`ProteinModList:`/`RequireVariableMod:` are removed and become ordinary `comet.params` keys, read at
search time exactly as they already are for a non-indexed FASTA search. The footer drops from 4
pointers to 2 (permutation-table and compact-variant-array sections removed). This is another
breaking format-version bump: Phase-0-format `.idx` files (and older) fail the version check and
require a rebuild. `index_search_type` dispatch is unaffected by Phase 0.5 -- it's an orthogonal
axis (which search algorithm to run) from what Phase 0.5 changes (what's persisted vs. regenerated
per session).

## 6. Performance risk

The one behavior genuinely new to PI_DB is per-candidate `DBIndex` reconstruction inside the
mass-window scan loop, replacing an array index. Risk is low given FI_DB's `XcorrScoreI` caller
already does the equivalent (arguably more expensive, since it also resolves posting-list entries)
per-candidate reconstruction today at thousands of Hz. Still, `MaterializeOneEntry()` walks
`MOD_NUMBERS`/`MOD_SEQS` per call (`CometPeptideIndex.cpp:284-306`), which is more work than an array
index -- confirm with a real-data before/after Hz comparison (same methodology as
`feedback_rts_harness_hz_methodology`: measure both pure search-call time and full wall-clock, both
old and new) before calling this change performance-neutral. If `MaterializeOneEntry()` shows up as
a hot spot, the fallback design in Section 3 (`pcVarModSites` kept inline, no `MOD_NUMBERS` lookup
needed per candidate) trades some of the memory win back for lower per-candidate CPU cost.

## 7. Testing plan

- Existing unit/integration coverage: T19 (FI_DB build+search+AScorePro) and T20 (PI_DB
  build+search+AScorePro regression) per `docs/20260715_fusedflush.md`'s test notes -- both must
  still pass.
- Per this project's established integration-test convention, validate PSM-count *stability ranges*
  across the change, not exact cross-version byte-for-byte comparison (a known v2026.01.1 I/L
  long-path dedup bug already makes exact comparison unreliable as a baseline).
- Per this project's established convention for changes touching shared
  preprocessing/scoring code paths, don't rely on unit tests alone: run a full-scale real `.raw` +
  real-database batch-vs-RTS PI_DB comparison (old format/materialized-array build vs. new
  compact-record build) and confirm byte-identical scored output (peptide, xcorr, e-value, charge,
  masses, AScore, sites, protein) across all PSMs, the same methodology already used to validate the
  RTS/batch `iHighestIon` fix and the E-value jitter fix.
- FDR/q-value comparisons, if run, must use rank-1 PSMs only (all-rank FDR is not methodologically
  valid for this codebase's conventions) -- use `tools/qvalue.py --diff` against old vs. new builds.
- If a no-enzyme integration test is included, cap `len_max` at 13 (len_max=25 is known to time out
  at 300s against `human.small.fasta`).

## 8. Open questions -- resolved

1. **Does the dedup pass in current `WritePeptideIndex()` (`CometPeptideIndex.cpp:585-586`) still
   need an equivalent in the new build?** No -- **it's a no-op given the changes to PI index
   creation.** Phase A's per-thread digestion already guarantees unique raw peptides and Phase B's
   enumeration is deterministic and non-duplicating per peptide, so the compact-array build (Section
   4, Phase 1) drops the dedup step entirely rather than porting it to work on
   `(iWhichPeptide, modNumIdx, cNtermMod, cCtermMod)` tuples.

2. **Recompute vs. reserialize `MOD_SEQS`/`MOD_NUMBERS`/`PEPTIDE_MOD_SEQ_IDXS` at load time?**
   Recompute -- **acceptable for PI_DB, the same as it already is for FI_DB.** No new serialization
   format needed for the mod-permutation tables; `ReadPeptideIndex()` calls
   `PermuteIndexPeptideMods()` at load time exactly as FI_DB's RTS startup already does (~20s of its
   ~60s "generate fragment ion index" step), and that cost is accepted rather than engineered around.

3. **Should PI_DB and FI_DB share one `.idx` file/format, or stay separate?** Share --
   **PI_DB and FI_DB can definitely share the same single `.idx`.** This is folded into the design as
   Section 4's new Phase 0, not deferred as a follow-up: one build path (retiring the `-i`/`-j`
   split), one on-disk format, one load path producing `g_vRawPeptides` + `g_vFragmentPeptides` for
   both. Since the file itself can no longer imply which search mode to run (today's dispatch,
   `CometSearchManager.cpp:1490-1550`, sniffs the file's own header -- see Phase 0 for why that stops
   working once both backends share a header), an explicit runtime selector is required: a new
   `comet.params` search-parameter entry for batch search, and a corresponding new RTS CLI argument
   (since `RealtimeSearch.exe`'s `Main()` doesn't load `comet.params` at all today) for RTS. FI_DB
   additionally builds the fragment-ion posting list from the shared data at load time; PI_DB does
   not; that's the only remaining divergence between the two modes once this design lands.

4. **Does `VariableMod:` (and `ProteinModList:`/`RequireVariableMod:`) need to persist in the `.idx`
   header, given modified peptides are generated from the raw-peptide table at search time anyway?**
   Not if nothing downstream of the raw-peptide table is *also* persisted. Phase 0's answer was "yes"
   because it persisted a compact variant array whose `modNumIdx` entries were foreign keys into a
   specific, frozen `MOD_NUMBERS` table -- `VariableMod:` existed solely to keep the live
   `varModList[]` pinned to whatever built that frozen table. Phase 0.5 (Section 4) removes the
   frozen table and the variant array entirely and regenerates both, from `g_vRawPeptides` +
   live `comet.params`, once per search session -- with nothing persisted to go stale, the source of
   mod definitions stops being load-bearing for correctness, so `comet.params` can be the sole
   source, the same as it already is for non-indexed FASTA search. The cost (full
   `PermuteIndexPeptideMods()` + variant enumeration once per session, ~20s at 8.9M-peptide scale per
   Phase 0's numbers) and the session lifecycle (regenerate once at session start, immutable for the
   session's duration, no mid-session `comet.params` mutation is possible) were confirmed acceptable.

## 9. Reproducing the measurement

The `MEMPROBE` instrumentation used for Section 1's numbers is a small uncommitted diff to
`RealtimeSearch/SearchMS1MS2.cs` (prints `Process.WorkingSet64`/`PrivateMemorySize64` right after
`InitializeSingleSpectrumSearch()` returns, after a forced `GC.Collect()`). The fresh index builds
used for the isolated measurement live at `20260420-human-phosho/memprobe/{pi,fi}/` (~6.9 GB +
~1.0 GB on disk) and can be reused directly rather than rebuilt.
