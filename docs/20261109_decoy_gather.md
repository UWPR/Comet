# Decoy-score gather for the E-value histogram (2026-11-09)

Branch `RTSchimeric`. Code: `CometSearch/CometPostAnalysis.cpp` (`GenerateXcorrDecoys()`,
`DecoyScoresScatter()`, `DecoyScoresGather()`, `ScatterWorkEstimate()`, `GatherWorkEstimate()`,
the gather tables built in `InitPrecomputedDecoyBins()`). Test: T87.

## 1. Problem

When a query scores fewer than `EXPECT_DECOY_SIZE` (3,000) candidates, `CalculateEValue()`
calls `GenerateXcorrDecoys()` to score synthetic decoy peptides (`CometDecoys.h`) against the
spectrum and pad the xcorr histogram. The original code is a scatter: for every nonzero bin of
the spectrum's fast-xcorr matrix it adds the bin's value to every decoy with an ion there,
through an inverted index (bin -> decoy ions, ~400,000 entries below a typical precursor).

At low resolution the E-value step dominated: on the first 5,000 MS2 scans of
`20170103_HelaQC_01` (`rts_repro`, one thread; `docs/20261105_RTSchimeric.md` section 10.8) it
cost 448 us per call at `fragment_bin_tol` 1.0005 against 82 us at 0.02 Da -- 46% of a
low-resolution search. Counters in a scratch build showed why:

| | 0.02 Da | 1.0005 Da |
|---|---|---|
| binned array | 65,134 bins | 1,385 bins |
| nonzero bins | 14% | 73% |
| index entries visited per decoy generation | 78,000 of 399,000 | 345,000 of 407,000 |
| time per decoy generation | 130 us | 475 us |

The cost per entry was the same (1.4-1.7 ns, one random write per entry); at low resolution the
peak bands cover nearly the whole array, so the scatter visits nearly every decoy ion.

## 2. Design

**Gather.** Each decoy's score is the sum of the spectrum's values at its own ion bins. The
scatter adds to a decoy in ascending bin order (and, within a bin, in the inverted index's
per-decoy flat order), so if each decoy's ions are presorted by (bin, flat) the gather adds the
same floats in the same order: **bit-identical scores**, and so identical histograms and
E-values. Bins at or above the query's `iArraySize` and charges above its `usiMaxFragCharge`,
which the scatter skips, contribute `0.0f` in the gather, and adding `0.0f` leaves a float sum
unchanged.

**Layout.** A first version stored each decoy's ions with their charges and summed eight decoys
at a time with per-lane bounds and charge checks; it was slower than the scatter (607 vs 443 us
per call at 1.0005 Da) -- three dependent loads and selects per element. The kept version
precomputes, once per process, one table per fragment-charge limit `c` (ions with charge <= `c`
only), lane-interleaved in groups of eight decoys, short lanes padded with a bin index (the pad
bin: one past the highest decoy ion bin) that the dense spectrum copy keeps at zero. The inner loop is then one
index load, one value load and one add per lane, with eight independent accumulators. The dense
copy is allocated once per thread; only the rows the previous query copied are cleared.

**Choosing per query (auto, the default).** The gather's cost follows its table size, the
scatter's the spectrum's nonzero bins, so neither wins everywhere: at 0.02 Da the chimeric
search's 3+/4+ candidates (charge limit 3, a three-times larger table) and its stripped spectra
favour the scatter. Two O(1)-ish estimates, in table entries:

- `ScatterWorkEstimate()`: the inverted-index entries under the spectrum's nonempty 100-bin
  rows, from the index offsets (one step per row: ~650 at 0.02 Da, ~14 at 1.0005 Da);
- `GatherWorkEstimate()`: the gather table entries of the groups covering the decoys needed at
  the query's charge limit (a prefix sum per table).

The gather runs when the scatter estimate exceeds 0.4x the gather estimate. Per-query timings
of both paths (scratch build, the 5,000 HeLa scans, one thread) set the factor:

| run | scatter only | gather only | auto at 0.4 |
|---|---|---|---|
| plain, 0.02 Da | 124 us | 123 us | 96 us |
| chimeric, 0.02 Da | 110 us | 195 us | 98 us |
| plain, 1.0005 Da | 450 us | 57 us | 57 us |
| chimeric, 1.0005 Da | 402 us | 141 us | 141 us |

