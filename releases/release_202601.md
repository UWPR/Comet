### Comet releases 2026.01

Documentation for parameters for release 2026.01 [can be found 
here](/Comet/parameters/parameters_202601/).

Download release [here](https://github.com/UWPR/Comet/releases).

#### release 2026.01 rev. 0 (2026.01.0), release date 2026/02/03

- Compute deltaCn for the decoy results if the decoy search is separate, by @fabianegli in [https://github.com/UWPR/Comet/pull/94](https://github.com/UWPR/Comet/pull/94)
- Make idx files independent as before (no requirements of keeping fasta around), by @jke000 in [https://github.com/UWPR/Comet/pull/100](https://github.com/UWPR/Comet/pull/100). Thanks to C. McGann for the request.
- Minor code changes to facilitate Crux integration.  Note: Crux-Comet runs 60% slower than standalone Comet under Linux.