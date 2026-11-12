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
favour the scatter. Two cheap estimates, in table entries:

- `ScatterWorkEstimate()`: the inverted-index entries under the spectrum's nonempty 100-bin
  rows, from the index offsets (one step per row: ~650 at 0.02 Da, ~14 at 1.0005 Da);
- `GatherWorkEstimate()`: the gather table entries of the rows the kernel will process -- for the
  groups covering the decoys needed at the query's charge limit, the rows before the early stop of
  section 6 (one binary search per group, shared with the kernel). (Before 2026-11-10 it was a
  prefix sum over whole groups.)

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
bit, the first ten mismatches (scan, charge, precursor MH+, array size, decoy count and charge limit, since RTS scans are all 0) and a summary on stderr -- which also counts the sets auto would give each path and the
sets per fragment-charge limit; the histogram uses the gather's scores), and
`verify-selftest` (as verify, the last score of the discarded scatter copy perturbed, so every comparison must fail while the search output stays unchanged).

## 3. Verification

- Verify mode, zero mismatches: `rts_repro` on the 5,000 HeLa scans (8 threads) -- plain 0.02
  Da (2,796 decoy sets), chimeric 0.02 Da (15,012), plain 1.0005 Da (4,404), chimeric 1.0005 Da
  (16,208), NL ions (2,796); batch `comet.exe` FI_DB on the full file at 0.02 Da (27,813) and
  1.0005 Da (38,516).
