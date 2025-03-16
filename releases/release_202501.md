### Comet releases 2025.01

Documentation for parameters for release 2025.01 [can be found 
here](/Comet/parameters/parameters_202501/).

Download release [here](https://github.com/UWPR/Comet/releases).

#### release 2025.01 rev. 1 (2025.01.1), release date 2025/03/15

- Bug fix: address issue #75 where a peptide index search using internal decoys would throw an error attempting to retrieve the previous/next flanking amino acid residues.
- Extend RTS DoSingleSpectrumMultiResult() to return semicolon delimited full protein descriptions instead of the current first word/accession string.
- Add actions to build ARM64 versions for both MacOS14 and Ubuntu 22.04 by @poshul in https://github.com/UWPR/Comet/pull/76

#### release 2025.01 rev. 0 (2025.01.0), release date 2025/02/19

- Added support for a second fragment neutral loss associated with the [variable_modXX](https://uwpr.github.io/Comet/parameters/parameters_202501/variable_modXX.html) parameters. The second neutral loss is specified as a comma separated number after the first neutral loss mass. For any fragment ion that contains the modification, Comet will analyze the intact fragment ion mass (including the modification) as well as the neutral loss ion(s).
- Address issue [#72](https://github.com/UWPR/Comet/issues/72) where a variable (iArraySizeGlobal) wasn't properly sized, thus causing memory issues during the search that manifested itself as a segfault at the end of the search when the memory was freed. Thanks to bfcrampton for reporting this issue.
- Restored peptide index support. Comet now supports 3 modes: (a) the classic mode of directly searching a FASTA file, (b) searching against a peptide index, and (c) searching using a fragment-ion index.  RTS searches through the CometWrapper.dll interface can query against either peptide index or fragment-ion index databases.
- Address a bug where PEFF modifications were being applied on the wrong amino acid on N-terminal peptides after start methionine removal with the parameter "[clip_nterm_methionine](https://uwpr.github.io/Comet/parameters/parameters_202501/clip_nterm_methionine.html)". Thanks to Z. Sun for reporting this bug.
- Address a bug where PEFF searches were segfaulting during the Post Analysis steps in Comet. Thanks to Z. Sun for reporting this bug.
- Known bug: Comet peptide index searches using internal decoys will fail (attempting to match a decoy peptide against the target sequences to pull out the previous/next flanking amino acids). This issue has been addressed with commit 8312ea6.
