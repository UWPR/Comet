# Migrating from MSFileReader (COM) to RawFileReader (.NET)

**Date**: 2026-06-18
**Decision**: `.raw` file support remains **Windows-only**. Linux and macOS will not read `.raw`
directly; users on those platforms must supply mzML/mzXML input, optionally pre-converting `.raw`
files with an existing external tool (ThermoRawFileParser, ProteoWizard `msconvert`) outside of
Comet. This scopes the migration down to a same-platform library swap rather than a new
cross-platform interop architecture.
**Scope**: `MSToolkit/` (native `.raw` reading) and `CometWrapper/` cleanup. `RealtimeSearch/`
(RTS) was already, and remains, Windows-only -- untouched in spirit, only its dependency hygiene
changes (see Section 3).
**Goal**: Replace the legacy Thermo MSFileReader COM library used by `RAWReader.cpp` with
Thermo's newer, actively-maintained RawFileReader .NET library, using the same in-process C++/CLI
interop `CometWrapper` already uses elsewhere in this codebase.

---

## 1. Current State

Comet has **two independent, unrelated `.raw`-reading code paths** today. They use different
Thermo libraries and were clearly never unified:

### 1.1 Batch search path: MSToolkit -> MSFileReader COM (Windows-only)

- `MSToolkit/include/RAWReader.h:19-31` gates the whole class behind `_MSC_VER` /
  `_NO_THERMORAW`, and imports the COM type library directly by GUID:
  ```cpp
  #ifdef _MSC_VER
  #ifndef _NO_THERMORAW
  #import "libid:F0C5F3E3-4F2A-443E-A74D-0AABE3237494" rename_namespace("XRawfile") rename("value", "xValue")
  ```
- `MSToolkit/src/MSToolkit/RAWReader.cpp`:
  - `RAWReader::RAWReader()` calls `CoInitialize(NULL)` and `initRaw()`.
  - `initRaw()` cascades through `IXRawfile5Ptr` -> `IXRawfile4Ptr` -> `IXRawfile3Ptr` ->
    `IXRawfile2Ptr` -> `IXRawfilePtr`, calling `CreateInstance("MSFileReader.XRawfile.1")` on
    each, i.e. it depends on the MSFileReader COM redistributable being installed on the
    machine.
  - `readRawFile()` pulls peaks via `m_Raw->GetMassListFromScanNum(...)` into a VARIANT
    `SAFEARRAY`, and metadata via `GetScanHeaderInfoForScanNum`, `RTFromScanNum`,
    `GetPrecursorMassForScanNum`, etc.
- `MSToolkit/include/MSReader.h` is the format-dispatching reader that CometSearch calls.
  It holds a `RAWReader cRAW` member, again gated by `_MSC_VER` / `_NO_THERMORAW`.
  `MSReader::readFile()` (`MSToolkit/src/MSToolkit/MSReader.cpp:1206+`) switches on file
  extension and calls `cRAW.readRawFile()` for the `raw` case; on any non-Windows build (or
  `_NO_THERMORAW`) it just prints `"Thermo RAW file format not supported."` and returns false.
- `MSToolkit/Makefile:138-143` filters `RAWReader.cpp` out of the source list on Linux/macOS
  builds -- it isn't even compiled there. That stays true after this migration, just for a
  different reason (Windows-only by decision, not COM-only by limitation -- see Section 4).
- `CometSearch/CometPreprocess.cpp` (`ReadPrecursors`, `LoadAndPreprocessSpectra`,
  `FusedLoadAndSearchSpectra`) calls `MSReader::readFile()` directly. This is the only
  consumer of `RAWReader` -- there is no other native code path to a `.raw` file.

### 1.2 Real-time search path: RealtimeSearch -> RawFileReader .NET (Windows-only)

