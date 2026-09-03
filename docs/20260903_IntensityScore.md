# Carafe predicted-intensity datasets available for intensity-score work (2026-09-03)

Context for this branch (`Comet-intensityscore`, branched off `carafe`): investigate
replacing/augmenting Comet's cross-correlation (XCorr) PSM score with a score that uses
Carafe-predicted fragment intensities. Two complete, production-quality prediction
datasets in **parquet format** already exist on this machine -- do not re-run inference
(hours of CPU) before checking these.

Both live under `/mnt/c/Work/Comet-master/20260420-human-phosho/` and were produced
2026-09-02/03 by `tools/carafe.py prerun` (same tooling as on this branch) against the
same FASTA: `human.canonical.target-decoy.fasta` (27,606,405 B, 40,908 sequences,
targets + decoys concatenated; decoy variants ARE included in the prediction input via
`--include-decoys`). Full measurement provenance (wall time, RSS, disk per stage) is in
each workdir's `meas/`; runbook: Comet-master `docs/20260831_carafe_paper.md` Sections
9-10.

## The two datasets

| | OxMet | Phospho large |
|---|---|---|
| Workdir | `carafe_oxmet_parquet_20260902/` | `carafe_phospholarge_parquet_20260902/` |
| Params file | `comet.params.oxmet.7-35` (as edited 2026-09-02) | `comet.params.phospholarge` |
| Variable mods | M+15.9949 only, max 3 | M+15.9949 + STY+79.966331 (NL 97.976896), max 3 total |
| Digest | trypsin, 1 missed cleavage, length 7-35, mass 700-5000 (both) | same |
| Peptide-mod variants (rows) | 3,760,672 | 46,588,597 |
| Chunks (50,000 rows each) | 76 | 932 |
| Carafe mode | `general` (4 fragment channels) | `phosphorylation` (8 channels, adds modloss) |
| `prediction/` tree size | 1.5 GB | 21 GB |
| Compact store (`<flavor>.cps`) | `oxmet.cps`, 530 MB | `phospholarge.cps`, 8.7 GB |
| Inference settings | charge 2, NCE 27, instrument Lumos, `tf_type ms2`, CPU, seeded/deterministic (both) | same |

## Parquet layout (identical structure in both)

`<workdir>/prediction/chunk_preds/chunk_NNNNN/` (zero-based, `.done` marker = complete):

- `chunk_NNNNN_ms2_df.parquet` -- one row per peptide-mod variant (50,000/chunk; last
  chunk short). Columns: `sequence`, `mods` (alphabase names, e.g. `Oxidation@M`,
  `Phospho@S`, `;`-joined), `mod_sites` (1-based residue positions, `;`-joined),
  `charge` (always 2), `instrument`, `nce`, `nAA`, and **`frag_start_idx`/
  `frag_stop_idx`** -- the half-open row range into the two fragment tables below
  (`nAA-1` rows per peptide, one per backbone cleavage position, N-terminal fragment
  ordering: row j = b(j+1)/y(nAA-1-j)).
- `chunk_NNNNN_ms2_mz_df.parquet` -- fragment m/z, float. Columns `b_z1 b_z2 y_z1 y_z2`
  (OxMet) plus `b_modloss_z1 b_modloss_z2 y_modloss_z1 y_modloss_z2` (Phospho; the
  -97.976896 phospho neutral-loss series; 0 where no mod loss applies).
- `chunk_NNNNN_ms2_pred.parquet` -- **predicted relative intensities**, float32,
  row/column-aligned 1:1 with `ms2_mz_df`, normalized per peptide (max ion = 1.0).
- `chunk_NNNNN_rt_pred.parquet` -- per-peptide `rt_pred`/`rt_norm_pred` (0-1) and
  `irt_pred` (iRT scale).

Read with `pandas.read_parquet` -- pyarrow is installed in the Carafe venv:
`~/.carafe/.venv/bin/python3` (do NOT pip-install a new environment; see the
`project_carafe_python_env` memory / CLAUDE.md).

## Mapping predictions to Comet peptide-index variants

