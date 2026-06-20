# Code Review: RawFileReader branch (2026-06-19)

## Scope

Review of the `RawFileReader` branch versus `master` (11 commits, 39 files changed,
+1,505/-726 lines): the `/clr` rewrite of MSToolkit's `.raw` reading from MSFileReader COM to
Thermo's RawFileReader .NET (`MSToolkit/src/MSToolkit/RAWReader.cpp`,
`MSToolkit/include/RAWReader.h`), the structured-API activation-method detection follow-up
(`IReaction.ActivationType`/`.MultipleActivation`, adding ECD/PQD/IRMPD support), the build-system
changes needed to compile one file in a project with `/clr` (`MSToolkit/VisualStudio/MSToolkit.vcxproj`),
removal of the now-dead COM wrapper (`CometWrapper/MSFileReaderWrapper.{h,cpp}`), a dead-code guard
fix in `mzMLReader.cpp` surfaced by the build-system change, the `RealtimeSearch.csproj`
`PackageReference` migration and version-pin fix, and documentation
(`docs/20260618_RawFileReaderMigration.md`, `docs/20260619_sidecar.md`, `README.md`).

Method: full `Comet.sln` Release|x64 rebuild (clean, zero errors); full unit test run
(`tests/unit/run_tests.py`, 19/19 passed); the existing `tests/regression/test_raw_vs_mzxml.py`
regression test re-run against the Windows binary with a real production `.raw` file (100%
top-peptide agreement on common scans, all 5 output formats near-exact vs. the `.mzXML` baseline);
line-by-line read of `RAWReader.cpp`/`.h` against the Bug Category Checklist, with each finding
cross-checked against `master`'s version of the same file (`git show master:...`) to separate
issues introduced by this branch from pre-existing ones carried over verbatim.

---

## 1. Summary

The rewrite is sound and field-for-field faithful to the prior COM-based behavior on real data
(verified, not just asserted). One critical gap: only one of the many RawFileReader API calls in
the rewritten `readRawFile()` is exception-guarded, leaving the function exposed to an uncaught
managed exception from a malformed/edge-case `.raw` file taking down the entire Comet process
mid-batch-search. A handful of smaller maintainability and performance items are worth addressing
but aren't blocking.

---

## 2. Critical Issues

### 2a. Only one of many RawFileReader API calls in `readRawFile()` is exception-guarded

**File:** `MSToolkit/src/MSToolkit/RAWReader.cpp:294-579`

`readRawFile()` wraps exactly one call -- `GetCentroidStream` (lines 473-483) -- in
`try`/`catch(Exception^)`. Every other call into the vendor library is unguarded:
`RawFileReaderAdapter::FileFactory` (334), `GetScanEventForScanNumber`/
`GetScanEventStringForScanNumber` (367-369, 405), `GetScanStatsForScanNumber` (406),
`RetentionTimeFromScanNumber` (407), `GetTrailerExtraInformation` (146, via `ReadTrailerFields`),
`GetReaction` (448), `GetSegmentedScanFromScanNumber` (485).

RawFileReader is closed-source code reading arbitrary, sometimes malformed or edge-case,
user-supplied binary files -- a genuine system boundary. If any of these throw on a corrupted or
unusual file/scan, the managed exception propagates uncaught into plain-native
`CometPreprocess.cpp`, which has no awareness of `System::Exception^` and isn't written to catch
it. The likely outcome is the entire Comet process going down, including whatever multi-hour
unattended batch search was in progress on other files.

**Fix:** extend the pattern already applied to the centroid call to the rest of the function --
wrap the file-open path and the per-scan extraction body in `try { ... } catch(Exception^) { return
false; }`, consistent with the function's existing `bool` failure contract.

**Status: fixed.** The former `readRawFile()` body was renamed to a private `readRawFileImpl()`
(`RAWReader.h:108`, `RAWReader.cpp`), and `readRawFile()` is now a thin wrapper that calls it inside
`try { ... } catch(Exception^ ex) { ...; return false; }`. This catches any exception from any
RawFileReader call reachable from `readRawFileImpl` -- not just the already-guarded centroid-stream
call -- and converts it to the function's existing `false` failure contract instead of letting it
propagate into native callers. Verified: full solution rebuild clean (zero errors); unit suite
19/19 passed; `test_raw_vs_mzxml.py` re-run produced byte-identical PSM/record counts to the
pre-fix baseline, confirming no behavior change on the non-exceptional path.

---

## 3. Code Quality & Maintainability

### 3a. No automated test coverage for the new/changed logic paths

`tests/regression/test_raw_vs_mzxml.py` predates this branch (already on `master`) and was reused,
not extended. It exercises one real Orbitrap HCD file's MS1/MS2 scans only -- the cases already
known to work. None of the following have any automated coverage:

