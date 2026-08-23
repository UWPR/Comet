# Code Review: `fablereview` branch (2026-08-23)

## 1. Summary

Reviewed the `fablereview` branch (HEAD `607d38de`) against `master`. The branch is a large
performance/correctness pass over the RTS and fragment-index search paths (P1-P11 fixes),
plus a batch of new regression tests (T25-T33) and CLAUDE.md doc updates. Nine independent
finders swept the diff by category (memory/ownership, correctness, efficiency, duplication,
test coverage); every candidate finding below was then adversarially re-verified against the
actual working tree by a separate agent before being recorded here. 22 findings confirmed,
1 downgraded to plausible/pre-existing, 6 refuted outright (kept below, with reasoning, for
transparency).

The most serious cluster is in `CometWrapper/` (C++/CLI layer): three independent,
newly-introduced double-free / use-after-free hazards around `InputFileInfo` ownership and
the new managed finalizer. The most serious native-side finding is a silent target/decoy
misclassification on a truncated `.idx` read -- it corrupts FDR calculations without any
visible error. Two test-suite integrity issues (a Windows-blind crash check in the very test
meant to catch that crash, and a T31 fixture silently dropped by `.gitignore`) were also found
by fact-checking the new T25-T33 suite itself.

**Fix pass status (2026-08-23):** all 11 critical/high/medium findings (2.1-2.11) fixed and
verified -- see each subsection below for the specific change. Of the 11 confirmed
duplication/efficiency findings in sections 3-4, 6 were fixed (3.3-3.7, 4.4) and 5 were
deliberately left as documented follow-ups (3.1, 3.2, 4.1-4.3) because a correct fix requires
either full-scale build/perf data this review pass doesn't have, or restructuring a
just-stabilized hot correctness path with regression coverage but no differential/fuzz harness
to validate an "equivalent" rewrite against -- see each item's **Skipped** note for the specific
reasoning. 2.12 (plausible/pre-existing) and section 5 (refuted) were left untouched as
instructed. Full unit suite (52/52), T17/T18 (integration), and T22 RTS repro (both FI_DB and
PI_DB, 1-thread/8-thread byte-identical) all pass against the rebuilt binary; T23/T24
(`--integration --bigdata`, full-scale ~350MB real HeLa data, both current-vs-baseline
`v2026.02.2` and internal-decoy-vs-target-decoy / plain-FASTA-vs-FI_DB-vs-PI_DB parity) also
pass, confirming the fix pass didn't change FDR-level search outcomes or regress timing. The
Windows `CometWrapper`/`RealtimeSearch` side was also rebuilt clean via MSBuild (Clean + Build,
zero errors) to verify the double-free/finalizer fixes compile on their actual target platform.

**Wall-clock cost of validating this fix pass** (for anyone re-running the same
before/considering-a-fix-done cycle CLAUDE.md's Testing section describes): the fast unit
suite (52 tests, no `--integration`) is ~3s; adding `--integration` for T17/T18/T22 (needs
`data/human.small.fasta` + the `tests/rts_repro/` fixtures) brings a targeted run of just
those four to ~4m; the full `--integration --bigdata` sweep including T23/T24 (real ~350MB
HeLa data, current-vs-baseline timing comparisons at both decoy modes and all three DB types)
took ~23m on this machine. Budget accordingly -- T23/T24 alone dominates total suite wall time
by more than an order of magnitude over everything else combined, so don't run it reflexively
on every small edit; the fast suite plus a targeted T17/T18/T22 pass is enough to validate
most changes, reserving the full `--bigdata` sweep for changes that touch the search-quality
or timing-sensitive code paths those two tests exist to catch.

## 2. Critical Issues

### 2.1 `InputFileInfoWrapper` dual ownership -- double-free (CRITICAL)
**`CometWrapper/CometDataWrapper.h:279-291`, `CometSearch/CometSearchManager.cpp` ~406**

`InputFileInfoWrapper` `new`s its own `InputFileInfo` and its destructor does
`delete _pInputFileInfo;`. `get_InputFileInfoPtr()` hands that same raw pointer through
`AddInputFiles` into `g_pvInputFiles`, which the new destructor loop added by this diff
(`for (InputFileInfo* p : g_pvInputFiles) delete p;`) *also* deletes. Any C# host that
`Dispose`s/`using`s an `InputFileInfoWrapper` after passing it to `AddInputFiles` double-frees.
The diff's own comment claiming "AddInputFiles()/SetParam() are the only places these
pointers are ever created ... doesn't risk a double-free" is factually wrong about this path.

**Fix applied:** `CometWrapper::AddInputFiles` (`CometWrapper.cpp`) now deep-copies each
`InputFileInfoWrapper`'s native object (`new InputFileInfo(*inputFile->get_InputFileInfoPtr())`)
before handing it to the manager, instead of passing the wrapper-owned pointer through. The
manager's `g_pvInputFiles` now owns pointers nothing else deletes; each `InputFileInfoWrapper`
still separately deletes its own copy. Updated the ownership comment in
`CometSearchManager.cpp`'s destructor accordingly. This also incidentally fixes 2.2 below (see
that entry).

