# Recipe: TMTpro predicted-fragment mask and masked real-time search (ion-trap CID MS2)

**Date:** 2026-09-22, revised 2026-09-30 to match the manuscript run
**Branch:** carafe (commit `2cc05b52` or later)
**Purpose:** every command needed to go from a protein FASTA and a TMTpro `comet.params` to a `.fi_mask`, and then to
masked and unmasked real-time searches (RTS) of ion-trap CID MS2 data, as run for the Comet + Carafe technical brief
(TMTpro 16-plex, Orbitrap Ascend RTS-MS3, PXD043660). All steps run natively on Windows (PowerShell); the Python tools
also run unchanged under Linux.

Companion docs: `docs/20260826_carafe.md` (pipeline design and full-scale results) and `docs/20260805_carafe.md` (the
masking feature). This document prescribes commands; it does not re-explain the design.

---

## 0. Pipeline at a glance

```
comet.exe -i          FASTA + params        ->  NAME.fasta.idx                          [s1]
comet.exe -x          .idx                  ->  NAME.variants_export.tsv                [s2]
carafe.py convert     .idx + export         ->  NAME.carafe_peptides.tsv (+ .variants.tsv) [s3]
carafe.py predict     peptide TSV           ->  prediction\chunks, prediction\chunk_preds [s4]  (the expensive stage)
carafe.py cps         chunk predictions     ->  NAME.cps                                [s5]  (durable artifact)
carafe.py mask        .idx + variants + cps ->  NAME.fi_mask                            [s6]
RealtimeSearch.exe    .idx [+ .fi_mask]     ->  rts.out  -> rts_out_to_txt.py -> qvalue.py / mokapot
```

`carafe.py prerun` runs s1-s6 as resumable stages (`OUT\.prerun\<stage>.done` markers; re-running skips finished stages).
The peptide list always flows from Comet to Carafe: the mask builder joins Carafe's echoed rows back to
`NAME.carafe_peptides.tsv` on the exact (sequence, mods, mod_sites, charge) tuple, so predictions made from any other
peptide enumeration cannot be used. A variant with no mask entry falls back to its full, unfiltered fragment set.

---

## 1. Prerequisites

| Item | Value used for the manuscript run |
|---|---|
| Comet | `x64\Release\Comet.exe` and `RealtimeSearch\bin\x64\Release\RealtimeSearch.exe` from this branch (MSBuild Release x64; see the `comet-build` skill). `RealtimeSearch.exe` must include the `--isotope-error` / `--fragment-bin-*` / `--theoretical-fragment-ions` options (commit `2cc05b52`). |
| Python for the tools | any Python 3 (stdlib only); `py -3.12` on Windows |
| Carafe | `C:\Work\Carafe`, **`tmt` branch** (adds `ai_pred.py --ms2_model`); `src\main\resources\py\v2\ai_pred.py` |
| Carafe venv | torch 2.5.1+cpu, peptdeep 1.1.0, alphabase 1.2.1, pandas 2.2.3, pyarrow 21.0.0, numpy 1.26.4 (Windows: `%USERPROFILE%\.carafe\.venv`, the default the drivers look for) |
| MS2 model | the Carafe developers' **CID-fragmentation TMT model** (`ms2tmtcid.pt`, 2026-09-25 release). Ion-trap CID data needs a CID model; see Section 3. |
| FASTA | target + decoy concatenated, decoy accessions prefixed `DECOY_`. Decoys must be in the FASTA (not `decoy_search`), because decoy peptides need their own predictions. Manuscript: UniProt human canonical, 20,454 targets + 20,454 decoys. |

---

## 2. The `comet.params`

Start from `comet.exe -p` and set the keys below. Everything affecting the peptide population is baked into the `.idx`
at build time; changing any of it later means rebuilding the index, the predictions and the mask.

