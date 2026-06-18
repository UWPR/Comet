# Code Review: architecture_update branch (2026-06-17) -- independent pass

## Scope

Independent review of the `architecture_update` branch versus `master`, at branch tip
commit `0e10e71f` (74 files changed, +5,936/-3,275 lines). Performed without reference
to the same-day reviews in `docs/20260617_codereview.md` and `docs/20260617_codereview2.md`,
per request, as a second independent pass over the Strategy/Pipeline refactor:
`ISearchStrategy` (`FiStrategy` / `PiStrategy` / `FastaStrategy`) + `Pipeline` replacing
the monolithic `CometSearchManager::DoSearch` per-file loop, `SearchSession` replacing
the batch-path globals, `SearchMemoryPool` with RAII slot guards, and a new
`output/IResultWriter` layer wrapping the existing `CometWrite*` classes.

Method: clean rebuild (`make cclean && make -j$(nproc)`) with a warning scan; full unit
+ integration test run (19/19 passed, including the T18 byte-identical determinism
check); manual line-by-line trace of the ~1,232 lines removed from
`CometSearchManager.cpp` against their new homes in `SearchUtils.cpp` / the strategy
classes to confirm behavior was preserved; targeted reads of `SearchMemoryPool`,
`Pipeline`, `SearchSession`, all three strategies, all five `IResultWriter`
implementations, and `core/Params.h` / `Types.h` / `Constants.h`.

---

## 1. Summary

Build is clean under `-Wall -Wextra` (zero warnings) and all 19 tests pass. The
extraction of `DoSearch`'s per-file loop into `Pipeline` + strategy classes is largely
faithful -- the diff was traced line-by-line and the removed logic reappears intact in
`SearchUtils.cpp` and the three strategy `.cpp` files, including the per-batch writer
open/write/close lifecycle and the FASTA/idx file-handle handling. The latest commit's
exception-safety fix (`SearchMemoryPoolSlotGuard` applied at all five
acquire/release sites) is correctly done. One concrete correctness regression was found:
reordering AScore initialization relative to fragment-index loading silently breaks
AScorePro phosphosite scoring for batch FI_DB searches. A few maintainability gaps in
the new abstraction are also worth hardening before this lands on `master`.

**Status (2026-06-17): all items closed, plus one additional critical bug (2b) found
during live testing after this review.** Issue 2a, 2b, all of section 3, and all of
section 4 have been fixed -- see the per-item status notes below, including two new
regression tests (`t19`, `t20`) each verified to fail against its respective pre-fix
code and pass against the fix. Rebuilt clean (`make cclean && make -j$(nproc)`, zero
warnings) and re-ran the full unit + integration suite (21 passed, 0 failed, 0 skipped)
after the final round of fixes.

---

## 2. Critical Issues

### 2a. AScorePro configured with stale variable-mod data for batch FI_DB searches

**Files:** `CometSearch/CometSearchManager.cpp:2110-2119` (new AScore-init call site)
vs. `CometSearch/search/FiStrategy.cpp:67-83` (index load, now run afterward)

`SetAScoreOptions(g_AScoreOptions)` is now called once, unconditionally, near the top
of `DoSearch()` -- *before* `Pipeline::run()` constructs and initializes the strategy.
For `FI_DB` (fragment-index) searches, `FiStrategy::initialize()` subsequently calls
`CometFragmentIndex::ReadPlainPeptideIndex()`, which **overwrites**
`g_staticParams.variableModParameters.varModList[].dVarModMass / szVarModChar /
dNeutralLoss` from the `.idx` file's `VariableMod:` header line
(`CometFragmentIndex.cpp:1276-1310`). `SetAScoreOptions()` reads exactly those fields
(`CometSearchManager.cpp:3225-3258`) to build the AScore differential-mod list.

Pre-refactor, this was correctly sequenced: the diff shows the old code ran
`ReadPlainPeptideIndex()` / `CreateFragmentIndex()` *first*, then `SetAScoreOptions()`
second, inside the per-file loop guarded by a `bPerformAScoreInitialization` flag. The
RTS path (`InitializeSingleSpectrumSearch`, `CometSearchManager.cpp:2268-2287`) still
gets this right and even carries a comment explaining why: *"normally set at end of
InitializeStaticParams; must do here again after ReadPlainPeptideIndex for single
spectrum search."* The same re-sync was not preserved for the batch path after the
refactor.

