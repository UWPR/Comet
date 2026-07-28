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
deps (`OpenMcdf`, `System.IO.FileSystem.AccessControl`), and this package version is typically
already warm in `~/.nuget/packages/thermofisher.commoncore.*`.

To read a `.raw` file (e.g. to extract spectra for a batch-vs-RTS comparison test), write a
small **pure C# console project** (SDK-style `.csproj`, e.g. `TargetFramework=net8.0`) that
`PackageReference`s `ThermoFisher.CommonCore.RawFileReader`/`.Data` and calls
`RawFileReaderAdapter.FileFactory(path)` directly -- mirror the scan-reading logic in
`RealtimeSearch/SearchMS1MS2.cs` (`GetCentroidStream`, `GetScanEventForScanNumber(...).GetReaction(0).PrecursorMass`,
`GetTrailerExtraInformation` for `"Monoisotopic M/Z:"`/`"Charge State:"`) for field-accurate
results. This bypasses the Windows-only C++/CLI `CometWrapper` bridge entirely (that bridge is
what actually keeps `RealtimeSearch.exe` Windows-only, not the RawFileReader library itself), so
`dotnet build`/`dotnet run` works unmodified on Linux. This only gets you spectra out of the
file for tooling/testing purposes -- it does not make `comet.exe` or `RealtimeSearch.exe`
themselves able to read `.raw`; that remains Windows-only.
