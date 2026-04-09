# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## What Is Comet

Comet is an open-source tandem mass spectrometry (MS/MS) sequence database search engine written in C/C++. It searches experimental MS/MS spectra against protein sequence databases to identify peptides.

## Build Commands

### Linux / macOS
```bash
make          # Full build (MSToolkit + AScorePro + CometSearch + Comet.exe)
make clean    # Full clean including MSToolkit and AScorePro
make cclean   # Quick clean: only CometSearch and root object files
```

### Windows (Visual Studio)
- Load `Comet.sln` in Visual Studio 2022 (build tools v143)
- Set configuration to **Release / x64**
- Right-click the **Comet** project → **Build**
- Output: `x64/Release/Comet.exe`

The build requires Thermo's MSFileReader to be installed first (Windows only).

### CometSearch library only
```bash
cd CometSearch && make
```

## Repository Structure

```
Comet/                      # Top-level: main entry point (Comet.cpp), solution files
CometSearch/                # Core C++ search library (compiled to libcometsearch.a)
CometWrapper/               # C++/CLI managed wrapper (CometWrapper.dll) bridging C++ to C#
RealtimeSearch/             # C# application layer for real-time (RTS) searches
AScorePro/                  # AScore phosphosite localization library (git submodule-like)
MSToolkit/                  # Mass spec file format reader library (Mike Hoopmann)
extern/                     # Third-party dependencies (expat, zlib)
docs/                       # Architecture docs, coding style, threading design records
```

## Architecture

The codebase has three layers:

1. **Native C++ core** (`CometSearch/`): The search engine library. Key classes:
   - `CometSearchManager` — implements `ICometSearchManager`; top-level orchestrator
   - `CometSearch` — fragment index querying, XCorr scoring, peptide matching
   - `CometPreprocess` — spectrum preprocessing (binning, noise reduction)
   - `CometPostAnalysis` — SP score, E-value, delta-Cn, AScorePro localization
   - `CometFragmentIndex` / `CometPeptideIndex` — index building and lookup
   - `CometSpecLib` — MS1 spectral library loading and search
   - `CometAlignment` — MS1 RT alignment

2. **C++/CLI wrapper** (`CometWrapper/`): `CometSearchManagerWrapper` (ref class) marshals data between managed C# and native C++. `CometDataWrapper.h` defines managed wrapper types (`ScoreWrapper`, `FragmentWrapper`, etc.).

3. **C# application** (`RealtimeSearch/`): `SearchMS1MS2.cs` drives concurrent real-time searches by launching parallel C# `Task` threads that call into the wrapper.

### Key Globals (CometSearch)

| Global | Thread-safe? | Notes |
|--------|-------------|-------|
| `g_staticParams` | ✅ Read-only after init | All search parameters |
| `g_iFragmentIndex`, `g_vFragmentPeptides`, `g_vRawPeptides` | ✅ Read-only after init | Fragment index |
| `g_pvProteinNames`, `g_pvProteinsList` | ✅ Read-only after init | |
| `g_vSpecLib` | ✅ Read-only after init | MS1 spectral library |
| `g_pvQuery` | ❌ Shared mutable | Batch search path only |
| `g_pvQueryMS1` | ❌ Shared mutable | Batch MS1 path only |
| `g_cometStatus` | ❌ Shared mutable | Error reporting |

### Threading Model (RTS path)

The real-time search (`DoSingleSpectrumSearchMultiResults` and `DoMS1SearchMultiResults`) is designed for concurrent calls from C# Task threads:

- **MS2 RTS**: `PreprocessSingleSpectrumThreadLocal()` creates a caller-owned `Query*`; `CometSearch::RunSearch(Query*, time_point)` searches against the read-only fragment index; thread-local `CalculateSP/CalculateEValue/CalculateDeltaCn(Query*)` do post-analysis. No `g_pvQuery` access.
- **MS1 RTS**: `PreprocessMS1SingleSpectrumThreadLocal()` creates a caller-owned `QueryMS1*`; `RunMS1Search(QueryMS1*, ...)` scores against read-only `g_vSpecLib`. No `g_pvQueryMS1` access. Reference library is loaded once in `InitializeSingleSpectrumMS1Search()`.
- **Batch search**: Still uses `g_pvQuery` / `g_pvQueryMS1` with the original mutex-guarded path.

## Coding Style

From `docs/CometCodingStyleGuidelines.md`:

- **Allman brace style**: opening brace on its own line at the same indentation as the control structure
- **3 spaces** per indentation level (no tabs)
- **Windows-style line endings** (`\r\n`)
- Use `//` for inline comments (reserve `/* */` for commenting out blocks)
- **Systems Hungarian Notation** for variable names (e.g., `iCount`, `dMass`, `szName`, `bFlag`, `p` prefix for pointers)
- No trailing whitespace
