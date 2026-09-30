# Full-scale Carafe pipeline, parquet mode: production phospho search space (2026-08-30)

> **PARTIAL RECONSTRUCTION (2026-09-03).** The original of this document was written and
> maintained only as an uncommitted file in the `Comet-master2` checkout, which was
> deleted 2026-09-03; no other copy existed. Sections 1-2 below are verbatim fragments
> recovered from session context; the run-outcome material is re-summarized from the
> surviving measurement data in `carafe_phos_parquet_20260830/meas/` + `.prerun/` logs.
> The original's Sections 3-5 (execution narrative, per-stage result tables, planned
> byte-equivalence outcomes) are lost except where quoted below.
>
> **Also superseded (2026-09-02):** the manuscript's phospho search space was redefined
> before this run's outputs were ever used -- single flavor `comet.params.phospholarge`
> (STY-phospho with NL + M-ox, `peptide_length_range = 7 35`, `allowed_missed_cleavage =
> 1`, `digest_mass_range = 700.0 5000.0`, 46,588,597 variants), re-run in
> `carafe_phospholarge_parquet_20260902/`. This run's ~150GB of outputs were deleted
> 2026-09-02. See `docs/20260831_carafe_paper.md` Sections 9-10. This doc remains the
> methodology reference for the harness and the 124.9M-variant-scale observations.

## 1. Purpose (recovered verbatim)

Run the complete Carafe ahead-of-time pipeline (`tools/carafe.py prerun`) end to end on the
full, production-scale phosphoproteomic search space -- the 124,863,304-variant
`phospho_charge2` population of `docs/20260805_carafe.md` / `docs/20260826_carafe.md` -- in
**parquet transient mode**, which was adopted in `docs/20260826_carafe.md` Section 2.5 on
per-chunk measurements but (deliberately -- see the M4 milestone note there) never before
executed at full scale. Every stage is run **strictly sequentially on an otherwise-idle
machine** and individually instrumented for wall time, peak memory, disk high-water mark,
output file sizes, and throughput, for publication use.

Two secondary goals:

1. **Replace the earlier, partially-contaminated full-scale timing numbers.** The original
   TSV-mode full-scale run's stage timings were measured opportunistically (some stages ran
   concurrently with unrelated work -- see `docs/20260826_carafe.md` Section 6.2.1 for the
   CPU-contention methodology finding at the smaller phosphosmall scale).
2. **Full-scale parquet-vs-TSV equivalence.** The retained TSV-era durable artifacts
   (`phospho_charge2_withNL.cps`, 31.1GB; `phospho_charge2_{withNL,noNL}_fromcps.fi_mask`,
   4.9GB each; both `.idx` files) are byte-comparison targets for this run's outputs:
   seeded-model determinism + the exact float32 round-trip (validated per-chunk in Section
   2.5 of `docs/20260826_carafe.md`) predict byte-identical stores from the same machine.
   [The original's Section 5 recorded the planned outcome; the comparison was never
   completed -- the run's outputs and the config itself were superseded 2026-09-02.]

## 2. Configuration (recovered verbatim through the transient-format row)

Identical to the production `phospho_charge2` population except for `--parquet`:

| Item | Value |
|---|---|
| FASTA | `20260420-human-phosho/human.canonical.target-decoy.fasta` (27,606,405 bytes; decoys included as real raw peptides) |
| Flavor params (withNL) | `phospho_idx_build_withNL.params` (`variable_mod02 = 79.966331 STY 0 3 -1 0 0 97.976896`) |
| Flavor params (noNL) | `phospho_idx_build_noNL.params` (identical except neutral-loss delta `0.0`) |
| Common mods | `variable_mod01 = 15.9949 M 0 3`, `max_variable_mods_in_peptide = 3` |
| Digest | trypsin, 2 missed cleavages, length 7-50, mass 700-5000 |
| Charges (inference) | `--charges 2` (production convention; mask is charge-independent at search time) |
| Chunking | `--chunk-size 50000` (2,498 chunks expected at 124.86M rows) |
| Transient format | **`--parquet`** (zstd; `ai_pred.py --fast`) -- the one deviation from the TSV-era run |

Additional recovered result fragments from the original's stage table:

- Stage 2a, variant export withNL (`comet.exe -x`): 1,951s (32.5 min), 3.58GiB peak;
  `withNL.variants_export.tsv` 12,129,565,607 B -- 124,863,304 variants (exact match to
  the production population). "wrote 12.13GB / 124.9M variants in 1,951s (~6.2MB/s)".
- Stage 2b, variant export noNL: 1,841s (30.7 min), ~2.3GiB.
- Stage 3a, convert withNL (`idx_to_carafe.py`): 3,893s (64.9 min), 15.39GiB peak;
  `withNL.carafe_peptides.tsv` 9,492,385,700 B + variants map 3,886,497,122 B.
  (phosphosmall scale, 39.5M rows: 949s / 5.35GiB -- both scale ~linearly with rows.)

## 3. Run outcome (re-summarized 2026-09-03 from surviving meas/ data)

The run completed 2026-09-02 11:05:48 after one interruption: an OS reboot killed the
first predict attempt 2026-08-30 ~23:28 at chunk 534/2,498; predict was restarted from
scratch 2026-08-31 09:05 for a single uninterrupted wall-clock measurement (partial run-1
artifacts: `meas/run1_aborted_20260830/`). Headline numbers (details in `meas/`):

| Stage | Wall clock | Peak tree RSS | Notes |
|---|---|---|---|
| idx (bundles s1+s2+s3, both flavors) | 3:18:56 | 15.7GiB | per-flavor: idx build 49s/48s, export 32.5/30.7 min, convert 64.9/~64 min |
| predict | 48:59:46 | 2.5GiB | 2,498 chunks, ~50 -> ~100+ s/chunk (rate degraded over the run) |
| cps | 12:12 | 27.3GiB | `withNL.cps`, 30GB |
| mask | 48:15 | 32.8GiB | both flavors; `{withNL,noNL}.fi_mask`, 5,244,259,14x B each |

The `--stop-after idx|export|convert` bundling gotcha, the tree-RSS-vs-/usr/bin/time
reporting rule, and the harvest procedure are documented in
`docs/20260831_carafe_paper.md` (Sections 3, 7, 9-10).