(decoy generation per query; factors 0.3-0.5 are within a few us of each other everywhere.)

**Switch.** `COMET_DECOYGEN` (environment, read once; not a parameter): unset / `auto`,
`scatter`, `gather`, `verify` (both paths on every decoy generation, scores compared bit for
bit, first mismatches and a summary on stderr; the histogram uses the gather's scores), and
`verify-selftest` (as verify, the last score of the discarded scatter copy perturbed, so every comparison must fail while the search output stays unchanged).

## 3. Verification

- Verify mode, zero mismatches: `rts_repro` on the 5,000 HeLa scans (8 threads) -- plain 0.02
  Da (2,796 decoy sets), chimeric 0.02 Da (15,012), plain 1.0005 Da (4,404), chimeric 1.0005 Da
  (16,208), NL ions (2,796); batch `comet.exe` FI_DB on the full file at 0.02 Da (27,813) and
  1.0005 Da (38,516).
- Search output identical between `scatter` and the default in every timing run below.
- T87: verify, verify-selftest, and identical results for all modes on T67's fixture (RTS at
  high and low resolution and with NL ions; batch FI_DB).

## 4. Results

`scatter` (the previous code) vs the default, output identical in every row; RTS: 5,000 HeLa
scans, one thread; batch: the full file, one thread.

| run | scatter | auto |
|---|---|---|
| RTS plain, 1.0005 Da | 4.22 s | 2.44 s (-42%) |
| RTS chimeric, 1.0005 Da | 97.8 s | 90.0 s (-8%) |
| RTS plain, 0.02 Da | 1.38 s | 1.32 s (-4%) |
| RTS NL ions, 0.02 Da | 1.86 s | 1.76 s (-5%) |
| RTS chimeric, 0.02 Da | 18.81 / 19.07 / 18.75 s | 18.67 / 18.78 / 18.67 s (-0.9%, alternating repeats) |
| batch FI_DB, 1.0005 Da | 933 spectra/s | 1,585 (+70%) |
| batch FI_DB, 0.02 Da | 2,132 spectra/s | 2,172 (+2%) |

Memory: the gather tables add one lane-interleaved copy of the decoy ions per fragment-charge
limit `c` (the table for `c` holds the ions of charges 1 .. `c`, ~240,000 per charge, as 4-byte
bins): roughly 6-7 MB for `max_fragment_charge` 3, built once per process with the existing
tables.

## 5. Table bound, build time, and re-init (2026-11-10, code review)

- **Tables independent of `iArraySizeGlobal`.** The inverted index and the gather tables used to
  be cut at the first session's `iArraySizeGlobal` (the pad bin equalled it), and the gather's
  per-thread dense buffer was sized from that. A same-process re-init with a larger mass range
  (a higher `digest_mass_range` ceiling or a wider tolerance) left the buffer too small for the
  bigger queries' rows: `memcpy` wrote past it (AddressSanitizer: heap-buffer-overflow on the
  review's two-session driver, gather forced; the scatter stayed in bounds through
  `b >= iInvIdxBins`). The bound is now one past the highest decoy bin (`s_iGatherPadBin`),
  whatever the session's array size, and the dense copy takes only bins below
  `min(iArraySize, s_iGatherPadBin)`: no decoy ion lies above, so scores are unchanged, and the
  same pair of sessions is clean under AddressSanitizer with E-values equal to a fresh process
  and to the scatter. Each row is also recorded before it is copied, so a failed allocation
  cannot leave a dirty row unrecorded.
- **Build at session start.** `CometPostAnalysis::PrepareDecoyTables()` (the `call_once` build
  plus the stale-parameter check) is called at the end of `InitializeSingleSpectrumSearch()`;
  `GenerateXcorrDecoys()` calls it too, so batch still builds on first use. The first RTS
  spectrum no longer pays for the build against `max_index_runtime` (it takes tens of ms with
  many ion series or a high fragment-charge limit), and a same-process re-init with different
  ion parameters now fails at init rather than on the first spectrum.
- **Scatter mode builds no gather tables** (`COMET_DECOYGEN=scatter`).