```
database_name = C:\full\path\to\human.canonical.target-decoy.fasta   # rewritten per stage by the driver
decoy_search = 0
decoy_prefix = DECOY_
num_threads = 8                          # index build only; RTS takes --threads

search_enzyme_number = 1                 # trypsin
num_enzyme_termini = 2
allowed_missed_cleavage = 2
digest_mass_range = 700.0 5000.0
peptide_length_range = 7 50
equal_I_and_L = 1

peptide_mass_tolerance_upper = 20.0
peptide_mass_tolerance_lower = -20.0
peptide_mass_units = 2
isotope_error = 2                        # 0/1/2 13C offsets; pass the same value to RealtimeSearch.exe

add_C_cysteine = 57.021464
add_K_lysine = 304.207146                # TMTpro on K
add_Nterm_peptide = 304.207146           # TMTpro on every peptide N-terminus
variable_mod01 = 15.9949 M 0 3 -1 0 0 0.0
max_variable_mods_in_peptide = 3

fragment_bin_tol = 1.0005                # ion-trap MS2
fragment_bin_offset = 0.4
theoretical_fragment_ions = 1            # M peak only; flanking peaks are for high-resolution bins
use_B_ions = 1
use_Y_ions = 1

fragindex_min_ions_score = 3
fragindex_min_ions_report = 3
fragindex_num_spectrumpeaks = 150
fragindex_min_fragmentmass = 200.0
fragindex_max_fragmentmass = 2000.0

fragment_index_predicted_mask_file =     # empty for the build; the RTS search takes --mask
carafe_mask_min_relative_intensity = 0.10
carafe_mask_min_peaks = 6
```

**Put every added key above the `[COMET_ENZYME_INFO]` section.** Keys after it are silently ignored.

On the Carafe side, the static mods come out as `TMTpro@Any N-term` (site 0) on every row and `TMTpro@K` at each 1-based K
position; Met oxidation is `Oxidation@M`. These are AlphaBase 1.2.1 spellings (terminal sites contain a space); the
converter emits them, and the model must have been trained on the same names. With no neutral-loss mod in the params,
the pipeline runs in general mode (four output channels, no modloss) and the mask stage adds `--ignore-modloss` itself.

---

## 3. Choosing the model, instrument and NCE

peptdeep-based models condition on an instrument token (LUMOS, QE, SCIEXTOF, THERMOTOF, TIMSTOF) and an NCE, and a
model trained on one fragmentation type can badly misrepresent another. Before a full run, score the candidate models
against real spectra: take confident PSMs (rank 1, target, q <= 0.01) from a search of the data, predict exactly those
peptidoforms under each (model, instrument, NCE), and compare with the experimental spectra (normalized spectral contrast
angle over predicted b/y ions, matched within the fragment tolerance, ignoring predicted ions below the MS2 scan's low
m/z limit, here 400).

Manuscript result, 281 confident PSMs from Ascend fraction A2 (median spectral angle):

| Model | Setting | Median angle |
|---|---|---|
| peptdeep generic | QE / 34 | 0.63 |
| Carafe HCD TMT model (`ms2tmt.pt`) | Lumos / 34 | 0.73 |
| Carafe CID TMT model (`ms2tmtcid.pt`) | Lumos / 30, 34, 38; QE / 34 | 0.805-0.810 |

The CID model fits ion-trap CID MS2 clearly best and is insensitive to the instrument/NCE inputs over NCE 30-38. The
manuscript run used **Lumos, NCE 35**. The scoring script used (`model_check.py`) is not in this repository.

---

## 4. Build the mask: `carafe.py prerun`

```powershell
$REPO  = "C:\Work\Comet-master"
$OUT   = "C:\work\tmt_run\work"                 # holds every artifact
$VENV  = "$env:USERPROFILE\.carafe\.venv\Scripts\python.exe"

py -3.12 "$REPO\tools\carafe.py" prerun `
    --fasta C:\Work\fasta\human.canonical.target-decoy.fasta `
    --out $OUT --comet "$REPO\x64\Release\Comet.exe" `
    --flavor tmt=C:\work\tmt_run\comet.params `
    --charges 2,3 --include-decoys `
    --carafe-mode general --parquet --chunk-size 50000 --quant u16 `
    --min-relative-intensity 0.10 --min-kept-peaks 6 --workers 18 `
    --venv-python $VENV --ai-pred-py C:\Work\Carafe\src\main\resources\py\v2\ai_pred.py `
    --ms2-model C:\Work\20260922_carafe\ms2tmtcid.pt --instrument Lumos --nce 35 `
    --threads 8 --jobs 3
