# Code Review -- architecture_update branch
# 2026-06-16

Reviewed by: Claude Sonnet 4.6 (high-effort, 7-angle finder + per-candidate verification)
Scope: `git diff master...HEAD` -- 70 files, +5542 / -3127 lines

---

## Summary

This branch introduces a major architectural refactor: Strategy pattern for search
(`FastaStrategy`, `FiStrategy`, `PiStrategy`), a `Pipeline` orchestrator, a `SearchSession`
object, an `IResultWriter` interface with four concrete writer classes, a `SearchMemoryPool`,
and a `core/` split of `CometDataInternal.h` into `Constants.h`, `Params.h`, and `Types.h`.
The structural changes are sound, but the refactor introduced four confirmed bugs (two of
which corrupt search results or violate real-time latency guarantees) and two plausible bugs
on error paths.

---

## Critical Issues

### 1. `StorePeptideI` ignores `bDecoyPep` -- decoys written to target list (FDR corruption)
**File:** `CometSearch/CometSearch.cpp` ~line 8618
**Severity:** Critical -- wrong results, silent

The new `Query*`-based overload of `StorePeptideI` (added for the FI/PI index path in
Task 1.2) comments out the `bDecoyPep` parameter:

```cpp
void CometSearch::StorePeptideI(Query* pQuery, ..., bool /*bDecoyPep*/, ...) {
```

The parameter is dead. The function body always writes to `pQuery->_pResults` and
increments `iMatchPeptideCount`, regardless of whether the peptide is a decoy.
`pQuery->_pDecoys` and `iDecoyMatchPeptideCount` are never touched by this overload.

The callers at lines ~8591 and ~8608 correctly pass `bDecoyPep=true` for decoy hits.
The old `StorePeptide` overload (line ~5221) has the correct branch:

```cpp
if (g_staticParams.options.iDecoySearch == 2 && bDecoyPep)
   // write to _pDecoys
```

The new overload is missing this branch entirely.

**Impact:** Any FI_DB or PI_DB search with `iDecoySearch=2` (separate decoy mode) silently
mixes all decoy PSMs into the target result list. FDR estimation is corrupted for every
index-path search in separate-decoy mode.

**Fix:** Restore the `iDecoySearch==2 && bDecoyPep` branch in `StorePeptideI`, writing
to `pQuery->_pDecoys[siLowestDecoyXcorrScoreIndex]` and comparing against
`pQuery->dLowestDecoyXcorrScore` when the decoy condition holds.

---

### 2. `SearchMS1Library` uses global `g_pvQueryMutex` instead of per-query lock
**File:** `CometSearch/CometSearch.cpp` ~line 3275
**Severity:** High -- RTS latency violation; cross-path serialization

`SearchMS1Library` (the MS1 real-time search path) guards score updates on a caller-owned
`QueryMS1*` with the process-wide `g_pvQueryMutex`:

```cpp
ThreadMutexLock(&g_pvQueryMutex);   // line ~3275
// update pMS1Query->dBestXcorr, etc.
ThreadMutexUnlock(&g_pvQueryMutex);
```

The MS2 RTS path (`RunSearch`) correctly uses `pQuery->accessMutex` for per-query
isolation (lines ~5135, ~8554). `SearchMS1Library` should do the same.

`g_pvQueryMutex` is also held during batch speclib loading
(`CometPreprocess.cpp:1007`). A running batch search therefore blocks every concurrent
RTS MS1 thread for the full duration of the speclib load, violating the real-time latency
guarantee.

**Fix:** Add an `accessMutex` field to `QueryMS1` (mirroring `Query::accessMutex`) and
use it in `SearchMS1Library` for score-update critical sections.

---

### 3. `MzIdentMlWriter::FinalizeOne` silently produces invalid `.mzid` on temp-file reopen failure
**File:** `CometSearch/output/MzIdentMlWriter.h` ~line 116
**Severity:** High -- silent data corruption, no error reported

`FinalizeOne()` closes the temp file then immediately reopens it for reading:

```cpp
fclose(fpTmp);                          // line 115
fpTmp = fopen(sTmp.c_str(), "r");      // line 116
if (fpTmp) {                            // line 117
   CometWriteMzIdentML::WriteMzIdentML(...);
   fclose(fpTmp);
}
fclose(fpFinal);                        // line 129
```

If the `fopen` at line 116 fails (network filesystem, external cleanup, non-atomic
close-reopen), the `if` block is skipped: `WriteMzIdentML` is never called, the
spectrum results are never appended, and the output file is closed at line 129 containing
only the XML header -- no spectrum results, no closing tags. `g_cometStatus` is never
updated; `DoSearch` returns `true`. Downstream tools receive a structurally invalid file.

**Fix:** Check the return value of the second `fopen` and call
`g_cometStatus.SetStatus(CometResult_Failed, ...)` on failure before returning.

---

## Code Quality & Maintainability

