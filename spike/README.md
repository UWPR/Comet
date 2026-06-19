# Phase 0 spikes (RawFileReader migration)

Throwaway validation code for Phase 0 of `docs/20260618_RawFileReaderMigration.md`. Not part of
any production build target (not referenced from `Comet.sln`). Findings are summarized in the
migration doc; this README just covers how to rebuild/rerun them.

## LegacyRawReaderSpike

Plain native console app (no `/clr`) linking the existing `MSToolkit.lib`/`MSToolkitExtern.lib`
(built via `Comet.sln`) to read a `.raw` file through the current COM-based `RAWReader`/`MSReader`
path, printing the same fields `RawFileReaderSpike` prints, for side-by-side diffing.

```
cd spike/LegacyRawReaderSpike
build.bat
LegacyRawReaderSpike.exe <path-to-raw> [scanNum ...]
```

Requires `x64\Release\MSToolkit.lib`/`MSToolkitExtern.lib` to already exist (build `Comet.sln`
first) and MSFileReader COM to be installed/registered on the machine.

## RawFileReaderSpike

`/clr`-enabled native console app exercising `ThermoFisher.CommonCore.RawFileReader` directly
(`IRawDataPlus`), mixed in one project with a plain-native translation unit (`NativeUtils.cpp`,
`CompileAsManaged=false`) to validate per-file `/clr` overrides.

```
cd spike/RawFileReaderSpike
"<path-to-MSBuild.exe>" RawFileReaderSpike.vcxproj /p:Configuration=Release /p:Platform=x64
```

`PackageReference` restore does **not** work for this project -- see the comment in
`RawFileReaderSpike.vcxproj` -- so it references the managed DLLs directly via `HintPath` against
the global NuGet cache (`%userprofile%\.nuget\packages\thermofisher.commoncore.*\5.0.0.93\...`),
which must already be populated by restoring `RealtimeSearch.csproj` at least once. After
building, copy `ThermoFisher.CommonCore.*.dll` and `OpenMcdf.dll` from that cache next to the
built `.exe` (no copy-local mechanism without `PackageReference`).

```
RawFileReaderSpike.exe <path-to-raw> [scanNum ...]
RawFileReaderSpike.exe <path-to-raw> --stress [samescan]
```

`--stress` loops 130000 scan reads + managed-to-native marshals, printing managed heap size and
process working set every 10000 reps, to check for leaks. `--stress samescan` repeats one scan
number instead of cycling through new ones, to isolate per-scan-visited cache growth from a true
per-read leak.
