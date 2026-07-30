### Comet releases 2026.02

Documentation for parameters for release 2026.02 [can be found 
here](/Comet/parameters/parameters_202602/).

Download release [here](https://github.com/UWPR/Comet/releases).

#### release 2026.02 rev. 1 (2026.02.1), release date 2026/07/29

**What's Changed**

This release combines a Thermo raw file reading infrastructure migration, RTS peptide index multithreading, numerous batch and real-time-search (RTS) performance work, and correctness fixes.

#### New Features

- Native `.raw` file reading on Windows now uses Thermo's RawFileReader .NET library, replacing the legacy MSFileReader COM dependency entirely in MSToolkit code. No COM registration required,  just two DLLs  shipped alongside `comet.win64.exe`.
- Batch peptide-index (PI) search now uses the same fused, streaming pipeline as fragment-ion index (FI) search. PI index builds also now reuse FI's faster peptide-generation code and the search is now multithreaded.
- New dependency-free C++ unit test harness (`CometUnitTests`), wired into both the Linux and Windows build systems.

#### Performance Improvements

- Batch FI and PI search: per-worker memory arenas now pool per-spectrum scratch allocations instead of individually allocating/freeing them resulting in ~70 to ~200% higher throughput, with memory holding flat rather than degrading as batch size grows.
- RTS spectrum reading in SearchMS1MS2.cs changed from per-scan locked reads to a single-threaded upfront preload, so the parallel search phase touches only in-memory data with no locking. This allows measuring the maximum theoretical search throughput instead of being limited by raw file reading. 
- RTS PI search is now multithreaded. The peptide index is now loaded in memory versus being parsed from disk previously.
- Asynchronous spectrum readahead for fused FI whole-file batch searches, overlapping file I/O with search work instead of stalling on it.

#### Bug Fixes

**Search-result determinism and RTS/batch parity**:

- Fixed non-deterministic FASTA_DB search results: identical searches could report different results across runs when many candidates were exactly tied in score.
- Fixed the same class of tie-break bug in FI and PI's RTS-reachable scoring path.
- Fixed a stale-buffer bug causing RTS-specific run-to-run E-value/peptide jitter under concurrency.
- Fixed RTS FI fragment-peak selection ranking candidate peaks by mass instead of intensity, silently excluding low-mass/high-intensity peaks that batch correctly included, the single largest driver of FI batch vs RTS divergence found this release.
- Fixed RTS PI search previously never running AScorePro phosphosite localization; not a bug, just never implemented.
- Fixed RTS not enforcing the `minimum_peaks` and `clear_mz_range` spectrum filters that batch always applied, so RTS could weakly score spectra that batch search correctly skips.
- Fixed FI_DB top-peak selection (both RTS and batch) not respecting the configured `fragindex_min_fragmentmass` and `fragindex_max_fragmentmass` bounds.
- Fixed an invalid AScorePro site-scoring tie-break comparator that made phosphosite placement on exact ties depend on internal enumeration order rather than a deterministic rule.
- Fixed RTS never including the build's git commit hash in its reported version string.

**Memory safety and crashes:**

- Fixed heap corruption reading certain compact, non-indexed mzML files.
- Fixed a peptide-packing collision that could silently drop or truncate fragment-index peptides containing non-standard residue codes.

**Modification and mass correctness:**

- Fixed double-application of static modifications to parent-ion mass when reading a fragment-index (`.idx`) file, which corrupted reported modification masses in pepXML output.
- Fixed duplicate-row reporting for internal decoys in PI search.
- Fixed I/L-equivalent peptides from different proteins surviving as separate index entries instead of being correctly merged.
- Raised internal modification-combination limits for the index searches.

**RTS reliability:**

- Fixed an asymmetric RTS init/finalize lifecycle that could leak native resources (thread pool, scratch memory) if an exception occurred mid-session.
- RTS now respects the configured search timeout before running AScorePro localization, matching its other post-analysis steps.

#### Tools and Build

- Migrated MSToolkit's `.raw` file reading from MSFileReader COM to Thermo's RawFileReader .NET (see New Features above).
- Windows release packages now includes the two RawFileReader DLLs needed to read `.raw` files given the COM to .NET file reading migration.
- Added a dependency-free C++ unit test harness (`CometUnitTests`).

**Full Changelog**: https://github.com/UWPR/Comet/compare/v2026.02.0...v2026.02.1

---


#### release 2026.02 rev. 0 (2026.02.0), release date 2026/06/10

New Features

- Concurrent multi-threaded real-time search (RTS)
  - The RTS path (`RealtimeSearch.exe`) now supports *N* concurrent C# Task threads sharing a single `CometSearchManagerWrapper` instance. The MS2 fragment index search and MS1 spectral library alignment are both thread-safe: preprocessing uses per-thread `RtsScratch` scratch pools, `DoSingleSpectrumSearchMultiResults` operates on a thread-local `Query*` without touching `g_pvQuery`, and `DoMS1SearchMultiResults` serializes only the RT alignment history update. This delivers significant throughput improvement for MS2 RTS searches on multi-core hardware.

- Compound modifications aka Comet Multi-Modification
  - Merged the compound modifications branch to facilitate future code support. A new `compoundmods_file` parameter accepts a file listing J-residue mass modifications. These are searched via a dedicated `CompoundModSearch()` path integrated into `SearchForPeptides()` and `MergeVarMods()`, with output writers and post-analysis updated to handle the new modification encodings. Utility is for adduct screening.

- Peak memory reporting
  - Comet now reports peak resident set size at the end of index creation and search steps. Peak memory is also surfaced to the RTS C# layer via `CometSearchManagerWrapper::GetPeakMemory()`.

- Python q-value / FDR tool
  -  A new `tools/qvalue.py` script computes q-values from Comet tab-delimited output and supports side-by-side comparison of two result files with an optional `--diff` flag to list differing PSMs.

Performance Improvements

- Parallel .idx index building
    -  `GeneratePlainPeptideIndex` now uses a parallel per-length sort+dedup phase followed by a k-way heap merge write. On benchmarks with the human proteome this reduces index creation time by 1.3× (tryptic) to 1.9× (no-enzyme/MHC) compared to v2026.01.1.

-  RTS preprocessing thread-local pool (`RtsScratch`)
    -  All six scratch arrays used during single-spectrum preprocessing (raw data, fast XCorr, correlation, sparse matrix blocks) are pre-allocated once per thread and reused across spectra, eliminating per-spectrum heap allocation. Only the elements actually read/written are zeroed on each reuse.

-  E-value computation restructured with CSR inverted index
    -  GenerateXcorrDecoys() now uses a pre-built CSR inverted index (`s_invIdx_data`, `s_invIdx_start`) and a thread-local 3000-element float accumulator, replacing the previous per-decoy inner loop. Decoy scores are accumulated via scatter then histogrammed once, reducing cache pressure significantly.

-  AScore optimizations
    - Eliminated redundant `Scan` copies in `AScoreCalculator` and `AScoreDllInterface` (two copies reduced to one via pass-by-value + `std::move`).
    - `getMassList()` now caches its result; repeated calls with identical parameters return immediately without recomputation.
    - `matchPeaks()` replaces an `unordered_map` with two `vector<bool>` arrays, removing all hash operations from the hot matching loop.

-  In-memory protein name cache for RTS
    -  `g_pvProteinNameCache` (an `unordered_map<file_offset, string>`) is populated once at index load time. RTS protein lookups are now O(1) in-memory instead of seeking into the FASTA on every hit.

- `AcquirePoolSlot()` contention reduction
    -  The previous busy-spin wait on `_pbSearchMemoryPool` is replaced by a `std::condition_variable::wait_for` with proper lock/notify at all release sites, eliminating CPU waste under thread contention.

Bug Fixes

- I/L deduplication: When `equal_I_and_L=1`, the FASTA-original (L-containing) peptide sequence is now preserved in the index; the I-containing variant is the one discarded. Previously the choice was arbitrary, causing extra spurious entries in the index.
- `g_pvProteinsList` heap-allocation storm: Replaced element-by-element vector growth with a CSR (compressed sparse row) pre-allocation, eliminating O(N²) reallocation behavior on large databases.
- `DBIndex::sPeptide` / `PlainPeptideIndexStruct::sPeptide`: Refactored from `std::string` to `char[MAX_PEPTIDE_LEN]` fixed-size arrays, eliminating per-peptide heap allocations during index construction and search.
- set_`Z_user_amino_acid` parameter: Was incorrectly setting the X residue mass; now sets Z as intended.
- Peptide length range error message: Was displaying scan range values instead of peptide length values.
- `logout()` routing: All `logout()` calls now go to `stdout` instead of `stderr`.

Tools and Build

- Fragment ion index parameters added to the params file generated by `comet -p`.
- Visual Studio Clean Solution now removes Linux-built expat and zlib directories, preventing stale headers from interfering with subsequent builds.
- expat source distribution switched from .tar.gz to .zip for consistent cross-platform unpacking.
- Linux binary restored to static linking (`-static`) for compatibility with older glibc environments (e.g., Ubuntu 18.04 Docker images).