```

Flag notes:

- `--charges 2,3`: TMT-labeled peptides skew to higher charge. Cost is linear in the number of charges; the mask itself
  is charge-independent (a position is kept if it clears the threshold at any predicted charge).
- `--include-decoys` is mandatory with a target+decoy FASTA; otherwise decoy variants get no mask entry, stay unfiltered,
  and become easier to retrieve than targets, which biases the FDR estimate.
- `--threads 8 --jobs 3`: one `ai_pred.py` process stops scaling at about 8 CPU threads; three concurrent 8-thread
  processes gave 1.6x the throughput of one on a 20-core CPU, with byte-identical predictions. Calibrate on a new machine.
- `--parquet`: parquet chunk input/output (smaller transient tree); the store step then runs under the venv python.
- `--stop-after <stage>` ends after a given stage, e.g. to time stages separately or to hand the peptide list to someone
  else for prediction. Prediction chunks are resumable: re-running skips chunks that carry a `.done` marker.
- For a single-flavor run, `--params FILE` can replace `--flavor tmt=FILE`, and the two thresholds are then read from the
  params file's `carafe_mask_*` keys.

Manuscript run (Intel Core Ultra 7 265K, 20 cores, CPU inference, Windows 11), 6,503,072 variants / 13,006,144 rows:

| Stage | Wall | Peak process-tree working set | Output |
|---|---|---|---|
| s1-s3 prep (index, export, convert) | 79 s | 2.6 GiB | `.idx` 268 MB, export 385 MB, peptide TSV 928 MB |
| s4 predict | 2 h 6 min (1,723 rows/s) | 3.4 GiB | `prediction\` 6.2 GB (transient) |
| s5 store | 34 s | 6.7 GiB | `tmt.cps` 2.17 GB |
| s6 mask | 19 s | 2.8 GiB | `tmt.fi_mask` 273 MB, 6,503,072 entries |

Prediction throughput falls steadily through the run (about 3x from first to last chunk) because the peptide list is in
mass order and later chunks hold longer peptides; estimate run time from the whole-run rate, not the first chunks.

Checks on the finished mask:

```powershell
py -3.12 -c "import sys; sys.path.insert(0, r'C:\Work\Comet-master\tools'); import carafe_ms2_to_fi_mask as m; h, e = m.read_mask_file(r'C:\work\tmt_run\work\tmt.fi_mask'); print(h); print(len(e), 'entries')"
```

Expect `GeneralMode: 1`, a `VarModConfig` identical to the first line of `tmt.carafe_peptides.variants.tsv`, the `.idx`
fingerprint, and one entry per variant (6,503,072 here). A shortfall means missing predictions and unfiltered variants.
Once the store is built, `prediction\` can be deleted; re-sweeping thresholds needs only `.cps` + `carafe.py mask`.

---

## 5. Masked and unmasked real-time searches

`RealtimeSearch.exe` takes its enzyme, modification and digest configuration from the `.idx` header, but its **runtime
scoring settings from command-line options, not from `comet.params`**. Their defaults are high-resolution settings, so
ion-trap data must pass them explicitly, as must a nonzero isotope error:

```powershell
$RTS  = "C:\Work\Comet-master\RealtimeSearch\bin\x64\Release\RealtimeSearch.exe"
$IDX  = "C:\work\tmt_run\work\tmt.fasta.idx"
$MASK = "C:\work\tmt_run\work\tmt.fi_mask"
$RAW  = "C:\Work\data\TMT_RTSMS3_carafe\Ascend_A2_65min_4cell.raw"
$SCORING = @("--threads", "4", "--isotope-error", "2", "--fragment-bin-tol", "1.0005",
             "--fragment-bin-offset", "0.4", "--theoretical-fragment-ions", "1")

