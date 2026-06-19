# Migrating from MSFileReader (COM) to RawFileReader (.NET)

**Date**: 2026-06-18
**Decision**: `.raw` file support remains **Windows-only**. Linux and macOS will not read `.raw`
directly; users on those platforms must supply mzML/mzXML input, optionally pre-converting `.raw`
files with an existing external tool (ThermoRawFileParser, ProteoWizard `msconvert`) outside of
Comet. This scopes the migration down to a same-platform library swap rather than a new
cross-platform interop architecture.
**Scope**: `MSToolkit/` (native `.raw` reading) and `CometWrapper/` cleanup. `RealtimeSearch/`
(RTS) was already, and remains, Windows-only — untouched in spirit, only its dependency hygiene
changes (see §3).
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
  builds — it isn't even compiled there. That stays true after this migration, just for a
  different reason (Windows-only by decision, not COM-only by limitation — see §4).
- `CometSearch/CometPreprocess.cpp` (`ReadPrecursors`, `LoadAndPreprocessSpectra`,
  `FusedLoadAndSearchSpectra`) calls `MSReader::readFile()` directly. This is the only
  consumer of `RAWReader` — there is no other native code path to a `.raw` file.

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
  — a **C++/CLI** assembly bridging into native `CometSearch`. This is the proven, in-process
  managed/native bridge this migration will reuse for `RAWReader` (see §3).
- `CometWrapper/MSFileReaderWrapper.cpp` is a *third*, currently-unused bridge: a C++/CLI wrapper
  around `MSReader`/`RAWReader` (i.e. the COM path) exposed to managed code. It is not what
  `SearchMS1MS2.cs` actually calls — that class talks to `RawFileReaderAdapter` directly. It's
  effectively dead code today and should be deleted as part of this work (see §5, Phase 3).

### 1.3 Summary table

| | Batch (`Comet.exe` / `CometSearch`) | RTS (`RealtimeSearch.exe`) |
|---|---|---|
| Library | MSFileReader COM (`IXRawfile2..5`) | RawFileReader .NET (`IRawDataPlus`) |
| Reader class | `MSToolkit::RAWReader` | inline in `SearchMS1MS2.cs` / `Search.cs` |
| Platform | Windows only | Windows only |
| Requires separate install | Yes — MSFileReader COM redistributable | No — managed DLLs, ships with the app |

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
  outright if it's missing). RawFileReader ships as ordinary NuGet-restored managed assemblies —
  one less manual setup step for Windows users building or running Comet.
- **Unifies on one library.** Batch search and RTS currently depend on two unrelated Thermo
  libraries with different APIs for the same underlying `.raw` format. After this migration both
  go through RawFileReader .NET, so there's one library version to track, not two.

---

## 3. The interop approach: C++/CLI, in-process, Windows-only

RawFileReader .NET is a managed .NET library; `RAWReader`/`MSReader` are native C++. Because
`.raw` support stays Windows-only, the interop problem is much simpler than it would be for a
cross-platform design: Windows already has a proven, in-process bridge for calling managed code
from native code — **C++/CLI** — the same mechanism `CometWrapper` already uses (in the opposite
direction, native-called-from-managed) to connect `RealtimeSearch.exe` to native `CometSearch`.

Plan: recompile `RAWReader.cpp`/`.h` with `/clr` and replace its COM internals with calls into
`RawFileReaderAdapter`/`IRawDataPlus`, in the same process, with **no IPC, no sidecar process, and
no CoreCLR hosting API** (`nethost`/`hostfxr`) — those would only be necessary to reach a platform
with no CLR available at all, which is no longer a goal.

| `RAWReader.cpp` today (COM) | Replacement (RawFileReader .NET via C++/CLI) |
|---|---|
| `m_Raw->GetMassListFromScanNum(...)` -> SAFEARRAY | `rawFile->GetCentroidStream(...)` / `GetSegmentedScanFromScanNumber(...)` -> managed `double[]` |
| `GetScanHeaderInfoForScanNum`, `RTFromScanNum` | `GetScanStatsForScanNumber`, `RetentionTimeFromScanNumber` |
| `GetPrecursorMassForScanNum` (via `IXRawfile4Ptr`) | `GetScanEventForScanNumber(...).GetReaction(0)` + `GetTrailerExtraInformation` (same fields `SearchMS1MS2.cs` already reads: `"Monoisotopic M/Z:"`, `"Charge State:"`) |
| `CoInitialize` / `CreateInstance("MSFileReader.XRawfile.1")` | `RawFileReaderAdapter.FileFactory(path)` |