- Search output identical between `scatter` and the default in every timing run below.
- T87 (T67's fixtures plus a 4+ and a 5+ scan): zero mismatches under verify, every
  fragment-charge-limit table read (1-4), and identical results from scatter, gather, the default,
  verify and verify-selftest, for `rts_repro` (plain search, both chimeric forms; high resolution, low
  resolution, a/b/y ions at `max_fragment_charge` 4) and batch `comet.exe` on the FASTA, an FI_DB index
  and a PI_DB index. At low resolution auto takes the gather for most sets. A Windows `Comet.exe` runs
  the batch parts and the RTS path through `RealtimeSearch.exe chimericms2`, with the same counts and
  choices as Linux. T88 covers same-process re-initialization (section 5). Per-test detail:
  `tests/tests.md`.

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
limit `c` (the table for `c` holds the ions of charges 1 .. `c`, ~203,000 per charge for b/y, as
4-byte bins) plus one int per row (the row minimums, section 6). Measured (section 7): 4.9 MB + 0.6 MB
for b/y at `max_fragment_charge` 3, on top of 6.1 MB for the bin table and the inverted index; built
once per process.

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
- **Batch searches prepare the tables at the start of `DoSearch()`** (when an output needs E-values), as RTS
  does at init, so a same-process re-init with different ion parameters is refused whether or not any query in
  the run happens to need decoys (a search that finds no candidates never reached the check).
- **Tests (2026-11-10).** T87 covers both chimeric forms, every charge-limit table (a 4+ and a 5+ scan are added
  to T67's fixtures), batch FASTA, FI_DB and PI_DB, the auto choice, and a Windows `--comet`; T88 re-initializes in
  one process (`tests/unit/reinit_driver.cpp`, built with AddressSanitizer on the decoy code) -- see `tests/tests.md`.

## 6. Early stop (2026-11-10, code review)

A lane of the gather table is sorted by bin and padded with the highest bin, so the lowest bin of a
row (stored per row, `s_gatherRowMin`) never decreases down a group. For a query, the rows from the
first one whose lowest bin reaches its `iArraySize` hold only zeros (the dense copy has none at or
above it), so the kernel stops there: exact, since the skipped terms are `+0.0f`. Each group's stop
is a binary search, computed once per query (`GetGatherStops()`) and shared by the auto rule -- whose
gather estimate is now the entries of the rows actually processed, so the 0.4 factor keeps its meaning
-- and the kernel. The per-row minimum adds ~12% to the gather tables; the cumulative-entries array
is gone.

Verify (zero mismatches): `rts_repro` on the 5,000 HeLa scans, 8 threads -- plain (2,796 decoy sets),
chimeric (15,012), low resolution (4,404), low-resolution chimeric (16,208), NL ions (2,796); batch
FI_DB on the full file at 0.02 Da (27,813) and 1.0005 Da (38,516). Output identical to the previous
build in every RTS run and in the 1-thread batch runs.

Search time, 1 thread, 5,000 scans, before / after (two alternating runs; low-resolution chimeric one):

| run | before | after |
|---|---|---|
| plain | 1,272 / 1,278 ms | 1,139 / 1,138 (-10.5%) |
| NL ions | 1,755 / 1,694 | 1,572 / 1,577 (-9%) |
| chimeric | 18,289 / 17,964 | 17,584 / 17,424 (-3.5%) |
| low resolution | 2,334 / 2,355 | 2,274 / 2,338 (-1.5%) |
| low-resolution chimeric | 87,437 | 88,661 (+1.4%, one run) |

The decoy step is a slice of a search, so these are far below the per-call gain. The auto factor was
re-checked against the cheaper gather (high resolution, two rounds): plain scatter 1,278 / 1,308,
gather 1,135 / 1,119, auto 1,103 / 1,128; chimeric scatter 17,720 / 17,723, gather 18,275 / 18,220,
auto 17,322 / 17,303 ms. Auto beats both forced paths, and forced gather is still slower than
scatter for the chimeric run, so the factor stays at 0.4.

## 7. Table size and build time (2026-11-10, code review)

Measured with temporary instrumentation (not kept) on the decoy tables, one build per process (the
b/y, 1 row's bin table and inverted index follow from the charge-3 row: the bin table and the entries
scale with the charge limit, the index's start array does not):

| ions, `max_fragment_charge` | bin table | inverted index | gather tables (+ row minimums) | gather build before -> after |
|---|---|---|---|---|
| b/y, 1 | 1.0 MB | 1.6 MB | 0.8 MB (+0.1) | 4.7 -> 1.3 ms |
| b/y, 3 | 2.9 MB | 3.2 MB | 4.9 MB (+0.6) | 21.0 -> 8.1 ms |
| b/y, 5 | 4.8 MB | 4.9 MB | 12.2 MB (+1.5) | 45.8 -> 19.8 ms |
| a-z, 3 | 8.6 MB | 8.1 MB | 14.7 MB (+1.8) | 75.2 -> 22.7 ms |
| a-z + z1, 5 | 16.8 MB | 15.0 MB | 42.7 MB (+5.3) | 181.9 -> 59.3 ms |

The table for limit `c` repeats the ions of every lower charge, so the gather tables grow as
about C(C+1)/2 times the ions per charge. The build used to collect each decoy's ions in per-decoy
vectors (~28 MB transient for the last row) and sort 3,000 of them. It now fills the tables from the
inverted index: walking its bins in ascending order visits each decoy's ions in ascending bin order,
so the lanes are filled by transposition with no per-decoy lists and no sort. The tables are
byte-identical to the old build's (hash of all four table arrays, five configurations above) and the
build is 2.6-3.1 times faster.

Not done: building the tables for limits above 3 lazily, or serving those queries (5+ precursors, ~1%
of the HeLa queries) from the scatter, would cut the memory of heavy configurations by up to 60%
(`max_fragment_charge` 5), but changes nothing for the default of 3 and the largest case is 43 MB
against a multi-GB index, so the tables stay eager.

## 8. Table bookkeeping (2026-11-10, code review)

Each per-charge table now has one row-offsets vector (`s_gatherRowOff[c]`, one entry per group plus
a sentinel holding the total) where it had a start and a length per group (and, until section 6, a
cumulative-entries array): a group's rows are the range between two consecutive offsets, a row's
bins are `s_gatherBin[c][row * 8 ..]`, and the row minimums are indexed by row. The group count
(`GatherGroups()`) and the query's charge limit (`DecoyChargeLimit()`) are computed in one place each.
The scatter takes its charge limit from the same helper, so the two paths agree for any input; the
limit is a query's `usiMaxFragCharge`, 1 .. `max_fragment_charge`, so the clamp inside it never acts
on real queries (it is a guard: a limit below 1 used to give the scatter no ions and the gather
charge-1 ions). Scores are unchanged: the 5,000-scan HeLa outputs equal the previous build's and
verify reports zero mismatches.
