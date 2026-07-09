---
name: comet-build
description: Build the Comet mass spectrometry search engine from source. Use when building, rebuilding, or diagnosing compile/link errors in the Comet project (Comet.sln).
---

# Comet Build

## Full solution build (Release x64)

```powershell
& "C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\amd64\MSBuild.exe" `
  "C:\Work\Comet-master\Comet.sln" /p:Configuration=Release /p:Platform=x64 /m /nologo /v:minimal
```

Show only errors and C4-level warnings:
```powershell
... 2>&1 | Where-Object { $_ -match "error|warning C4" }
```

## Building from WSL (Bash tool)

`MSBuild.exe` is directly invocable from the Bash tool in a WSL session on this machine (no
`powershell.exe`/`cmd.exe` wrapper needed) — Windows interop lets WSL bash exec `.exe` paths
under `/mnt/c/...` directly:

```bash
"/mnt/c/Program Files/Microsoft Visual Studio/2022/Community/MSBuild/Current/Bin/amd64/MSBuild.exe" \
  "/mnt/c/Work/Comet-master/Comet.sln" /p:Configuration=Release /p:Platform=x64 /m /nologo /v:minimal
```

Use forward-slash `/mnt/c/...` paths (not `C:\...`) when invoking from bash. This means a WSL-based
Claude Code session can build and verify the Windows/VS side of a change directly, not just the
Linux `make` build — don't assume Windows-only code (e.g. `/clr` files, `CometWrapper`,
`RealtimeSearch`) is unverifiable just because the working shell is bash.

## After building: copy the wrapper DLL

CometWrapper.dll must be manually copied after each build for RealtimeSearch.exe to pick it up:
```powershell
Copy-Item "C:\Work\Comet-master\x64\Release\CometWrapper.dll" `
          "C:\Work\Comet-master\RealtimeSearch\bin\x64\Release\CometWrapper.dll" -Force
```

## Build targets and outputs

| Project | Output |
|---------|--------|
| MSToolkit | `x64\Release\MSToolkit.lib` |
| CometSearch | `x64\Release\CometSearch.lib` |
| CometWrapper | `x64\Release\CometWrapper.dll` |
| Comet | `x64\Release\Comet.exe` |
| RealtimeSearch | `RealtimeSearch\bin\x64\Release\RealtimeSearch.exe` |

To rebuild only CometSearch + CometWrapper (fastest for C++ changes):
```powershell
... /t:CometSearch`;CometWrapper
```

## RTS_TIMING instrumentation flag

Location: `CometSearch\CometSearch.vcxproj`, Release config `<PreprocessorDefinitions>`.

- `RTS_TIMING_OFF` — production build (default)
- `RTS_TIMING` — enables per-spectrum `printf("TIMING\t...")` in `DoSingleSpectrumSearchMultiResults`

**Always disable before benchmarking.** With 40k spectra and 20 threads, the stdout contention from printf adds ~8 seconds to the search time and makes Hz numbers meaningless for comparison.

## C# language version

`RealtimeSearch\RealtimeSearch.csproj` targets .NET Framework 4.7.2. Both Debug|x64 and Release|x64 configurations have `<LangVersion>7.3</LangVersion>`. Do not use C# 8.0+ syntax (e.g., `using` declarations without braces) without bumping this.

## Linux / macOS build

```bash
make          # full build
make cclean   # clean CometSearch only (fast)
make clean    # full clean
```