### 2.2 `_pvInputFilesList` accumulates across calls with no `clear()` -- double-free (CRITICAL)
**`CometWrapper/CometWrapper.cpp:241-255`, `CometSearch/CometSearchManager.cpp:1740-1746`**

`AddInputFiles` `push_back`s into `_pvInputFilesList` with no `clear()` anywhere in the file,
then passes the whole accumulated vector to the manager, whose `AddInputFiles`
(`CometSearchManager.cpp:1740-1746`) also only `push_back`s into `g_pvInputFiles`, never
clearing it. A second `AddInputFiles` call therefore duplicates the first batch's pointers in
`g_pvInputFiles`, and the new destructor delete loop (2.1) double-deletes them. Pre-existing
duplication bug, newly weaponized by the new delete loop.

**Fix applied:** as part of the 2.1 fix, `AddInputFiles` now builds a fresh local
`vector<InputFileInfo*>` of newly-allocated deep copies on every call and passes only that to
`_pSearchMgr->AddInputFiles`, so a second call can no longer re-add the first batch's pointers
-- each call contributes only its own, uniquely-owned copies to `g_pvInputFiles`. The
`_pvInputFilesList` member that accumulated across calls was removed entirely, since nothing
needs the wrapper-owned pointers to persist past the call now that copies are made per-call.

### 2.3 New finalizer can destroy the shared native manager under live wrappers (CRITICAL)
**`CometWrapper/CometInterfaces.cpp`, `CometWrapper/CometWrapper.h`**

`CometInterfaces.cpp` has zero refcounting: `GetCometSearchManager()` returns/creates a single
global `g_pCometSearchManager`; `ReleaseCometSearchManager()` unconditionally
`delete`s it. The finalizer `!CometSearchManagerWrapper()` is new in this diff (previously only
the destructor was declared). GC-finalizing one undisposed wrapper now destroys the shared
native manager while any other live wrapper's cached `_pSearchMgr` dangles (only null-checked,
never re-fetched) -- and with no `GC::KeepAlive`, the finalizer can fire mid-native-call on the
same wrapper.

**Fix applied:** removed the finalizer entirely. `~CometSearchManagerWrapper()` (the
`Dispose()` path) is now the sole place that calls `ReleaseCometSearchManager()`. An undisposed
wrapper now leaks the singleton for the life of the process instead of risking destroying it
out from under another live wrapper or mid-call -- leaking is the safe failure mode given the
singleton has no refcounting. Documented the rationale in the header where the finalizer used
to be declared, so a future change doesn't reintroduce it without addressing the refcounting
gap in `CometInterfaces.cpp` first.

### 2.4 Silent target/decoy misclassification on truncated `.idx` read (CRITICAL)
**`CometSearch/CometPeptideIndex.cpp:328-340`, `CometSearch/CometSearch.cpp:2277-2293`**

