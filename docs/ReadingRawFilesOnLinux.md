# Reading `.raw` Files on Linux (for test/data-extraction purposes)

`comet.exe` itself cannot open `.raw` on Linux -- `.raw` support is Windows-only by design
(`docs/20260618_RawFileReaderMigration.md`), and this environment's `msconvert` is a non-MSVC
build with vendor DLLs disabled (`no Thermo, Bruker, Waters etc input`), so it cannot convert
`.raw` -> `.mzXML` either. Do not conclude `.raw` files are simply unreadable here, though --
**`dotnet`/`msbuild` is available and can read them directly.**

Thermo's `ThermoFisher.CommonCore.RawFileReader` package targets `netstandard2.0` and is
genuinely cross-platform (confirmed cross-platform in production by the open-source
`ThermoRawFileParser` project; see the Appendix of the migration doc above). The repo already
has everything needed to restore it on Linux: a root `nuget.config` with a local
`ThermoFisher-local` feed (`RealtimeSearch/ThermoNuGet/*.nupkg`) plus `nuget.org` for transitive
deps (`OpenMcdf`, `System.IO.FileSystem.AccessControl`). That `nuget.config` also sets
`globalPackagesFolder` to the repo-local, gitignored `packages/` folder (the CI runner runs as
LocalSystem, whose per-user cache the C# compiler cannot reliably read), so a project inside the
repo restores into `<repo>/packages/` and not `~/.nuget/packages/`. A project outside the repo does
not see this `nuget.config` and falls back to `~/.nuget/packages/thermofisher.commoncore.*`, where
this package version (5.0.0.93) is typically already present.

To read a `.raw` file (e.g. to extract spectra for a batch-vs-RTS comparison test), write a
small **pure C# console project** (SDK-style `.csproj`, e.g. `TargetFramework=net8.0`) that
`PackageReference`s `ThermoFisher.CommonCore.RawFileReader`/`.Data` and calls
`RawFileReaderAdapter.FileFactory(path)` directly -- mirror the scan-reading logic in
`RealtimeSearch/SearchMS1MS2.cs` (`GetCentroidStream`, `GetScanEventForScanNumber(...).GetReaction(0).PrecursorMass`,
`GetTrailerExtraInformation` for `"Monoisotopic M/Z:"`/`"Charge State:"`) for field-accurate
results. That is the RTS reader. The batch reader -- `MSToolkit/src/MSToolkit/RAWReader.cpp`, the
C++/CLI file `comet.exe` uses on Windows -- takes its peaks from `Scan.FromFile(...)` with
`PreferCentroids = true` instead (the instrument's centroid stream when it has one, the profile
scan otherwise), skips scans outside the requested MS level before reading any peaks, and also
reads the `"Master Scan Number:"` and `"SPS Masses:"` trailer fields. Mirror `RAWReader.cpp` if the
extracted spectra must match what a batch search sees, `SearchMS1MS2.cs` if they must match RTS.
This bypasses the Windows-only C++/CLI `CometWrapper` bridge entirely (the C++/CLI wrappers --
`CometWrapper` for .NET Framework 4.7.2 and `CometWrapperCore` for net8.0, both built from the same
`CometWrapper.cpp` -- are what actually keep `RealtimeSearch.exe` Windows-only, not the
RawFileReader library itself), so
`dotnet build`/`dotnet run` works unmodified on Linux. This only gets you spectra out of the
file for tooling/testing purposes -- it does not make `comet.exe` or `RealtimeSearch.exe`
themselves able to read `.raw`; that remains Windows-only.

If the goal is to *search* a `.raw` file with Comet rather than to extract spectra, the Windows build
can be run straight from this WSL shell through Windows interop, the same way `MSBuild.exe` can (see
the `comet-build` skill): execute `/mnt/c/.../x64/Release/Comet.exe -Pcomet.params file.raw` from a
working directory under `/mnt/c`. It is a Windows process, so every path it opens -- the input
`.raw`, `database_name`, the `.idx` -- must be one Windows can resolve (a relative path from that
working directory, or `C:\...`), not a WSL-only path such as one under `/tmp`. The Linux
`comet.exe` still cannot open `.raw`.
