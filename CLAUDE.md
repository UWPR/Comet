# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## What Is Comet

Comet is an open-source tandem mass spectrometry (MS/MS) sequence database search engine written in C/C++. It searches experimental MS/MS spectra against protein sequence databases to identify peptides.

## Git Workflow

**Do NOT run `git commit` or `git push` unless the user explicitly asks.**

Make code changes, build, and run tests; then stop and report results.
Wait for the user to say "commit" or "commit and push" before touching git history.

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
- Right-click the **Comet** project -> **Build**
- Output: `x64/Release/Comet.exe`

`.raw` file reading uses Thermo's RawFileReader .NET library via a `/clr` (C++/CLI) build in
`MSToolkit` -- no separate Thermo software installation is required (Windows only).

**If the build fails with `error C1083: Cannot open include file: 'unistd.h'`** (usually
in `MSToolkit.vcxproj` or `zlibstat.vcxproj`): this repo was just built with Linux `make`,
which regenerates `MSToolkit/include/zconf.h` in a Unix-configured form that breaks MSVC.
Run **Clean Solution**, then **Build Solution** again -- see the `comet-build` skill for
the full explanation and the Unix-side equivalent (`make clean`, not `make cclean`).

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
   - `CometSearchManager` -- implements `ICometSearchManager`; top-level orchestrator
   - `CometSearch` -- fragment index querying, XCorr scoring, peptide matching
   - `CometPreprocess` -- spectrum preprocessing (binning, noise reduction)
   - `CometPostAnalysis` -- SP score, E-value, delta-Cn, AScorePro localization
   - `CometFragmentIndex` / `CometPeptideIndex` -- index building and lookup
   - `CometSpecLib` -- MS1 spectral library loading and search
   - `CometAlignment` -- MS1 RT alignment

2. **C++/CLI wrapper** (`CometWrapper/`): `CometSearchManagerWrapper` (ref class) marshals data between managed C# and native C++. `CometDataWrapper.h` defines managed wrapper types (`ScoreWrapper`, `FragmentWrapper`, etc.).

3. **C# application** (`RealtimeSearch/`): `SearchMS1MS2.cs` drives concurrent real-time searches by launching parallel C# `Task` threads that call into the wrapper.

### Key Globals (CometSearch)

| Global | Thread-safe? | Notes |
|--------|-------------|-------|
| `g_staticParams` | [x] Read-only after init | All search parameters |
| `g_iFragmentIndex`, `g_vFragmentPeptides`, `g_vRawPeptides` | [x] Read-only after init | Fragment index |
| `g_pvProteinNames`, `g_pvProteinsList` | [x] Read-only after init | |
| `g_vSpecLib` | [x] Read-only after init | MS1 spectral library |
| `g_pvQuery` | [ ] Shared mutable | Batch search path only |
| `g_pvQueryMS1` | [ ] Shared mutable | Batch MS1 path only |
| `g_cometStatus` | [ ] Shared mutable | Error reporting |

### Threading Model (RTS path)

The real-time search (`DoSingleSpectrumSearchMultiResults` and `DoMS1SearchMultiResults`) is designed for concurrent calls from C# Task threads:

- **MS2 RTS**: `PreprocessSingleSpectrumThreadLocal()` creates a caller-owned `Query*`; `CometSearch::RunSearch(Query*, time_point)` searches against the read-only fragment index; thread-local `CalculateSP/CalculateEValue/CalculateDeltaCn(Query*)` do post-analysis. No `g_pvQuery` access.
- **MS1 RTS**: `PreprocessMS1SingleSpectrumThreadLocal()` creates a caller-owned `QueryMS1*`; `RunMS1Search(QueryMS1*, ...)` scores against read-only `g_vSpecLib`. No `g_pvQueryMS1` access. Reference library is loaded once in `InitializeSingleSpectrumMS1Search()`.
- **Batch search**: Still uses `g_pvQuery` / `g_pvQueryMS1` with the original mutex-guarded path.