A short `fread` of the protein-name bulk span leaves `g_pvProteinNameCache` completely empty,
logging only a warning ("protein names may be missing from output"). Downstream,
`bDecoyPep` is set `true` only on a cache hit (`g_pvProteinNameCache.find(lProtFilePos)`),
with **no fallback re-read on a miss** -- so every peptide is silently classified as target,
and the target/decoy split (and every FDR number derived from it) is destroyed with no error
surfaced to the user. Trigger requires an I/O failure or truncated `.idx`, but truncated
`.idx` files are a documented real occurrence in this repo (see the T24 note in CLAUDE.md).

**Fix applied:** on a failed bulk read, `ReadPeptideIndex` (`CometPeptideIndex.cpp`) now retries
per-protein with individual `fseek`/`fread` calls (the pre-P5 approach) instead of silently
proceeding with an empty cache. If a name still can't be read at its offset, the function now
fails loudly through `g_cometStatus.SetStatus(CometResult_Failed, ...)` / `logerr` and returns
`false`, aborting the index load instead of letting every peptide silently misclassify as
target.

### 2.5 Charge-0 clamp: silent "finds nothing" -> silent wrong-mass 1+ search (HIGH)
**`CometSearch/CometPreprocess.cpp:1663-1666`, `RealtimeSearch/SearchMS1MS2.cs:298`**

`CometPreprocess.cpp` now clamps charge `< 1` to `1`. `SearchMS1MS2.cs:298` initializes
`iPrecursorCharge = 0` and only overwrites it when a "Charge State:" trailer is present, so an
unknown-charge spectrum reaches the engine as charge 0. The RTS entry gate in
`DoSingleSpectrumSearchMultiResults` uses the *unclamped* charge (charge 0 gives a mass of
~1.007 Da, which passes the gate), and only the core clamps to 1 -- producing a wrong-mass 1+
search. Previously (no clamp), charge 0 produced `dMass ~= PROTON_MASS`, which matched
nothing -- a silent behavior change from "finds nothing" to "spurious 1+ match," and a gate/
core inconsistency either way.

**Fix applied:** `PreprocessSingleSpectrumCore` (`CometPreprocess.cpp`) now rejects an
out-of-range charge (`< 1` or `> MAX_PRECURSOR_CHARGE`) by returning `nullptr` instead of
clamping it into range and searching anyway. Verified `PreprocessSingleSpectrumCore` has exactly
one caller (`PreprocessSingleSpectrumThreadLocal`, RTS-only), which itself has exactly one
caller (`DoSingleSpectrumSearchMultiResults`), and that caller already null-checks the returned
`Query*` and returns `false` -- so charge-0/out-of-range spectra now cleanly report "no
results" again, matching pre-clamp behavior, without the wrong-mass 1+ search. The gate/core
inconsistency is moot now that the core rejects rather than silently substituting a different
mass.

### 2.6 Uninitialized `iNumPeaks` read in spectral-library validation (HIGH)
**`CometSearch/CometSpecLib.cpp:408-417,447,468`, `CometSearch/core/Types.h:545`**

This diff added five initializers to the fresh local `pTmp` (`fRTime`, `fScaleMinInten`,
`fScaleMaxInten`, `pfUnitVector`, `uiArraySizeMS1`) but **not** `iNumPeaks`.
`SpecLibStruct` (`core/Types.h:545`) has no default member initializer, so default-init
leaves `iNumPeaks` indeterminate. It's only assigned inside the "Num peaks:" branch (line
447), so an MSP entry lacking that line makes line 468's `|| pTmp.iNumPeaks == 0` validation
read an indeterminate value -- formally UB every iteration, and in practice a nonzero-garbage
read lets a peak-less library entry pass validation into `g_vSpecLib`. Pre-existing gap, but
this diff specifically hardened this exact initializer block and missed the one field the
validation actually reads.

**Fix applied:** added `pTmp.iNumPeaks = 0;` to the same initializer block, so an entry
lacking a "Num peaks:" line is deterministically rejected by the existing
`pTmp.iNumPeaks == 0` check instead of reading indeterminate memory.

