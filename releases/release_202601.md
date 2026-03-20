### Comet releases 2026.01

Documentation for parameters for release 2026.01 [can be found 
here](/Comet/parameters/parameters_202601/).

Download release [here](https://github.com/UWPR/Comet/releases).

#### release 2026.01 rev. 1 (2026.01.1), release date 2026/03/16
- Incorporated BS:thread_pool (C++ threads; replaces pthreads on Linux) by @jke000 in [#109](https://github.com/UWPR/Comet/pull/109).
- The restored peptide indexing now supports Comet's internal decoys.
- Bug fix: address wrong mass being returned by RTS fragment ion index searches in the 2026.01 rev. 0 release that did not take into account static modifications in the mass calculation.
- Known bug: reported RTS decoy proteins have the decoy tag repeated twice e.g. DECOY_DECOY_sp|P21675|TAF1_HUMAN instead of DECOY_sp|P21675|TAF1_HUMAN.
- Note v2026.01.1 was originally tagged from commit [9df704c](https://github.com/UWPR/Comet/commit/9df704c073559738e7148c45a165cdc29021893b) on 2026/03/16; it was retagged on 2026/03/20 to incorporate a small bug fix with commit [31e0634](https://github.com/UWPR/Comet/commit/31e063404b528e5b806b24a15b7bbd1a309e05b6).

#### release 2026.01 rev. 0 (2026.01.0), release date 2026/02/03

- Compute deltaCn for the decoy results if the decoy search is separate, by @fabianegli in [#94](https://github.com/UWPR/Comet/pull/94)
- Make idx files independent as before (no requirements of keeping fasta around), by @jke000 in [#100](https://github.com/UWPR/Comet/pull/100). Thanks to C. McGann for the request.
- Minor code changes to facilitate Crux integration.  Note: Crux-Comet runs 60% slower than standalone Comet under Linux.
- Known bug: the RTS fragment ion index returned peptide mass is wrong for any peptide that contains a static modification; the static modification mass is not accounted for. Everything else is correct otherwise; just the reported mass is wrong.

