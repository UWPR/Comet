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

#### Known Bugs

The following bugs present in this release were identified and addressed in later releases, as noted for each item:

- Known bug: in RTS searches of a peptide index built with internal decoys, decoy hits were returned with the target protein names, indistinguishable from real identifications. Addressed in [release 2026.03 rev. 0](/Comet/releases/release_202603.html).
- Known bug: in peptide index searches of a pre-built .idx file, every internal decoy candidate was classified as a target. Addressed in [release 2026.03 rev. 0](/Comet/releases/release_202603.html).
- Known bug: the .idx file did not store decoy_prefix, num_enzyme_termini, allowed_missed_cleavage or clip_nterm_methionine, so a search-time parameter mismatch could corrupt the target/decoy split or misreport the enzyme metadata in the pep.xml and mzIdentML output. Addressed in [release 2026.03 rev. 0](/Comet/releases/release_202603.html).
- Known bug: AScorePro could relocalize a position-restricted variable modification onto a residue that the search itself does not allow, e.g. N-terminal pyroglutamate onto an internal Q. Addressed in [release 2026.03 rev. 0](/Comet/releases/release_202603.html).
- Known bug: in FASTA searches with distance-restricted variable modifications (fifth and sixth variable_modNN fields): an n or c modification with a distance restriction could shift the site assignments of later modifications of the same entry; binary modifications with a protein C-terminus, peptide C-terminus or -2 restriction were never applied; in a binary group each member's sites were judged by the first member's restriction; a c modification with a protein N-terminus restriction was tested against the wrong position; and an n modification with a peptide C-terminus restriction was never placed. Addressed in [release 2026.03 rev. 0](/Comet/releases/release_202603.html).
- Known bug: the variable_modNN position restrictions (fifth and sixth fields) were ignored by fragment ion index and peptide index searches; the modification was applied without the restriction. Addressed in [release 2026.03 rev. 0](/Comet/releases/release_202603.html).
- Known issue: the index_search_type parameter read like a search mode switch, but it only selects which index type to build when database_name names an .idx file that does not yet exist; e.g. index_search_type = 1 with a PEFF database runs a regular PEFF search. Clarified, with warnings when the value has no effect, in [release 2026.03 rev. 0](/Comet/releases/release_202603.html).
- Known bug: fragment ion index searches never applied the add_Nterm_protein and add_Cterm_protein static modifications to peptides at a protein terminus, resulting in lower xcorr scores for those peptides when a non-zero value is set. Addressed in [release 2026.03 rev. 0](/Comet/releases/release_202603.html).
- Known bug: in FASTA searches, the reported precursor mass of a peptide with a variable modification at the protein N-terminus was short by the add_Nterm_protein mass when that parameter is non-zero. Addressed in [release 2026.03 rev. 0](/Comet/releases/release_202603.html).
- Known bug: mzIdentML output from fragment ion index and peptide index searches contained garbage protein accessions. Addressed in [release 2026.03 rev. 0](/Comet/releases/release_202603.html).
- Known bug: in mzIdentML output, the massDelta of a variable terminal modification double counted the static protein terminal modification mass. Addressed in [release 2026.03 rev. 0](/Comet/releases/release_202603.html).
- Known issue: the E-value linear regression used a narrow initial fit window of 5 histogram bins; the wider initial fit window was restored in [release 2026.03 rev. 0](/Comet/releases/release_202603.html) (PR [#126](https://github.com/UWPR/Comet/pull/126)).
- Known bugs identified in a code review of the search engine and addressed in [release 2026.03 rev. 0](/Comet/releases/release_202603.html) (PR [#128](https://github.com/UWPR/Comet/pull/128)): the modification count caps were bypassed for variable_mod10 through variable_mod15 in FASTA searches and in fragment ion/peptide index searches; stale precursor neutral loss and start position state in index searches; incorrect decoy neutral loss ladders; the -F command line option without -L could search zero spectra; the preprocessing memory pool wait held its mutex for the entire 240 second wait, preventing a slot from being released; plus several off-by-one heap reads/writes.
- Known bug: enabling AScorePro with variable_mod10 through variable_mod15 in use corrupted the reported modification sites. As of [release 2026.03 rev. 0](/Comet/releases/release_202603.html) this combination is rejected with an error (AScorePro supports variable_mod01 through variable_mod09).
- Known bug: the add_U_selenocysteine parameter was never read; the code expected the parameter name add_U_user_amino_acid. Addressed in [release 2026.03 rev. 0](/Comet/releases/release_202603.html).
- Known bug: a non-numeric entry in the mass_offsets parameter hangs the parameter parser at 100% CPU. Addressed in [release 2026.03 rev. 0](/Comet/releases/release_202603.html).
- Known bug: a search_enzyme_number, search_enzyme2_number or sample_enzyme_number value with no matching entry in the enzyme list silently used a default enzyme instead of reporting an error. Addressed in [release 2026.03 rev. 0](/Comet/releases/release_202603.html).
- Known bug: very long parameter values (e.g. a long database_name path) overflowed a fixed size buffer. Addressed in [release 2026.03 rev. 0](/Comet/releases/release_202603.html).
- Known bug: PEFF parsing, see issue [#132](https://github.com/UWPR/Comet/issues/132): with "# HasAnnotationIdentifiers=true" the annotation identifier was read as the modification position, so labeled modifications landed on the wrong residue and labeled variants were dropped; modification names containing parentheses were split, and a name ending in ")" caused that and all later modifications on the protein to be dropped; a \VariantSimple entry with an empty residue field inherited the previous entry's residue. Addressed in [release 2026.03 rev. 0](/Comet/releases/release_202603.html).
- Known bug: in peptide index batch searches with AScorePro, the static modifications were applied twice to the AScorePro residue masses (e.g. C off by +57.02), giving incorrect AScorePro scores. Addressed in [release 2026.03 rev. 0](/Comet/releases/release_202603.html).
- Known bug: with internal decoys (decoy_search = 1), AScorePro relocalizing an internal decoy top hit could create a duplicate result row. Addressed in [release 2026.03 rev. 0](/Comet/releases/release_202603.html).

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
- Windows release packages now include the two RawFileReader DLLs needed to read `.raw` files given the COM to .NET file reading migration.
- Added a dependency-free C++ unit test harness (`CometUnitTests`).

#### Known Bugs

The following bugs present in this release were identified and addressed in later releases, as noted for each item:

- Known bug: fragment ion index searches used a compacted variable modification slot index as the variable_modNN slot index. For peptides whose active modifications are not contiguous starting at variable_mod01, or that carry more than one modification type, the precursor mass and modified fragment ion masses were computed against the wrong modification, corrupting both xcorr and Sp for the affected peptides. Peptide index and FASTA searches were not affected. Addressed in [release 2026.02 rev. 2](/Comet/releases/release_202602.html).
- Known bug: the digest_mass_range and peptide_length_range values set at search time were not applied when searching an existing .idx file (never checked for peptide index searches and only coincidentally applied for fragment ion index searches). Addressed in [release 2026.02 rev. 2](/Comet/releases/release_202602.html).
- Known bug: several fields of the search results structure were left uninitialized, causing intermittent memory corruption that depended on the heap layout. Addressed in [release 2026.02 rev. 2](/Comet/releases/release_202602.html).
- Known bug: the .idx file did not store the per-modification maximum count or max_variable_mods_in_peptide, so searching an existing index with values different from those used to build it gave incorrect results. Addressed in [release 2026.02 rev. 2](/Comet/releases/release_202602.html).
- Known bug: a corrupt or truncated .idx file could cause a segfault or a silent search against an empty index instead of a clean error. Addressed in [release 2026.02 rev. 2](/Comet/releases/release_202602.html).
- All known bugs listed under release 2026.02 rev. 2 above also apply to this release.

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
- `set_Z_user_amino_acid` parameter: Was incorrectly setting the X residue mass; now sets Z as intended.
- Peptide length range error message: Was displaying scan range values instead of peptide length values.
- `logout()` routing: All `logout()` calls now go to `stdout` instead of `stderr`.

#### Tools and Build

- Fragment ion index parameters added to the params file generated by `comet -p`.
- Visual Studio Clean Solution now removes Linux-built expat and zlib directories, preventing stale headers from interfering with subsequent builds.
- expat source distribution switched from .tar.gz to .zip for consistent cross-platform unpacking.
- Linux binary restored to static linking (`-static`) for compatibility with older glibc environments (e.g., Ubuntu 18.04 Docker images).

#### Known Bugs

The following bugs present in this release were identified and addressed in later releases, as noted for each item:

- Known bug: search results were not deterministic when many candidate peptides had exactly tied scores; identical searches could report different sp_rank values or, rarely, a different top ranked peptide across runs. This affected FASTA, fragment ion index and peptide index searches. Addressed in [release 2026.02 rev. 1](/Comet/releases/release_202602.html).
- Known bug: in real-time searches (RTS) using the fragment ion index, the top peaks were selected in mass order rather than by intensity whenever more than fragindex_num_spectrumpeaks peaks passed the intensity cutoff, so RTS scored against a worse peak subset than the batch search. Addressed in [release 2026.02 rev. 1](/Comet/releases/release_202602.html).
- Known bug: the RTS single spectrum search did not apply the minimum_peaks and clear_mz_range spectrum filters that the batch search applies, so RTS could weakly score spectra that the batch search skips. Addressed in [release 2026.02 rev. 1](/Comet/releases/release_202602.html).
- Known bug: the top peak selection for fragment ion index searches (batch and RTS) did not respect the fragindex_min_fragmentmass and fragindex_max_fragmentmass bounds, so peaks that can never match an indexed fragment could occupy a top peak slot. Addressed in [release 2026.02 rev. 1](/Comet/releases/release_202602.html).
- Known bug: the AScorePro site scoring tie-break comparator was invalid, so modification site placement on exact ties depended on the internal enumeration order. Addressed in [release 2026.02 rev. 1](/Comet/releases/release_202602.html).
- Known bug: AScorePro localization never ran in RTS peptide index searches, regardless of the print_ascorepro_score setting. Addressed in [release 2026.02 rev. 1](/Comet/releases/release_202602.html).
- Known bug: concurrent RTS searches could show run-to-run E-value jitter and, rarely, a different top peptide for roughly 0.5% of PSMs due to a stale thread-local buffer. Addressed in [release 2026.02 rev. 1](/Comet/releases/release_202602.html).
- Known bug: reading compact, non-indexed mzML files (e.g. Proteome Discoverer output where several closing tags share a line) could corrupt the heap or fail with "is not indexed" errors, and spectra in non-indexed mzML/mzXML files with indented or multi-line opening tags could be mislocated ("parseOffset(): Syntax error"). Addressed in [release 2026.02 rev. 1](/Comet/releases/release_202602.html).
- Known bug: fragment ion index peptides containing non-standard residue codes (B, J, O, U, X, Z) could be silently dropped from the index or have their sequence truncated due to a peptide packing collision. Addressed in [release 2026.02 rev. 1](/Comet/releases/release_202602.html).
- Known bug: static modifications were applied twice to the parent residue masses when reading a fragment ion index .idx file, corrupting the mass attribute of the aminoacid_modification elements and the residuemass parameter lines in the pep.xml output. Addressed in [release 2026.02 rev. 1](/Comet/releases/release_202602.html).
- Known bug: in peptide index searches with internal decoys, a peptide matching both a target and its reversed decoy was reported as two separate PSM rows. Addressed in [release 2026.02 rev. 1](/Comet/releases/release_202602.html).
- Known bug: with equal_I_and_L = 1, peptides from different proteins that differ only by I/L were kept as separate index entries, so a single query could match twice. Addressed in [release 2026.02 rev. 1](/Comet/releases/release_202602.html).
- Known bug: internal limits in the index build (24 modifiable residues or 2000 modification combinations per peptide) silently dropped all modified forms of highly modifiable peptides from fragment ion and peptide indexes. Addressed in [release 2026.02 rev. 1](/Comet/releases/release_202602.html).
- Known bug: in PEFF searches, the mass tolerance check for variant peptides used the unmodified peptide mass without the PEFF mass difference. Addressed in [release 2026.02 rev. 1](/Comet/releases/release_202602.html).
- Known bug: in fragment ion index batch searches, AScorePro read the variable modification settings before they were replaced by the values stored in the .idx header, so it could run with stale modification settings. Addressed in [release 2026.02 rev. 1](/Comet/releases/release_202602.html).
- Known bug: RTS peptide index search results were missing the protein names, and RTS searches with compound modifications configured performed an out-of-bounds read. Addressed in [release 2026.02 rev. 1](/Comet/releases/release_202602.html).
- Known issue: for Thermo .raw files on Windows, the charge state discrimination for spectra without a known precursor charge was inert, so such spectra were always searched as both 2+ and 3+. Addressed in [release 2026.02 rev. 1](/Comet/releases/release_202602.html).
- Known bug: peptide index batch searches of an existing .idx file did not apply the spectrum_batch_size limit, so memory use was effectively unbounded (e.g. 7.4 GB for a 20K spectra search). Addressed in [release 2026.02 rev. 1](/Comet/releases/release_202602.html).
- Known bug: the RTS finalize calls did not free the preprocessing buffers (MS2 search) and freed the search memory pool that they did not allocate (MS1 search). Addressed in [release 2026.02 rev. 1](/Comet/releases/release_202602.html).
- All known bugs listed under release 2026.02 rev. 1 and rev. 2 above also apply to this release.