The public surface `MSReader::readFile()` calls into (`readRawFile()`, `getInstrument()`,
`getManufacturer()`, etc.) stays the same, so `CometSearch/CometPreprocess.cpp` and everything
above `MSReader` needs **zero changes** — this is purely an implementation swap inside MSToolkit.

Open items to confirm in Phase 0 (§5): whether MSVC's per-translation-unit `/clr` flag mixes
cleanly with the rest of MSToolkit's plain-native `.cpp` files inside the same static library
(it's supported by MSVC, but not exercised anywhere else in this codebase — `CometWrapper` is an
entirely separate, all-`/clr` project, not a mixed static lib), and how to marshal the managed
`double[]` peak arrays back into native buffers without leaks (the `pin_ptr`/`Marshal::Copy`
equivalent of the SAFEARRAY copy `RAWReader.cpp` already does today at lines ~724-727).

*(Cross-platform interop options considered and set aside under the current Windows-only
decision — out-of-process converter, long-running IPC sidecar, in-process CoreCLR hosting via
`nethost`/`hostfxr` — are recorded in the Appendix in case cross-platform `.raw` reading becomes a
goal again later.)*

---

## 4. RTS (RealtimeSearch) stays Windows-only

`RealtimeSearch.exe` already uses RawFileReader .NET for reading, and was already Windows-only
regardless of which Thermo library MSToolkit uses, because `RealtimeSearch.csproj:118-121`'s
`ProjectReference` to `CometWrapper.vcxproj` is a C++/CLI assembly with no Linux/macOS equivalent.
Nothing in this migration changes that, and making RTS cross-platform is not a goal.

What *should* still happen to RTS as a byproduct of this migration — dependency hygiene, not
portability:
- `RealtimeSearch.csproj` is on .NET Framework 4.7.2 with DLL `HintPath` references — already
  overdue for modernization regardless of this migration. Moving to NuGet `PackageReference`s for
  the Thermo packages (instead of vendored DLLs in `RealtimeSearch/DLLs/`) gets dependency
  versioning and security updates "for free," and lets it track the same package version as
  MSToolkit's new `RAWReader` (§5, Phase 3).
- `CometWrapper/MSFileReaderWrapper.{h,cpp}` (the COM-based wrapper that nothing currently calls)
  should be deleted once `RAWReader`/COM is retired — keeping an unused bridge to a library being
  removed is exactly the kind of dead code the project's review checklist already flags.

---

## 5. Proposed migration plan

### Phase 0 — Spike (Windows only, no production code changes)
1. Build a throwaway `/clr`-enabled `.cpp` inside a copy of the MSToolkit project (or a minimal
   standalone VS project) and reference the `ThermoFisher.CommonCore.RawFileReader`/`.Data`
   assemblies from a native `.vcxproj` for the first time in this codebase — confirm the
   reference/restore mechanics work the same way they do in `RealtimeSearch.csproj` today.
2. Open a real `.raw` file end to end through `IRawDataPlus`, and confirm marshaling a managed
   `double[]` (from `GetCentroidStream`) into a native buffer works without leaks, mirroring the
   SAFEARRAY-copy pattern already in `RAWReader.cpp` (~lines 724-727).
