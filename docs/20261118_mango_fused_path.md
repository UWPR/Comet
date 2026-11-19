# Mango on the fused batch search path

2026-11-18

## Summary

Mango (`mango_search = 1`) batch searches against a fragment-ion (FI_DB) or peptide
(PI_DB) index now use the fused execution path (`FusedLoadAndSearchSpectra` ->
`FusedSearchSpectrum`) like every other non-speclib database search. Before this change
`FiStrategy::executeBatch()` and `PiStrategy::executeBatch()` kept Mango on the legacy
three-sweep path with `!bMango`.

FASTA_DB has no fused path (`FastaStrategy` is always three-sweep) and spectral-library
runs still use the legacy path, so neither is affected.

## Why the gate existed and why it is no longer needed

`docs/20260611_batchFIoptimization.md` excluded Mango because the old Mango code assigned
the 000_A / 000_B / 001_A ... pair labels *after* the batch-wide sort, from a query's
position among the queries that survived the mass/charge filter (`Query::dMangoIndex`,
`compareByMangoIndex()`). That needed every query of the batch in one list.

The label is now set on the Query when it is created, from the Z-line ordinal
(`SetMangoLabel()` in `CometPreprocess.cpp`, `BuildMangoChargeStates()` builds the charge
list and the ordinals). Nothing depends on batch-wide ordering any more: the Query is
self-describing, and the per-batch by-scan sort uses the label as its last tie-break
(`compareByScanNumber`), so output order is fixed no matter which thread finished first.
A Mango pair's two queries come from the same spectrum, so they always land in the same
fused flush.

`FusedSearchSpectrum` already carried the same Mango logic as `PreprocessSpectrum` (it was
kept in sync); it was unreachable until the gate was removed.

## Measurements (before the change was made)

Same source built twice, with and without the gate, on 16 and fewer threads; human
canonical target-decoy plain peptide index (FI_DB), 20,000 MS/MS scans of
`20170103_HelaQC_01` converted to pseudo-Mango `.ms2` (S-line m/z 0.0, two Z lines per scan
carrying the scan's own precursor, so 40,000 queries). Wall time includes the 2-3 s index
load.

| Threads | Legacy | Fused | Gain |
|---|---|---|---|
| 16 | 5.10 s | 3.92 s | 1.3x |
| 8 | 6.95 s | 4.95 s | 1.4x |
| 4 | 12.71 s | 8.17 s | 1.6x |

Peak RSS at 16 threads was 2.28 GB fused vs 1.77 GB legacy on this small file (the fused
path's per-thread arenas); its memory advantage comes from flushing and only shows on
large files. Not measured here.

Output equivalence: the `.txt` was identical line for line including row order, and the
pepXML identical apart from header timestamps/paths, on that data (about 40 flush
boundaries at 1000 queries per flush), on three repeat fused runs, and on a synthetic
6000-scan file mixing five Mango scan shapes (including a charge-0 pair, an unpaired Z
line and a Z line above `digest_mass_range`) for FI_DB and PI_DB with
`fragindex_skipreadprecursors` 0 and 1 at 8 threads.

## Real Mango data

Two real Mango `.ms2` files (a 2 h and a 4 h acquisition of one fraction: 1330 and 2053 scans, 6418 and 10422 Z lines; S-line m/z 0.0, four Z lines per scan), searched
against a small protein FASTA with the production-style `comet.params`
(`mango_search = 1`, isotope_error 4, variable mods incl. K 197.03, pepXML output; 8
threads). The binary built before this change (legacy) was compared with the one built after
it (fused):

| Database | Legacy | Fused | `.txt` (rows, order) | pepXML |
|---|---|---|---|---|
| FASTA_DB (control: no fused path) | 5.66 s | 5.52 s | identical | identical |
| FI_DB | 2.99 s | 1.38 s | identical | identical |
| PI_DB | 4.14 s | 2.75 s | identical | identical |

Peak RSS: FI 685 MB legacy vs 917 MB fused; PI 603 vs 833 MB. These files are small, so the
flush-based memory saving does not show; per-thread arenas dominate.

Determinism: the fused FI_DB output was byte-identical to a 1-thread legacy run on every one
of 15 runs (2, 4, 8, 16 and 20 threads, 3 runs each). The files contained no Z lines the
Mango skipping logic rejects (0 warnings).

The pseudo-Mango HeLa timing above (before this was available) remains the only
large-file measurement.

Not covered by real data: scans with a charge-0 Z line, an unpaired Z line or several S-line
m/z values (none occur in these files); those are covered by T91b/T91g/T91h only.

## Tests

- `t91_mango_fi` (T91e): FI_DB, `fragindex_skipreadprecursors = 0`.
- `t91_mango_pi` (T91f): the same for a PI_DB index.
- `t91_mango_skip_fi` (T91g): the pair/scan skipping and warnings on the fused path.
- `t91_mango_threads` (T91h): 3000 generated scans of mixed shapes, FI_DB, 8 threads;
  checks every row and the global row order. `t91_mango_threads_pi` (T91i) and
  `t91_mango_threads_fasta` (T91j) run the same scans on PI_DB and FASTA_DB, which must give
  the same rows. Mango supports all three database types; FASTA_DB (the historical Mango
  path) has no fused path and stays three-sweep.

There is no longer a way to run Mango through the legacy path to diff against, so the
tests assert the expected rows directly.

## Related Mango changes made alongside

- Mango ignores the S-line m/z: every Z line, the first included, takes its precursor from its
  own MH+ (`BuildMangoChargeStates()`, and `ReadPrecursors()` for the precursor mask). Real
  Mango files always write 0.0 there; tests `t91_mango_smz` / `t91_mango_smz_fi` use a wrong
  non-zero value to pin the behaviour.
- All MH+ <-> m/z conversions go through the global `MhToMz()` / `MzToMh()` in
  `core/Constants.h`. The neutral-mass forms ((M + z*PROTON)/z) and the neutral-loss forms
  are intentionally not folded in: they round differently, so replacing them would not be
  bit-identical.

## Build note

`make` does not rebuild when only `CometSearch/search/*.cpp` changed; use
`make cclean && make` (a first rebuild after this kind of edit silently left the old
binary in place during the measurements above).
