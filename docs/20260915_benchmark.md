# RTS thread-sweep benchmark: FI vs. PI, FASTA decoys vs. Comet internal decoys (2026-09-15)

Follow-up to the published note
[20260729_RTS_2026021](https://uwpr.github.io/Comet/notes/20260729_RTS_2026021.html), whose
first table compared PI_DB and FI_DB real-time search (RTS) throughput on `v2026.02.1` against a
human canonical target+decoy FASTA. This note re-runs that sweep on the current `master`
(commit `70d7125e`, reports `Comet version "2026.02 rev. 3"`, the first build with FI_DB internal
decoys -- `docs/20260914_FI_internal_decoys.md`) in four configurations:

| Tag | Search mode | Database | Decoys |
|-----|-------------|----------|--------|
| FI target+decoy | FI_DB | `human.canonical.target-decoy.fasta` (40,908 entries) | in the FASTA (`DECOY_` prefix), `decoy_search = 0` |
| FI internal | FI_DB | `human.canonical.fasta` (20,454 entries) | Comet pseudo-reverse internal decoys, `decoy_search = 1` |
| PI target+decoy | PI_DB | `human.canonical.target-decoy.fasta` | in the FASTA, `decoy_search = 0` |
| PI internal | PI_DB | `human.canonical.fasta` | internal decoys, `decoy_search = 1` |

Every number below is a single wall-clock sample on shared desktop hardware. Differences of a
few percent between rows are within run-to-run noise (see `CLAUDE.md`'s note on
`TIMING_NOISE_TOLERANCE`).

## Environment and method

- CPU Intel Core Ultra 7 265K (8 P-cores + 12 E-cores, 20 threads), 64 GB RAM, Windows 11;
  driver run from a WSL bash session (same machine as the July 17 sweep).
- Query file `20240924_Hela_01.raw` (89,593 scans, 63,488 MS2 spectra), also used as the MS1
  reference file. Same file as the published note.
- `comet.params`: byte-for-byte the phospho params of the July sweep
  (`Comet-master/20260420-human-phosho/memprobe/fi/comet.params`) except for the two lines
  `database_name` and `decoy_search`. Trypsin, 2 missed cleavages, peptide mass 700-5000,
  length 7-50, static C+57.021464, variable M+15.9949 and STY+79.966331 (3 per residue type,
  3 total per peptide), 20 ppm precursor tolerance, 0.02 fragment bins.
- Indexes built with `Comet.exe -i` (FI_DB) or `Comet.exe -j` (PI_DB). Since
  `docs/20260811_restore_idx_header_mods.md` the `.idx` header is self-describing
  (`IndexSearchType:`, `DecoySearch:`, mods, enzyme, mass/length range), and RealtimeSearch.exe
  takes all of that from the header. Build time is 1-3 s for every index because the modified
  variants and the fragment index are regenerated at load; the target+decoy index is 289 MB
  (4.659e6 unmodified peptides) and the canonical one 144 MB (2.328e6).
- Driver: `RealtimeSearch.exe <raw> <raw> <idx> <threads> 1 <0=PI|1=FI>` for threads
  1, 2, 4, 8, 20, one process per thread count, sweeps run strictly sequentially with nothing
  else running. AScorePro on (as in July). Per-spectrum results in `<tag>.NN`, console in
  `<tag>.NN.run`. Scripts: `run_sweep_generic.sh` / `run_all_sweeps.sh` in the FI-internal
  directory listed under "Data" below.
- Metrics are the driver's own summary lines: "MS2 average search time" (wall-clock of the
  parallel phase / MS2 spectra), its Hz, and the final `Done. (X GB)` peak working set.
  Tail statistics are computed from the per-spectrum `N ms` field in `<tag>.NN`, exactly as
  `run2.out/results/generate_report.py` did for the published table.

## Table 1. Same binary (`master` 70d7125e), all four configurations

Header layout follows the first table of the published note (search threads, then one
column group each for average search time, average search speed and peak memory; here each
group has four sub-columns instead of two).

