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

### Windows (Visual Studio / MSBuild)

**`MSBuild.exe` is directly runnable from a WSL Bash session on this machine** --
Windows interop lets bash exec `.exe` paths under `/mnt/c/...` directly, no
`powershell.exe`/`cmd.exe` wrapper needed. Do not assume the Windows/C# side of this repo
(`RealtimeSearch`, `CometWrapper`, any `/clr` file) is unbuildable or untestable just
because the current shell is bash -- **check the `comet-build` skill and try MSBuild
before ever telling the user a Windows-only change "can't be verified in this
session."** The skill has the exact invocation, the post-build wrapper-DLL copy step,
and the `zconf.h` / `error C1083: unistd.h` cross-platform gotcha (Clean Solution +
Build Solution on Windows, or `make clean` -- not `cclean` -- on Linux).

In Visual Studio itself (2022, build tools v143): load `Comet.sln`, set configuration to
**Release / x64**, right-click the **Comet** project -> **Build**. Output:
`x64/Release/Comet.exe`.

`.raw` file reading uses Thermo's RawFileReader .NET library via a `/clr` (C++/CLI) build in
`MSToolkit` -- no separate Thermo software installation is required (Windows only). This is
a *runtime data-file* limitation (no `.raw` reader on Linux), not a build-capability one --
it doesn't mean the Windows/C# code itself can't be compiled from a WSL session; see above.

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
| `g_iFragmentIndex`, `g_fragmentPeptides`, `g_vRawPeptides` | [x] Read-only after init | Fragment index. With `decoy_search != 0` (the `.idx` header's `DecoySearch:` value wins at load) every target variant in `g_fragmentPeptides` has a pseudo-reverse decoy twin flagged by `VariantArray::DECOY_FLAG` (bit 7 of `vucFlags`; `IsDecoy()`), sharing its raw-peptide row, mods, mass key and protein list -- `SearchFragmentIndex()` reverses the sequence + mod sites on the fly for flagged candidates. See `docs/20260914_FI_internal_decoys.md`. Terminal variable mods (`n`/`c` any peptide terminus, `^`/`$` protein terminus only) are permuted by `ModificationsPermuter` like residue mods and live in bytes 0/1 of the `MOD_NUMBERS_POOL` entry (`ModEntryTermSlot()`), not in the variant array -- see `docs/20260915_permuter_terminal_mods.md`. |
| `g_iTermSlotBytes`, `MOD_NUMBERS_POOL` and the other `MOD_*` tables | [x] Read-only after init | Mod-permutation tables built once per session by `PermuteIndexPeptideMods()`. `g_iTermSlotBytes` is 2 when any terminal variable mod is active (every pool entry then starts with the N-/C-term slot bytes and every modifiable sequence with two sentinel positions), else 0 -- consumers walk residues from `ModEntryResidueOffset()`. |
| `g_pvProteinsList` | [x] Read-only after init | Populated on both a fresh build and a search-only read-back of an existing `.idx`. Each protein occurrence carries `PROT_NTERM_HERE` / `PROT_CTERM_HERE` / `PROT_BOTH_TERM_HERE` context bits (`Row::flags(j)`, checked through `ProteinsListCSR::flagsSatisfy()`) used to place and attribute protein-scoped terminal variable mods (`^` / `$`) exactly -- see `docs/20260915_permuter_terminal_mods.md` section 11. |
| `g_pvProteinNames` | [ ] Build-time only | **Not** search-time readable: populated only while *building* a `.idx`, never repopulated when an existing `.idx` is read back for a search -- reading it during a search-only run silently finds nothing (this exact confusion caused a real bug, decoy peptides misclassified as targets in every PI_DB search of a pre-built index). Use `g_pvProteinNameCache` for search-time protein-name lookups instead. |
| `g_pvProteinNameCache` | [x] Read-only after index load | Protein-name strings indexed by name-section ordinal (`vector<string>`; every protein, read sequentially at index load) -- the search-time-safe counterpart to `g_pvProteinNames` above. `g_pvProteinsList` rows hold matching ordinals after a load (FASTA offsets during a build). |
| `g_bIndexPrecursors` | [x] Read-only after init | Per-mass-bin bool array sized `BIN(dPeptideMassHigh) + 1`; freed and nulled by `FiStrategy::finalize()` at the end of a search |
| `g_vSpecLib` | [x] Read-only after init | MS1 spectral library |
| `SearchSession::queries` | [ ] Shared mutable | Batch search path only; guarded by `SearchSession::queriesMutex`. Formerly the bare global `g_pvQuery` -- moved into `SearchSession` by the OOP architecture migration (`docs/20260612_architecture_migration.md`); `core/Types.h` still has a comment noting the move, but plenty of code comments across the tree still say `g_pvQuery` informally. |
| `SearchSession::ms1Queries` | [ ] Shared mutable | Batch MS1 path only; same `queriesMutex`. Formerly the bare global `g_pvQueryMS1`, same migration as above. |
| `g_cometStatus` | [ ] Shared mutable | Error reporting |

### Threading Model (RTS path)

The real-time search (`DoSingleSpectrumSearchMultiResults` and `DoMS1SearchMultiResults`) is designed for concurrent calls from C# Task threads:

- **MS2 RTS**: `PreprocessSingleSpectrumThreadLocal()` creates a caller-owned `Query*`; `CometSearch::RunSearch(Query*, time_point)` searches against the read-only fragment index; thread-local `CalculateSP/CalculateEValue/CalculateDeltaCn(Query*)` do post-analysis. No `SearchSession::queries` access.
- **MS1 RTS**: `PreprocessMS1SingleSpectrumThreadLocal()` creates a caller-owned `QueryMS1*`; `RunMS1Search(QueryMS1*, ...)` scores against read-only `g_vSpecLib`. No `SearchSession::ms1Queries` access. Reference library is loaded once in `InitializeSingleSpectrumMS1Search()`.
- **Batch search**: Still uses `SearchSession::queries` / `ms1Queries` with the original mutex-guarded path.

For a file-by-file ownership map, the full global-variable table, and RTS/batch call-path
diagrams, use the `comet-codebase` skill.

## Testing

### Unit and Integration Tests

Tests live in `tests/unit/`; the runner is `run_tests.py`. **Per-test summaries, the T21
legacy case table, the T23/T24 big-data methodology, the C++ `CometUnitTests`, and the
`tests/regression/`, `tests/perf/` and `tests/rts_repro/` suites are all documented in
`tests/tests.md` -- keep that file, not this one, current when adding or changing tests.**

```bash
# All unit tests -- fast, no large data required
python tests/unit/run_tests.py --comet /mnt/c/Work/Comet-master/comet.exe

# One test by ID
python tests/unit/run_tests.py --comet /mnt/c/Work/Comet-master/comet.exe t13

# Linux and Windows builds in one invocation (--comet is repeatable)
python tests/unit/run_tests.py \
  --comet /mnt/c/Work/Comet-master/comet.exe \
  --comet /mnt/c/Work/Comet-master/x64/Release/Comet.exe

# Unit + integration tests (the INTEGRATION_TESTS tuple in run_tests.py)
python tests/unit/run_tests.py --comet /mnt/c/Work/Comet-master/comet.exe --integration
```

Rules and gotchas:

- Always pass `--comet` as a full path; the default `../../comet.exe` only works when
  invoked from inside `tests/unit/`.
- `--integration` tests need `tests/unit/data/human.small.fasta` (not in the repo) and/or
  `--bigdata DIR` (default: the sibling `20130226-comet-tests/` directory; that data is
  never copied into the repo). They skip cleanly, without failing, when the data or the
  baseline binary is absent.
- T23/T24 also compare against a pinned previous release (`BASELINE_TAG` in
  `run_tests.py`, currently `v2026.02.2`), auto-downloaded on first use into
  `tests/regression/baselines/<tag>/` (gitignored; override with `--baseline PATH`).
- A single `_check_timing` failure (current build more than `TIMING_NOISE_TOLERANCE`, 25%,
  slower than the baseline) means "re-run to confirm", not a regression: these are
  single-sample, multi-minute wall-clock measurements and machine noise alone can reach
  10-20%. A *repeated* failure is the real signal.
- `no-enzyme + len_max > 13` will time out (a ~1.1 GB index, >300 s). Use `len_max=13`
  for integration tests; it still covers both the short (<= 12, 5-bit packed) and long
  (> 12, plain string) index paths and builds in ~110 s.
- Fixtures live in `tests/unit/data/`: crafted FASTAs, `.ms2`/`.mzXML`/`.msp` spectra,
  committed reference `.idx` files for byte-exact comparisons, and `legacy/` for T21.
  `tests/unit/compare_idx.py` structurally diffs two `.idx` files when an index change
  needs debugging.

### Reading `.raw` files on Linux (for test/data-extraction purposes)

`comet.exe` cannot open `.raw` on Linux and `msconvert` here can't convert it either, but
`dotnet`/`msbuild` can read Thermo `.raw` files directly via the cross-platform
`ThermoFisher.CommonCore.RawFileReader` package -- see `docs/ReadingRawFilesOnLinux.md`
for the full explanation and a working approach.

## Benchmarking and FDR Analysis

For search-speed benchmarking (batch vs. RTS throughput, Hz, ms/spectrum), use the
`comet-benchmark` skill instead -- this section covers result-quality (FDR) analysis only.

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

Full conventions (brace style, indentation, comments, Hungarian notation, and the
documented exceptions for `MSToolkit/` third-party code and the newer OOP layer under
`CometSearch/search/`, `CometSearch/output/`) live in `docs/CometCodingStyleGuidelines.md`
-- read it before writing or editing C++ in `CometSearch/`.

**Windows-style line endings (`\r\n`) are MANDATORY for every file in this repo.** This
applies to Claude Code's own Edit/Write behavior specifically, not just human-authored
code -- see the enforcement rules below.

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

