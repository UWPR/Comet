# Carafe end-to-end phospho analysis: MM2_R1/R2, `comet.params.phosphosmall` (2026-08-24)

Full ahead-of-time Carafe predicted-fragment-mask pipeline run start to finish against two
independent real-world phospho-enriched acquisitions (`MM2_R1.raw`, `MM2_R2.raw`), with
timing/memory tracked at every major step and a masked-vs-unmasked RTS (real-time search)
comparison at 1% FDR, including a scan-level breakdown of which identifications agree,
disagree, or are exclusive to one search mode. This document is written to be sufficient
both to reproduce the run and to serve as the analysis-section source material for a Carafe
manuscript.

## 1. Summary

A masked FI_DB RTS search cut the in-memory fragment index by **65.3%** (1.580e9 -> 5.483e8
entries) and peak search memory by **35.6-36.4%** (8.7-8.8GB -> 5.6GB, after a same-session
fix -- Section 7 -- that frees the predicted-fragment mask's own ~1.9GB resident lookup table
once it's no longer needed, on top of the FI-array shrinkage itself) against both of two
independent acquisitions, while simultaneously *increasing* PSMs identified at 1% FDR in
both: +2.8%/+4.0% (xcorr-sorted, R1/R2) and +1.0%/+0.2% (e-value-sorted, R1/R2). A
scan-level comparison shows the two search modes agree on >99.9% of PSMs both consider
confident; of the small remainder, the PSMs masking uniquely recovers are systematically
short, low-charge peptides sitting near the FDR boundary, while the few PSMs masking loses
are systematically long, high-charge, more heavily modified peptides -- consistent with
masking acting as a genuine candidate-quality filter rather than merely a memory/speed
optimization (Section 8.1). The one-time cost is an ahead-of-time pipeline (built once, then
reused for both acquisitions) dominated by an ~11.95-hour CPU-only Carafe (`ai_pred.py`)
inference pass over 39,466,180 peptide-index variants; every other pipeline stage (index
build, export, TSV conversion, compact-store build, mask build) together took under 30
minutes.

## 2. System / software environment

- **Host**: WSL2 (Ubuntu) on Windows, shared physical cores with the Windows host (relevant
  to Section 6.1 below). 20 logical CPUs, 54GB RAM, no GPU (`nvidia-smi` absent) -- all Carafe
  inference in this run was CPU-only (`--device cpu`).
- **Comet**: `carafe` branch, commit `40f93fb3bf1b29bf1d2358864c1cd9fc0067f8e3` (the C++ search
  code -- `CometSearch/`, `CometFragmentIndex.cpp`, `CometPredictedMask.cpp` -- was last
  touched by the merge at `4f51fc7b`; the three commits after it were test/doc-only). Linux
  `comet.exe` built via `make` (full build). Windows `RealtimeSearch.exe` / `CometWrapper.dll`
  / `CometWrapperCore.dll` built fresh from the same commit via MSBuild (Release/x64, full
  solution, Clean-then-Build to avoid the `zconf.h` Linux/Windows cross-build issue -- see
  the `comet-build` skill).
- **Carafe**: `/mnt/c/Work/Carafe/src/main/resources/py/v2/ai_pred.py`, `--mode
  phosphorylation --device cpu --tf_type ms2`. Python environment: `~/.carafe/.venv`
  (Python 3.9.x, `torch` 2.5.1+cpu, `pandas` 2.2.3, `alphabase`/`peptdeep` per prior session
  setup). Driven via `tools/run_carafe_chunked.sh` (chunk size 50,000 rows, `--jobs 1`
  sequential chunks) under `tools/carafe_prerun.sh`, the ahead-of-time pipeline driver
  (`docs/20260822_carafe_prerun.md` milestone M4).
- Downstream Python steps (`idx_to_carafe.py`, `carafe_pred_to_cps.py`,
  `carafe_cps_to_fi_mask.py`) are stdlib-only and ran under the system `python3` (no venv
  needed) -- only `ai_pred.py` itself needs the Carafe venv.

## 3. Input data

| Input | Path | Notes |
|---|---|---|
| Protein database | `20260420-human-phosho/human.canonical.target-decoy.fasta` | Human canonical proteome, target+decoy concatenated at the FASTA level: 40,908 entries total, exactly 20,454 `DECOY_`-prefixed. 27.6MB. |
| Search parameters | `20260420-human-phosho/comet.params.phosphosmall` | See Section 3.1 for the full effective configuration. |
| Query spectra (replicate 1) | `20260420-human-phosho/MM2_R1.raw` | Thermo `.raw`, 1,025,936,442 bytes (~978MB), 68,586 total scans, 45,806 of which are MS2 (the rest MS1). |
| Query spectra (replicate 2) | `20260420-human-phosho/MM2_R2.raw` | Thermo `.raw`, 1,051,405,153 bytes (~1002MB), 62,887 total scans, 42,406 of which are MS2. |

Both `.raw` files were read directly by `RealtimeSearch.exe` via
`ThermoFisher.CommonCore.RawFileReader` -- no `.mzXML` conversion is needed for the RTS
step (`.mzXML` conversions of both files already existed from prior project work, at
`MM2_R1.mzXML`/`MM2_R2.mzXML`, but were not used here; they would only matter for a batch
`comet.exe` search on Linux, which cannot read `.raw` directly).

### 3.1 `comet.params.phosphosmall` -- effective configuration

Deliberately scaled down from earlier full-scale phospho runs in this project (which used
`max_variable_mods_in_peptide=3`, `digest_mass_range 700-5000`, `peptide_length_range 7-50`)
specifically so a from-scratch Carafe inference pass would be tractable in a single session
while still exercising the real, full canonical-proteome peptide population.

```
database_name = human.canonical.target-decoy.fasta
decoy_search = 0                        # decoys already concatenated into the FASTA
num_threads = 8

search_enzyme_number = 1 (Trypsin), num_enzyme_termini = 2, allowed_missed_cleavage = 2
digest_mass_range = 700.0 3500.0        # MH+ Da
peptide_length_range = 7 35
decoy_prefix = DECOY_
equal_I_and_L = 1

variable_mod01 = 15.9949   M   0 2 -1 0 0 0.0            # Met oxidation, max 2/peptide
variable_mod02 = 79.966331 STY 0 2 -1 0 0 97.976896       # phospho, max 2/peptide, NL 97.976896 Da
max_variable_mods_in_peptide = 3
require_variable_mod = 0

peptide_mass_tolerance_upper/lower = +20.0 / -20.0 ppm (peptide_mass_units=2, precursor_tolerance_type=1)
isotope_error = 0
fragment_bin_tol = 0.02, fragment_bin_offset = 0.0, theoretical_fragment_ions = 0 (flanking peaks)
use_B_ions = 1, use_Y_ions = 1 (others off)
min_precursor_charge = 1, max_precursor_charge = 6, max_fragment_charge = 3
minimum_peaks = 10, minimum_intensity = 0

fragindex_min_ions_score = 3, fragindex_min_ions_report = 3
fragindex_num_spectrumpeaks = 150
fragindex_min_fragmentmass = 200.0, fragindex_max_fragmentmass = 2000.0
fragindex_skipreadprecursors = 1

max_duplicate_proteins = 10, clip_nterm_methionine = 0
static mod: add_C_cysteine = 57.021464 (carbamidomethyl)
```

Full file: `20260420-human-phosho/comet.params.phosphosmall` (committed alongside the other
`comet.params.*` variants in that working directory, not in the git repo itself).

## 4. Pipeline: exact commands (reproduction recipe)

All commands run from `/mnt/c/Work/Comet-master`. `$FASTA`, `$OUT`, `$COMET`, `$PARAMS` as
defined below.

```bash
FASTA=/mnt/c/Work/Comet-master/20260420-human-phosho/human.canonical.target-decoy.fasta
OUT=/mnt/c/Work/Comet-master/20260420-human-phosho/carafe_phosphosmall
COMET=/mnt/c/Work/Comet-master/comet.exe
PARAMS=/mnt/c/Work/Comet-master/20260420-human-phosho/comet.params.phosphosmall

tools/carafe_prerun.sh \
  --fasta "$FASTA" --out "$OUT" --comet "$COMET" \
  --flavor phosphosmall="$PARAMS" \
  --charges 2 --include-decoys \
  --stop-after mask
```

This single command drives all six ahead-of-time stages (idx build -> `-x` export ->
`idx_to_carafe.py` convert -> Carafe `ai_pred.py` predict -> `.cps` build -> `.fi_mask`
build), each resumable via `$OUT/.prerun/<stage>.done` markers. `--charges 2` matches the
already-validated production convention from earlier full-scale runs (the FI mask itself is
charge-independent at search time -- see `CometPredictedMask::Lookup()` -- Carafe predicts
per-charge intensities and the mask builder takes the max across predicted charges, so more
charges would mean more thorough but proportionally more expensive inference).
`--include-decoys` is required here because the FASTA's decoy peptides are real raw
peptides in the `.idx` needing their own mask coverage, not a synthetic decoy-generation
flag.

Then, for the RTS comparison (Windows, paths must be Windows-style even though the `.exe`
itself is invoked from WSL bash -- see `comet-build` skill's WSL-interop path-format note).
Run once per replicate (`$RAW` = `MM2_R1.raw` or `MM2_R2.raw`) against the *same*
`phosphosmall.fasta.idx`/`phosphosmall.fi_mask` pair built above -- the ahead-of-time
pipeline is per-database/mod-configuration, not per-raw-file, so it is built once and reused:

```bash
cd /mnt/c/Work/Comet-master/RealtimeSearch/bin/x64/Release
RAW='C:\Work\Comet-master\20260420-human-phosho\MM2_R1.raw'      # or MM2_R2.raw
IDX='C:\Work\Comet-master\20260420-human-phosho\carafe_phosphosmall\phosphosmall.fasta.idx'
MASK='C:\Work\Comet-master\20260420-human-phosho\carafe_phosphosmall\phosphosmall.fi_mask'

./RealtimeSearch.exe --query "$RAW" --ms1ref "$RAW" --db "$IDX" --threads 20              # unmasked
./RealtimeSearch.exe --query "$RAW" --ms1ref "$RAW" --db "$IDX" --threads 20 --mask "$MASK" # masked
```

Each run writes `rts.out` (overwritten each invocation -- rename/move it between runs, e.g.
to `rts_unmasked.out` / `rts_masked.out` for R1, `rts_r2_unmasked.out` / `rts_r2_masked.out`
for R2). Note `RealtimeSearch.exe` reads all of its peptide-index/mod/enzyme/digest-range
configuration from the *existing* `.idx`'s own self-describing header (post
`docs/20260811_restore_idx_header_mods.md`) -- it never reads `comet.params.phosphosmall`
directly. Its hardcoded runtime scoring defaults (fragment/precursor tolerance, charge
range, minimum peaks, etc., in `SearchMS1MS2.cs`) independently match this params file's
values, with one minor, search-time-inconsequential exception: `equal_I_and_L` is hardcoded
to `0` in RTS vs. `1` in the params file (affects only build-time I/L-collapsed
peptide-string dedup, already baked into the `.idx`, not live per-spectrum scoring).

FDR comparison, per replicate:

```bash
python3 tools/rts_out_to_txt.py rts_unmasked.out  rts_unmasked.txt   # new: tools/rts_out_to_txt.py
python3 tools/rts_out_to_txt.py rts_masked.out    rts_masked.txt
python3 tools/qvalue.py --threshold 0.01 --diff rts_unmasked.txt rts_masked.txt
```

`tools/rts_out_to_txt.py` (added as part of this analysis) reformats `rts.out`'s
custom `" MS2 {scan}\t{peptide}  {xcorr}  {evalue}  z {charge}  exp {mass} calc {mass} ..."`
line format into the standard Comet tab-delimited `.txt` shape (2 header lines,
`e-value`/`xcorr`/`protein` at their usual column indices) so `tools/qvalue.py` -- which
otherwise only understands batch Comet output -- can consume RTS results directly. Rank-1
only (RTS only ever reports one hit per scan, satisfying `qvalue.py`'s rank-1-only
requirement by construction).

For the scan-level same/different-ID breakdown (Section 8.1), a short ad hoc analysis script
imports `tools/qvalue.py` directly (`load_rank1()`, `_sort_psms()`, `compute_qvalues()`,
`_passing_set()`) to reduce each `.txt` file to its 1% FDR xcorr-sorted passing set as a
`{scan: (xcorr, evalue, is_decoy, scan, charge, modified_peptide, protein)}` map, keyed by
scan number (RTS reports at most one PSM per scan, so this is lossless), and compares the two
maps' key sets and, for keys present in both, their peptide-string values -- classifying any
disagreement as either "same amino-acid backbone, different phospho-site" (bracket-stripped
sequences match) or "genuinely different peptide" (they don't). This script was written for
this analysis and was not committed to the repository (it is a one-off analysis snippet, not
reusable tooling in the way Section 5's tools are) -- reproducing Section 8.1 requires
re-deriving it from this description, or the two runs' `.txt` output plus `tools/qvalue.py`'s
own functions.

## 5. Methods: Carafe-support tooling

The masked-FI-search feature required a chain of new tools bridging Comet's `.idx` format,
Carafe's own AlphaBase-derived input/output conventions, and the compact on-disk
representations needed to make full-proteome scale tractable. Everything except the two
shell drivers and the native C++/C# integration is pure-Python-3, stdlib-only (no
third-party dependency beyond the Carafe venv itself, which only the actual inference step
needs). Data flows in this order: `.idx` -> `variants_export.tsv` -> `carafe_peptides.tsv`
(+ `.variants.tsv`) -> raw Carafe per-chunk predictions -> `.cps` -> `.fi_mask`.

### 5.1 `tools/idx_to_carafe.py` -- Comet `.idx` to Carafe input format

**Language**: Python 3, stdlib only (`argparse`, `struct`, `hashlib`, `os`, `sys`).
**Purpose**: bridges Comet's on-disk peptide index and Carafe's ML input format. Takes a
built `.idx` (PI_DB, built via `comet.exe -j`) plus that same `.idx`'s `comet.exe -x`
variant-enumeration export, and produces the peptide list Carafe's `ai_pred.py` consumes
directly. The `.idx` binary format has no existing Python (or C++/Java) reader library --
this script's `IdxReader` class is a from-scratch reimplementation in Python of
`CometPeptideIndex::ReadPeptideIndex()`'s raw-peptide-table and protein-list decoding.
**Input**: `idx_file` (Comet `.idx`), `variants_export_tsv` (`comet.exe -x` output: one row
per (peptide, mod-combination) variant, columns including `iWhichPeptide`/`modNumIdx`/
`cNtermMod`/`cCtermMod`/site information), plus `--charges` (comma-separated precursor
charges to emit, e.g. `2` or `2,3`), `--include-decoys`, `--carafe-mods-tsv` (path to
Carafe's curated PTM name table for resolving Comet mod masses to AlphaBase mod names).
**Output**: `out_tsv` -- four columns (`sequence`, `mods`, `mod_sites`, `charge`) in
AlphaBase's `"<UniModTitle>@<Site>"` / 1-based-position convention, one row per
(peptide, mod-combination, charge), deduplicated by content (multiple source variant
tuples sharing an identical sequence/mods/mod_sites/charge collapse to one output row).
Also writes a **variant-map sidecar** (`<out_tsv>` with `.variants` inserted before the
extension) with columns `row_index`/`iWhichPeptide`/`modNumIdx`/`cNtermMod`/`cCtermMod` --
a long/repeated-`row_index` table mapping each output row back to every FI variant identity
tuple it represents, plus a leading `# VarModConfig: ...` comment line (the serialized
active variable-mod configuration, later checked at mask-load time). This map is the *only*
safe join key between a Carafe prediction and a specific FI variant, since the fragment
index regenerates and re-sorts its own variant list independently of the `.idx`'s on-disk
order.

### 5.2 `tools/run_carafe_chunked.sh` -- chunked, resumable Carafe inference driver

**Language**: Bash. **Purpose**: `ai_pred.py` reads its entire `--in_file` into one pandas
DataFrame and performs one single-threaded (`--device cpu`) `predict_ms2()` call over the
whole thing with no progress signal and no resume point -- infeasible at tens-of-millions-
of-row scale (a full run without chunking died silently after ~51h wall time with zero
bytes written, per project history). This wrapper splits the input into fixed-size chunks,
invokes `ai_pred.py` once per chunk as its own OS process (via the Carafe venv's Python,
default `~/.carafe/.venv/bin/python`), and marks each chunk `.done` on success, so progress
is visible and an interrupted run resumes at the first incomplete chunk. Also samples each
chunk's own child-process RSS and system swap usage every 5s (written to a per-chunk
`mem_samples.tsv`), giving built-in memory tracking with no external instrumentation needed.
**Input**: a `tools/idx_to_carafe.py` `out_tsv` (`sequence`/`mods`/`mod_sites`/`charge`).
**Output**: `<out>/chunks/chunk_NNNNN.tsv` (the row-sliced inputs) and
`<out>/chunk_preds/chunk_NNNNN/` per chunk, each containing `ai_pred.py`'s own two-file
output -- `<prefix>_ms2_df.tsv` (echoed input rows, reordered by AlphaBase to be sorted by
peptide length, plus `nAA`/`frag_start_idx`/`frag_stop_idx`) and `<prefix>_ms2_pred.tsv`
(the flat per-fragment-position intensity table: `b_z1`/`b_z2`/`y_z1`/`y_z2`, plus
`b_modloss_z1`/`b_modloss_z2`/`y_modloss_z1`/`y_modloss_z2` when run with `--mode
phosphorylation`) -- plus a `.done` marker, `.elapsed_seconds`, `.rate_rows_per_sec`, and
`mem_samples.tsv` per chunk.

### 5.3 `tools/carafe_cps.py` -- compact prediction store format (library)

**Language**: Python 3, stdlib only (`struct`, `zlib`). Pure library module (`CpsWriter`/
`CpsReader` classes, no CLI of its own) imported by 5.4 and 5.6 below. **Purpose**: defines
and implements the `.cps` ("Compact Prediction Store") binary format -- a custom,
project-specific format designed to hold *exactly* what mask-building needs and nothing
else, at ~35-45x smaller than the raw Carafe TSV output it replaces (386-395GB raw at full
real-proteome-phospho scale, vs. ~31GB as a `.cps`). Per source-row: peptide length, two
float32 base-peak reference values (max intensity across all 8 channels, and across the
first 4 non-modloss channels only), and per cleavage site the 4 z1-channel intensities
(`b_z1`/`y_z1`/`b_modloss_z1`/`y_modloss_z1`) mask-building actually consumes, quantized
relative to the base peak as `u8` or `u16` (header-declared). **Format** (little-endian):
ASCII magic line `Comet Carafe CPS v1`, `Key: Value` header lines (source-TSV row count,
a CRC32 provenance check, quantization width, phospho-vs-general mode) terminated by a
blank line, a `u64` row count, a directory of `u64` file offsets (one per row), then the
per-row binary payloads. Keyed by `.idx_to_carafe.py` `out_tsv` row index, making one store
`.idx`-flavor-neutral (a single withNL-flavor store can serve both a withNL mask build and,
via `--ignore-modloss`, a noNL mask build for the same peptide population).

### 5.4 `tools/carafe_pred_to_cps.py` -- raw predictions to compact store

**Language**: Python 3, stdlib only, `multiprocessing.Pool`. **Purpose**: translates a
completed chunked Carafe run (5.2's output) into one `.cps` store (5.3's format). Resolves
Carafe's internal peptide-length-sort reordering per chunk by joining on the content tuple
(`sequence`/`mods`/`mod_sites`/`charge`, unique by construction) rather than row position,
quantizes, and appends to the store in `row_index` order via a worker pool (`--workers`)
whose results are consumed in chunk order so output is deterministic regardless of worker
count. **Input**: `--chunks-dir` (5.2's `chunks/`), `--preds-dir` (5.2's `chunk_preds/`),
`--source-out-tsv` (the original unchunked `idx_to_carafe.py` output, read only for a
row-count + CRC32 provenance header, not row-by-row). **Output**: one `.cps` file
(`--quant u8|u16`); writes to a `.building` path during the run so a crash never leaves a
misleadingly-named partial store.

### 5.5 `tools/carafe_ms2_to_fi_mask.py` -- mask decision logic + TSV-direct builder

**Language**: Python 3, stdlib only. Both a standalone CLI and the shared library (`fi_mask`
module) that 5.4's sibling (5.6) and the store format (5.3) call into for their identical
threshold/floor/packing decisions. **Purpose**: implements the actual masking decision --
per FI variant, take the max intensity across predicted charge states, threshold each
candidate unshifted b/y ion (length >= 3) against `--min-relative-intensity` of the base
peak, independently threshold each modloss-shifted b/y ion the same way (a separate keep/
drop pool, since the FI inserts unshifted and NL-shifted ions as separate entries), then
apply a `--min-kept-peaks` floor (top up by intensity if thresholding kept fewer than the
floor) -- and defines the on-disk **`.fi_mask` v3 binary format** all mask-producing paths
in this project emit. **Input** (as a CLI, for small-scale/test use -- see 5.6 for the
production store-based path): the four files a single `idx_to_carafe.py` export + Carafe run
produces (`out_tsv`, its `.variants.tsv` sidecar, Carafe's `_ms2_df.tsv`, `_ms2_pred.tsv`).
**Output / format**: `.fi_mask`, magic `Comet Carafe FI mask v3`, ASCII header lines
(`SourceIdxFingerprint`, `SourceIdxNumRawPeptides`, `SourceIdxPath`,
`MinRelativeIntensity`, `MinKeptPeaks`, `GeneralMode`, `VarModConfig`) terminated by a blank
line, a `u64` entry count, then fixed-size binary entries (`struct` format `<IibbQQQQ`:
`u32 iWhichPeptide, i32 modNumIdx, i8 cNtermMod, i8 cCtermMod, u64 bMask, u64 yMask, u64
bModlossMask, u64 yModlossMask`) sorted by `(iWhichPeptide, modNumIdx, cNtermMod,
cCtermMod)` for binary search at FI-build time. Each `u64` mask is a bitfield: bit `(i-2)`
of the relevant mask corresponds to `AddFragments()`'s 0-based ladder index `i` (only ever
queried for `i > 1`, i.e. fragment length >= 3).

### 5.6 `tools/carafe_cps_to_fi_mask.py` -- production mask builder (from the store)

**Language**: Python 3, stdlib only, `multiprocessing.Pool`, `heapq.merge`. **Purpose**: the
mask build path actually used for this analysis (superseding both the CLI form in 5.5 and
the earlier chunked-TSV path in 5.7) -- rebuilds a `.fi_mask` from the compact `.cps` store
in minutes instead of hours, without needing the (now-deletable) raw Carafe TSV output, so
re-sweeping `--min-relative-intensity`/`--min-kept-peaks`/`--ignore-modloss` later costs
almost nothing. Shares all decision logic with 5.5 (`compute_variant_mask_from_cps()`
calls the same threshold/floor/pack helpers) so a store-built mask is bit-identical to what
the TSV path would produce from the same underlying predictions, modulo quantization.
Streams the (potentially tens-of-GB) variant-map sidecar in parallel byte ranges rather than
loading it whole, and has each worker pack its output entries to raw bytes (not Python
objects) before returning them to the parent process -- a lesson learned when an earlier
attempt returning object graphs from worker processes drove parent RSS to 44GB. Workers'
sorted output ranges are then k-way merged (`heapq.merge`) while streaming the file to disk,
and the written file is re-read and verified strictly increasing (proving both sort order
and key uniqueness) before being considered valid. **Input**: `idx_file` (the `.idx` this
mask will be used with -- supplies the fingerprint/raw-peptide-count header fields),
`variant_map_tsv` (5.1's sidecar for that `.idx`), `cps_file` (5.3/5.4's store -- may come
from a *different* `.idx` flavor's export, since the store is keyed by the shared peptide
population's row index, not by `.idx`-specific tuples), plus `--min-relative-intensity`,
`--min-kept-peaks`, `--ignore-modloss`, `--workers`. **Output**: one `.fi_mask` file,
identical format to 5.5.

### 5.7 Legacy / superseded: `tools/build_carafe_mask_chunked.sh` + `tools/merge_carafe_fi_masks.py`

**Language**: Bash + Python 3 (stdlib). The chunked-TSV mask-build path that preceded the
`.cps` store (5.3-5.6): runs 5.5's CLI once per already-existing Carafe-inference chunk
(each chunk's fragment table fits in memory even when the full concatenated table, ~3.63
billion fragment rows at real full-proteome-phospho scale, does not: ~116GB just for that
one structure against a 54GB machine), then `merge_carafe_fi_masks.py` concatenates the
resulting per-chunk `.fi_mask` files (which partition the population by non-overlapping
`row_index` ranges, so no merge-conflict resolution is needed) into one final mask using
the same read/write functions as 5.5, so the binary result is indistinguishable from a
non-chunked build. Superseded by the `.cps`-based path because it required keeping the
full raw Carafe TSV output around (hundreds of GB) merely to *re-sweep* mask thresholds;
kept in the repository for TSV-only situations where no store has been built.

### 5.8 Native integration: `CometPredictedMask` (C++) and RTS `--mask` (C#)

**Language**: C++ (`CometSearch/CometPredictedMask.h`/`.cpp`), C# (`RealtimeSearch/
SearchMS1MS2.cs`). **Purpose**: the search-time consumer of a `.fi_mask` file. `Load()`
parses the format from 5.5/5.6, verifying the mask's embedded `SourceIdxFingerprint`/
`SourceIdxNumRawPeptides` against the currently-loaded `.idx` and its `VarModConfig` against
the live search's active variable mods -- either mismatch is a hard, loud failure rather
than a silently-misapplied mask. `Lookup(iWhichPeptide, modNumIdx, cNtermMod, cCtermMod, ...)`
returns the four per-variant bitmasks (or "not found", which callers must treat as fully
unfiltered, never as fully masked-out); `CometFragmentIndex::AddFragments()` consults it
once per candidate b/y/modloss-shifted fragment-ion insertion during FI generation, so
masking only ever changes *which fragment-ion positions of a kept variant get written into
the index* -- never which peptide variants exist in the index at all. Enabled via the
`fragment_index_predicted_mask_file` `comet.params` key for batch/FI_DB search, or via
`RealtimeSearch.exe`'s `--mask <path>` flag (which sets the same underlying parameter
through `CometSearchManager::SetParam()`) for RTS.

### 5.9 Analysis tools used to produce this document's results

- **`tools/qvalue.py`** (Python 3, stdlib): computes target-decoy FDR/q-values from Comet
  tab-delimited `.txt` output, rank-1 PSMs only, reporting both xcorr-descending and
  e-value-ascending rankings side by side; `--diff` additionally lists PSMs unique to each
  of two compared files at a given q-value threshold. Pre-existing project tool, not written
  for this analysis. Input: one or more Comet `.txt` files (2 header lines, tab-delimited,
  `e-value`/`xcorr`/`protein` at fixed column indices). Output: a console FDR-vs-threshold
  table (and, with `--diff`, per-file unique-PSM listings).
- **`tools/rts_out_to_txt.py`** (Python 3, stdlib, ~55 lines) -- **new, written for this
  analysis**. `RealtimeSearch.exe` writes its own custom, partly-space-delimited `rts.out`
  format (`" MS2 {scan}\t{peptide}  {xcorr}  {evalue}  z {charge}  exp {mass}  calc {mass}
  AScore {score}  Sites '{sites}'  {ms} ms  prot '{protein}'"` per MS2 PSM line), which
  `qvalue.py` cannot parse directly. This script regex-parses each MS2 line and re-emits it
  in the standard Comet tab-delimited `.txt` shape (matching column layout/indices exactly,
  including the two leading header lines `qvalue.py` skips), so RTS output can be fed to the
  existing FDR tool with no changes to either. Only MS2 lines with a non-empty peptide hit
  are converted (one row per scan, satisfying `qvalue.py`'s rank-1-only assumption by
  construction); MS1-alignment lines, the slowest-runs summary, and the timing/memory footer
  are skipped. Input: an `rts.out` file. Output: a Comet-`.txt`-compatible tab-delimited
  file.

## 6. Pipeline results: timing and memory

Peak memory for the two `comet.exe` steps (`.idx` build, `-x` export) is Comet's own
self-reported figure (printed at the end of each run, e.g. `(37s, 1.1GB)`); everything else
was measured externally (`/usr/bin/time -v` for the Python steps' peak RSS, or
`run_carafe_chunked.sh`'s own built-in per-chunk RSS sampler for the Carafe inference step).

| # | Stage | Tool | Wall time | Peak memory | Output |
|---|---|---|---|---|---|
| 1 | `.idx` build | `comet.exe -i` | 37s | 1.1GB | `phosphosmall.fasta.idx`, 235MB -- 3,961,583 unmodified peptides, 3,961,583 protein groups |
| 2 | Variant export | `comet.exe -x` | **38.79s** (see 6.1) | 2.12GB | `phosphosmall.variants_export.tsv`, 3.05GB -- 39,466,180 peptide-index variants |
| 3 | `idx_to_carafe.py` convert | `python3` (stdlib) | 949s (15.8 min) | 5.35GB | `phosphosmall.carafe_peptides.tsv` (2.15GB) + `.variants.tsv` provenance (1.14GB), 39,466,180 rows |
| 4 | Carafe inference | `ai_pred.py` via `run_carafe_chunked.sh`, 790 chunks x 50,000 rows (CPU) | **43,020s (11.95h)** | 0.93-1.4GB per chunk (grows with peptide length; see 6.2) | `prediction/chunk_preds/*` raw per-chunk predictions |
| 5 | Compact store build | `carafe_pred_to_cps.py`, 18 workers | 264s (4.4 min) | ~800MB/worker | `phosphosmall.cps`, 6.88GB, u16-quantized |
| 6 | Mask build | `carafe_cps_to_fi_mask.py`, 18 workers, 72 ranges | 286s (4.8 min; 285s self-reported) | ~2.0GB on the final merge worker | `phosphosmall.fi_mask`, 1.658GB, 39,466,180 entries (sorted+unique verified) |
| | **Total, `.idx` -> mask** | | **~12.2 hours** | | |

Mask-build parameters: `--min-relative-intensity 0.10 --min-kept-peaks 6 --quant u16
--workers 18`; modloss channels active (auto-detected: `variable_mod02`'s neutral-loss
delta is non-zero, so `--ignore-modloss` was *not* set).

### 6.1 A methodological finding: CPU contention between WSL2 and the Windows host

The variant-export step (#2) was originally measured at **490s** while a Windows MSBuild
Release rebuild of `RealtimeSearch.exe`/`CometWrapper.dll` (needed later for the RTS step)
ran concurrently. WSL2 and the Windows host share the same physical CPU cores -- they are
not resource-isolated -- so this was real contention, not a measurement artifact. A clean
re-run of the identical command on an otherwise-idle machine (confirmed via `ps`/`uptime`:
no other heavy process, load average ~4.5/20) took **38.79s**, a **12.6x** difference from
only a partial-duration overlap (roughly 35% of the original run's wall-clock coincided with
the build). The lesson generalizes: *any* wall-clock timing claim taken on a shared WSL2/host
machine must confirm nothing else was competing for CPU at the time, because the slowdown
from partial contention here was far larger than the overlap fraction would suggest --
almost certainly from cache and memory-bandwidth contention with a build process's own
heavy multi-process compilation, not simple round-robin CPU-time slicing. All other timings
in this table were confirmed to run on an otherwise-idle machine.

### 6.2 Carafe inference cost scales with peptide length, not row count

`idx_to_carafe.py`'s peptide-index-variant TSV is sorted such that chunks are traversed in
increasing average peptide length (chunk 0: 7.08 residues average; chunk 789 [last, partial,
16,180 rows]: 30.20 residues average -- the configured `peptide_length_range` is 7-35). Per-
chunk wall time tracked this closely: fitting `time_sec = a * length^b` by least-squares
linear regression on `log(time)` vs. `log(length)` across all 790 chunks with a logged
completion time gives **b = 0.852** (a = 4.124) -- i.e., close to linear in peptide length,
not the naive flat-rate-per-row assumption a uniform-chunk-size scheme might suggest. Early
progress-based ETAs that didn't account for this (extrapolating from the first ~100,
short-peptide chunks) underestimated total inference time by roughly 2.5x; a length-aware
model fit at the 50% mark (401/791 chunks, 4.73h elapsed) projected 11.85h total, matching
the actual 11.95h to within 1%.

## 7. RTS FI search: masked vs. unmasked

Two independent acquisitions were searched against the *same* `phosphosmall.fasta.idx` /
`phosphosmall.fi_mask` pair -- the ahead-of-time pipeline (Sections 4-6) only depends on the
database + mod configuration, not on which spectra get searched, so it does not need to be
re-run per raw file. Both runs per replicate: `RealtimeSearch.exe`, `--threads 20`, the same
raw file for both `--query` and `--ms1ref` (self-referential MS1 alignment, the standard
convention for single-file RTS benchmarking in this project), `--ascorepro` left at its
default (1, AScorePro site localization on). Only `--mask` differs between the masked/
unmasked pair for a given replicate. Both replicates were run back-to-back on an otherwise
idle machine (confirmed via `ps`/`uptime` beforehand, as in Section 6.1).

| Metric | R1 unmasked | R1 masked | R2 unmasked | R2 masked |
|---|---|---|---|---|
| Total scans / MS2 scans searched | 68,586 / 45,806 | 68,586 / 45,806 | 62,887 / 42,406 | 62,887 / 42,406 |
| FI entries in memory | 1.580e9 | 5.483e8 (**-65.3%**) | 1.580e9 | 5.483e8 (**-65.3%**) |
| Peak process memory | 8.8GB | 5.6GB (**-36.4%**) | 8.7GB | 5.6GB (**-35.6%**) |
| MS2 search elapsed | 4.90s | 4.77s (-2.7%) | 4.54s | 4.22s (-7.0%) |
| MS2 average search rate | 9,349 Hz | 9,603 Hz (+2.7%) | 9,334 Hz | 10,053 Hz (+7.7%) |
| Total RTS elapsed | 16.09s | 19.48s | 17.68s | 18.56s |

(FI entry counts and their reduction are identical between replicates by construction --
same `.idx`/`.fi_mask` pair for both. Peak memory now matches exactly between replicates too
(5.6GB/5.6GB, masked) -- expected, since freeing a fixed-size, mask-content-determined
structure is deterministic given the same `.idx`/`.fi_mask` pair. Search-speed deltas are
noisier and not the focus of this re-run -- see the note below and Section 9's general
single-sample-timing caveat.)

**2026-08-25 re-run: `CometPredictedMask::FreeAfterIndexBuild()` fix.** The masked-column
figures above were re-measured after a same-session fix on top of commit `7d4e6427` (still
uncommitted at time of writing): `CometPredictedMask::s_entries` -- the mask's resident
lookup table, 48 bytes/entry, ~1.9GB for this run's 39,466,180 entries -- was previously never
freed after `CometFragmentIndex::GenerateFragmentIndex()` finished with it, even though
`AddFragments()` (`CometFragmentIndex.cpp:854`) is its only consumer and that consumption is
entirely confined to the one-time FI-build pass. The fix frees it (`std::vector<Entry>().
swap(s_entries)`) immediately after `GenerateFragmentIndex()` returns, inside
`CometFragmentIndex::CreateFragmentIndex()`. Original (pre-fix) masked peak memory was
**6.7GB** for both replicates; the post-fix **5.6GB** is a further **-1.1GB** on top of the
already-reported masking benefit -- smaller than the ~1.9GB the freed `s_entries` structure
itself accounts for. That gap is expected, not a discrepancy: freeing a `std::vector` returns
its memory to the process heap, but Windows' `PeakWorkingSetSize` tracks actual resident pages,
not heap accounting, so the reduction only shows up to the extent the OS reclaims those pages
before the process's next high-water point; any of it retained by the allocator for reuse (or
backfilled by later allocations -- per-thread search buffers, raw-file read buffers, CLR GC
activity -- before the process's true peak is reached during the search phase) doesn't reduce
the observed peak by its full size. **-1.1GB** is the real, measured number either way; treat
the ~1.9GB structure size as an upper bound on what freeing it *could* save, not a prediction
of what a peak-RSS measurement will show. The unmasked columns are untouched by this fix and were not
re-run -- `CometPredictedMask::Load()` is a no-op (`s_entries` stays empty) whenever no mask
file is configured, so `FreeAfterIndexBuild()` swaps an already-empty vector to empty there.
Both re-run replicates were verified byte-identical to the original masked run's PSM calls
(`tools/rts_out_to_txt.py` output, sorted, `diff`: 0 differing lines for both R1 and R2) and
produce identical 1% FDR PSM counts to Section 8's existing table (R1: 15,793 xcorr / 16,906
e-value; R2: 15,275 xcorr / 15,865 e-value) -- confirming the fix is memory-only and Section 8's
PSM-quality analysis needs no changes. MS2 search-elapsed/rate deltas above shifted modestly
(a few percent, in both directions across the two replicates) relative to the original
masked run -- consistent with ordinary single-sample wall-clock noise (Section 9), not a
systematic effect of the fix; the memory reduction is the reproducible result here.

The mask itself was accepted without a fingerprint or `VarModConfig` mismatch in either run
(both are checked and would hard-fail the search otherwise -- see
`CometPredictedMask::Load()`): `loaded 39466180 predicted-fragment mask entries` matches the
mask file's own row count from stage 6 exactly, for both R1 and R2.

## 8. PSM-quality comparison at 1% FDR

FDR computed via `tools/qvalue.py` (standard target-decoy competition, `FDR(i) =
n_decoy(i)/n_target(i)` with no +1/2x correction, `q(i)` = running minimum from `i` to the
end of the ranked list, decoys identified by the `DECOY_` protein prefix), separately for
xcorr-descending and e-value-ascending ranking, rank-1 PSMs only.

| | R1 unmasked | R1 masked | R2 unmasked | R2 masked |
|---|---|---|---|---|
| Total MS2 hits reported | 31,427 | 27,643 | 26,561 | 23,842 |
| PSMs @ 1% FDR (xcorr-sorted) | 15,362 (cutoff >= 1.6820) | 15,793 (cutoff >= 1.5860) | 14,685 (cutoff >= 1.6120) | 15,275 (cutoff >= 1.4840) |
| PSMs @ 1% FDR (e-value-sorted) | 16,734 (cutoff <= 2.44e-02) | 16,906 (cutoff <= 1.85e-02) | 15,828 (cutoff <= 3.69e-02) | 15,865 (cutoff <= 2.74e-02) |
| Net masked delta (xcorr) | +431 (+2.8%) | | +590 (+4.0%) | |
| Net masked delta (e-value) | +172 (+1.0%) | | +37 (+0.2%) | |

**Interpretation.** Both independent acquisitions show the same qualitative pattern: the
masked search reports *fewer* raw top-hit rows overall (R1: -12.0%; R2: -10.2%) --
masking removes some marginal candidate matches that the unmasked full index would still
report as a (typically low-confidence) top hit for a given spectrum -- but a larger fraction
of what the masked search *does* report survives the 1% FDR cutoff, for a net PSM gain in
both replicates (xcorr-sorted: +2.8% R1, +4.0% R2; e-value-sorted: +1.0% R1, +0.2% R2).
Section 8.1 below breaks down exactly which PSMs move and why. This is consistent with
predicted-fragment masking acting as a **candidate-quality filter, not merely a memory/
speed optimization**: by restricting each candidate peptide's fragment-ion index footprint to
the subset of ions Carafe predicts will actually be intense, spurious low-quality matches
that happened to share a few index-eligible fragments with the true peptide are less likely
to accumulate enough matching ions to outscore the correct PSM, while the correct PSM's own
score is essentially unaffected (candidate masking does not change the theoretical-spectrum
scoring itself -- `CometSearch.cpp`'s XCorr scoring always scores the full theoretical
spectrum regardless of what survived into the FI's posting list; masking only affects which
peptides get *found as index candidates* in the first place).

### 8.1 Which PSMs are the same, and which differ, between masked and unmasked

Method: for each replicate, both the unmasked and masked xcorr-sorted, rank-1, target-only
PSM sets passing 1% FDR were reduced to `{scan -> (peptide, xcorr, charge)}` maps (RTS
reports at most one candidate per scan, so this is lossless), then compared by scan number.
Every scan present in a run's passing set was also checked for whether the *other* run even
attempted that scan (RTS always reports a rank-1 hit per searched MS2 scan when one exists,
so "passing" here specifically means "identified AND survived the 1% FDR cutoff").

| | R1 | R2 |
|---|---|---|
| Scans passing in both (shared) | 15,213 | 14,535 |
| Scans passing in unmasked only | 149 | 150 |
| Scans passing in masked only | 580 | 740 |
| Of shared scans, identical peptide call | 15,200 (99.91%) | 14,520 (99.90%) |
| Of shared scans, same backbone but shifted phospho-site call | 9 (0.06%) | 7 (0.05%) |
| Of shared scans, genuinely different peptide call | 4 (0.03%) | 8 (0.06%) |

**Shared scans agree essentially perfectly.** For the >99.9% of scans both searches
identify and both consider confident, masking changes nothing about *which* peptide wins --
exactly as expected, since masking never alters XCorr scoring itself, only which candidates
reach the fragment index in the first place. The tiny remainder splits into two mechanistically
distinct categories: a handful of scans (9 R1, 7 R2) where the two runs agree on the peptide
backbone but disagree on which of two adjacent/nearby S/T/Y residues carries the phospho
group (genuine site-localization ambiguity, not a masking artifact -- AScorePro's own site
score would be the right tool to adjudicate these, not raw xcorr), and a smaller handful (4
R1, 8 R2 -- 0.03-0.06% of all shared scans) where masking's altered candidate pool tips a
near-tied contest toward a *different* peptide backbone entirely, presumably two
near-isobaric candidates whose relative index footprints happened to change enough under
masking to flip which one wins rank-1.

**Characteristics of the PSMs unique to each run** (mean/median over the scan-exclusive
set; "phospho-containing"/"Met-oxidized" = fraction of peptides carrying that mod anywhere):

| | R1 unmasked-only (n=149) | R1 masked-only (n=580) | R1 shared (n=15,213, reference) | R2 unmasked-only (n=150) | R2 masked-only (n=740) | R2 shared (n=14,535, reference) |
|---|---|---|---|---|---|---|
| Mean xcorr | 2.27 | 1.64 | 3.14 | 1.97 | 1.55 | 3.04 |
| Median peptide length (residues) | 21.0 | 12.0 | 16.0 | 20.0 | 12.0 | 16.0 |
| Charge 3+ fraction | 96.6% | 33.6% | 59.3% | 94.0% | 35.3% | 57.7% |
| Phospho-containing | 98.0% | 95.9% | 98.8% | 98.0% | 98.4% | 99.5% |
| Met-oxidized | 14.8% | 7.9% | 5.7% | 24.0% | 7.9% | 7.1% |

(An earlier draft of this table reported median lengths of 8.0/4.0/5.0 and 7.0/4.0/5.0 --
those were wrong, from a peptide-length parser bug that split the modified-peptide string on
every `.` character, including the decimal points inside modification masses like
`[79.9663]`, truncating the sequence before counting it. Fixed by extracting the core
sequence with a regex anchored on the flanking-residue dots specifically
(`^([A-Z-])\.(.*)\.([A-Z-])$`) before stripping bracketed mod annotations. The corrected
values above are internally consistent with `peptide_length_range = 7 35` (Section 3.1);
the original values were not (4 and 5 residues are below the configured minimum of 7, which
should have been caught before this document was first written).)

**Interpretation, in both replicates independently:**

- Peptide length **increases monotonically** from masked-only (median 12) to the shared
  population (median 16) to unmasked-only (median 20-21) in both replicates -- a clean,
  reproducible ordering, not just a two-group contrast.
- **PSMs unique to the unmasked run** skew toward *longer, higher-charge, more heavily
  (Met-)oxidized* peptides than the shared population (median length 20-21 residues vs. 16;
  charge 3+ in 94-97% vs. ~58-59% of shared PSMs; oxidized-Met roughly 2.5-4x enriched).
  Longer, more heavily and multiply modified, higher-charge peptides are exactly the
  population a fragment-intensity predictor is intrinsically hardest to get right for (more
  cleavage sites over which to distribute intensity correctly, more co-occurring
  modifications, higher-charge fragment channels) -- consistent with masking's small recall
  cost concentrating specifically in the harder tail of the peptide-length/modification-
  complexity distribution, where Carafe's own predictions are least reliable.
- **PSMs unique to the masked run** skew toward *shorter, lower-charge* peptides (median
  length 12 residues vs. 16 for the shared population and 20-21 for unmasked-only; charge 2
  in roughly two-thirds of cases vs. ~40-42% of shared PSMs) sitting right at the 1% FDR
  boundary (mean xcorr 1.55-1.64, vs. 3.04-3.14 for the shared population). This is
  consistent with masking's benefit being concentrated in the *shorter* end of the peptide
  population: a shorter peptide has fewer candidate fragment-ion positions in total, so the
  smaller number of index-eligible fragments it does have face less competing noise from
  unrelated, coincidentally-mass-matching candidates in the far larger unmasked index --
  letting its true (correct but modest-scoring) identification surface as rank-1 instead of
  being edged out by a spurious competitor that unmasked's full index still considers a
  candidate.
- The **net gain is reproducible in direction and rough magnitude across two independent
  acquisitions**: masked-only PSMs outnumber unmasked-only PSMs by 3.9x in R1 (580 vs. 149)
  and 4.9x in R2 (740 vs. 150).

## 9. Limitations / caveats for a manuscript writeup

- **Two acquisitions, same sample-prep lineage.** `MM2_R1.raw` and `MM2_R2.raw` are two runs
  from the same directory/project (their naming suggests technical or closely-related
  biological replicates of the same experiment, not independent biological samples or
  independently-processed cohorts) -- both show the same direction and similar magnitude of
  effect (Sections 7-8), which is reassuring for reproducibility but does not substitute for
  a broader validation panel (multiple distinct samples, instruments, or labs) before
  generalizing the magnitude of the FDR improvement in a manuscript.
- **CPU-only Carafe inference.** No GPU was available on this machine; a prior, separate
  full-scale run on this project used GPU inference and was retired specifically because its
  search-space configuration diverged from what's used here (see project history) -- so this
  run's ~11.95h inference time is not directly comparable to that GPU run's timing, only to
  itself as a CPU-only baseline.
- **One FDR threshold.** Only 1% FDR was computed for the masked-vs-unmasked comparison
  (per the original request); `tools/qvalue.py` supports arbitrary thresholds and a full
  FDR curve (e.g. 0.1%-10%) would give a fuller picture of where the masked/unmasked gap is
  largest.
- **One mod configuration, one mask-threshold setting.** `--min-relative-intensity 0.10
  --min-kept-peaks 6` were used as-is (the project's previously-validated defaults) --  no
  sweep over these thresholds was performed in this run.
- **RTS timing reflects a single run each**, not averaged over multiple repetitions; prior
  work in this project (`tests/rts_repro/`) has established RTS is deterministic
  byte-for-byte across thread counts for identical inputs, but wall-clock timing itself is
  a single-sample wall-clock measurement subject to ordinary machine noise (see Section 6.1's
  general point about shared-host contention -- confirmed not a factor for the RTS runs
  specifically, both run back-to-back on an otherwise idle machine).
- **`equal_I_and_L` mismatch** (Section 4) between the batch `.idx`-build config and RTS's
  hardcoded runtime default is present but judged inconsequential to scoring; not
  independently verified by a targeted test in this session.
- **Site-localization disagreements were classified by string comparison, not re-scored.**
  Section 8.1's "same backbone, different site" vs. "genuinely different peptide"
  categorization is a bracket-stripped sequence-equality check, not an independent
  localization-confidence adjudication (e.g. via AScorePro's own site scores, which RTS
  already computes -- `--ascorepro 1` -- but which this analysis did not pull into the
  comparison). A manuscript claiming these are true site-localization ambiguities rather
  than coincidentally-adjacent mod placements should verify against the AScorePro scores in
  `rts.out`'s own `AScore`/`Sites` fields.
- **The Section 8.1 comparison script is not committed** (Section 4) -- it is a short,
  one-off analysis snippet built directly on `tools/qvalue.py`'s existing functions, not a
  reusable tool in the sense of Section 5's.

## 10. Artifact locations

All working files for this run live under
`/mnt/c/Work/Comet-master/20260420-human-phosho/carafe_phosphosmall/` (not committed to the
git repository -- this is a scratch/data working directory, matching the convention of the
other dated working directories alongside it). Key files, for anyone re-opening this
analysis:

- `phosphosmall.fasta.idx` -- the FI_DB index (input to both RTS runs)
- `phosphosmall.variants_export.tsv` -- raw `comet.exe -x` export
- `phosphosmall.carafe_peptides.tsv` / `.carafe_peptides.variants.tsv` -- Carafe-ready
  peptide list + row-to-variant provenance
- `prediction/chunks/`, `prediction/chunk_preds/` -- per-chunk Carafe inference inputs/
  outputs (chunk_preds retained; pass `--delete-raw` to `carafe_prerun.sh` on a future run to
  reclaim this space once the `.cps` store is verified)
- `phosphosmall.cps` -- compact prediction store (durable artifact; regenerating the mask
  with different threshold settings only needs this file + `carafe_cps_to_fi_mask.py`, not a
  re-run of Carafe inference)
- `phosphosmall.fi_mask` -- the predicted-fragment mask consumed by
  `fragment_index_predicted_mask_file` (batch) / `--mask` (RTS)
- `.prerun/*.log`, `.prerun/*.done` -- per-stage driver logs and resume markers
- `rts_unmasked.out` / `rts_masked.out` (R1) and `rts_r2_unmasked.out` / `rts_r2_masked.out`
  (R2) -- written to `RealtimeSearch/bin/x64/Release/` at run time, moved into
  `carafe_phosphosmall/` after each run -- and their `rts_unmasked.txt` / `rts_masked.txt` /
  `rts_r2_unmasked.txt` / `rts_r2_masked.txt` qvalue.py-ready conversions