## Testing

### Unit and Integration Tests

Tests live in `tests/unit/`. The runner is `run_tests.py`.

```bash
# Run all unit tests (T1-T7, T11-T16) -- fast, no large data required
python tests/unit/run_tests.py --comet /mnt/c/Work/Comet-master/comet.exe

# Run a specific test by ID
python tests/unit/run_tests.py --comet /mnt/c/Work/Comet-master/comet.exe t13

# Run unit + integration tests (T17, T18) -- requires data/human.small.fasta
python tests/unit/run_tests.py --comet /mnt/c/Work/Comet-master/comet.exe --integration
```

Always pass `--comet` as a full path; the default `../../comet.exe` only works when
invoked from inside `tests/unit/`.

### Test Data

Small crafted FASTA files for T1-T16 live in `tests/unit/data/`. Pre-built `.idx`
reference files are committed alongside them for byte-exact comparison tests.

Integration tests (T17, T18) require `data/human.small.fasta` (not in repo -- must
be present manually before running `--integration`).

### Key Design Decisions in the Test Suite

- **`no-enzyme + len_max > 13` will time out.** No-enzyme with `len_max=25` generates
  a ~1.1 GB index and takes >300 s. Use `len_max=13` for integration tests; it covers
  both the short path (len <= 12, 5-bit packed) and the long path (len > 12, plain
  string) while building in ~110 s.

- **T17 uses count-stability, not cross-version byte comparison.** The v2026.01.1
  baseline has a known I/L long-path dedup bug (uses byte-exact `memcmp` instead of
  canonical L==I comparison), producing ~8,102 extra entries when `equal_IL=1`. Even
  with `equal_IL=0` there is an 8-peptide algorithmic difference from the flat-sort
  vs per-length sort change. Cross-version byte-exact or count-exact comparison is
  therefore unreliable; T17 verifies that the peptide count falls in [8,800,000,
  9,100,000] for a no-enzyme len 8-13 build on human.small.fasta.

- **T18** verifies determinism: two independent builds of the same FASTA produce
  byte-identical `.idx` files.

### compare_idx.py

`tests/unit/compare_idx.py` structurally compares two plain-peptide `.idx` files.
It checks header fields (peptide count, protein-list count, mass range) and then
streams both files in parallel to compare every peptide entry. Aborts early if
peptide counts differ. Useful for debugging index changes.

```bash
python tests/unit/compare_idx.py old.idx new.idx
```

## Benchmarking and FDR Analysis

### tools/qvalue.py

`tools/qvalue.py` computes q-values (FDR) from Comet tab-delimited output files for
benchmarking search result quality using rank-1 PSMs only

Each run always reports results for both xcorr (descending) and e-value (ascending)
sorting side by side.

```bash
# Single file:
python tools/qvalue.py results.txt

# Compare two files side-by-side:
python tools/qvalue.py results_a.txt results_b.txt

# Also diff the specific passing PSMs between two files (shown per scoring method):
python tools/qvalue.py --diff results_a.txt results_b.txt

# Custom q-value threshold(s):
python tools/qvalue.py --threshold 0.01 --threshold 0.05 results.txt
```

Output columns per file: q-value threshold | xcorr PSMs | xcorr cutoff | evalue PSMs | evalue cutoff.
When multiple files are given, a summary table follows with all counts side by side.
When `--diff` is used with two files, unique PSMs are listed for each scoring method
separately, showing scan, charge, score, and modified peptide sequence.

FDR formula:
- Standard TDA: `FDR(i) = n_decoy(i) / n_target(i)`, no +1 correction, no 2x scaling
- `q(i) = running minimum FDR from position i to the end`
- Decoys identified by protein column starting with `DECOY_` or `rev_` (case-insensitive)

## Coding Style

From `docs/CometCodingStyleGuidelines.md`:

