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

## Cross-platform gotcha: `zconf.h` breaks MSVC after a Linux build

**Symptom:** `MSToolkit.vcxproj` (and `zlibstat.vcxproj`) fail with
`error C1083: Cannot open include file: 'unistd.h': No such file or directory`,
pointing at `MSToolkit\include\zconf.h` or `MSToolkit\extern\zlib-1.2.11\zconf.h`.

**Root cause:** `zconf.h` is a gitignored, generated file. The Linux `make` build
extracts `MSToolkit/extern/zlib1211.zip` and runs zlib's own `./configure` on it,
which writes a *Unix-configured* `zconf.h` — hardcoding `#if 1` immediately before
`#define Z_HAVE_UNISTD_H` — then copies it into `MSToolkit/include/zconf.h` via
`make install`. That hardcoded `#if 1` unconditionally pulls in `<unistd.h>`,
which doesn't exist under MSVC. (The pristine, un-configured form uses
`#ifdef HAVE_UNISTD_H` instead, which only defines it if the build actually sets
`HAVE_UNISTD_H` — never true for an MSVC build.) Building on Windows right after a
Linux `make` (or in a shared/mounted checkout used by both) hits this every time.

**Fix — already built into the project, just needs to be invoked:**
`zlibstat.vcxproj` has a `CleanZlib` target (`AfterTargets="Clean"`) that removes
the extracted `MSToolkit/extern/zlib-1.2.11/` directory and the two copied headers
(`include/zconf.h`, `include/zlib.h`). Once those are gone, the next Build
re-extracts a pristine `zlib1211.zip` and MSVC falls back to the Windows-safe
copies already checked into `MSToolkit/include/extern/`.

**Validated workflow — Clean *then* Build (a plain Build alone will still fail):**
```bash
"/mnt/c/Program Files/Microsoft Visual Studio/2022/Community/MSBuild/Current/Bin/MSBuild.exe" \
  "/mnt/c/Work/Comet-master/Comet.sln" /t:Clean /p:Configuration=Release /p:Platform=x64 /v:minimal

"/mnt/c/Program Files/Microsoft Visual Studio/2022/Community/MSBuild/Current/Bin/MSBuild.exe" \
  "/mnt/c/Work/Comet-master/Comet.sln" /p:Configuration=Release /p:Platform=x64 /v:minimal
```
(In Visual Studio: **Build → Clean Solution**, then **Build → Build Solution**.)
Confirmed this produces a fully clean build of `CometWrapper.dll` and
`RealtimeSearch.exe` with zero errors, with no manual file edits.

**Unix-side equivalent:** the top-level `make clean` (not `make cclean` — `cclean`
only touches `CometSearch/` and never removes `MSToolkit/include/zconf.h` or the
extracted zlib tree) removes the same files via `MSToolkit`'s own `zlib-realclean`
target. Confirmed a subsequent `make` on Linux correctly regenerates a
Linux-appropriate (Unix-configured) `zconf.h` again — the clean/rebuild cycle is
safe in both directions and doesn't require picking one platform to "win."

**Do not** manually copy `zconf.h.in` over `zconf.h` as a workaround — it works,
but it's unnecessary and the Clean-then-Build cycle above is what the project
already expects (see the comment on `CleanZlib` in
`MSToolkit/VisualStudio/extern/zlibstat.vcxproj`).

If you see a parallel (`/m`) MSBuild run fail with `LNK1104` on `zlibstat.vcxproj`
(`cannot open file 'MSToolkitExtern.lib'`) even with a clean `zconf.h`, that's an
unrelated output-path race between parallel projects — drop `/m` (serial build)
and it resolves.