<table>
<thead>
<tr><th rowspan="2">Search<br />threads</th><th colspan="4">Avg. search time (ms)</th><th colspan="4">Avg. search speed (Hz)</th><th colspan="4">Peak memory (GB)</th></tr>
<tr><th>FI<br />target+decoy</th><th>FI<br />internal decoy</th><th>PI<br />target+decoy</th><th>PI<br />internal decoy</th><th>FI<br />target+decoy</th><th>FI<br />internal decoy</th><th>PI<br />target+decoy</th><th>PI<br />internal decoy</th><th>FI<br />target+decoy</th><th>FI<br />internal decoy</th><th>PI<br />target+decoy</th><th>PI<br />internal decoy</th></tr>
</thead>
<tbody>
<tr><td>1</td><td>0.63</td><td>0.63</td><td>7.60</td><td>7.70</td><td>1,594</td><td>1,588</td><td>132</td><td>130</td><td>18.4</td><td>18.1</td><td>5.2</td><td>2.7</td></tr>
<tr><td>2</td><td>0.33</td><td>0.33</td><td>3.87</td><td>3.87</td><td>3,071</td><td>3,073</td><td>258</td><td>258</td><td>18.4</td><td>18.1</td><td>5.3</td><td>2.7</td></tr>
<tr><td>4</td><td>0.16</td><td>0.17</td><td>1.96</td><td>1.96</td><td>6,067</td><td>6,024</td><td>510</td><td>510</td><td>18.5</td><td>18.1</td><td>5.3</td><td>2.8</td></tr>
<tr><td>8</td><td>0.09</td><td>0.09</td><td>0.97</td><td>0.97</td><td>11,360</td><td>11,516</td><td>1,028</td><td>1,032</td><td>18.5</td><td>18.2</td><td>5.3</td><td>2.8</td></tr>
<tr><td>20</td><td>0.07</td><td>0.07</td><td>0.40</td><td>0.40</td><td>13,690</td><td>13,630</td><td>2,502</td><td>2,470</td><td>18.6</td><td>18.3</td><td>5.4</td><td>3.0</td></tr>
</tbody>
</table>

## Table 2. Phase timings (s) and index statistics

"Init" is `initialize elapsed time` (read `.idx`, regenerate variants, and for FI_DB build the
in-memory fragment index). "Search" is the parallel-phase wall clock the Hz is derived from.

| Configuration | Threads | Init | Preload | Search | Total | FI variants / postings |
|---|---:|---:|---:|---:|---:|---|
| FI v2026.02.1 (Jul) | 1 / 8 / 20 | 64.4 / 65.4 / 65.0 | 4.0 / 4.1 / 4.1 | 46.8 / 7.1 / 5.0 | 115.5 / 76.9 / 74.5 | 1.249e8 / 3.700e9 |
| FI target+decoy | 1 / 8 / 20 | 82.4 / 30.8 / 24.5 | 3.9 / 3.8 / 3.6 | 39.8 / 5.6 / 4.6 | 126.4 / 40.4 / 33.0 | 1.249e8 / 3.540e9 |
| FI internal | 1 / 8 / 20 | 74.4 / 26.7 / 24.8 | 5.5 / 3.7 / 3.8 | 40.0 / 5.5 / 4.7 | 120.2 / 36.3 / 33.5 | 1.249e8 / 3.541e9 |
| PI v2026.02.1 (Jul) | 1 / 8 / 20 | 30.3 / 29.5 / 30.0 | 4.4 / 4.0 / 4.0 | 841.5 / 115.9 / 50.4 | 876.4 / 149.7 / 84.7 | n/a |
| PI target+decoy | 1 / 8 / 20 | 22.4 / 21.4 / 31.4 | 4.0 / 3.7 / 3.7 | 482.2 / 61.8 / 25.4 | 509.0 / 87.2 / 60.8 | n/a |
| PI internal | 1 / 8 / 20 | 11.2 / 10.8 / 11.1 | 4.1 / 3.8 / 4.0 | 488.8 / 61.5 / 25.7 | 504.4 / 76.4 / 41.0 | n/a |

## Table 3. Per-spectrum tail latency

Percent of scored MS2 spectra by per-call search time (`N ms` field, integer milliseconds).

| Configuration | Threads | Avg (ms) | Max (ms) | <=1 ms | <=5 ms | >10 ms |
|---|---:|---:|---:|---:|---:|---:|
| FI v2026.02.1 (Jul) | 1  | 0.74  | 17  | 96.67% | 99.96% | 0.02% |
| FI v2026.02.1 (Jul) | 20 | 0.08  | 32  | 74.42% | 97.88% | 0.35% |
| FI target+decoy     | 1  | 0.63  | 57  | 99.17% | 99.96% | 0.01% |
| FI target+decoy     | 20 | 0.07  | 111 | 78.99% | 98.39% | 0.31% |
| FI internal         | 1  | 0.63  | 14  | 99.12% | 99.97% | 0.00% |
| FI internal         | 20 | 0.07  | 35  | 78.48% | 98.39% | 0.29% |
| PI v2026.02.1 (Jul) | 1  | 13.25 | 406 | 11.15% | 48.64% | 29.72% |
| PI v2026.02.1 (Jul) | 20 | 0.79  | 459 | 7.87%  | 38.40% | 37.64% |
| PI target+decoy     | 1  | 7.60  | 335 | 42.58% | 73.63% | 16.34% |
| PI target+decoy     | 20 | 0.40  | 327 | 38.43% | 71.80% | 17.10% |
| PI internal         | 1  | 7.70  | 304 | 38.15% | 72.78% | 16.46% |
| PI internal         | 20 | 0.40  | 293 | 34.37% | 71.12% | 17.31% |