- ECD/PQD/IRMPD activation detection (`MapActivation`, `RAWReader.cpp:119-130`)
- MSX/zoom-scan/SRM token classification (`EvaluateScanTokens`, `RAWReader.cpp:65-109`)
- The centroid-stream-exception-fallback path (`RAWReader.cpp:473-489`)

All validation of these paths during this work was manual/interactive (spike harnesses, ad-hoc
scan printouts), not captured as a repeatable test. A future change -- even a RawFileReader package
version bump -- could silently regress any of these with nothing to catch it.

**Status: open.**

### 3b. Exception-as-control-flow in the centroid-stream fallback was never benchmarked against an alternative

**File:** `MSToolkit/src/MSToolkit/RAWReader.cpp:473-483`

The `GetCentroidStream` try/catch is sound for FT/Orbitrap files (no exception in the common path),
but for a genuinely profile-only, non-FT instrument file, *every single scan read* would throw and
catch a .NET exception -- a known-expensive pattern when used as routine control flow rather than
for truly exceptional conditions. `ThermoFisher.CommonCore.Data.Business.Scan.HasCentroidStream`
(via `Scan::FromFile`) appears designed for exactly this check without relying on an exception; it
wasn't evaluated as an alternative before settling on try/catch.

**Status: fixed.** Confirmed via `ThermoFisher.CommonCore.Data.xml` (the NuGet package's IntelliSense
doc) that `Scan` exposes exactly this as a first-class, non-throwing feature:
`Scan::FromFile(rawFile, scanNumber)` returns a `Scan` object whose `PreferredMasses`/
`PreferredIntensities` "Gets ... for default data stream (usually centroid stream, if present).
Falls back to SegmentedScan data if centroid stream is not preferred or not present" -- i.e. the
exact centroid-or-profile fallback this code needed, built into the vendor API with no exception
involved either way. `HasCentroidStream` reports which path was used, replacing the `gotCentroid`/
`bCentroid` bookkeeping the try/catch version needed.

`RAWReader.cpp`'s try/catch around `GetCentroidStream`, plus the separate `GetSegmentedScanFromScanNumber`
fallback call, were replaced with:
```cpp
Scan^ scanObj = Scan::FromFile(rawFile, (int)rawCurSpec);
scanObj->PreferCentroids = true;
bool bCentroid = scanObj->HasCentroidStream;
cli::array<double>^ peakMasses = scanObj->PreferredMasses;
cli::array<double>^ peakIntensities = scanObj->PreferredIntensities;
int numPeaks = peakMasses->Length;
```
`PreferCentroids` is set explicitly (rather than relying on its undocumented default) so intent --
"try centroid first, fall back to profile" -- isn't left implicit.

The narrowly-scoped try/catch this removes was the *only* exception handling in the function before
issue 2a's fix; any exception from `Scan::FromFile` itself (e.g. a genuinely invalid scan number)
is now caught by 2a's outer `readRawFile()`/`readRawFileImpl()` boundary instead, which is the
correct division of responsibility: that boundary handles real failures, while this code no longer
manufactures an exception for the routine, expected "no centroid stream" case.

Verified: full solution rebuild clean; unit suite 19/19 passed; `test_raw_vs_mzxml.py` re-run
produced byte-identical PSM/record counts to every prior baseline in this document (1524/1491
TXT PSMs, 1464/1464 top-peptide agreement, 49853/49844 SQT and PepXML, 44756/44758 mzIdentML and
Percolator) -- confirming `Scan::FromFile`/`PreferredMasses`/`PreferredIntensities` reproduces the
prior `GetCentroidStream`/`GetSegmentedScanFromScanNumber` behavior exactly on this real Orbitrap
HCD file, while removing the exception-as-control-flow path entirely (not just benchmarking it).

### 3c. Dead API surface inherited and left in place

`RAWReader::setRawFilterExact` (`RAWReader.h:78`) is declared but never defined (same for
`lookupRT`, `setAverageRaw`, `setLabel`, `calcPepMass`) -- pre-existing, not introduced by this
branch. Because of this, `rawUserFilterExact` permanently stays `true` (set once in the
constructor, never settable), making the substring-match branch at `RAWReader.cpp:379`
(`curFilter.find(rawUserFilter)`) unreachable dead code. Flagging per the project's own checklist
("stub that could resolve incorrectly later," future-refactor risk even though currently harmless).

**Status: open, low priority.**

### 3d. Pre-existing bug noticed while reading this file, out of scope for this branch

