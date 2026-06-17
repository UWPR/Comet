Code Review: architecture_update branch, uncommitted working-tree diff (2026-06-17)
=====================================================================================

Scope
-----
Reviewed the current uncommitted changes on top of commit c971a2dd (13 modified
files + 1 new file, +58/-573 lines). This diff implements the fix pass for the
findings recorded earlier today in docs/20260617_codereview.md: replacing
hand-written `operator=` bodies in Params.h with `= default`, adding a SlotGuard
RAII wrapper in SearchThreadProc, fixing the FusedLoadAndSearchSpectra batch-size
check, extracting SearchUtils.h's non-trivial functions into a new SearchUtils.cpp,
and factoring the three strategies' batch bodies into a shared executeBatchLegacy
helper.

Method: verified each "Status: Fixed" claim against the actual diff line-by-line,
rebuilt from clean (`make cclean && make`), ran the full unit suite, checked CRLF /
non-ASCII / trailing-whitespace compliance per CLAUDE.md, then searched for
structurally identical instances of the bug pattern that was just fixed.

---

1. Summary
----------
All six fixes claimed in docs/20260617_codereview.md are present in the diff and
verified correct: the `operator=` replacements are safe (every member of every
affected struct is a value type with correct default-copy semantics, no owning raw
pointers), the SlotGuard correctly releases the pool slot on exceptional exit, the
batch-size counter now reflects processed rather than queued spectra, and the
SearchUtils split / executeBatchLegacy extraction preserve behavior exactly
(FastaStrategy keeps `bVerbose=true`, FiStrategy/PiStrategy keep `bVerbose=false`,
matching their pre-diff behavior). Clean rebuild produces zero warnings; all 17 unit
tests pass. One gap was found: the SlotGuard fix addressed only one of five call
sites that share the identical acquire-slot/run/release-slot pattern, leaving the
production batch-FI hot path and the RTS single-spectrum path exposed to the same
240-second slot-leak hazard the fix was written to close.

**Status (2026-06-17): all items closed.** The critical issue (2a) and both
actionable improvements (4a, 4b) have been fixed -- see per-item status notes below.
Rebuilt clean (`make cclean && make`, zero warnings) and re-ran the full unit suite
(17 passed, 0 failed, 0 skipped) after the fix.

---

2. Critical Issues
-------------------

### 2a. SlotGuard fix is incomplete -- four sibling call sites still leak the pool
    slot on exception

**Files:** `CometSearch/CometSearch.cpp:128, 170, 214, 266`

The diff adds a `SlotGuard` RAII wrapper around the one call site in
`SearchThreadProc` (line ~1263) so `s_pool.releaseSlot()` fires even if `DoSearch`
throws. The same bare `AcquirePoolSlot() -> run -> s_pool.releaseSlot()` pattern,
with no guard, exists at four other sites that were not touched:

- `CometSearch::RunSearch(Query*)` line 128 (RTS thread-local FI search -- the
  documented concurrent RTS path in CLAUDE.md)
- same function, line 170 (RTS thread-local PI search)
- `CometSearch::RunSearch(ThreadPool*, vector<Query*>&)` line 214 (single-query FI
  fallback)
- `CometSearch::RunSearch(int, int, ThreadPool*, vector<Query*>&)` line 266 -- inside
  a per-query lambda dispatched to the thread pool; this is the production batch-FI
  search hot path, executed once per query in every FI_DB batch

`SearchFragmentIndex` (called at all four sites) builds a
`std::unordered_map<unsigned int,int>`, a `std::vector<std::pair<...>>` via
`push_back`, and calls `std::sort` -- all of which can throw `std::bad_alloc` under
memory pressure, the same failure mode that motivated the original fix.
`SearchPeptideIndex` (lines 170, 244) has equivalent allocations.

If any of these throw, the slot is never released. `SearchMemoryPool::acquireSlot()`
(threading/SearchMemoryPool.cpp:76) then blocks every subsequent caller for up to
240 seconds (the same symptom described for the issue that was just fixed) before
giving up and returning -1. For the RTS single-spectrum path this directly
contradicts the threading-model guarantee in CLAUDE.md that the RTS path stays
responsive; for the batch FI path it can stall an entire search batch.

**Fix:** lift `SlotGuard` out of `SearchThreadProc` into a shared location (e.g.
`SearchMemoryPool.h`, since `s_pool` already lives in that translation unit) and
apply it at all five acquire/release sites, or wrap the post-acquire body of each
site in a `try { ... } catch (...) { s_pool.releaseSlot(slot); throw; }`. Since this
is the same author and same diff that recognized and fixed the pattern once, doing
it everywhere now is cheap; finding the next instance after a production stall is
not.