## Table 4. Result-population sanity check

Rank-1 rows written to `<tag>.NN` (identical across thread counts within a sweep) and how many
of them are `DECOY_`-prefixed.

| Configuration | Spectra with a reported hit | Rank-1 decoys | Decoy fraction |
|---|---:|---:|---:|
| FI v2026.02.1 (Jul) | 44,887 | 8,830  | 19.7% |
| FI target+decoy     | 44,461 | 8,533  | 19.2% |
| FI internal         | 44,489 | 8,494  | 19.1% |
| PI v2026.02.1 (Jul) | 61,944 | 17,288 | 27.9% |
| PI target+decoy     | 61,944 | 17,284 | 27.9% |
| PI internal         | 61,929 | 16,805 | 27.1% |

## Observations

1. **Internal decoys cost nothing in search speed.** At the same binary, FI internal vs. FI
   target+decoy agree to within 1-2% at every thread count (Table 1), as do PI internal vs. PI
   target+decoy. The fragment index has the same 1.249e8 variants and the same ~3.54e9 postings
   either way, so the FI search does identical work; PI_DB scores the same number of candidates
   because each target variant gets exactly one pseudo-reverse twin.
2. **Internal decoys save memory, a lot for PI_DB.** FI_DB drops ~0.3 GB (18.4 -> 18.1 GB): the
   fragment index dominates and is the same size, only the raw-peptide table and protein tables
   halve. PI_DB drops from 5.2-5.4 GB to 2.7-3.0 GB because its resident state *is* the peptide
   table, and initialization halves (22 s -> 11 s) for the same reason. Index files on disk also
   halve (289 MB -> 144 MB). See "Memory accounting (PI_DB)" below for the ledger.
3. **`master` vs. `v2026.02.1` on the same target+decoy FASTA.** FI_DB search is 8-26% faster
   (1,594 vs. 1,356 Hz at 1 thread; 11,360 vs. 8,991 at 8; 13,690 vs. 12,681 at 20) with ~2 GB
   less memory (18.4 vs. 20.2 GB); the in-memory index has 3.54e9 postings vs. 3.70e9 for the
   same variants. FI generation is now parallel, so Init falls from a flat ~64 s to 82/55/44/31/24 s
   across 1/2/4/8/20 threads (the 1-thread Init is slower than July's, 82 vs. 64 s). PI_DB search
   is 1.75-2.0x faster (7.60 vs. 13.25 ms/spectrum at 1 thread; 2,502 vs. 1,259 Hz at 20) with
   less than half the memory (5.2 vs. 11.7 GB), and its >10 ms tail shrank from 30-38% of spectra
   to 16-17%.
4. **Scaling.** FI_DB reaches ~11.4-11.5 kHz on the 8 P-cores and only ~13.6-13.7 kHz at 20
   threads, the same saturation the published note showed; the <=1 ms fraction drops from 99% to
   78% at 20 threads as the E-cores and shared caches come into play. PI_DB scales nearly linearly
   to 8 threads (7.8x) and 2.5x further to 20 threads (19x total).
5. **Decoy populations are close but not identical.** FASTA decoys and internal pseudo-reverse
   decoys are different sequence sets, so a small shift in the reported population is expected:
   the FI sweeps differ by 28 spectra with hits and 39 rank-1 decoys, the PI sweeps by 15 spectra
   and 479 rank-1 decoys (2.8% fewer decoys with internal decoys). This does not affect timing but
   should be kept in mind when comparing PSM counts at a fixed FDR between the two decoy modes.
6. **Outliers.** FI target+decoy shows single-spectrum maxima of 57-111 ms where every other FI
   sweep tops out at 14-35 ms; the >10 ms fractions are unchanged (0.01-0.31%), so these are one
   or two isolated stalls per run, not a shift in the distribution.

## Memory accounting (PI_DB)

Why PI target+decoy peaks at 5.2 GB where `v2026.02.1` peaked at 11.7 GB, and why internal
decoys halve it again. Ledger from the current binary with `COMET_MEMREPORT=1` set, logged at
the end of `ReadPeptideIndex()` (index-resident state, before any spectra are loaded):

| Structure | PI target+decoy | PI internal |
|---|---:|---:|
| `g_dbIndexVariants` (13 B/entry SoA) | 124,863,304 entries, 1,548 MB | 62,465,748 entries, 774 MB |
| `MOD_NUMBERS_POOL` (mod combinations) | 42,175,396 entries, 498 MB | 23,200,353 entries, 268 MB |
| `g_vRawPeptides` (pooled) | 4,658,764 entries, 197 MB | 2,328,462 entries, 99 MB |
| Protein lists, name cache, aux arrays | ~65 MB | ~32 MB |
| **Index-resident total** | **~2.3 GB** | **~1.15 GB** |

- **`v2026.02.1` (tag dated 2026-07-28) predates all of the PI memory work.** Its PI_DB held one
  flat 88-byte `DBIndex` record per modified variant, sequence text included: 1.249e8 x 88 B =
  10.24 GiB, measured 10.48 GB resident after load and 11.7 GB full-run peak
  (`docs/20260730_PI_reduction.md` Section 3). Since then: PR #121 (raw-peptide table + compact
  variant record, unified PI/FI `.idx`), PR #124 (variants and mod tables regenerated at load
  instead of persisted), and the `docs/20260827_PI_memory.md` phases (flat-pooled mod tables,
  13 B/entry SoA variant array, pooled raw table, `uint32` protein ordinals, streamed reader).
  That document projected ~2.34 GB index-resident for exactly this configuration; the ledger
  above measures ~2.3 GB.