### 2.7 Bare `assert()` as the only guard against stale static-params state (MEDIUM)
**`CometSearch/CometPostAnalysis.cpp` ~1341, `GenerateXcorrDecoys`**

The guard is a bare `assert(...)` in a `bool`-returning function whose file's established
error pattern elsewhere is `g_cometStatus.SetStatus(CometResult_Failed, ...); return false;`.
The stale state is reachable (per-instance `staticParamsInitializationComplete`
(`CometSearchManager.h:117`) vs. the process-lifetime `std::call_once` table). No current
build defines `NDEBUG` (Linux Makefiles don't; `CometSearch.vcxproj` Release|x64 doesn't
either -- only `CometWrapper.vcxproj` does, and that doesn't affect the `CometSearch` lib), so
today this `abort()`s an embedding RTS host on the stale-state path, and any future `NDEBUG`
build would silently disable the guard entirely.

**Fix applied:** replaced the `assert()` with an `if (!(...)) { ... return false; }` block using
this file's established `g_cometStatus.SetStatus(CometResult_Failed, ...); logerr(...); return
false;` pattern, so the guard now (a) fires identically in every build configuration regardless
of `NDEBUG`, and (b) returns a caller-visible error through the normal status channel instead of
aborting the process.

### 2.8 AScorePro slot guard rejects `variable_mod06`-`09`, which are not ambiguous (MEDIUM)
**`CometSearch/CometSearchManager.cpp`, guard `for (int i = 5; i < VMODS; ++i)`**

The two-digit-encoding ambiguity this guard is meant to catch only begins at slot 10: the
encoding handoff (`sequence += std::to_string(piVarModSites[i])`) is single-character and
unambiguous for values 1-9, and `SetAScoreOptions` registers AScorePro symbols for all fifteen
slots (`pepMod.setSymbol(i + 1 + '0')` for `i < VMODS`, `CometSearchManager.cpp:3491/3498`) --
so slots 6-9 round-trip exactly like 1-5. The current `i >= 5` bound hard-errors
previously-working batch-FASTA AScorePro configs using `variable_mod06`-`09` (the
`FRAGINDEX_VMODS=5` limit only applies to index searches). Should be `i >= 9`.

**Fix applied:** changed the loop bound from `i = 5` to `i = 9` (`variable_mod10` through
`variable_mod15`) and updated the error message to say "through variable_mod09" instead of
"through variable_mod05", matching where the two-digit encoding ambiguity actually begins.

### 2.9 T33's crash check is blind to Windows crash return codes (MEDIUM -- test integrity)
**`tests/unit/run_tests.py:3181`**

`check(result.returncode >= 0, ...)` -- the comment above it says signal deaths "show up as a
negative returncode on POSIX," which is true, but on Windows a crash surfaces as a large
*positive* returncode (e.g. `0xC0000409` = 3221226505 for a `/GS` stack-buffer-overrun --
precisely the 600-char-overflow failure T33 itself targets). The check passes trivially on
Windows, and the test explicitly supports Windows binaries (`_binary_uses_win_paths`,
line 3138). T33 currently cannot detect the exact crash class it was written to catch when run
against a Windows build.

**Fix applied:** changed the check to `result.returncode in (0, 1)` -- Comet's only two clean
exit codes (success, and `exit(1)` on a parameter error) -- so any other code (negative on
POSIX signal death, or a large positive NTSTATUS-derived code on Windows) now fails the test on
either platform.

### 2.10 T31 fixture silently dropped by a blanket `.gitignore` rule (MEDIUM -- test integrity)
**`.gitignore:157` (`*.msp`), `tests/unit/data/t31_speclib.msp`**

`tests/unit/data/t31_speclib.msp` is required by `test_t31_speclib_sizing` and exists in the
current working tree, but `git ls-files` shows it is **not tracked** -- it's caught by the
blanket `*.msp` ignore rule intended for large Carafe spectral-library exports. A fresh clone
of this branch will have T31 fail (or silently not exist to run against) because its fixture
was never committed. Needs a `.gitignore` exception (e.g.
`!tests/unit/data/*.msp`) or an explicit `git add -f`.

**Fix applied:** added `!tests/unit/data/*.msp` to `.gitignore` immediately after the
`*.msp` (Windows Installer Patch) rule, with a comment explaining the extension collision.
`git add -n tests/unit/data/t31_speclib.msp` now succeeds and `git status` lists the file as
untracked rather than ignored. Also created the fixture itself (a 3-entry synthetic `.msp`
library whose one relevant entry's tolerance window reaches `digest_mass_range`'s top bin,
per the test's docstring) -- it did not exist anywhere in the working tree before this
session, so the file was missing outright, not merely mis-ignored; T31 now passes.

### 2.11 New T25-T31 fixtures committed with LF, violating the repo's CRLF policy (LOW)
**`tests/unit/data/{t26_b1_fasta_decoy,t26_b2_fi_nl_order,t27_modcap,t29_decoyprefix,
t30_massboundary,t31_speclib}.{fasta,ms2}`**

All six new fixture pairs are committed at `HEAD` with Unix LF line endings; CLAUDE.md
mandates CRLF for every source file in this repo, fixtures included. (Working tree currently
shows these as CRLF already -- from an in-progress, uncommitted `unix2dos` fix -- so this is a
one-line "commit the conversion" away from resolved, not something requiring further edits.)

**Fix applied:** confirmed all twelve files (six `.fasta`/`.ms2` pairs, T25 fixtures were
already CRLF pre-branch) are now CRLF via `file`; also created and CRLF-converted the new
`t31_speclib.msp` fixture (2.10). No further action needed.

### 2.12 Plausible / pre-existing, narrowed by this diff -- not a regression
**Stale precursor-NL bin on out-of-range clear -- `CometSearch/CometSearch.cpp:3425-3438`**

The clear pass zeroes `_uiBinnedPrecursorNL[ctNL][ctCharge]` only inside
`if (iVal > 0 && iVal < g_staticParams.iArraySizeGlobal)`, so an out-of-range NL bin leaves a
stale slot. The guard itself is unchanged context -- this diff only widened the loop bound
from `usiChargeState` to `iPrecursorNLMaxCharge`, which strictly *reduces* staleness (higher
charge slots previously were never cleared at all). `XcorrScore` skips
`bin <= 0 || x > iMax || page == NULL`, so a stale slot at worst adds a small spurious xcorr
contribution, never an OOB access. Out-of-range itself is nearly unreachable for sane
`precursor_NL_ions` configs. Real latent hazard, but pre-existing and not introduced or
worsened by this diff in a way that needs fixing here.

## 3. Code Quality & Maintainability (confirmed duplication)

1. **Mirrored set/clear passes in `CometSearch.cpp`** -- the P3 clear pass in
   `SearchFragmentIndex` (1832-1906) mirrors its set pass (1908-1991) line-for-line except the
   per-bin action/dedup gate; `AnalyzePeptideIndex`'s decoy clear (2679-2739) mirrors its set
   pass (2741-2806) the same way. Every `pbDuplFragment[iVal] = true` in a set pass is
   immediately preceded by recording `iVal` into `uiBinnedIonMasses`/`uiBinnedPrecursorNL`, and
   the buffer starts all-`false` (`new bool[iArraySize]()`, `SearchMemoryPool.cpp:30`) -- a
   read-and-reset clear driven by those recorded indices would be equivalent and collapse both
   mirrored passes into one.

   **Skipped:** this is the single hottest, most correctness-sensitive loop nest in the FI/PI
   scoring path -- exactly the code T25-T28 were written to pin down after several real
   scoring bugs in this branch's own history (mis-ordered clear/set, mod-slot sentinels,
   NL-boundary breaks). Collapsing it into a read-and-reset would require proving
   `uiBinnedIonMasses`/`uiBinnedPrecursorNL` always hold a fully accurate "what did the set
   pass touch last time" record with no stale entries from an earlier candidate leaking
   through the `[x + 1 + iWhichNL]` sub-indexing -- a nontrivial data-flow argument, done under
   this review's time budget, in a hot path with real regression coverage but no perf/fuzz
   harness to validate an equivalence rewrite against. The duplication cost is real but the
   risk of a silent scoring regression from an under-verified "equivalent" rewrite is higher;
   left as a follow-up for someone with room to add a differential test first.
2. **Precursor-NL binning formula duplicated 9 more times** across `CometSearch.cpp`
   (`SearchFragmentIndex` 1900/1982, `AnalyzePeptideIndex` decoy 2734, `CompoundModSearch`
   9247/9261/9402/9416, among 18 total occurrences in the file) -- differs only by mass
   variable, charge bound, and destination array; a single parameterized helper fits every
   site.

   **Skipped, same reasoning as #1 above:** the 18 occurrences span five different functions
   with subtly different charge bounds (per-query `usiChargeState` vs. cached
   `iPrecursorNLMaxCharge` vs. `g_staticParams.options.iMaxPrecursorCharge`) and destination
   arrays (member vs. local, target vs. decoy) -- exactly the kind of "looks identical, isn't
   quite" code where a mechanical extraction risks silently picking the wrong bound at one of
   the nine sites. Left as-is rather than risk introducing the next P-numbered bug in the same
   pass that just finished fixing several.
3. **`iZeroBound` formula duplicated** -- `CometPreprocess.cpp:1264-1266` (new) verbatim-copies
   the pre-existing RTS formula at 1782-1784; the diff's own P9 comment admits it "mirrors
   iZeroBound's formula directly." The `50.0` cushion also appears at 1917 and 2951. A shared
   `ComputeZeroBound` helper covers all sites.

   **Fix applied:** added `static inline int ComputeZeroBound(int iArraySize, double
   dExpPepMass)` and a named `FASTXCORR_ZERO_BOUND_CUSHION` constant (both near the top of
   `CometPreprocess.cpp`); both `iZeroBound` computations now call the helper, and all three
   bare `50.0` cushion literals (the two `iZeroBound` sites plus `dMassCutoff` at ~1935 and the
   `LoadIons()` peak-acceptance check at ~2969) now reference the same named constant, so they
   can no longer drift apart independently.
4. **Array-size clamp duplicated 3x** -- `CometSearch.cpp` 1734-1735, 2689-2690, 3550-3551,
   each immediately following an identical `iArraySize = (int)((dExpPepMass + dCushion) *
   dInverseBinWidth)` compute. One compute-and-clamp helper serves all three.

   **Fix applied:** added `static inline int ComputeClampedArraySize(double dExpPepMass)`
   (calls the existing `GetMassCushion()`, then clamps to `iArraySizeGlobal`) next to
   `ComputeZeroBound` in `CometPreprocess.cpp`; all three call sites (RTS
   `PreprocessSingleSpectrumCore`, batch `PreprocessSpectrum`, and `FusedSearchSpectrum`) now
   call it instead of duplicating the compute-then-clamp pair.
5. **Redundant `bCountOnly` flag alongside a pointer that already encodes it** --
   `CometFragmentIndex.cpp`: all 10 call sites pass `bCountOnly == (pLocalFragPeptides !=
   nullptr)` consistently (false+nullptr at 321/358; true+`&localFragPeptides` at 404, 419,
   430, 445, 483, 493, 503, 518). The three in-function uses of `bCountOnly` (686, 790, 808)
   could simply null-test the pointer, dropping the parameter entirely.

   **Fix applied:** removed the `bCountOnly` parameter from `AddFragments()`'s signature (both
   `.h` declaration and `.cpp` definition) and derived it as a local
   `const bool bCountOnly = (pLocalFragPeptides != nullptr);` at the top of the function body,
   keeping every downstream use of the name unchanged. Updated all 10 call sites to drop the
   now-removed boolean argument.
6. **`g_bIndexPrecursors` allocation duplicated** -- `CometSearchManager.cpp` 458-473 (new
   block) and 1658-1675 (pre-existing, modified for `+1` sizing) are functionally identical
   (`malloc((BIN(dPeptideMassHigh)+1)*sizeof(bool))`, NULL check, same fill condition). Both
   touched by this diff; a shared helper is trivial to extract.

   **Fix applied:** extracted a file-local `static bool AllocateIndexPrecursors()` helper in
   `CometSearchManager.cpp` (right before `InitializeStaticParams()`); both call sites now call
   it and return `false` on failure instead of duplicating the malloc/NULL-check/fill loop.
7. **Now-dead throw** -- `CometFragmentIndex.cpp:241-245` adds `if (iTotal >= UINT_MAX)
   exit(1)` (a stricter, relocated version of a check formerly done per-push in
   `AddFragments`). Between that check and the pre-existing throw at 288-292,
   `g_vFragmentPeptides` is only ever move-inserted with exactly `iTotal` elements (247-253)
   then sorted (278), so `size() == iTotal` always holds at 288 -- the `>=` check is strictly
   stronger and makes the 288 throw unreachable dead code.

   **Fix applied:** removed the dead `if (g_vFragmentPeptides.size() > ...) throw
   std::overflow_error(...)` block, replacing it with a comment explaining why the earlier
   `iTotal >= UINT_MAX` check already covers this invariant.

## 4. Actionable Improvements (confirmed efficiency findings)

1. **Redundant ladder walk in the fragment-index count pass** --
   `CometSearch/CometFragmentIndex.cpp`: the count pass pushes each variant (line 702) *before*
   walking the fragment ladder and doing per-ion atomic `fetch_add`s (791/809), but nothing
   reads the offsets or index array between there and the CSR prefix-sum (261-271) or the
   fill-count sub-pass (312-326), which recomputes identical per-bin totals anyway (the mass
   sort is a permutation, so per-bin totals don't change). The count pass could return right
   after the push, eliminating a full O(variants x length) ladder walk plus all its atomic RMW
   traffic; the existing prefix-sum loop can derive CSR base offsets with allocation deferred
   to after fill-count.

   **Skipped:** this is the dominant cost center of a whole-proteome FI build (the P1 fix this
   very branch added), and removing the ladder walk from the count pass would mean the CSR
   flat-array allocation (`new unsigned int[uiTotal]`, line 270) has to move to *after* the
   fill-count sub-pass instead of right after the count pass -- a real restructuring of the
   three-pass pipeline's data dependencies, not a local edit. Verifying it doesn't change
   build output or the T18 byte-identical-build guarantee needs a full-scale FI build
   (tens of minutes, `--bigdata`) to measure, which is outside what this review pass can
   validate. Left as a follow-up with a clear enough description (above) for someone to pick
   up with that data available.
2. **Unnecessary sort in the P5 protein-name cache build** --
   `CometSearch/CometPeptideIndex.cpp:310-334` pushes every protein occurrence of every
   peptide, then sort+unique. Sorted order is only used for `front()`/`back()` min/max and to
   iterate emplacing into `g_pvProteinNameCache`; the name slice is random-access and
   order-independent, and map `emplace` is a no-op on duplicate keys. A streaming min/max pass
   plus a second emplace pass drops the O(occ log occ) sort and its transient
   8-bytes-per-occurrence vector. Efficiency-only; correctness unaffected either way.

   **Skipped:** this exact block was just rewritten for 2.4's correctness fix (the
   bulk-read-failure fallback). Re-touching the same function's happy path for a
   performance-only change in the same pass as its correctness fix increases the chance of a
   regression in the fallback path with no error-return test to catch it (T25-T33 don't
   exercise a truncated `.idx`). Left as a follow-up once 2.4's fix has had a chance to be
   exercised on its own.
3. **Coarse-grained thread partitioning causes systematic (not just random) skew** --
   `CometFragmentIndex.cpp:217-231` splits `g_vRawPeptides` into `iNumThreads` equal index
   ranges, one monolithic job per thread. `g_vRawPeptides` comes from the mass-sorted peptide
   index, so later partitions systematically hold heavier peptides with more mod-variant work,
   and the pass finishes at the slowest partition's pace. Finer-grained chunking would preserve
   correctness (bin counts are commutative atomics; the concatenation at 233-255 only needs
   ascending raw-peptide-index order, which ordered chunk concatenation reproduces exactly --
   T18 determinism holds) while balancing load better.

   **Skipped:** a real fix here is a load-balancing tuning exercise (chunk size vs. thread
   count vs. scheduling overhead) that needs before/after wall-clock measurements on a
   full-scale build to justify -- exactly the kind of change the P1-P11 fixes in this branch
   were validated with (see `docs/20260822_carafe_prerun.md`-style measurement rigor), which
   this review pass doesn't have the data or time budget to reproduce. Left as a follow-up.
4. **Missing `setvbuf` on the SQT writer's `fopen` sites** (low severity, possibly
   deliberate) -- present in `TxtWriter.h:43/55`, `PepXmlWriter.h:37/49`,
   `PercolatorWriter.h:39`, `MzIdentMlWriter.h:116`, but absent at `SqtWriter.h:32,42`. The
   diff's own P11 doc notes SQT already builds each result line via `ostringstream` + one
   `fprintf` ("as SqtWriter does") rather than 25-40 `fprintf`s per line like the other
   writers, so the win from adding a 1MB buffer here is real but much smaller than it was for
   the other formats.

   **Fix applied:** added `setvbuf(_fpout/_fpoutd, NULL, _IOFBF, 1 << 20)` immediately after
   each `fopen()` in `SqtWriter.h`, matching the pattern (and buffer size) used by the other
   four writers, with a comment noting the smaller expected win given SQT's already-batched
   per-line output.

## 5. Refuted candidates (kept for transparency -- no action needed)

- **`LoadIons` per-charge-guess re-sort** -- pre-existing behavior, and this diff *reduced*
  its cost (old code sorted a full spectrum copy taken by value on every charge guess; new
  code takes a reference and copies at most once, only when needed). Not a regression.
- **Static-params map mutation "previously accidentally safe"** -- the premise is wrong: the
  old code (`erase` + `insert`) was already UB under concurrent `find()`; the new
  `ret.first->second = pParam` assignment *removes* the rebalancing risk and only frees the
  payload. No lock has ever guarded this map, and the supported RTS usage pattern does all
  `SetParam` calls single-threaded before init/search.
- **Param name changes (`add_U_selenocysteine`, `spectral_library_ms_level`, etc.)** -- not
  introduced by this branch; `master` already had the reader/writer using these exact names
  while the *old* manager-side code read different, dead spellings. This diff fixes previously
  silently-broken config keys (documented as B10 in `docs/20260819_fablereview.md`); no
  previously-working config is broken by it.
- **`ParsePeptideIndexHeader` restoring high mod slots past the AScorePro guard** -- it only
  resets/repopulates slots 0..`FRAGINDEX_VMODS-1` (0-4); slots 5-14 are untouched by the
  header restore and retain exactly what `InitializeStaticParams` set, so this path cannot
  activate a slot the guard didn't already see.
- **`strlen(szParamVal) - 2` underflow in `Comet.cpp`** -- the 8-field-count validation runs
  before the chomp, so `szParamVal` is provably >= 15 chars whenever the subtraction executes;
  the underflow path is unreachable.
- **`nChooseK` narrowing to `int`** -- overflow is provably unreachable today: `n` is clamped
  to 50 and `k` to `FRAGINDEX_MAX_MODS_PER_MOD = 5`, giving `C(50,5) = 2,118,760 << INT_MAX`.
  A real landmine if that `#define` is ever raised (e.g. to 10, `C(50,10) ~= 1.03e10`
  silently truncates), but not a live bug against current constants.

## 6. Notes on scope

Two verifier-cluster items reported earlier in this review's process were self-checks of the
new T25-T33 suite rather than diff findings, and are folded into 2.9-2.11 above rather than
listed separately. Findings are ranked by severity within each section, not by discovery
order.

Fixes for sections 2-4 were applied directly to the working tree as each item was verified --
see the **Fix applied** / **Skipped** notes under each finding above for what changed and why.
No commits were made; the working tree is left for the user to review and commit at their
discretion, per this repo's git workflow rules.