**Status (2026-06-17):** Fixed. Added `SearchMemoryPoolSlotGuard` to
`threading/SearchMemoryPool.h` (a small RAII struct holding a `SearchMemoryPool&`
and the slot index, releasing in its destructor) and applied it at all five
acquire/release sites in `CometSearch.cpp`: the two thread-local RTS overloads
(`RunSearch(Query*)`, FI and PI branches), the single-query FI fallback
(`RunSearch(ThreadPool*, vector<Query*>&)`), the batch-FI per-query lambda
(`RunSearch(int, int, ThreadPool*, vector<Query*>&)`), and the original
`SearchThreadProc` site (whose function-local `SlotGuard` struct was removed in
favor of the shared one). All five bare `s_pool.releaseSlot(...)` calls following a
search body were removed; the guard now owns release in every case, including
exception unwind.

---

3. Code Quality & Maintainability
----------------------------------

Nothing new beyond what docs/20260617_codereview.md already recorded and the diff
already fixed. No trailing whitespace, no non-ASCII characters, and CRLF line
endings are correct in every changed/added line (verified with `file` and
`grep -P "[^\x00-\x7F]"` / `grep -P "[\t ][\r]?$"` restricted to lines actually
touched by this diff -- the unrelated pre-existing trailing-whitespace lines found
elsewhere in CometSearch.cpp/CometPreprocess.cpp/CometSearch.h/CometSearchManager.cpp
are untouched by this diff and out of scope).

---

4. Actionable Improvements
----------------------------

### 4a. Share one SlotGuard definition instead of risking drift

`SlotGuard` is currently a function-local struct defined only inside
`SearchThreadProc`. Move it next to `SearchMemoryPool` (e.g. as a nested type or a
free struct in `threading/SearchMemoryPool.h`) so the four other call sites in 2a
can reuse it directly:

```cpp
// threading/SearchMemoryPool.h
struct SearchMemoryPoolSlotGuard
{
   SearchMemoryPool& pool;
   int slot;
   ~SearchMemoryPoolSlotGuard() { if (slot >= 0) pool.releaseSlot(slot); }
};
```

```cpp
int iSlot = AcquirePoolSlot();
if (iSlot < 0) { logerr(...); return false; }
SearchMemoryPoolSlotGuard guard{s_pool, iSlot};
SearchFragmentIndex(pQuery, _ppbDuplFragmentArr[iSlot]);
```

**Status (2026-06-17):** Done as part of fixing 2a -- `SearchMemoryPoolSlotGuard` was
added to `threading/SearchMemoryPool.h` exactly as proposed and is now the only
release mechanism used anywhere in `CometSearch.cpp`.

### 4b. Batch-FI lambda swallows AcquirePoolSlot failure

**File:** `CometSearch/CometSearch.cpp:258-266` (pre-existing, not introduced by
this diff, surfaced while tracing 2a)

When `AcquirePoolSlot()` returns -1 inside the per-query lambda, the lambda logs and
returns, but `RunSearch`'s `bSucceeded` is never set to `false` -- the query is
silently dropped from the batch with no caller-visible failure. Not in scope for
this diff's fix pass, but worth a follow-up ticket since it compounds 2a (a slot
leaked by one query makes the next query's acquire more likely to time out and be
silently dropped too).

**Status (2026-06-17):** Fixed. Added a `std::atomic<bool> bAllSlotsAcquired(true)`
captured by reference in the per-query lambda; on `AcquirePoolSlot() < 0` the lambda
now sets it `false` (in addition to the existing `logerr`) instead of just
returning. After `wait_on_threads()`, `RunSearch` checks the flag and, if any query
failed to acquire a slot, calls `g_cometStatus.SetStatus(CometResult_Failed, ...)`
and sets `bSucceeded = false` before returning, making the failure visible to the
caller instead of silently dropping the affected queries from the batch.

---

Appendix: Verified, no changes needed
----------------------------------------
- `Options`/`DBInfo`/`StaticMod`/`PrecalcMasses`/`VarModParams`/`MassUtil`/
  `ToleranceParams`/`IonInfo`/`StaticParams` `operator= = default`: every member of
  every struct is a value type (POD scalar, fixed array of scalars, `std::string`,
  `std::vector`, `std::multimap`, `std::chrono::time_point`) -- no owning raw
  pointers anywhere in `Params.h`, so compiler-generated copy is correct and copies
  every field, closing the original drift bug for good rather than just patching the
  fields named in the original finding.
- `executeBatchLegacy` / `SearchUtils.cpp` extraction: behavior-preserving: verbose
  flag wiring matches each strategy's pre-diff console output exactly; locking around
  `CheckExit`'s new `session.queries.size()` argument is consistent with the existing
  `queriesMutex` discipline at the push site (CometPreprocess.cpp:3236).
- `Pipeline::cleanupBatch` now also drains `session.ms1Queries` -- consistent with
  the dead/not-yet-wired batch MS1 path noted in the prior review; no active leak
  today, but correct hygiene if that path is wired in later.
- Build (pre-fix and post-fix): `make cclean && make -j20` from a clean tree --
  zero warnings both times.
- Tests (pre-fix and post-fix): `python3 tests/unit/run_tests.py --comet
  ./comet.exe` -- 17 passed, 0 failed, 0 skipped both times.
