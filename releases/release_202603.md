### Comet releases 2026.03

Documentation for parameters for release 2026.03 [can be found 
here](/Comet/parameters/parameters_202603/).

Download release [here](https://github.com/UWPR/Comet/releases).

#### release 2026.03 rev. 0 (2026.03.0), release date 2026/10/01

**What's Changed**

This release brings terminal and protein-terminal variable modifications, and the position restrictions of [variable_modNN](/Comet/parameters/parameters_202603/variable_modXX.html), to indexed database searches. It also adds Comet's internal decoys to fragment ion index (FI) searches and a .NET 8 build of the real-time search wrapper. Peptide index (PI) and FI searches are substantially faster and use less memory, and the release includes a broad set of correctness and memory-safety fixes. **Existing .idx files must be rebuilt**; see Breaking Changes.

#### New Features

- **Protein-terminal variable modifications.** The residue field of [variable_modNN](/Comet/parameters/parameters_202603/variable_modXX.html) accepts two new codes, `^` (protein N-terminus only) and `$` (protein C-terminus only), alongside `n`/`c` (any peptide N-/C-terminus), e.g. `variable_mod01 = 42.010565 ^ 0 1 -1 0 0 0.0` for protein N-terminal acetylation. Terminal and protein-terminal variable modifications now work in regular FASTA, PI and FI searches alike and count toward `max_variable_mods_in_peptide` in all three ([#130](https://github.com/UWPR/Comet/pull/130)).
- **Variable modification position restrictions in indexed searches.** The fifth and sixth fields of [variable_modNN](/Comet/parameters/parameters_202603/variable_modXX.html) (distance from a terminus, and which terminus) were applied by regular FASTA searches only; FI and PI searches applied the modification without the restriction. They are now honored by FI and PI searches too, e.g. N-terminal pyroglutamate `variable_mod03 = -17.026549 Q 0 1 0 2 0 0.0` is applied only to a peptide N-terminal Q on every search path. Peptide-terminus restrictions, `-2` and protein-terminus distance 0 are exact in all searches. A protein-terminus distance greater than 0 is applied by FI/PI searches only to peptides at that protein terminus, since the index does not record a peptide's position within its protein; Comet reports a warning for this case. `n 0 0` and `c 0 1` (protein-terminus distance 0) are equivalent to `^` and `$` ([#134](https://github.com/UWPR/Comet/pull/134)).
- **Internal decoys in FI searches.** [decoy_search](/Comet/parameters/parameters_202603/decoy_search.html) = 1 or 2 is now supported for FI searches: each target peptide in the index gets a pseudo-reversed decoy with its own fragment postings. With internal decoys the index holds half as many unmodified peptides as a target-decoy FASTA and search speed is unchanged. For .idx searches the decoy mode is the value the index was built with. With `decoy_search = 2`, decoys are written to a separate `.decoy.txt` file rather than the target output. In real-time searches (RTS) use `decoy_search = 1`: with `decoy_search = 2` the decoys are scored but not returned, so RTS output contains targets only ([#129](https://github.com/UWPR/Comet/pull/129)).
- **CometWrapperCore**, a .NET 8 (`net8.0`) build of the C++/CLI real-time search wrapper alongside the existing .NET Framework `CometWrapper`, including Thermo .raw support. `CometWrapperCore.dll` and `Ijwhost.dll` are published as Windows release assets ([#127](https://github.com/UWPR/Comet/pull/127)).

#### Bug Fixes

- **Real-time search decoy labeling:** internal-decoy hits returned by the real-time search interface for indexed databases were reported with the target's protein names, indistinguishable from real identifications; they are now labeled with `decoy_prefix` ([#129](https://github.com/UWPR/Comet/pull/129)).
- **PI decoy classification:** every internal-decoy candidate in a PI search of a pre-built .idx was misclassified as a target ([#128](https://github.com/UWPR/Comet/pull/128)).
- **.idx header completeness:** `decoy_prefix`, `num_enzyme_termini`, `allowed_missed_cleavage` and `clip_nterm_methionine` are now stored in the .idx header, so a search-time params mismatch can no longer corrupt the target/decoy split or misreport enzyme metadata in pepXML/mzIdentML output ([#128](https://github.com/UWPR/Comet/pull/128)).
- **Deterministic .idx builds** ([#134](https://github.com/UWPR/Comet/pull/134)): building the same .idx twice now produces identical files, also across x86 and ARM builds. Peptides repeated within one protein (e.g. the ubiquitin repeats of UBC) could otherwise be represented by either copy depending on thread timing, with a mass differing in the last bits, and the mass sort used a non-transitive tolerance comparison.
- **AScorePro and position-restricted modifications** ([#134](https://github.com/UWPR/Comet/pull/134)): AScorePro could relocalize a position-restricted variable modification onto a residue the search itself does not allow, e.g. N-terminal pyroglutamate onto an internal Q. Such peptidoforms are now skipped, and the reported MOB and site scores are computed among the allowed peptidoforms.
- **Distance-restricted variable modifications (FASTA searches), inherited from 2026.02** ([#134](https://github.com/UWPR/Comet/pull/134)): an `n` or `c` variable modification with a distance restriction could shift the site assignments of later modifications of the same `variable_modNN` entry; binary modifications with a protein C-terminus, peptide C-terminus or `-2` restriction were never applied; in a binary group, each member's sites were judged by the first member's restriction instead of its own; a `c` modification with a protein N-terminus restriction was tested against the wrong position; an `n` modification with a peptide C-terminus restriction (e.g. `n 0 1 8 3`, a peptide-length limit) was silently never placed. Fifth/sixth field values that no search path defines (distance below -2, or a distance of 0 or more with a terminus outside 0-3) are now rejected with an error instead of giving different results on different search paths.
- **Protein attribution of protein-terminus-restricted modifications (FI/PI searches)** ([#134](https://github.com/UWPR/Comet/pull/134)): a peptide shared by several proteins and carrying a modification restricted to a protein terminus was reported against every protein containing it; it is now reported only against the proteins in which it is at that terminus, matching FASTA searches.
- **`index_search_type` scope** ([#134](https://github.com/UWPR/Comet/pull/134)): the parameter only selects which index type to build when `database_name` names an .idx file that does not exist yet; it read like a search-mode switch (issue [#132](https://github.com/UWPR/Comet/issues/132) comment: `index_search_type = 1` with a PEFF database is a regular PEFF search, not an index search). It is no longer written by `comet -p`; `comet -q` writes `index_search_type = -1` (not set, the default). An explicit 0 or 1 that has no effect (FASTA/PEFF database, an existing .idx of the other type, or a `-i`/`-j` build of the other type) now reports a warning, and any other value warns and is treated as -1. See the [parameter page](/Comet/parameters/parameters_202603/index_search_type.html). `RealtimeSearch.exe`'s optional sixth argument is the same selector and is sent only when given.
- **FI static protein-terminal masses:** FI searches never applied `add_Nterm_protein`/`add_Cterm_protein` to peptides at a protein terminus ([#130](https://github.com/UWPR/Comet/pull/130)).
- **E-value:** restored the wider initial fit window in the E-value linear regression, by @rrad42 ([#126](https://github.com/UWPR/Comet/pull/126)).
- **Wrong-result and crash fixes** from a code review of the search engine ([#128](https://github.com/UWPR/Comet/pull/128)), including: modification-count caps bypassed for `variable_mod10`-`15` and in FI/PI searches; stale precursor neutral-loss and start-position state in FI/PI; decoy neutral-loss ladders; AScorePro enabled alongside `variable_mod10`-`15`, which corrupted modification sites (now rejected with an error, as AScorePro supports `variable_mod01`-`09`); off-by-one heap reads/writes; and robustness against corrupt .idx, MSP, mzIdentML and PEFF input.
- **Parameter file parsing:** [add_U_selenocysteine](/Comet/parameters/parameters_202603/add_U_selenocysteine.html) is now read (the code expected a different name); the spectral library MS level parameter name is fixed (`spectral_library_ms_level`); a non-numeric `mass_offsets` entry no longer hangs the parser at 100% CPU; a `search_enzyme_number`, `search_enzyme2_number` or `sample_enzyme_number` with no matching enzyme entry now reports an error instead of silently using a default; very long parameter values (e.g. a long `database_name` path) no longer overflow a buffer.
- **PEFF parsing** ([#133](https://github.com/UWPR/Comet/pull/133), issue [#132](https://github.com/UWPR/Comet/issues/132)):
  - Annotation identifiers (`# HasAnnotationIdentifiers=true`, e.g. `\ModResPsi=(1:25|MOD:00798|half cystine)`) were read as the position: labeled modifications landed on the wrong residue and labeled variants were dropped.
  - Modification names containing parentheses (e.g. `N6-(4-amino-2-hydroxybutyl)-L-lysine`) were split into pieces; when a name ended in `)`, that modification and every later one on the protein were silently dropped.
  - A `\VariantSimple` entry with an empty residue field inherited the previous entry's residue and was searched as a bogus variant.
  - [peff_verbose_output](/Comet/parameters/parameters_202603/peff_verbose_output.html) warnings now report one clean line per ignored entry, quoting it as written.

#### Performance Improvements

Measured on the human canonical proteome, Met-oxidation and Met-oxidation + STY-phosphorylation, versus v2026.02.2 (pre-release build, 2026/09/17):

- PI searches are 1.7-2.7x faster (batch and real-time search), with identical results.
- FI in-memory index regeneration is 2-3x faster and now scales with threads (phospho: ~65 s to 37 s at 4 threads, 25 s at 8); the FI search phase itself is 30-40% faster in batch.
- Lower memory throughout: FI phospho peak 20.0 GB to 18.3 GB; PI phospho 7.9 GB to 5.2 GB (2.8 GB with internal decoys); .idx build peak 1.1 GB to 0.55 GB. The in-memory PI/FI index uses a compact 13-byte-per-variant layout, pooled raw-peptide and modification tables, and a streaming .idx reader.
- Results: three of the four target-decoy configurations are PSM-for-PSM identical to v2026.02.2; FI phospho changes 0.4% of rank-1 peptides (1% FDR PSMs -0.8% by xcorr, +0.7% by E-value).
- Honoring position restrictions in indexed searches: a phosphorylation search with N-terminal pyroglutamate from Q and E (`0 2` restriction) and internal decoys identifies 10.8% more PSMs at 1% FDR by xcorr and 3.2% more by E-value than when the restriction is ignored, and every target pyroglutamate is placed on the peptide's first residue in FASTA, FI and PI searches.

#### Breaking Changes

- **.idx format v5.** Existing .idx files must be rebuilt; Comet rejects older-format files with a clear error. Indexes built by pre-release builds of this release load but carry no variable modification position restrictions; rebuild them too.

#### Known Limitations

- Internal decoys reverse the peptide and move each modification with its residue, so a decoy's N-terminal pyroglutamate ends up on an internal residue. Targets and decoys stay mass-matched, but position-restricted modifications appear more often on decoys (about 2:1 for pyroglutamate); release 2026.02 behaved the same way.
- Binary modifications (non-zero third field of `variable_modNN`) with a position restriction are supported by FASTA searches only.
- In FASTA searches, AScorePro evaluates a protein-terminus restriction against the proteins recorded for the PSM (at most [max_duplicate_proteins](/Comet/parameters/parameters_202603/max_duplicate_proteins.html)); an alternative site that is legal only in a protein not on that list is not considered.

#### Tools and Build

- CI workflows updated to Node 24 ([#131](https://github.com/UWPR/Comet/pull/131)).
- `tools/qvalue.py` now locates columns by header name, so it works on PEFF search output.
- New regression tests T26-T60 in `tests/unit/run_tests.py`; the C++ unit tests (`CometUnitTests`) now have 77 cases.
- `tests/regression/run_regression.py` runs the internal-decoy variants in FI mode too (marked target-side-only against baselines older than 2026.03.0).

**New Contributors**

- @rrad42 made their first contribution in [#126](https://github.com/UWPR/Comet/pull/126)

**Full Changelog**: https://github.com/UWPR/Comet/compare/v2026.02.2...v2026.03.0