3. Confirm `/clr`-compiling `RAWReader.cpp` alongside the rest of MSToolkit's plain-native `.cpp`
   files in the same static library builds and links cleanly (MSVC supports per-file `/clr`
   overrides, but this codebase hasn't exercised that combination before).
4. Field-parity check: cross-reference every value `RAWReader::readRawFile()` extracts today
   (scan number, RT, centroid/profile flag, TIC, base peak, precursor isolation m/z, monoisotopic
   m/z from trailer, charge state, MS level — `MSToolkit/src/MSToolkit/RAWReader.cpp` lines
   ~545-727) against the equivalent `IRawDataPlus` calls, using `SearchMS1MS2.cs` as the existing
   working reference for those calls.

### Phase 1 — Rewrite `RAWReader` against RawFileReader .NET
1. Replace the `#import "libid:..."` COM block and `IXRawfile*Ptr` members in
   `MSToolkit/include/RAWReader.h` with an `IRawDataPlus^` member populated via
   `RawFileReaderAdapter::FileFactory()`.
2. Re-implement each extraction point in `MSToolkit/src/MSToolkit/RAWReader.cpp` per the mapping
   table in §3.
3. Keep `RAWReader`'s and `MSReader`'s public method signatures unchanged so
   `CometSearch/CometPreprocess.cpp` requires no changes.

### Phase 2 — Build system changes (Windows only)
1. Enable `/clr` for `RAWReader.cpp` (and any header that doesn't tolerate it, isolating into its
   own translation unit if needed — confirmed feasible in Phase 0).
2. Add `<Reference>` entries for `ThermoFisher.CommonCore.RawFileReader`/`.Data` to the relevant
   `.vcxproj`, replacing the COM `#import` GUID.
3. Update the comment at `MSToolkit/Makefile:138-143` — `RAWReader.cpp` is still excluded from
   Linux/macOS builds, now because `.raw` support is Windows-only by decision, not because COM is
   Windows-only by limitation. No functional Makefile change needed.
4. Drop the MSFileReader COM redistributable from Windows build/install documentation — it's no
   longer a dependency.

### Phase 3 — Modernize `RealtimeSearch.csproj` and delete dead code
1. Replace the `HintPath` DLL references (`RealtimeSearch/RealtimeSearch.csproj:64-75`) with
   NuGet `PackageReference`s to the same packages now referenced from the Phase 2 `.vcxproj`, so
   both consumers track one Thermo library version.
2. Delete `CometWrapper/MSFileReaderWrapper.{h,cpp}` (dead, COM-based, unused).

---

## 6. Summary

| Phase | Outcome | Platform |
|---|---|---|
| 0 | Validated `/clr` + RawFileReader marshaling approach | Windows (spike) |
| 1 | `RAWReader` reimplemented against RawFileReader .NET | Windows |
| 2 | Build system updated; COM redistributable dependency removed | Windows |
| 3 | `RealtimeSearch` on NuGet-based Thermo deps; dead COM wrapper removed | Windows (hygiene only) |

Linux and macOS are unchanged by this migration: `.raw` is not supported; users supply mzML/mzXML
input, optionally pre-converted from `.raw` with an external tool (ThermoRawFileParser,
`msconvert`) outside of Comet.

---

## Appendix — cross-platform options considered, not pursued

An earlier revision of this plan explored making `.raw` reading work on Linux/macOS as well.
Recorded here in case cross-platform `.raw` support becomes a goal again later:

- **Confirmed during that research**: `ThermoFisher.CommonCore.RawFileReader` does ship a
  `netstandard2.0` build and is used cross-platform in production by the open-source
  [ThermoRawFileParser](https://github.com/compomics/ThermoRawFileParser) project (self-contained
  builds for Windows/Linux/macOS on plain .NET 8). The library itself is not what blocks
  cross-platform support — only the native/managed interop mechanism is, since there's no
  COM/C++-CLI equivalent on Linux/macOS.
- **Out-of-process converter** (build a Comet-owned `.raw -> mzML` tool, auto-invoked by
  `MSReader`): rejected even when cross-platform was in scope, because mature external tools
  (ThermoRawFileParser, `msconvert`) already solve this — a user can already convert by hand and
  feed Comet mzML directly.
- **Long-running sidecar process** (a persistent .NET process answering scan requests over local
  IPC — named pipe/Unix domain socket): was the leading cross-platform candidate, since it avoids
  full-file conversion and preserves random-access scan reads. Reintroduce this first if
  cross-platform `.raw` reading is revisited.
- **In-process CoreCLR hosting** via `nethost`/`hostfxr` (a native process loads the CLR itself
  and calls managed code directly, with no separate process): the most complete cross-platform
  answer, but with the most build-system, deployment-footprint (conflicts with Comet's `-static`
  Linux build), and memory-safety overhead of the options considered.
