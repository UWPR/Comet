# PI_DB memory reduction: splitting DBIndex into a raw-peptide table + compact per-variant record

Status: PLANNING ONLY. No implementation yet.

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

## 5. Compatibility

This changes the on-disk `.idx` file format for **both** PI_DB and FI_DB -- per Section 8's
resolution of Open Question 3, they converge onto one shared format rather than PI_DB alone getting
a new format. Existing `.idx` files built with older Comet versions (either kind) will fail the
header/version check (by design, per Phase 0/1) and require a rebuild with the new unified build
step. The `-i`/`-j` build-flag distinction goes away at the same time (Phase 0) since there is only
one build path left to invoke. A new `comet.params` key (and, for RTS, a new
`RealtimeSearch.exe` CLI argument) is required at search time to select PI-style vs. FI-style search
against a given index -- omitting it needs a defined default (candidate: default to `FI_DB`, matching
today's existing behavior when a `.idx` path is given that doesn't parse as a known PI_DB header,
`CometSearchManager.cpp:1499-1506`).

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

## 9. Reproducing the measurement

The `MEMPROBE` instrumentation used for Section 1's numbers is a small uncommitted diff to
`RealtimeSearch/SearchMS1MS2.cs` (prints `Process.WorkingSet64`/`PrivateMemorySize64` right after
`InitializeSingleSpectrumSearch()` returns, after a forced `GC.Collect()`). The fresh index builds
used for the isolated measurement live at `20260420-human-phosho/memprobe/{pi,fi}/` (~6.9 GB +
~1.0 GB on disk) and can be reused directly rather than rebuilt.