- `RealtimeSearch/SearchMS1MS2.cs:1-17` and `RealtimeSearch/Search.cs` already use Thermo's
  newer managed API:
  ```csharp
  using ThermoFisher.CommonCore.Data.Business;
  using ThermoFisher.CommonCore.Data.FilterEnums;
  using ThermoFisher.CommonCore.Data.Interfaces;
  using ThermoFisher.CommonCore.RawFileReader;
  ...
  using (IRawDataPlus rawFile = RawFileReaderAdapter.FileFactory(rawFileName))
  ```
  and pull peaks via `rawFile.GetCentroidStream(...)` / `GetSegmentedScanFromScanNumber(...)`,
  metadata via `GetScanStatsForScanNumber`, `RetentionTimeFromScanNumber`,
  `GetScanEventForScanNumber(...).GetReaction(0)`, and `GetTrailerExtraInformation` (string
  matching on `"Monoisotopic M/Z:"` / `"Charge State:"`).
- `RealtimeSearch/RealtimeSearch.csproj:13` targets **`v4.7.2`** (.NET Framework), and lines
  64-75 reference the Thermo assemblies via local `HintPath`s into `RealtimeSearch/DLLs/*.dll`,
  not via NuGet `PackageReference`.
- `RealtimeSearch.csproj:118-121` has a `ProjectReference` to `CometWrapper/CometWrapper.vcxproj`
  -- a **C++/CLI** assembly bridging into native `CometSearch`. This is the proven, in-process
  managed/native bridge this migration will reuse for `RAWReader` (see Section 3).
- `CometWrapper/MSFileReaderWrapper.cpp` is a *third*, currently-unused bridge: a C++/CLI wrapper
  around `MSReader`/`RAWReader` (i.e. the COM path) exposed to managed code. It is not what
  `SearchMS1MS2.cs` actually calls -- that class talks to `RawFileReaderAdapter` directly. It's
  effectively dead code today and should be deleted as part of this work (see Section 5, Phase 3).

### 1.3 Summary table

| | Batch (`Comet.exe` / `CometSearch`) | RTS (`RealtimeSearch.exe`) |
|---|---|---|
| Library | MSFileReader COM (`IXRawfile2..5`) | RawFileReader .NET (`IRawDataPlus`) |
| Reader class | `MSToolkit::RAWReader` | inline in `SearchMS1MS2.cs` / `Search.cs` |
| Platform | Windows only | Windows only |
| Requires separate install | Yes -- MSFileReader COM redistributable | No -- managed DLLs, ships with the app |

Both paths stay Windows-only after this migration; the change is that both end up calling the
same actively-maintained Thermo library instead of two divergent ones, and the COM redistributable
dependency goes away.

---

## 2. Why migrate at all, if not for cross-platform reach?

- **MSFileReader is the legacy library.** RawFileReader .NET is Thermo's actively maintained
  successor and is more likely to support newer instrument firmware/file format versions going
  forward; staying on MSFileReader risks falling behind on `.raw` files from newer instruments.