- **Allman brace style**: opening brace on its own line at the same indentation as the control structure
- **3 spaces** per indentation level (no tabs)
- **Windows-style line endings (`\r\n`) — MANDATORY for every file in this repo.**
  See the enforcement rules below.
- Use `//` for inline comments (reserve `/* */` for commenting out blocks)
- **Systems Hungarian Notation** for variable names (e.g., `iCount`, `dMass`, `szName`, `bFlag`, `p` prefix for pointers)
- No trailing whitespace
- No non-ASCII characters allowed in the code or documentation

### Line-ending enforcement (CRLF)

**Every source file — `.cpp`, `.h`, `.c`, `.cs`, `.py`, `.md`, `.txt`, `.params` — must
use Windows CRLF (`\r\n`) line endings.  Unix LF (`\n`) is wrong for this repo.**

Rules for Claude Code:

1. **Editing existing files** (`Edit` tool): the tool preserves the file's existing line
   endings, so edits to a CRLF file stay CRLF automatically.  No special action needed.

2. **Writing a new file or fully replacing one** (`Write` tool): the content string passed
   to `Write` must contain `\r\n` at every line break.  Plain `\n` produces a Unix-LF
   file.  **Always verify after writing:**
   ```bash
   file <path>   # must show "CRLF line terminators"
   ```
   If the output shows only "ASCII text" (no CRLF mention), the file has Unix LF —
   re-write it with correct line endings before proceeding.

3. **After any session that creates or modifies files**, run a quick sanity check on the
   touched files:
   ```bash
   file CometSearch/*.h CometSearch/*.cpp | grep -v CRLF
   ```
   Any line printed is a file with wrong line endings — fix it with `unix2dos <file>`.

A `.gitattributes` file at the repo root enforces CRLF for all tracked source files
at the git level, providing a second safety net.


## Development Workflows

### Code Review Protocol (Copilot Mode)
When requested to perform a code review, always execute the following multi-step workflow before writing your feedback:
1. **Tooling Check:** Run the project's respective testing commands to gather concrete diagnostic data.
2. **Analysis:** Review the uncommitted files, staged changes, or the specified branch diff against every category in the
   **Bug Category Checklist** below. Walk every changed hunk through the full list -- don't skip a category just because
   it seems unlikely; confirm it doesn't apply rather than omitting it silently.
3. **Report Generation:** Structure the review using the exact template below.

#### Bug Category Checklist
- **Bounds & UB:** pointer arithmetic, array indexing, or string suffix/substring checks performed without first
  validating length (e.g. `buf + len - N` when `len < N` is possible).
- **Resource & memory safety:** leaks, double-free, use-after-free, missing RAII, unchecked `new`/`malloc` failures.
- **Concurrency:** data races, missing locks/`lock_guard`s, thread-unsafe access to globals marked "Shared mutable" in
  the Key Globals table above.
- **Dead/unreachable code:** unused functions, stub overloads that silently no-op, unreachable branches -- flag these
  as future-refactor risk even when currently harmless (e.g. an unused overload that could resolve incorrectly later).
- **Performance/memory efficiency:** loading an entire file/buffer into memory where streaming would suffice, O(n^2)
  patterns, unnecessary copies in hot paths.
- **Error handling:** unchecked return values, swallowed exceptions, missing validation at system boundaries (file
  I/O, user params, external APIs).
- **API contract changes:** signature or default-value changes that could silently break existing callers.
- **Test coverage:** new logic paths or edge cases introduced without a corresponding test.

## Code Review Template
Provide feedback using this exact format:
1. **Summary:** A 1-2 sentence overview of the changes.
2. **Critical Issues:** Bugs, security vulnerabilities, or breaking changes. Provide the file path, exact line numbers, and the core issue.
3. **Code Quality & Maintainability:** Poor practices, anti-patterns, or missing tests.
4. **Actionable Improvements:** Specific refactoring suggestions accompanied by concise code snippets.

*Constraint:* Keep critiques technical, objective, and ranked by severity. Avoid generic praise.