`RAWReader.cpp:245`: `if(iStart = 0) iStart++;` inside `calcChargeState` -- an assignment where a
comparison (`==`) was almost certainly intended, making the guarded `iStart++` permanently
unreachable. Confirmed unchanged from `master`'s version of this file via `git show
master:MSToolkit/src/MSToolkit/RAWReader.cpp`, so not introduced here. Worth a follow-up issue since
the function was otherwise carried over verbatim.

**Status: fixed.** Changed to `if(iStart == 0) iStart++;`. This activates the guard for the one
case it was apparently meant to handle (the "right side of the precursor window" scan starting at
array index 0). Verified: full solution rebuild clean; unit suite 19/19 passed;
`test_raw_vs_mzxml.py` re-run produced identical PSM/record counts to the pre-fix baseline -- the
existing test fixtures don't happen to exercise this specific edge case, so this is a narrow,
low-risk correctness fix with no observed behavior change on real data.

### 3e. Unrelated commit in this branch has a minor style inconsistency

Commit `00f3d665` ("sync MSToolkit with upstream mhoopmann/mstoolkit") is not part of the
RawFileReader work but is included in this branch's diff against `master`. `Spectrum.cpp`'s
`clear()` (around line 352) mixes 4-space indentation on its two new lines with the tab-indented
lines immediately around them. Mentioned for completeness only.

**Status: informational, not actionable for this branch.**

---

## 4. Actionable Improvements

### 4a. Resource leak on the open-failure path

**File:** `MSToolkit/src/MSToolkit/RAWReader.cpp:334-338`

```cpp
IRawDataPlus^ rf = RawFileReaderAdapter::FileFactory(gcnew String(c));
if(!rf->IsOpen || rf->IsError){
  cerr << "Cannot open " << c << endl;
  return false;   // rf is never delete'd
}
```

If `FileFactory` returns an allocated-but-failed-to-open object, it's never disposed. Likely holds
minimal resources in practice, but add `delete rf;` before `return false;` to match .NET's "always
dispose IDisposable" convention rather than relying on GC finalization timing.

**Status: fixed.** Added `delete rf;` immediately before the `return false;` in this branch.
Verified: full solution rebuild clean; unit suite 19/19 passed; `test_raw_vs_mzxml.py` re-run
produced identical PSM/record counts to the pre-fix baseline (this branch only runs on a genuine
open failure, which the regression fixture never hits, so no behavior change was expected or
observed on the success path).

### 4b. Hardcoded version string duplicated in `MSToolkit.vcxproj`

`5.0.0.93` appears twice in `<HintPath>` strings (around lines 171 and 175). A version bump
requires find-and-replace inside path strings rather than a single property change. Consider
`<ThermoVersion>5.0.0.93</ThermoVersion>` referenced as `$(ThermoVersion)` in both `HintPath`s.

**Status: fixed.** Added a `<ThermoVersion>5.0.0.93</ThermoVersion>` property next to the existing
`<ThermoNuGetCache>`, referenced as `$(ThermoVersion)` in both `HintPath`s. (One thing worth noting
for any future editor of this block: the explanatory comment above it must avoid a literal `--`
anywhere in its text, not just at the `-->` close -- MSBuild's XML parser rejects `--` inside a
comment with `MSB4025`, the same pitfall hit earlier in this branch's vcxproj edits.) Verified: full
solution rebuild clean; unit suite 19/19 passed; `test_raw_vs_mzxml.py` re-run produced identical
results to baseline (expected, since this is a pure build-file change with no effect on runtime
behavior).

### 4c. Per-element managed-array marshaling could use `Marshal::Copy`

**File:** `MSToolkit/src/MSToolkit/RAWReader.cpp:494-498`

```cpp
vector<double> masses(numPeaks), intensities(numPeaks);
for(int p=0; p<numPeaks; p++){
  masses[p]=peakMasses[p];
  intensities[p]=peakIntensities[p];
}
```

Negligible for the centroid path (typically hundreds of peaks), but for the profile-fallback path
(tens of thousands of points for a dense MS1 scan), a bulk
`System::Runtime::InteropServices::Marshal::Copy` would avoid the per-element bounds-check/indexing
overhead of a managed-array loop.

**Status: fixed.** Replaced the per-element loop with two `Marshal::Copy(peakMasses/peakIntensities,
0, IntPtr(masses.data()/intensities.data()), numPeaks)` calls (guarded by `numPeaks>0`), after adding
`using namespace System::Runtime::InteropServices;`. Verified: full solution rebuild clean; unit
suite 19/19 passed; `test_raw_vs_mzxml.py` re-run produced byte-identical PSM/record counts to every
prior baseline in this document, confirming the bulk copy reproduces the per-element loop's output
exactly.

### 4d. Add a regression test targeting the new activation-method mapping directly

Source (or construct) an ETD+SA or PQD fixture file, if available, and add a regression test
exercising `MapActivation` directly, rather than relying solely on the HCD-only fixture already in
the repo. Closes part of the gap noted in 3a.