Global `row_index = chunk_number*50000 + row_within_chunk`, in exactly the line order of
`<flavor>.carafe_peptides.tsv` (columns: sequence, mods, mod_sites, charge). The
companion `<flavor>.carafe_peptides.variants.tsv` maps each `row_index` to Comet's
peptide-index identity: `iWhichPeptide`, `modNumIdx`, `cNtermMod`, `cCtermMod` (-1 = no
mod), under a `# VarModConfig:` header naming the exact variable-mod slot config the
`.idx` was built with. **The variant map's enumeration order is NOT row_index order --
always sort/merge, never assume alignment** (hard-won invariant; see
`tools/carafe_cps_to_fi_mask.py` for a correct consumer). The matching FI_DB index is
`<flavor>.fasta.idx` in the same workdir, built from the same params file.

## Alternative access paths

- `<flavor>.cps` -- compact prediction store: the same fragment intensities u16-quantized
  (~0.009% divergence), keyed for merge against the variant map, ~12x smaller than raw
  parquet and the *durable* artifact (the `prediction/` trees are transient by
  convention and may be deleted later -- check existence; the `.cps` is what survives).
  Reader/format: `tools/carafe_cps.py` on this branch.
- `<flavor>.fi_mask` -- the existing downstream product (top-predicted-peak masks for FI
  trimming, >=10% relative intensity / >=6 peaks; `oxmet.fi_mask` 151 MiB,
  `phospholarge.fi_mask` 1.82 GiB). Relevant as prior art: the masking work proved these
  predictions improve PSM yield when used to *select* fragments; the intensity-score
  work would use the intensity *values* themselves.
- `rt_pred` parquet files -- predicted retention times per variant are also available
  (see layout above) if RT ever becomes a rescoring feature.

## Repo state -- do this before starting

This checkout (`IntensityScore` branch) is at `cfd94d76`, **behind the carafe head
`ddcd4280`** (pushed on `origin/carafe`). The missing commits include `2738ef25`, a large
memory-optimization merge that heavily rewrites `CometSearch/CometSearch.cpp`,
`CometFragmentIndex.{cpp,h}`, `CometPeptideIndex.{cpp,h}`, and `core/Types.h` -- exactly
the files scoring work modifies. **Merge carafe (`ddcd4280`) into this branch first**;
doing it after writing score code means painful conflicts in those files. (The
now-deleted `Comet-master2` checkout is gone; `/mnt/c/Work/Comet-master` is the only
carafe working copy and is already at `ddcd4280`.)

## Where the current score lives / benchmarking resources

- XCorr scoring and FI querying: `CometSearch/CometSearch.cpp`; SP score, E-value,
  delta-Cn: `CometSearch/CometPostAnalysis.cpp`; spectrum preprocessing/binning that
  feeds XCorr: `CometSearch/CometPreprocess.cpp`. The `comet-codebase` skill maps the
  rest.
- Query spectra for benchmarking live in the same `20260420-human-phosho/` directory:
  `20240924_Hela_01.raw` (62,576 MS2 scans -- the file the oxmet RTS validation used),
  several other Hela `.raw`, plus Linux-readable `.mzXML` (`20170103_HelaQC_01.mzXML`,
  `MM2_R1/R2.mzXML` -- the MM2 pair is the phospho-enriched dataset).
- PSM-quality metric: `tools/qvalue.py` (rank-1 PSMs only, q<=0.01 convention; `--diff`
  compares two result files). Baseline to beat/complement: Carafe *masking* alone gave
  +3.7-6.2% PSMs at 1% FDR over unmasked FI on four flavor/dataset pairings
  (`docs/20260826_carafe.md` Section 6.22).
- Pipeline cost context (how expensive regenerating predictions is): Comet-master
  `docs/20260831_carafe_paper.md` Section 11 -- inference was 53.7 min (oxmet) and
  18h39m (phospholarge) on this machine's CPU.

## Caveats for score-function work

- Predictions are charge-2 precursors only (`--charges 2`); fragment channels cover
  z1+z2. Scoring z>=3 precursors will need either new inference or an explicit
  approximation decision.
- Intensities are per-peptide relative (max=1.0), not absolute or library-normalized.
- OxMet lacks modloss channels entirely (mode `general`); Phospho's modloss columns are
  meaningful only where an STY-phospho sits on the fragment's side of the cleavage.
- Everything is CPU-inferred with a fixed seed -- re-running inference with identical
  inputs on this machine reproduces byte-identical parquet (validated at scale on the
  carafe branch; see Comet-master `docs/20260826_carafe.md` Section 2.5).