- **The 5.2 GB peak is the load-time transient, not the steady state.** Variant generation sorts
  a 24 B/entry staging array alongside the 13 B/entry final array. On Windows the staging is
  plain `malloc` (the page-decommit variant was quarantined by the machine's AV, see
  `docs/20260827_PI_memory.md` Phase 2 notes), so the transient is ~37 B/entry:
  37 B x 124.9M = 4.6 GB, plus the raw table and mod pool, ~5.3 GB predicted. A batch search of
  a 130-spectrum file against the same `.idx` peaks at 5.1 GB, confirming the peak is the index
  load rather than search-phase state. During the RTS search phase itself the process holds the
  ~2.3 GB resident index plus preloaded spectra and thread scratch.
- **PI_DB stores no decoys.** With `decoy_search = 1` the variant array holds only the 62.5M
  target variants from the canonical FASTA; each candidate's decoy is pseudo-reversed on the fly
  in `AnalyzePeptideIndex()` (`CometSearch::PseudoReversePeptide()`). Every ledger line and the
  transient are therefore half-size (37 B x 62.5M + 0.1 + 0.27 GB = ~2.7 GB predicted; 2.7 GB RTS
  and 2.6 GB batch measured), while search time is unchanged because the same number of
  candidates is scored.
- **FI_DB saves only ~0.3 GB** from internal decoys because the posting list (4 B x 3.54e9
  entries, ~14 GB) dominates and is the same size either way, and `g_fragmentPeptides` keeps the
  decoy twins as flagged entries (1.249e8 either way); only the raw table, mod pool and protein
  lists halve. The FI drop from 20.2 GB (`v2026.02.1`) to 18.4 GB comes from fewer postings
  (3.70e9 -> 3.54e9) and the 24 -> 13 B variant record.

## Data

All runs are on the Windows drive, outside the repo:

| Configuration | Directory |
|---|---|
| FI target+decoy | `/mnt/c/Work/20260915-phospho-FI-targetdecoy/` (`FI-td.NN`, `FI-td.NN.run`) |
| FI internal | `/mnt/c/Work/20260915-phospho-FI-internaldecoy/` (`FI-internal.NN`, `FI-internal.NN.run`, `run_sweep.sh`, `run_sweep_generic.sh`, `run_all_sweeps.sh`) |
| PI target+decoy | `/mnt/c/Work/20260915-phospho-PI-targetdecoy/` (`PI-td.NN`, `PI-td.NN.run`) |
| PI internal | `/mnt/c/Work/20260915-phospho-PI-internaldecoy/` (`PI-internal.NN`, `PI-internal.NN.run`) |
| July `v2026.02.1` FI/PI | `/mnt/c/Work/Comet-master/20260420-human-phosho/run2.out/` (`FI.NN`, `PI.NN`, `*.run`, `results/summary_stats.csv`) |
| Query raw + FASTAs | `/mnt/c/Work/Comet-master/20260420-human-phosho/` (`20240924_Hela_01.raw`, `human.canonical.fasta`, `human.canonical.target-decoy.fasta`) |

Each 2026-09-15 directory also holds the exact `Comet.exe`, `RealtimeSearch.exe`,
`CometWrapper.dll`, `comet.params`, FASTA, `.idx`, index-build log (`*.idx.txt`) and
`sweep_console.log` used, so any row can be re-run in place.
