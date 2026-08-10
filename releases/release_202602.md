### Comet releases 2026.02

Documentation for parameters for release 2026.02 [can be found 
here](/Comet/parameters/parameters_202602/).

Download release [here](https://github.com/UWPR/Comet/releases).

#### release 2026.02 rev. 2 (2026.02.2), release date 2026/08/10

**What's Changed**

This release fixes a bug in fragment-ion-index (FI) variable-modification handling that could silently mis-score modified peptides, and unifies the on-disk index format across both index search modes with a substantial reduction in peptide-index (PI) memory usage.

#### Bug Fixes

**Fragment-ion-index modification scoring:**

- Fixed FI (`AddFragments()`, `AddFragmentsThreadProc()`) using a compacted internal variable-mod-slot index directly as a raw `variable_modNN` slot index. The two only coincide when a peptide's active modifications happen to be contiguous starting at `variable_mod01` — any other combination (e.g. a lower-numbered `variable_modNN` left unused, or simply a peptide carrying more than one modification type at once) silently computed the precursor mass and modified fragment-ion masses against the wrong modification. A fourth, independent copy of the same bug existed in `CometSearch.cpp`'s `SearchFragmentIndex()`, the per-query FI scoring function — affecting every FI search with more than a trivial single-slot modification config, not just gap configs.
  - **Both XCorr and SP were corrupted for affected hits, confirmed by tracing the actual data flow**: `SearchFragmentIndex()` builds `piVarModSites[]` (using the buggy translation) and passes it directly into `XcorrScoreI()` for XCorr in the same call; when a hit becomes the new top score, that same array is copied verbatim into the stored result, which `CometPostAnalysis::CalculateSP()` later reads as-is for SP — one bad array, propagated into both scores, not two independent bugs.
  - **Scope: FI only.** PI (`CometPeptideIndex::MaterializeOneEntry()`/`EnumerateIndexPeptideMods()`) already performed the correct compacted-to-real-slot translation before this fix — confirmed by diff, this release only refactors that already-correct PI logic into shared helpers, with no behavior change. Plain, non-indexed search never uses the compacted-slot representation at all — it assigns real `variable_modNN` indices directly during on-the-fly digestion, so this bug class was structurally impossible there. **PI and non-indexed scoring were unaffected throughout.**
  - Real-world impact, measured against the v2026.02.1 release binary on a human phosphoproteomics FI search (Met-oxidation + STY-phosphorylation, 54,445 spectra, MM2_R1.raw): **+4.6% more PSMs at 1% FDR by xcorr** (16,009 → 16,745) and **+4.0% by E-value** (16,238 → 16,891), concentrated almost entirely in multiply-modified peptides — exactly the population most exposed to this bug.
- Fixed an accompanying out-of-bounds read: the fragment-ion b/y-ion mass loop indexed the mod-slot translation table with `-1` (the ordinary "not modified in this particular combination" sentinel) for peptides with more modifiable sites than `max_variable_mods_in_peptide` allows — confirmed as a real heap-buffer-overflow under AddressSanitizer.
- New regression test (`t25_fi_mod_slot_gap`) added to `tests/unit/run_tests.py`, deliberately configured with a non-contiguous modification slot so this class of bug can't hide behind an all-slot-0 test config again.


Bug fix analysis:  IMAC enriched sample, human canonical target-decoy, 16M, 80STY, 1% FDR cutoff by E-value

|   # variable mods  | v2026.02.1 (prev) | v2026.02.2 (current) | Δ |
|:---:|---:|---:|:---:|
| 0 (unmodified) | 304 | 256 | −48 |
| 1 | 13,123 | 13,397 | +274 |
| 2 | 2,656 | 3,019 | +363 |
| 3 | 153 | 210| +57 |
| 4 | 2 | 9 |   +7 |
| Total | 16,238 |  16,891 | +653 |

Composition:

| Phospho | Ox-Met | v2026.02.1 (prev) |  v2026.02.2 (current) |
|:---:|:---:|---:|---:|
|       0 |      0 |        304 |      256 |
|       1 |      0 |     13,112 |   13,382 |
|       2 |      0 |      1,889 |    2,093 |
|       0 |      1 |         11 |       15 |
|       1 |      1 |        765 |      924 |
|       2 |      1 |        141 |      181 |
|       0 |      2 |          2 |        2 |
|       1 |      2 |         12 |       29 |
|       2 |      2 |          2 |        9 |

**Corrupt/truncated index-file hardening** (carried over from PR #121):

- `ReadPeptideIndex()` now validates footer offsets and per-entry length fields against the file's actual size before trusting them for allocation or `memcpy` sizing, instead of risking a multi-GB allocation attempt or an out-of-bounds read on a truncated or corrupted `.idx` file.
- Fixed 4 call sites (`CreateFragmentIndex()`, `RunSearch()`'s legacy batch overload, `FiStrategy::initialize()`, `InitializeSingleSpectrumSearch()`'s FI branch) that never checked `ReadPeptideIndex()`'s return value — a corrupt-file error previously printed a clean message and then the search proceeded anyway with an uninitialized index, segfaulting.
- Hardened `MaterializeOneEntry()` and `SearchPeptideIndex()` against out-of-range indices from a corrupt `.idx`, failing the one affected candidate cleanly instead of risking an uncaught exception mid-search.

#### Performance Improvements

- PI memory usage reduced ~1.6× by splitting the in-memory index into a shared raw-peptide table plus a compact 24-byte-per-variant array, materializing full peptide records on demand during search instead of pre-expanding every modified variant up front — mirroring the approach FI already used. Measured on a 125M-variant real-world index: RTS memory 10.48GB → 6.58GB, index build time 3m30s → 46s, build peak memory 22.8GB → 7.6GB.
- `MaterializeOneEntry()`'s per-candidate modification-slot table is now built once per search (thread-safe one-time init) instead of being recomputed on every mass-window candidate in the search hot path.

#### Breaking Changes

- PI and FI now share a single unified `.idx` file format and reader/writer (`-i`/`-j` are now synonyms at build time; which mode a *search* uses is selected explicitly via the new `index_search_type` parameter). **The on-disk format version changed — existing `.idx` files built with v2026.02.1 or earlier must be rebuilt.** Comet detects and rejects old-format files with a clear error rather than misreading them.

#### Tools and Build

- `comet.exe -D<database>.idx -i`/`-j`-built index now correctly enforces the `digest_mass_range`/`peptide_length_range` set at search time (previously written to the `.idx` header but never read back, so a narrower search-time range had no effect on PI and only a coincidental effect on FI).
- Migrated the ~21 hand-run functional test cases into `tests/unit/run_tests.py` (T21), and added automated RTS FI/PI single-spectrum regression coverage (T22) plus full-scale internal-decoy/target-decoy and FI/PI-vs-plain-FASTA parity checks against real data (T23/T24, opt-in via `--bigdata`).
- T23/T24 now also compare every config against the v2025.03.0 release binary (auto-downloaded on first use) to catch cross-version regressions in both result counts and search/build timing.

**Full Changelog**: https://github.com/UWPR/Comet/compare/v2026.02.1...v2026.02.2

---

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

#### New Features

- Concurrent multi-threaded real-time search (RTS)
  - The RTS path (`RealtimeSearch.exe`) now supports *N* concurrent C# Task threads sharing a single `CometSearchManagerWrapper` instance. The MS2 fragment index search and MS1 spectral library alignment are both thread-safe: preprocessing uses per-thread `RtsScratch` scratch pools, `DoSingleSpectrumSearchMultiResults` operates on a thread-local `Query*` without touching `g_pvQuery`, and `DoMS1SearchMultiResults` serializes only the RT alignment history update. This delivers significant throughput improvement for MS2 RTS searches on multi-core hardware.

- Compound modifications aka Comet Multi-Modification
  - Merged the compound modifications branch to facilitate future code support. A new `compoundmods_file` parameter accepts a file listing J-residue mass modifications. These are searched via a dedicated `CompoundModSearch()` path integrated into `SearchForPeptides()` and `MergeVarMods()`, with output writers and post-analysis updated to handle the new modification encodings. Utility is for adduct screening.

- Peak memory reporting
  - Comet now reports peak resident set size at the end of index creation and search steps. Peak memory is also surfaced to the RTS C# layer via `CometSearchManagerWrapper::GetPeakMemory()`.

- Python q-value / FDR tool
  -  A new `tools/qvalue.py` script computes q-values from Comet tab-delimited output and supports side-by-side comparison of two result files with an optional `--diff` flag to list differing PSMs.

#### Performance Improvements

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

#### Bug Fixes

- I/L deduplication: When `equal_I_and_L=1`, the FASTA-original (L-containing) peptide sequence is now preserved in the index; the I-containing variant is the one discarded. Previously the choice was arbitrary, causing extra spurious entries in the index.
- `g_pvProteinsList` heap-allocation storm: Replaced element-by-element vector growth with a CSR (compressed sparse row) pre-allocation, eliminating O(N²) reallocation behavior on large databases.
- `DBIndex::sPeptide` / `PlainPeptideIndexStruct::sPeptide`: Refactored from `std::string` to `char[MAX_PEPTIDE_LEN]` fixed-size arrays, eliminating per-peptide heap allocations during index construction and search.
- set_`Z_user_amino_acid` parameter: Was incorrectly setting the X residue mass; now sets Z as intended.
- Peptide length range error message: Was displaying scan range values instead of peptide length values.
- `logout()` routing: All `logout()` calls now go to `stdout` instead of `stderr`.

#### Tools and Build

- Fragment ion index parameters added to the params file generated by `comet -p`.
- Visual Studio Clean Solution now removes Linux-built expat and zlib directories, preventing stale headers from interfering with subsequent builds.
- expat source distribution switched from .tar.gz to .zip for consistent cross-platform unpacking.
- Linux binary restored to static linking (`-static`) for compatibility with older glibc environments (e.g., Ubuntu 18.04 Docker images).