`PI_DB` is not affected: `CometSearch::SearchPeptideIndex` (`CometSearch.cpp:1880-1903`)
lazily re-parses the index header and re-calls `SetAScoreOptions` on first invocation,
guarded by `g_bPeptideIndexRead`, so it self-heals. `FI_DB` has no equivalent internal
correction.

**Impact:** any batch search against a prebuilt fragment index with
`print_ascore_score` enabled will configure AScorePro using whatever variable-mod
values happened to already be in `g_staticParams` *before* the index header was parsed
-- commonly empty/default, since FI_DB search-time params files don't need to redeclare
variable mods (they're embedded in the index). AScore site-localization scores would
silently be computed against the wrong (or no) differential mod, with no error raised.

**Fix:** move the `SetAScoreOptions` / `CreateAScoreDllInterface` block in `DoSearch()`
to after the strategy's `initialize()` has run, e.g.:

```cpp
// CometSearchManager.cpp, after strategy selection
if (!pStrategy->initialize(session, tp)) { pStrategy->finalize(); return false; }
if (g_staticParams.options.iPrintAScoreProScore)
{
   SetAScoreOptions(g_AScoreOptions);
   g_AScoreInterface = CreateAScoreDllInterface();
   if (!g_AScoreInterface) { std::cerr << "Failed to create AScore interface." << std::endl; return false; }
}
```

This also avoids creating an AScore interface for an `FI_DB` run whose index fails to
load.

**Status (2026-06-17):** Fixed. The AScore init/teardown block was moved out of
`CometSearchManager::DoSearch()` and into `Pipeline::run()` (`CometSearch/search/
Pipeline.cpp`), rather than patched in place, so the fix covers every strategy through
one call site instead of duplicating the re-sync logic per strategy (see Actionable
Improvement 4b, also closed by this change). `SetAScoreOptions()` /
`CreateAScoreDllInterface()` now run immediately after `_strategy->initialize(session,
&tp)` succeeds -- i.e. after `FiStrategy::initialize()` has already called
`ReadPlainPeptideIndex()` for FI_DB runs -- and `DeleteAScoreDllInterface()` now runs
right after `_strategy->finalize()` at the end of `run()`, matching the original
unconditional teardown. A failure to create the AScore interface now also calls
`_strategy->finalize()` before returning, so the strategy's allocated memory pools are
not leaked on that error path (a small improvement over the pre-fix code, which
returned without finalizing on this same error). Verified with a clean rebuild (zero
warnings) and the full 19-test unit + integration suite.

---

### 2b. Batch PI_DB search crashes on the first scored candidate (`_pQueries` never assigned)

**File:** `CometSearch/CometSearch.cpp:1862` (`SearchPeptideIndex(ThreadPool*, vector<Query*>&)`)

**Discovered:** 2026-06-17, reported against the VS-built Windows binary running a real
peptide-index (`-j`) search via WSL interop: the process printed
`- searching "<file>" ...` and then exited with no further output, no error message,
and no result file -- a silent crash, not a hang.

`CometSearch::BinarySearchMass()` and the `AnalyzePeptideIndex(int iWhichQuery, ...)`
overload read the active query list through a `CometSearch` member, `_pQueries`,
rather than a parameter. `CometSearch::DoSearch()` (the FASTA path) sets
`_pQueries = &queries;` at entry for exactly this reason. The architecture refactor
changed `BinarySearchMass()` from reading the old global `g_pvQuery` directly to
reading it through `_pQueries`, and updated `DoSearch()` accordingly, but
`SearchPeptideIndex(ThreadPool*, vector<Query*>&)` -- the PI_DB batch path, called from
a freshly constructed `CometSearch* sqSearch = new CometSearch();` in
`CometSearch::RunSearch(int, int, ThreadPool*, vector<Query*>&)` -- was never updated
to set `_pQueries`. It stayed `nullptr` (the class's default member initializer), and
the first call into `BinarySearchMass()` dereferenced it, segfaulting before any
output was written.

**Reproduced locally** with a minimal fixture (T19's phospho peptide/spectrum, built as
a PI_DB index instead of FI_DB) and confirmed via `gdb` backtrace:

```
#0  CometSearch::BinarySearchMass(int, int, double) const
#1  CometSearch::SearchPeptideIndex(ThreadPool*, vector<Query*>&)
#2  CometSearch::RunSearch(int, int, ThreadPool*, vector<Query*>&)
#3  RunSearchAndPostAnalysis(int, int, ThreadPool*, SearchSession&, bool)
#4  Pipeline::run(SearchSession&, vector<InputFileInfo*> const&, ThreadPool&)
#5  CometSearchManager::DoSearch()
```

matching the reported symptom exactly: the crash happens after the `"- searching ..."`
progress print and before any batch completes.

**Fix:** added `_pQueries = &queries;` at the top of
`SearchPeptideIndex(ThreadPool*, vector<Query*>&)`, mirroring `DoSearch()`.

**Status (2026-06-17):** Fixed and empirically validated both directions, not just
inspected. With the fix: a PI_DB search of the fixture completes and scores correctly
(`xcorr=3.4260`, `ascorepro=330.7289`, phospho correctly localized to position 7).
Then `git stash`-reverted just this one-line fix, rebuilt, and re-ran the same search:
it reproduced the identical segfault inside `BinarySearchMass`, confirming the fix is
both necessary and sufficient. Restored the fix and confirmed the full test suite
(21 tests, including the two new ones below) passes cleanly with zero build warnings.

Added two regression tests to `tests/unit/run_tests.py`:
- **t19** (already added for issue 2a) continues to cover the FI_DB AScore-ordering fix.
- **t20** (new) reuses T19's phospho fixture but builds a PI_DB (`-j`) index instead of
  an FI_DB (`-i`) index, then runs the same search and asserts it exits cleanly (rc=0)
  and produces the correct PSM. Verified to fail (non-zero exit from the crash) against
  the pre-fix code and pass against the fix, the same way 2a's test was validated.

---

## 3. Code Quality & Maintainability

### 3a. Pipeline relies on an undocumented "close() is always safe on an unopened writer" contract

**File:** `CometSearch/search/Pipeline.cpp:104-118`

When a writer's `open()` fails partway through the writer list, `close(false, false)`
is called on *every* writer, including ones whose `open()` was never reached. This only
works because every concrete `IResultWriter` happens to null-check its file handle
first in `close()`. The invariant is real and currently upheld by all five writers, but
it is not stated anywhere in `IResultWriter.h`; a future writer that forgets the
null-check will crash on a partial-open failure with no compiler or test signal.

**Fix:** add a one-line contract comment above `IResultWriter::close()` stating that
`close()` must be safe to call even if `open()` was never called or failed.

**Status (2026-06-17):** Fixed. Added a contract note to `IResultWriter::close()` in
`CometSearch/output/IResultWriter.h` stating that implementations must be safe to call
even when `open()` was never invoked or returned false, and explaining why
(`Pipeline::run()` calls `close(false, false)` on every writer in the vector, including
ones after the one whose `open()` failed). No behavior change; all five existing
writers already satisfy the contract.

### 3b. Stale "Phase 5" migration note in SearchSession.h

**File:** `CometSearch/search/SearchSession.h:23-28`

The header still says *"g_pvQueryMutex, g_bPlainPeptideIndexRead, and g_bSpecLibRead
remain as globals... Full removal is deferred to Phase 5."* Per `docs/20260612
_architecture_migration.md`'s own phase numbering, Phase 5 (Pipeline/Strategy) is
the work already present in this branch. The comment now reads as an open TODO with no
tracked follow-up. Either the deferral is permanent (the RTS path will never adopt
`SearchSession`) and the comment should say so plainly, or there is real follow-up work
that should be filed somewhere visible instead of living only in a header comment.

**Status (2026-06-17):** Fixed -- closed as part of fixing Actionable Improvement 4d,
which addressed this same `SearchSession.h:23-28` comment block ("state plainly" branch
chosen there). See 4d's status note for the detail; recorded separately here only
because this finding and 4d originally described the same fix as two different
write-ups (a critique and its corresponding improvement) rather than one item.

### 3c. `isIndexBased()` conflates two unrelated concerns

**File:** `CometSearch/search/ISearchStrategy.h:70-72`, used throughout
`CometSearch/search/Pipeline.cpp`

`Pipeline::run()` branches on `_strategy->isIndexBased()` both to decide whether to
print the FASTA-style "Search start:" banner / per-spectrum verbose logging *and* to
decide whether to print the index-style "searching... done" progress line. These are
really one decision ("which strategy is this") wearing the trappings of two
unrelated questions (whether reading the database needs an index, and which console
output style to use). Today the mapping happens to be 1:1, so it costs nothing, but a
fourth strategy with index-based storage and FASTA-style verbose logging (or vice
versa) would have no way to express that without a behavior change at every call site.
Not urgent, but worth a `progressStyle()`-type accessor if a fourth strategy is ever
added.

**Status (2026-06-17):** Addressed, narrower than originally framed. Re-checked every
call site of `isIndexBased()` (`grep` across `CometSearch/`): all nine are in
`Pipeline.cpp`, and every one of them is purely a console-output style switch (verbose
FASTA banners vs. the compact index-style progress line, including the "Reading all
spectra into memory" warning, which only ever changes what gets *printed*, not what
the strategy does). On closer inspection there isn't a second concern hiding in current
usage -- the original framing overstated the issue. Splitting the interface into two
accessors for a distinction the code doesn't actually have yet would be the kind of
premature abstraction this codebase's conventions warn against, so instead the
doc comment on `ISearchStrategy::isIndexBased()` was tightened to state explicitly that
`Pipeline::run()` is the only consumer, name the exact banners/lines it switches
between, and warn that the flag must not be used to gate actual search behavior. If a
fourth strategy ever needs index-based storage with FASTA-style verbose logging (or
vice versa), that is the trigger to revisit the split, not before.

### 3d. Redundant `operator=` declaration left over from the Params.h cleanup

**File:** `CometSearch/core/Params.h:98` (and similarly for the other structs touched
by the same cleanup)

`Options& operator=(const Options&) = default;` is now redundant: with no other
user-declared special member, the compiler already generates an identical copy
assignment implicitly. Harmless, but it's leftover noise from the "replace
hand-written operator= with = default" pass -- could simply be deleted now that the
hand-written bodies are gone.

**Status (2026-06-17):** Fixed. Removed all nine redundant `operator= = default`
declarations from `core/Params.h` (`Options`, `DBInfo`, `StaticMod`, `PrecalcMasses`,
`VarModParams`, `MassUtil`, `ToleranceParams`, `IonInfo`, `StaticParams`). None of these
structs declares a destructor, copy constructor, move constructor, or move assignment,
so a user-declared default constructor (only `StaticParams` has one) does not suppress
the implicit copy assignment operator either -- the compiler generates the identical
member-wise copy with the declarations removed. Verified with a clean rebuild (zero
warnings) and the full 19-test suite.

---

## 4. Actionable Improvements

### 4a. Add a regression test for AScore + FI_DB

No test in `tests/unit/` exercises `print_ascore_score` against a fragment index,
which is exactly why issue 2a was not caught by CI. A minimal test that builds a tiny
FI_DB index with a variable mod, runs a search with `print_ascore_score` set and a
deliberately different/blank `variable_mod01` in the search-time params, and asserts
the AScore differential-mod symbol/mass reflects the `.idx` file's value (not the
params file's) would catch this entire class of ordering bug permanently and guard
against it recurring during future refactors.

**Status (2026-06-17):** Fixed. Added `t19` to `tests/unit/run_tests.py`, with fixtures
`tests/unit/data/t19_ascore_fidb.fasta` (single 8-residue protein, one phospho-acceptor
S) and `t19_ascore_fidb.ms2` (synthetic singly-charged b/y ions for
`ACDEFGS[+79.966331]K`, precomputed from monoisotopic residue masses). The test builds
an FI_DB index with a real `variable_mod01` (phospho on S), then searches it with
`print_ascorepro_score=1` but a deliberately blank `variable_mod01` in the search-time
params -- the realistic case, since FI_DB search params don't need to redeclare mods
already baked into the index. It asserts the rank-1 PSM's `ascorepro` column is `> 0`.

Verified the test actually discriminates the bug, not just incidentally passes: with
the fix in place it reports `ascorepro = 330.7289`; temporarily reverting
`CometSearchManager.cpp`/`Pipeline.cpp` to the pre-fix ordering (`git stash`, rebuild)
reproduces the exact failure mode predicted in 2a's analysis -- `ascorepro` comes back
as the untouched default sentinel `0.0`, because with the bug `g_AScoreOptions`'s
symbol never gets set to the mod's index (`CometSearch.cpp:5584-5585`'s
`iVal == g_AScoreOptions.getSymbol() - '0'` check fails), so `cHasVariableMod` is never
set to `HasVariableModType_AScorePro` and `CometPostAnalysis::CalculateAScorePro()`
returns immediately without running. Restored the fix afterward and confirmed the full
20-test suite (19 prior + t19) passes cleanly with zero build warnings.

### 4b. Fix issue 2a at a single call site rather than inside FiStrategy

Patching `FiStrategy::initialize()` to re-call `SetAScoreOptions()` after
`ReadPlainPeptideIndex()` would work but duplicates a process-wide concern (AScore
setup) inside a per-strategy class. Fixing the ordering at one shared call site keeps
the AScore lifecycle in one place and automatically covers any future strategy that
loads an index with embedded mod definitions.

**Status (2026-06-17):** Done as part of fixing 2a. The shared call site chosen was
`Pipeline::run()` rather than `DoSearch()` itself, since `Pipeline::run()` is what
actually invokes `_strategy->initialize()`/`finalize()` and is already the single
caller of both -- placing the AScore lifecycle there means no strategy subclass needs
its own re-sync logic, present or future.

### 4c. Document the writer close()-after-failed-open contract

One sentence on `IResultWriter::close()` (issue 3a) removes the only undocumented
cross-class invariant `Pipeline::run()` currently depends on.

**Status (2026-06-17):** Done as part of fixing 3a -- see that item's status note.

### 4d. Resolve the stale Phase 5 comment

Either state plainly in `SearchSession.h` that the RTS-path globals are permanently
out of scope for `SearchSession`, or file the remaining migration work so it is
discoverable outside of a header comment (issue 3b).

**Status (2026-06-17):** Done -- "state plainly" branch chosen. Checked
`docs/20260612_architecture_migration.md`'s own phase plan: Phase 5 (Pipeline/Strategy)
is the last phase defined, and its own "RTS path" section already states the RTS entry
points are "explicitly out of scope for Phase 5" because they are
wrapper-compatibility-sensitive -- there is no Phase 6 deferring further removal.
Rewrote the comment block in `SearchSession.h` to say plainly that
`g_pvQueryMutex`/`g_bPlainPeptideIndexRead`/`g_bSpecLibRead` remaining as globals is
permanent, not a pending migration step: a single process can serve both RTS and batch
requests, so this once-per-process init state must stay process-global rather than move
into a per-batch-run `SearchSession`. Also re-flowed the trailing `g_cometStatus`
paragraph, which had been nested under the now-removed "Phase 4 migration note:"
sub-header, to match the rest of the comment's indentation.

---

## Appendix: Findings Not Requiring Code Changes

- **SearchMemoryPool / RAII slot guards**: `SearchMemoryPoolSlotGuard` is applied at
  all five `AcquirePoolSlot()` / `releaseSlot()` sites in `CometSearch.cpp`
  (`CometSearch::RunSearch(Query*)` FI and PI branches, the single-query FI fallback,
  the batch-FI per-query lambda, and `SearchThreadProc`). `SearchMemoryPool::allocate()`
  correctly unwinds partial allocations on `bad_alloc`. No exception-safety gaps found.
- **g_bIndexPrecursors alloc/free**: allocated with `malloc` in
  `CometSearchManager.cpp:1552`, freed with `free()` in `FiStrategy::finalize()` --
  consistent, no mismatched allocator.
- **Output writers**: `TxtWriter`, `SqtWriter`, `PercolatorWriter`, `PepXmlWriter`,
  `MzIdentMlWriter` all null-check their file handles in `close()`, including
  `MzIdentMlWriter`'s more involved temp-file merge/rename lifecycle (`FinalizeOne`).
  No double-close, no leaked temp files in the `bEmpty` or failed-merge paths observed.
- **RunSearchAndPostAnalysis / executeBatchLegacy**: only ever called with a
  non-empty `session.queries` (guarded by the empty-check in `executeBatchLegacy`
  before `RunSearchAndPostAnalysis` is invoked), so the unchecked
  `session.queries.at(0)` / `.at(size()-1)` mass-range calculation inside it is safe in
  every current call path.
- **Fused FI search path (`FusedLoadAndSearchSpectra` / `FusedSearchSpectrum`)**:
  pushes into `session.queries` under `session.queriesMutex`, consistent with the
  non-fused path's locking discipline; `Pipeline`'s post-batch stats and
  empty-batch handling work correctly for both paths.
- **Build / tests**: `make cclean && make -j$(nproc)` clean, zero warnings. 17 unit + 2
  integration tests (T17 peptide-count range, T18 byte-identical determinism) all pass.