cd C:\work\tmt_run\rts\a2_unmasked      # rts.out is written to the current directory
& $RTS --query $RAW --ms1ref $RAW --db $IDX @SCORING
cd C:\work\tmt_run\rts\a2_masked
& $RTS --query $RAW --ms1ref $RAW --db $IDX @SCORING --mask $MASK
```

Omitting the fragment-bin options searches ion-trap spectra with 0.02 Da bins and flanking peaks, which returns almost
no identifications. The mask is validated at load (index fingerprint, peptide count, VarModConfig) and a mismatch stops
the search with an explicit error.

FDR, per acquisition:

```powershell
py -3.12 C:\Work\Comet-master\tools\rts_out_to_txt.py rts\a2_unmasked\rts.out a2_unmasked.txt
py -3.12 C:\Work\Comet-master\tools\rts_out_to_txt.py rts\a2_masked\rts.out   a2_masked.txt
py -3.12 C:\Work\Comet-master\tools\qvalue.py --threshold 0.01 --diff a2_unmasked.txt a2_masked.txt
```

`qvalue.py` uses rank-1 PSMs, target-decoy competition, and reports XCorr- and E-value-ranked counts. The manuscript also
rescored with mokapot (features: XCorr, ln E-value, isotope-folded precursor ppm error, peptide length, missed cleavages,
modification count, charge one-hot).

Manuscript result (4 search threads; A2 / A4):

| | Unmasked | Masked |
|---|---|---|
| FI entries in memory | 1.340e8 | 7.44e7 (-44.5%) |
| MS2 search rate | 4,890 / 4,869 Hz | 5,252 / 5,187 Hz (+7.4% / +6.5%) |
| Peak process working set | 1.70 / 1.70 GiB | 1.58 / 1.58 GiB (-7.2% / -7.0%) |
| Initialize | 1.9 s | 2.5 s |
| PSMs at 1% FDR, E-value | 7,465 / 8,143 | 7,650 / 8,232 (+2.5% / +1.1%) |
| PSMs at 1% FDR, mokapot | 8,779 / 9,268 | 8,686 / 9,149 (-1.1% / -1.3%) |

Masked and unmasked searches assigned the same peptidoform to 99.96% / 99.99% of scans confident in both.

---

## 6. Invariants and gotchas

1. **Do not rebuild the `.idx` between export and search.** The mask header carries a CRC-32 over the peptide table
   plus the VarModConfig; any params change that alters the population makes the mask fail to load. A rebuild with
   identical params is byte-identical (T18).
2. **The peptide TSV is the contract.** Predictions are joined on exact strings. Re-sorting is fine; editing or
   re-encoding (for example an Excel round trip) is not.
3. **Static mods are invisible to the mask's identity key.** The mask keys on (peptide, mod combination, terminal mods);
   TMTpro appears only in the `mods` strings Carafe sees and in the `.idx` header. Mask bits are ladder positions, so
   nothing TMT-specific exists on the C++ side.
4. **Only singly charged b and y ions are in the fragment index.** Reporter ions, which here are only in the MS3 scans,
   are never indexed or masked. Scoring still uses whatever ion series the params enable.
5. **Search-time settings are not index settings.** Isotope error, fragment bin width and flanking peaks can change
   without rebuilding the index, the predictions or the mask; they only require rerunning the searches.
6. **Keys after `[COMET_ENZYME_INFO]` are ignored**, silently.
7. **Launch long stages detached** (`Start-Process powershell -ArgumentList ... -WindowStyle Hidden`), so that a closed
   terminal or session does not kill a multi-hour prediction.

---

## Appendix: handing the prediction step to someone else

If Carafe inference runs on another machine (for example a GPU host), run Section 4 with `--stop-after convert`, send
only `NAME.carafe_peptides.tsv` plus `tools\run_carafe_chunked.py` and `tools\carafe_chunk_common.py`, and have them run:

```
python run_carafe_chunked.py --in NAME.carafe_peptides.tsv --out prediction --chunk-size 50000 ^
    --mode general --device gpu --tf-type ms2 --limit-chunks 0 --jobs 1 --parquet ^
    --venv-python <their python> --ai-pred-py <their ai_pred.py> ^
    --ms2-model <model.pt> --instrument Lumos --nce 35
```

Put the returned `prediction\` directory (both `chunks\` and `chunk_preds\`) under `OUT\`, check that the number of
`chunks\chunk_*.tsv` equals the number of `chunk_preds\*\.done`, create `OUT\.prerun\s4_predict.done`, and rerun the
Section 4 command without `--stop-after`; it resumes at the store stage. Do not send the `.idx` or the variant map, and do
not let the peptide TSV be edited in transit.
