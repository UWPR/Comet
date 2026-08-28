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
| `g_iFragmentIndex`, `g_fragmentPeptides`, `g_vRawPeptides` | [x] Read-only after init | Fragment index |
| `g_pvProteinsList` | [x] Read-only after init | Populated on both a fresh build and a search-only read-back of an existing `.idx` |
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

Tests live in `tests/unit/`. The runner is `run_tests.py`.

```bash
# Run all unit tests (T1-T7, T11-T16, T19-T21, T25-T33) -- fast, no large data required
python tests/unit/run_tests.py --comet /mnt/c/Work/Comet-master/comet.exe

# Run a specific test by ID
python tests/unit/run_tests.py --comet /mnt/c/Work/Comet-master/comet.exe t13

# Run against both a Linux and a Windows build in one invocation (--comet is repeatable)
python tests/unit/run_tests.py \
  --comet /mnt/c/Work/Comet-master/comet.exe \
  --comet /mnt/c/Work/Comet-master/x64/Release/Comet.exe

# Run unit + integration tests (T17, T18, T22-T24) -- requires data/human.small.fasta
# and/or --bigdata (see below)
python tests/unit/run_tests.py --comet /mnt/c/Work/Comet-master/comet.exe --integration
```

Always pass `--comet` as a full path; the default `../../comet.exe` only works when
invoked from inside `tests/unit/`.

### Test Data

Small crafted FASTA files for T1-T16 live in `tests/unit/data/`. Pre-built `.idx`
reference files are committed alongside them for byte-exact comparison tests.

Integration tests T17/T18 require `data/human.small.fasta` (not in repo -- must be
present manually before running `--integration`).

### Legacy functional-correctness cases (T21) and RTS/big-data regressions (T22-T24)

T21 (`t21_*`, one per case, always run) migrates the ~21 hand-run cases from
`/mnt/c/Work/20130226-comet-tests/runall.sh` -- fixtures live in `tests/unit/data/legacy/`,
params are generated at runtime from `tests/unit/legacy_cases.py`'s template rather than
maintaining ~15 historical `comet.params.YYYYNNN` copies per case. See that module's
docstring for the full case table and what each one asserts.

T22 (`t22_rts_fi`, `t22_rts_pi`, `--integration`) exercises the real-time search (RTS)
single-spectrum path via `tests/rts_repro/` -- no C++/CLI or Thermo dependency, so it
runs on Linux. It checks (1) RTS finds the correct peptide against both an FI_DB and a
PI_DB built from a small unambiguous fixture, and (2) 1-thread and 8-thread runs over
197 real spectra are byte-identical (the determinism guarantee from
`tests/rts_repro/README.md`). `tests/rts_repro/ms2_to_fixture.py` converts any `.ms2`
into the driver's fixture format.

T23/T24 (`t23_decoy_modes`, `t24_index_parity`, `--integration` + `--bigdata`) migrate
`comet-debug3`/`comet-debug4`'s full-scale searches (~350MB of real data: a 177MB mzXML,
57MB/116MB FASTAs). `--bigdata DIR` (default: the sibling `20130226-comet-tests/`
directory) points at this data in place -- it is never copied into the repo. Both tests
skip cleanly if the directory isn't present. T23 checks that internal-decoy and
target-decoy searches agree on PSM counts at 1% FDR (via `tools/qvalue.py`) within 5%;
T24 checks the same for plain-FASTA vs. FI_DB vs. PI_DB searches -- all three currently
pass and agree within a few percent (17,660 / 17,033 / 17,660 PSMs at 1% FDR respectively).

Note: while developing T24, one manual (non-harness) attempt to search a full-scale
target-decoy FI_DB crashed with `std::length_error: cannot create std::vector larger
than max_size()`. That manual build was interrupted by a shell timeout mid-write, almost
certainly leaving a truncated/corrupt `.idx` on disk -- under T24's own clean
build-then-search sequence, FI_DB has run correctly every time. See the comment above
`test_t24_index_parity` in `run_tests.py` if this ever resurfaces.

**Cross-version comparison against a previous Comet release.** Both T23 and T24 also run
every one of their configs (T23: both decoy modes; T24: plain-FASTA, FI_DB, PI_DB --
each built fresh with the baseline binary, since `.idx` formats aren't guaranteed
compatible across versions) against a pinned previous release, `v2026.02.2`
(`BASELINE_TAG` in `run_tests.py`), fetched automatically on first use via
`tests/regression/setup_baselines.py`'s download logic into
`tests/regression/baselines/v2026.02.2/comet` (gitignored, like all fetched baselines --
override with `--baseline PATH` to point at something else; cross-version checks skip
cleanly, without failing the test, if no baseline is available). Same-version comparisons
(internal-vs-target-decoy, FI/PI-vs-plain-FASTA) use a 5% tolerance; current-vs-baseline
comparisons use 10%, since a different Comet version legitimately identifies a somewhat
different peptide population.

**Runtime regression check.** Each search and index build is timed, and every
current-vs-baseline pairing also asserts current isn't more than `TIMING_NOISE_TOLERANCE`
(currently 25%) slower than `v2026.02.2`'s wall-clock time for the same operation, via
`_check_timing()`. That threshold is deliberately generous: these are multi-minute,
single-sample wall-clock measurements on real (possibly shared) hardware, and run-to-run
variance from machine noise alone can plausibly reach 10-20% with no code change at all.
Treat one `_check_timing` failure as "worth re-running to confirm," not proof of a
regression on its own -- a *repeated* failure across multiple runs is the real signal.

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