### 4. `Pipeline::run` -- writer `open()` failure leaves already-opened writers unclosed
**File:** `CometSearch/search/Pipeline.cpp` ~line 108
**Severity:** Medium -- FILE* handle leak, truncated output files

When a writer's `open()` fails, the inner loop breaks and Pipeline calls
`_strategy->closeFiles()` then breaks out of the file loop entirely, bypassing the
`pw->close()` block at lines ~243-247. Writers that already opened successfully (with
partially-written headers on disk) are never closed.

**Fix:** On `open()` failure, iterate all writers that have already been successfully
opened and call their `close()` before returning `false`.

---

### 5. `Pipeline::run` returns on `initialize()` failure without calling `finalize()` (memory leak)
**File:** `CometSearch/search/Pipeline.cpp` ~line 38
**Severity:** Medium -- memory leak on error path

```cpp
if (!_strategy->initialize(session)) return false;   // line 38
// ...
_strategy->finalize(session);                        // line ~256, only reached on success
```

`finalize()` is the sole cleanup point for memory allocated by `initialize()` (thread-pool
scratch buffers, precursor arrays). If `initialize()` returns `false` midway -- e.g.,
`CometPreprocess::AllocateMemory` succeeds then `ReadPrecursors` fails -- those allocations
are never freed. On repeated calls (C# wrapper retrying after a failed search), each
failed init accumulates leaked memory.

**Fix:** Call `_strategy->finalize(session)` before returning `false` at line 38, or
structure the function with a `goto cleanup` / RAII guard so `finalize` always runs.

---

### 6. `WithinMassTolerancePeff` seek-back loop uses wrong reference mass
**File:** `CometSearch/CometSearch.cpp` ~line 4380
**Severity:** Medium -- false negatives in PEFF searches

After `BinarySearchMass` locates the correct position for `dCalcPepMass + dMassAddition`,
the seek-back while-loop compares against bare `dCalcPepMass` instead of
`dCalcPepMass + dMassAddition`. With a large positive PEFF modification (e.g., +80 Da
for phospho), the found position is 80 Da ahead of `dCalcPepMass` in the sorted index;
the seek-back stops far too early, and candidate peptides that are within tolerance of
the modified mass are never evaluated.

**Fix:** Change the seek-back comparison operand from `dCalcPepMass` to
`dCalcPepMass + dMassAddition`, mirroring the value passed to `BinarySearchMass`.

---

### 7. `SearchSession::bPlainPeptideIndexRead` / `bSpecLibRead` are dead fields
**File:** `CometSearch/search/SearchSession.h` ~line 44
**Severity:** Low -- architectural drift; misleads about ownership

`SearchSession` declares `bPlainPeptideIndexRead` and `bSpecLibRead` as session-owned
state, but `FiStrategy::initialize` reads the global `g_bPlainPeptideIndexRead` -- not
`session.bPlainPeptideIndexRead`. The session fields are never set or checked by any
code path. A reader auditing `SearchSession` to understand index state will draw the
wrong conclusion about where the authoritative value lives.

**Fix:** Either wire `FiStrategy::initialize` to read and write `session.bPlainPeptideIndexRead`
and retire the global, or remove the dead session fields until the migration is ready.

---

## Actionable Improvements

### 8. `FiStrategy::executeBatch` is a near-copy of `FastaStrategy::executeBatch` with a dead Mango block
**File:** `CometSearch/search/FiStrategy.cpp` ~line 59

The non-fused `executeBatch` body is an almost-exact copy of `FastaStrategy::executeBatch`,
including a Mango sort block that can never execute in this branch (`bFused` is false only
when `bMango || bSpecLib` is true, meaning the fused path is taken instead). Any future
change to the shared preprocessing sequence must be applied in both files.

Extract the shared preprocessing sequence into a free function in `SearchUtils.h` and call
it from both strategies.

---

### 9. `BuildNames()` copy-pasted verbatim into all four writer classes
**Files:** `CometSearch/output/SqtWriter.h`, `TxtWriter.h`, `PepXmlWriter.h`, `MzIdentMlWriter.h` ~line 43 each

Each concrete writer class contains an identical private static `BuildNames()` method;
only the default file extension string differs at the call site. Any fix to filename
construction logic (CRUX mode suffix, range-number format, path separator) must be applied
in four places and will inevitably diverge.

```cpp
// Replace four copies with one free function in IResultWriter.h:
static void BuildNames(const std::string& defaultExt,
                       std::string& sBaseName,
                       std::vector<std::string>& vFileNames);
```

---

### 10. `PrintPercolatorSearchHit` takes `vector<string>` by value -- per-PSM copy overhead
**File:** `CometSearch/CometWritePercolator.h` ~line 43

`PrintPercolatorSearchHit` accepts `vProteinTargets` and `vProteinDecoys` by value,
copying up to `iMaxDuplicateProteins` (default 20) `std::string` objects per PSM. The
vectors are assembled by the caller immediately before the call and used read-only inside
the function. Change to `const std::vector<std::string>&` to eliminate the per-PSM
allocation/copy/destruction with no other change required.