- **Drops an install-time dependency.** MSFileReader COM requires a separate redistributable
  installed on the machine (`initRaw()`'s `CreateInstance("MSFileReader.XRawfile.1")` calls fail
  outright if it's missing). RawFileReader ships as ordinary NuGet-restored managed assemblies --
  one less manual setup step for Windows users building or running Comet.
- **Unifies on one library.** Batch search and RTS currently depend on two unrelated Thermo
  libraries with different APIs for the same underlying `.raw` format. After this migration both
  go through RawFileReader .NET, so there's one library version to track, not two.

---

## 3. The interop approach: C++/CLI, in-process, Windows-only

RawFileReader .NET is a managed .NET library; `RAWReader`/`MSReader` are native C++. Because
`.raw` support stays Windows-only, the interop problem is much simpler than it would be for a
cross-platform design: Windows already has a proven, in-process bridge for calling managed code
from native code -- **C++/CLI** -- the same mechanism `CometWrapper` already uses (in the opposite
direction, native-called-from-managed) to connect `RealtimeSearch.exe` to native `CometSearch`.

Plan: recompile `RAWReader.cpp`/`.h` with `/clr` and replace its COM internals with calls into
`RawFileReaderAdapter`/`IRawDataPlus`, in the same process, with **no IPC, no sidecar process, and
no CoreCLR hosting API** (`nethost`/`hostfxr`) -- those would only be necessary to reach a platform
with no CLR available at all, which is no longer a goal.

| `RAWReader.cpp` today (COM) | Replacement (RawFileReader .NET via C++/CLI) |
|---|---|
| `m_Raw->GetMassListFromScanNum(...)` -> SAFEARRAY | `rawFile->GetCentroidStream(...)` / `GetSegmentedScanFromScanNumber(...)` -> managed `double[]` |
| `GetScanHeaderInfoForScanNum`, `RTFromScanNum` | `GetScanStatsForScanNumber`, `RetentionTimeFromScanNumber` |
| `GetPrecursorMassForScanNum` (via `IXRawfile4Ptr`) | `GetScanEventForScanNumber(...).GetReaction(0)` + `GetTrailerExtraInformation` (same fields `SearchMS1MS2.cs` already reads: `"Monoisotopic M/Z:"`, `"Charge State:"`) |
| `CoInitialize` / `CreateInstance("MSFileReader.XRawfile.1")` | `RawFileReaderAdapter.FileFactory(path)` |

The public surface `MSReader::readFile()` calls into (`readRawFile()`, `getInstrument()`,
`getManufacturer()`, etc.) stays the same, so `CometSearch/CometPreprocess.cpp` and everything
above `MSReader` needs **zero changes** -- this is purely an implementation swap inside MSToolkit.

Open items to confirm in Phase 0 (Section 5): whether MSVC's per-translation-unit `/clr` flag mixes
cleanly with the rest of MSToolkit's plain-native `.cpp` files inside the same static library
(it's supported by MSVC, but not exercised anywhere else in this codebase -- `CometWrapper` is an
entirely separate, all-`/clr` project, not a mixed static lib), and how to marshal the managed
`double[]` peak arrays back into native buffers without leaks (the `pin_ptr`/`Marshal::Copy`
equivalent of the SAFEARRAY copy `RAWReader.cpp` already does today at lines ~724-727).

*(Cross-platform interop options considered and set aside under the current Windows-only
decision -- out-of-process converter, long-running IPC sidecar, in-process CoreCLR hosting via
`nethost`/`hostfxr` -- are recorded in the Appendix in case cross-platform `.raw` reading becomes a
goal again later.)*

---

## 4. RTS (RealtimeSearch) stays Windows-only

`RealtimeSearch.exe` already uses RawFileReader .NET for reading, and was already Windows-only
regardless of which Thermo library MSToolkit uses, because `RealtimeSearch.csproj:118-121`'s
`ProjectReference` to `CometWrapper.vcxproj` is a C++/CLI assembly with no Linux/macOS equivalent.
Nothing in this migration changes that, and making RTS cross-platform is not a goal.

What *should* still happen to RTS as a byproduct of this migration -- dependency hygiene, not
portability:
- `RealtimeSearch.csproj` is on .NET Framework 4.7.2 with DLL `HintPath` references -- already
  overdue for modernization regardless of this migration. Moving to NuGet `PackageReference`s for
  the Thermo packages (instead of vendored DLLs in `RealtimeSearch/DLLs/`) gets dependency
  versioning and security updates "for free," and lets it track the same package version as
  MSToolkit's new `RAWReader` (Section 5, Phase 3).
- `CometWrapper/MSFileReaderWrapper.{h,cpp}` (the COM-based wrapper that nothing currently calls)
  should be deleted once `RAWReader`/COM is retired -- keeping an unused bridge to a library being
  removed is exactly the kind of dead code the project's review checklist already flags.

---

## 5. Proposed migration plan

### Phase 0 -- Spike (Windows only, no production code changes)

**Done (2026-06-18).** Spike code lives in `spike/RawFileReaderSpike/` (the new, `/clr`-based
path) and `spike/LegacyRawReaderSpike/` (the existing COM-based path, built for side-by-side
comparison); see `spike/README.md` for how to rebuild/rerun. Both were run against a real
production `.raw` file (`20170103_HelaQC_01.raw`, 56,152 scans). Findings per item:

1. **Reference mechanics: does NOT carry over from csproj to vcxproj as hoped.** `PackageReference`
   restore for a native `.vcxproj` resolves the project's framework as `native,Version=v0.0`
   regardless of `CLRSupport=true`, which NuGet considers incompatible with these
   netstandard2.0-only packages (`NU1202`). The working fallback: a plain `<Reference HintPath>`
   pointing at the package already extracted into the global NuGet cache by restoring
   `RealtimeSearch.csproj` (`%userprofile%\.nuget\packages\thermofisher.commoncore.*\5.0.0.93\lib\netstandard2.0\*.dll`).
   This means the real `RAWReader.cpp` rewrite (Phase 2) needs `<Reference>`/`HintPath` entries in
   the `.vcxproj`, not `<PackageReference>` -- the doc's Phase 2 item 2 is updated below to match.
   A handful of `C4691` warnings appear (managed types resolving against the current translation
   unit instead of the unreferenced `netstandard` forwarding assembly) -- benign; the code compiles
   and runs correctly.
2. **Marshaling a managed `double[]` into a native buffer: works, no leak in the marshal itself.**
   `GetCentroidStream(...)->Masses`/`->Intensities` copied element-by-element into
   `std::vector<double>` and passed to a plain-native function, exactly mirroring the SAFEARRAY
   copy pattern in `RAWReader.cpp` (~lines 724-727). A 130,000-iteration stress loop
   (`RawFileReaderSpike.exe <path> --stress`) showed process working set climbing from ~133 MB to
   a plateau at ~400 MB exactly once every one of the file's 56,152 scans had been visited once,
   then staying flat for 70,000+ further reps -- a one-time, file-size-bounded internal cache
   inside RawFileReader itself, not an unbounded leak. Re-running against a single repeated scan
   number (`--stress samescan`) confirmed working set is flat from the very first rep. Managed
   heap size (`GC::GetTotalMemory`) stayed in the same ~35-40 MB band throughout both runs.
3. **Mixing `/clr` and plain-native `.cpp` in one project: works cleanly.** `NativeUtils.cpp`
   (`CompileAsManaged=false` override) compiled and linked into the same binary as the
   `/clr`-enabled `Main.cpp` with no special handling beyond the per-file MSBuild property.
4. **Field parity: matches the legacy COM path almost exactly.** Ran both spikes against the same
   four scans (1, 1000, 2000, 5000) of the same file. Scan number, centroid flag, MS level,
   polarity, TIC, base peak m/z/intensity, precursor isolation m/z, charge state, peak count, and
   every individual peak m/z/intensity matched exactly. RT and base-peak-intensity differed only at
   the ~1e-6 relative level (legacy `Spectrum` stores these as `float`; RawFileReader returns
   `double`) -- a precision choice for Phase 1 to make deliberately, not a parity bug.

### Phase 1 -- Rewrite `RAWReader` against RawFileReader .NET
**Done (2026-06-19).**

1. `MSToolkit/include/RAWReader.h`: removed the `#import "libid:..."` COM block and
   `XRawfile::IXRawfilePtr m_Raw` member. The replacement is a PIMPL: `RAWReaderImpl* pImpl`, where
   `RAWReaderImpl` is only forward-declared in the header and fully defined inside
   `RAWReader.cpp`. This is required, not just a style choice -- `MSReader.h` (`MSReader.cpp:`
   `RAWReader cRAW;`) embeds `RAWReader` **by value**, and `MSReader.h` is included by plain-native,
   non-`/clr` code throughout `MSToolkit`/`CometSearch`. If `RAWReader.h` itself referenced a
   managed handle type (`IRawDataPlus^`), every translation unit that includes it transitively
   would need to compile `/clr`, which is not acceptable for the rest of the codebase. Dead
   declared-but-never-defined methods identified during the read-through
   (`lookupRT`, `setAverageRaw`, `setLabel`, `setRawFilterExact`, `calcPepMass`) were left alone --
   out of scope for a like-for-like rewrite.
2. `MSToolkit/src/MSToolkit/RAWReader.cpp`: full rewrite against
   `ThermoFisher::CommonCore::RawFileReader`. Key findings that shaped the implementation,
   none of which were knowable from the API surface alone -- all confirmed against real
   production `.raw` files (`data/20250520_Hela_60min_06.raw`, via the Phase 0
   `spike/LegacyRawReaderSpike` harness rebuilt against the new lib):
   - **A plain native struct cannot hold a `^` tracking handle as a member** (`error C3265`).
     `RAWReaderImpl` (and any other native-side holder of a managed handle) must use
     `gcroot<T>` from `<vcclr.h>`, not a bare `T^` field. `gcnew`/`delete` on the handle still
     work as expected through `gcroot`; only direct member storage needs the wrapper.
   - **`cli::array<T>^` must be written out in full** when `using namespace std;` is in scope --
     bare `array<T>^` is ambiguous with `std::array` and silently resolves to the wrong one in
     some contexts (compiles fine for one declaration, fails with "too few template arguments"
     for another in the same file), which is more confusing than an outright error.
   - **`IScanEventBase.SupplementalActivation`/`.CompensationVoltage` are filter-matching rule
     settings** (`TriState`/`CompensationVoltageType`: on/off/any), **not the per-scan value** --
     despite reading like per-scan booleans/doubles. There is no structured per-scan replacement
     for the legacy COM filter string's `"cv=..."` token, so `EvaluateScanTokens()` keeps
     tokenizing the human-readable string from `GetScanEventStringForScanNumber()` exactly as the
     old `evaluateFilter()` tokenized the COM filter string (same left-to-right precedence rules)
     for compensation voltage and for MSX/zoom-scan/SRM classification, since the token format
     itself is unchanged across both Thermo APIs.
   - **Activation method does have a genuine structured per-scan equivalent, despite the above** --
     `IReaction.ActivationType`/`IReaction.MultipleActivation` (the same `IReaction` object already
     fetched via `scanEvent->GetReaction(msLevel-2)` for precursor mass) are real per-scan values,
     not filter rules; confirmed by comparing `ActivationType` against 4 known-HCD scans, where it
     matched the legacy/token-derived `"hcd"` exactly every time. This is a strict improvement over
     the initial implementation (which used `EvaluateScanTokens()`'s `"<mz>@<act>"` 3-letter-code
     parsing for this too, mirroring `evaluateFilter()`'s `cid`/`etd`/`hcd`-only matching): the
     `ActivationType` enum also covers `ElectronCaptureDissociation` (ECD), `PQD`, and
     `MultiPhotonDissociation` (IRMPD) -- none of which the filter-string 3-letter code ever
     recognized, in either the legacy COM parser or this rewrite's first pass -- so switching to
     `MapActivation(IReaction^)` (`RAWReader.cpp`) added ECD/PQD/IRMPD support for `.raw` for the
     first time. `mstSID` remains permanently undetectable from `.raw` data: there is no `SID`
     value anywhere in Thermo's `ActivationType` enum, because surface-induced dissociation isn't
     a Thermo instrument capability -- it's a non-standard scan event a particular research lab
     implemented locally, with no representation in Thermo's own scan/filter vocabulary to read
     back, regardless of API.
   - **`ScanStatistics.IsCentroidScan` and `SpectrumPacketType` are not reliable predictors of
     whether a usable centroid stream exists.** A scan reporting `IsCentroidScan=false` (i.e.
     profile) on this Orbitrap file still had a valid, populated centroid stream -- confirmed by
     comparing `GetSegmentedScanFromScanNumber()` (returned real m/z positions but all-zero
     intensities for that scan) against `GetCentroidStream()` (returned correct non-zero
     intensities for the same scan, byte-for-byte matching the independent Phase 0
     `RawFileReaderSpike` harness's output). The implemented rule: always try
     `GetCentroidStream()` first; fall back to `GetSegmentedScanFromScanNumber()` (profile) only
     if it throws or returns zero peaks. `Spectrum::setCentroidStatus()` now reflects which path
     was actually used, not the scan's nominal `IsCentroidScan` flag.
   - Field-for-field output (RT, TIC, BPI/BPM, MS level, charge, isolation/mono m/z, SPS masses,
     ion injection time, peak m/z + intensity) was spot-checked against real MS1 and MS2 scans
     (1, 100, 5000, 30005) of the same file and looks correct; MSX/SRM/zoom-scan paths could not be
     exercised (no such scans available in the test files) and rely on the token-mirroring logic
     above for correctness.
3. `RAWReader`'s and `MSReader`'s public method signatures are unchanged; `CometSearch/CometPreprocess.cpp`
   required no changes.

### Phase 2 -- Build system changes (Windows only)
1. **Done (2026-06-19).** `MSToolkit/VisualStudio/MSToolkit.vcxproj`: project-level
   `<CLRSupport>true</CLRSupport>` + `<ExceptionHandling>Async</ExceptionHandling>` (both
   Debug/Release), with every file **except** `RAWReader.cpp` opted back out via a per-file
   `<CompileAsManaged>false</CompileAsManaged>` override. The reverse (project default = native,
   opt `RAWReader.cpp` *in* per-file) does not fully work -- MSBuild emits `warning MSB8077` and
   the compile fails with `error C1107: could not find assembly 'mscorlib.dll'`, because the
   project-level CLR build infrastructure (assembly search paths, etc.) only gets wired up when
   `CLRSupport` is set at the project level. `/clr` also requires
   `ExceptionHandling=Async` project-wide (`/EHa`); the project's previous implicit default
   conflicted with `/clr` (`error D8016`). No changes needed to `CometSearch.vcxproj`/`Comet.vcxproj`
   -- linking the now-mixed-mode `MSToolkit.lib` into the plain-native `Comet.exe` (and
   `CometWrapper.dll`/`RealtimeSearch.exe`) worked with no `RuntimeLibrary`/CLR-related changes
   needed there; full `Comet.sln` Release|x64 build verified clean.
2. **Done (2026-06-19).** Added `<Reference>`/`HintPath` entries (not `<PackageReference>` --
   confirmed incompatible with native `.vcxproj` restore in Phase 0 item 1) for
   `ThermoFisher.CommonCore.RawFileReader`/`.Data` to `MSToolkit.vcxproj`, pointing at
   `$(USERPROFILE)\.nuget\packages\thermofisher.commoncore.*\5.0.0.93\...` -- the same package
   version vendored for `RealtimeSearch.csproj` (Phase 3 item 1), populated into the global NuGet
   cache by restoring that project at least once.
3. **Done (2026-06-19).** Updated the comment at `MSToolkit/Makefile:136-140` to say
   `RAWReader.cpp` is excluded from Linux/macOS builds because `.raw` support is Windows-only by
   decision (it needs a `/clr` build, MSVC/Windows-only), not because of any Linux-specific
   limitation in RawFileReader .NET itself. No functional Makefile change.
4. **Done (2026-06-19).** Dropped the MSFileReader COM install step from `README.md` and
   `CLAUDE.md`'s Windows build instructions, and updated `MSToolkit/README.md`'s file-format list
   and `spike/README.md` (the `LegacyRawReaderSpike` harness no longer needs COM registered, since
   it now links the rewritten `MSToolkit.lib`).

### Phase 1/2 side fix -- dead `main()` in `mzMLReader.cpp`
**Done (2026-06-19).** `MSToolkit/src/mzParser/mzMLReader.cpp` had an unguarded, dead standalone
test `int main(int argc, char* argv[])` (interactive mzML browser, unrelated to this migration).
It was harmless under normal linking (never pulled from the archive, since nothing referenced it
and the consuming `.exe` always defines its own `main`), but once `MSToolkit.lib` contains a
`/clr`/`/GL`-tagged object, MSVC's linker restarts the link with `/LTCG`, which pulls in more
archive members than a normal link and surfaced `LNK2005: main already defined`. Fixed by guarding
the dead code with `#ifdef MZMLREADER_STANDALONE_MAIN` (never defined by any build target) rather
than deleting it, since it predates this migration and isn't otherwise in scope here.

### Phase 3 -- Modernize `RealtimeSearch.csproj` and delete dead code
1. **Done (2026-06-18).** Replaced the `HintPath` DLL references
   (`RealtimeSearch/RealtimeSearch.csproj:64-75`) with NuGet `PackageReference`s
   (`ThermoFisher.CommonCore.{BackgroundSubtraction,Data,MassPrecisionEstimator,RawFileReader}`,
   version `5.0.0.93`) and deleted the now-unused `RealtimeSearch/DLLs/` folder. Since these
   packages aren't published on nuget.org (confirmed: `api.nuget.org` 404s for them), the `.nupkg`
   files were pulled from Thermo's official `thermofisherlsms/RawFileReader` GitHub repo
   (`Libs/Net471`, matching this project's `v4.7.2` target) and committed to a local feed at
   `RealtimeSearch/ThermoNuGet/`, registered via `RealtimeSearch/nuget.config`. `nuget.config`
   does not clear the default sources, since `RawFileReader`'s own dependency on `OpenMcdf`
   resolves from nuget.org normally. Verified with `dotnet restore` -- all four packages and their
   transitive dependencies (`OpenMcdf`, `OpenMcdf.Extensions`) resolve cleanly. This was done
   ahead of Phases 0-2 (RTS already used RawFileReader .NET directly, so it didn't depend on
   the `RAWReader`/`MSReader` rewrite); Phase 2 item 2 should point its `.vcxproj` `<Reference>`
   entries at this same local feed/version once it happens, to keep both consumers on one
   version as originally intended.
2. **Done (2026-06-19).** Deleted `CometWrapper/MSFileReaderWrapper.{h,cpp}` (dead, COM-based,
   unused -- nothing referenced its `MSFileReaderWrapper`/`Peak_T_Wrapper` types) and removed their
   entries from `CometWrapper.vcxproj`/`.vcxproj.filters`. Full `Comet.sln` Release|x64 rebuild
   verified clean.

---

## 6. Summary

| Phase | Outcome | Platform | Status |
|---|---|---|---|
| 0 | Validated `/clr` + RawFileReader marshaling approach | Windows (spike) | Done (2026-06-18); see findings above and `spike/` |
| 1 | `RAWReader` reimplemented against RawFileReader .NET | Windows | Done (2026-06-19); see findings above |
| 2 | Build system updated; COM redistributable dependency removed | Windows | Done (2026-06-19) |
| 3 | `RealtimeSearch` on NuGet-based Thermo deps; dead COM wrapper removed | Windows (hygiene only) | Done (2026-06-19) |

Linux and macOS are unchanged by this migration: `.raw` is not supported; users supply mzML/mzXML
input, optionally pre-converted from `.raw` with an external tool (ThermoRawFileParser,
`msconvert`) outside of Comet.

---

## Appendix -- cross-platform options considered, not pursued

An earlier revision of this plan explored making `.raw` reading work on Linux/macOS as well.
Recorded here in case cross-platform `.raw` support becomes a goal again later:

- **Confirmed during that research**: `ThermoFisher.CommonCore.RawFileReader` does ship a
  `netstandard2.0` build and is used cross-platform in production by the open-source
  [ThermoRawFileParser](https://github.com/compomics/ThermoRawFileParser) project (self-contained
  builds for Windows/Linux/macOS on plain .NET 8). The library itself is not what blocks
  cross-platform support -- only the native/managed interop mechanism is, since there's no
  COM/C++-CLI equivalent on Linux/macOS.
- **Out-of-process converter** (build a Comet-owned `.raw -> mzML` tool, auto-invoked by
  `MSReader`): rejected even when cross-platform was in scope, because mature external tools
  (ThermoRawFileParser, `msconvert`) already solve this -- a user can already convert by hand and
  feed Comet mzML directly.
- **Long-running sidecar process** (a persistent .NET process answering scan requests over local
  IPC -- named pipe/Unix domain socket): was the leading cross-platform candidate, since it avoids
  full-file conversion and preserves random-access scan reads. Reintroduce this first if
  cross-platform `.raw` reading is revisited.
- **In-process CoreCLR hosting** via `nethost`/`hostfxr` (a native process loads the CLR itself
  and calls managed code directly, with no separate process): the most complete cross-platform
  answer, but with the most build-system, deployment-footprint (conflicts with Comet's `-static`
  Linux build), and memory-safety overhead of the options considered.
  - **DNNE** (.NET Native Exports) was also considered as a way to package this. Its "hosted"
    mode just auto-generates the native C-export shim around `hostfxr` described above -- same
    mechanism, same tradeoffs, just less hand-written boilerplate. Its "AOT" mode (compiling
    straight to native code with no CLR present at runtime) is a genuinely different idea, but
    doesn't change the conclusion: NativeAOT compiles from your own source/IL at publish time, so
    it can't retroactively AOT-compile a third-party precompiled `netstandard2.0` assembly with no
    source available. `ThermoFisher.CommonCore.RawFileReader` is closed-source, vendor-shipped IL
    with no stated AOT compatibility (it predates NativeAOT going mainstream) -- same closed-source
    wall that rules out the other options above.
