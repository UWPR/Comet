# Carafe as an ahead-of-time step: size, speed, and the search-time budget

**Date:** 2026-08-22
**Status:** IMPLEMENTED. All of M1-M4 and M6 are DONE (Section 8); M5 was CANCELLED (M1b's
own measurement showed the existing load path already passes the search-time budget by
>10x, so no load-path optimization was needed). This document was written as a plan and
updated in place as each milestone completed -- Section 8's per-milestone status lines are
the authoritative current state, not this top line's original "PLAN" framing.
**Companion docs:** `docs/20260805_carafe.md` (design + phase history through Section 6.24,
including the 2026-08-23/24 master-merge and two post-merge real-data reconfirmations of
this pipeline's output),
`docs/20260816_carafe_gpu_benchmark_setup.md` (GPU benchmark runbook and corrected disk
accounting -- also since retired, see that doc's own status note),
`docs/20260824_carafe_phoshoresults.md` and `docs/20260824_carafe_oxmet_fi_vs_fimask.md`
(two independent FI-vs-FI-masked analyses run entirely through this pipeline, 2026-08-24,
with full per-stage timing/memory on real data -- the pipeline's first real production use
after M1-M6 landed).

## 1. Problem statement

The Carafe/FI-masking feature works end-to-end at real full-proteome phospho scale (both the
withNL and noNL masks for the 124,863,304-variant `phospho_charge2` population exist and are
verified, doc Sections 6.20/6.21), but the first real full-scale run exposed two operational
problems and left one critical measurement missing:

1. **Output size.** One full Carafe prediction run produces ~386-395GB of TSV output
   (measured on both the CPU and GPU machines independently). This is not sustainable as a
   per-database artifact.
2. **Runtime.** The prediction pass takes 26h19m (GTX 1660 Super GPU) to 58h17m (CPU,
   single-threaded) for 124.8M rows. This is fine as an ahead-of-time batch job and
   categorically NOT fine anywhere near a search: when a Comet FI search triggers the
   in-memory FI build, that path must never trigger Carafe.
3. **Unmeasured: search-time mask cost.** The FI build that consumes the mask -- mask-file
   load + masked `GenerateFragmentIndex()` -- has never been timed at real scale. Every
   masked search to date used masks 35x smaller (oxmet: 3.5M entries / ~150MB) than the real
   phospho mask (124.8M entries / 4.9GB).

**Hard requirement (confirmed 2026-08-22):** total search-startup cost -- mask load + FI
build together, on BOTH the batch path and the RTS-initialization path -- must be tens of
minutes at most, ideally much less. Redesigning the mask-load path is in scope if the current
eager load blows that budget.

## 2. What already exists (so the plan builds on it rather than re-inventing it)

The ahead-of-time separation is already the architecture -- Comet has never triggered Carafe
inline. The pipeline today, with real measured full-scale numbers (124.8M-row
phospho_charge2 population, this machine unless noted):

| Stage | Tool | Time (measured) | Output (measured) |
|---|---|---|---|
| 1. `.idx` build | `comet.exe -i` | ~1 min | 289MB `.idx` |
| 2. Variant export | `comet.exe -x` | minutes | 6.6GB `variants_export.tsv`-style file |
| 3. Carafe-format conversion | `tools/idx_to_carafe.py` | tens of minutes | 9.5GB out_tsv + 3.9GB variant map |
| 4. MS2/RT prediction | `tools/run_carafe_chunked.sh` -> `ai_pred.py` (2,498 x 50K-row chunks) | **26.3h GPU / 58.3h CPU** | **386-395GB** TSV across chunk dirs |
| 5a. withNL mask build | `tools/build_carafe_mask_chunked.sh` (per-chunk `carafe_ms2_to_fi_mask.py`) | ~5.5h (19,996s, 2,498 chunks) | 2,498 per-chunk `.fi_mask` |
| 5b. noNL mask build | same, `--ignore-modloss` + noNL `.idx`/variant map | same order | same |
| 6. Merge | `tools/merge_carafe_fi_masks.py` | ~15 min | one 4.9GB `.fi_mask` (124,863,304 entries) |
| 7. Search-time consumption | `CometPredictedMask::Load()` + masked FI build | **UNMEASURED at this scale** | -- |

Search-time consumption today: `fragment_index_predicted_mask_file` in comet.params (batch)
or the RTS harness's `--mask` flag; `Load()` runs after `ReadPeptideIndex()`, before
`GenerateFragmentIndex()`, validates magic/version + `SourceIdxFingerprint` +
`SourceIdxNumRawPeptides` + `VarModConfig` against the live session, then eagerly reads
every entry into a sorted `std::vector<Entry>`; `Lookup()` binary-searches it per variant
during `AddFragments()`.

Two size facts that matter to this plan (measured, Section 6.21 aftermath):

- **~52% of the 386GB is files the mask pipeline never reads.** Per chunk, `ai_pred.py`
  writes 4 outputs; `carafe_ms2_to_fi_mask.py` consumes only `_ms2_df.tsv` (echoed input +
  frag row ranges) and `_ms2_pred.tsv` (intensities). `_ms2_mz_df.tsv` (~45% of bytes) and
  `_rt_pred.tsv` (~7%) are dead weight for masking.
- The final mask is 4.9GB -- 79x smaller than the predictions it came from -- but it bakes in
  one specific threshold/floor choice (`--min-relative-intensity`, `--min-kept-peaks`), so
  keeping ONLY masks forfeits cheap re-sweeps (doc Section 8 item 5's design intent).

## 3. Decisions taken 2026-08-22 (user-confirmed)

1. **Orchestration = one driver script in `tools/`** (working name `tools/carafe_prerun.sh`)
   that runs stages 2-6 end to end, resumable at every stage. `comet.exe` stays
   Carafe/Python-free, preserving doc Section 8 item 6's design decision.
2. **Parquet: measure first, then decide.** Real numbers from 1-2 re-run chunks with
   `ai_pred.py --fast` before any dependency decision. (See Section 6 -- under this plan's
   retention model, parquet's role shrinks to intermediate I/O, not archival.)
3. **Budget binds the total** -- mask load + FI build, batch AND RTS init -- at tens of
   minutes maximum. Load-path redesign in scope.
4. **Retention: a compact, Comet-oriented prediction store becomes the durable artifact.**
   Once predictions are translated into it and verified, ALL raw Carafe output (TSV or
   parquet, including `_ms2_mz_df`/`_rt_pred`) is discardable. This is the plan's central
   new design item (Section 5).

## 4. Target architecture

```
AHEAD-OF-TIME (hours; GPU machine or overnight CPU; run per database+mod-config)
  tools/carafe_prerun.sh:
    [stage 1]  comet.exe -i                       (per .idx flavor: withNL, noNL)
    [stage 2]  comet.exe -x  + idx_to_carafe.py    (per .idx flavor; populations identical,
                                                   only VarModConfig/variant-map differ)
    [stage 3]  run_carafe_chunked.sh              (ONCE, phospho mode -- the expensive step)
    [stage 4]  NEW: translate chunk predictions -> compact prediction store (.cps)
    [stage 5]  verify .cps against source chunks; DELETE raw Carafe output
    [stage 6]  build .fi_mask(s) from .cps        (minutes -- re-runnable at any
                                                   threshold/floor without Carafe)

SEARCH TIME (tens of minutes budget, hard)
    comet.exe / RTS init:
      ReadPeptideIndex() -> CometPredictedMask::Load(mask) -> GenerateFragmentIndex()
      (never touches Carafe, ai_pred.py, python, or the .cps -- only the small .fi_mask)
```

The search-time contract is unchanged from today -- Comet consumes a `.fi_mask` file, full
stop. Everything new happens upstream of it, plus measurement/optimization of the
consumption path itself (Section 7).

## 5. New design item: the compact prediction store (`.cps`)

### 5.1 What it must hold

Exactly what mask building consumes, nothing else. From `compute_variant_mask()`'s actual
reads: per out_tsv row, the per-ladder-position intensities for the four channels masking
thresholds (`b_z1`, `y_z1`, `b_modloss_z1`, `y_modloss_z1`) plus the row's base peak
(which is defined over ALL 8 predicted channels including z2 -- so it must be stored, not
recomputed from the 4 kept channels).

### 5.2 Keying: by out_tsv row_index, not by sequence strings

The store is aligned 1:1 with a specific out_tsv's row order (`row_index` = 0-based data-row
position, the same key `idx_to_carafe.py`'s variant-map sidecar already uses). This:

- eliminates all sequence/mods strings from the store (the single biggest size cost in the
  raw TSVs);
- resolves Carafe's internal `sort_values('nAA')` reordering ONCE at translation time (the
  same content-tuple join `carafe_ms2_to_fi_mask.py` already does per chunk), instead of at
  every mask build;
- keeps the store neutral across `.idx` flavors: the withNL and noNL `.idx` files share the
  identical peptide population and out_tsv row order (verified empirically, Section 6.20 --
  124,863,304 rows, spot-checked content-identical at matching line numbers), so ONE store
  serves both mask builds; each flavor's own variant map supplies its own
  (iWhichPeptide, modNumIdx, ...) tuples and VarModConfig.

Header must carry provenance to make misuse loud, mirroring the mask format's own design:
the source out_tsv's row count and a content fingerprint (e.g. CRC-32 of its first+last N
bytes + length, or a full-file CRC computed during the one sequential translation pass),
creation parameters, and the ai_pred.py mode. NOT the .idx fingerprint -- the store is
deliberately .idx-flavor-neutral; .idx binding stays where it already lives (variant map ->
mask file).

### 5.3 Layout sketch and size estimate

Fixed-size directory + variable-size payload, all little-endian, designed for a single
sequential write and a single sequential read (no random access needed -- mask builds
iterate the whole population anyway):

```
[magic/version line]
[header: source row count, fingerprint, mode, quantization params]
[directory: per row_index -> (payload offset : u64)]        8B x 124.8M = ~1.0GB
[payload per row:
   nAA            : u8
   base_peak      : f32                                      (or u16 scaled -- decide in M2)
   4 channels x (nAA-1) positions x u8                       (intensity as fraction of
                                                             base_peak, 1/255 granularity)]
```

Per-row payload at this population's length distribution: 5 + 4x(nAA-1) bytes; average
nAA ~15-20 -> ~60-80 bytes/row -> **~8-11GB total** for 124.8M rows, vs 386GB raw --
a ~35-45x reduction, before any general-purpose compression (optionally zstd the payload
region; decide by measurement in M2, weighing decompression time against the store's own
read path which is offline-only anyway).

### 5.4 Quantization correctness

uint8 relative intensity (1/255 ~ 0.4% granularity) against a 10%-of-base-peak default
threshold flips a keep/drop decision only for fragments within ~0.2% of the threshold.
Whether that is acceptable is an empirical question, answered in M2 by rebuilding an
already-verified real mask (the oxmet charge2 mask, and one full-scale phospho chunk range)
from a `.cps` and diffing against the TSV-built original:

- if bit-flips are zero or negligible (expected): uint8 stands;
- if not: u16 (payload grows to ~16-21GB, still ~20x under raw) or store the exact f32 for
  the 4 kept channels (~4x u8 size). Decide on the measured flip count, not speculation.

The floor path (`--min-kept-peaks` top-up) sorts candidates by intensity; quantization ties
could reorder marginal picks. The rebuild-diff in M2 measures this too. Tie-breaking in the
store-based builder must replicate `carafe_ms2_to_fi_mask.py`'s current stable ordering so
the comparison isolates quantization effects only.

### 5.5 New/changed tools

- **NEW `tools/carafe_pred_to_cps.py`** (stdlib-only, matching the existing convention):
  streams each chunk's `_ms2_df.tsv`/`_ms2_pred.tsv` (or parquet, pending Section 6's
  measurement -- parquet input would break the stdlib-only rule and is part of that
  decision), does the content join, writes the `.cps` in one pass. Chunked input means
  bounded memory by construction. Also the natural place to (optionally) delete each raw
  chunk directory after its rows are verified written -- keeping peak disk usage flat
  instead of 386GB + store.
- **CHANGED `tools/carafe_ms2_to_fi_mask.py`**: gains a `.cps` input mode (`--from-cps`)
  alongside the existing TSV mode (kept for tests and small runs). A full-population mask
  build from `.cps` should be a single-pass, memory-bounded operation -- target: minutes,
  replacing today's 5.5h chunked build + 15-min merge (which exist only because the TSV
  inputs are 386GB; the chunked mask-build tools from commit `1187d98d` remain for
  TSV-sourced builds but stop being the primary path).
  **[As built (M3, Section 8): not a `--from-cps` flag on this script.]** `carafe_ms2_to_
  fi_mask.py`'s 6 positional TSV arguments have no sensible meaning in `.cps` mode, so the
  store-based path shipped as its own CLI, `tools/carafe_cps_to_fi_mask.py`, instead --
  same capability, all decision logic still shared via `carafe_cps.
  compute_variant_mask_from_cps()` calling back into this script's own threshold/floor/pack
  helpers. `carafe_ms2_to_fi_mask.py` itself gained no `--from-cps` flag and remains
  TSV-only.
- **NEW `tools/carafe_prerun.sh`**: the Section 3 decision-1 driver. Thin orchestration over
  the existing + new tools; per-stage `.done` markers; `--resume` semantics identical to
  `run_carafe_chunked.sh`'s (which it invokes for stage 3); a `--start-at`/`--stop-after`
  pair for running partial pipelines (e.g. masks-only re-sweep from an existing `.cps`).

## 6. Parquet: what to measure and how the retention decision changes its role

Under Section 5, raw prediction files -- TSV or parquet -- are transient: they exist between
stage 3 and stage 5, then get deleted. So parquet is no longer an archival-compression
question; it matters only if it meaningfully improves the transient pipeline:

1. **Disk high-water mark** during stage 3-4 (386GB TSV today; zstd parquet of low-entropy
   float text plausibly 3-8x smaller -- measure, don't assume).
2. **Write time inside `ai_pred.py`** (TSV serialization of ~3.6B fragment rows is real CPU;
   parquet may be faster or slower -- measure).
3. **Read time in stage 4's translator** (pyarrow reads vs stdlib TSV parsing).

Measurement protocol (M1, cheap): re-run 2 already-completed real chunks (one small, one
large: e.g. chunk_00000 at 304K fragment rows and chunk_01500 at 1.66M) through `ai_pred.py
--fast` on this machine, same venv. Record: per-file sizes vs the existing TSV outputs,
wall time vs the chunk's original `.elapsed_seconds`, and a pyarrow read-back timing.
Decision rule (to be ratified when numbers exist): adopt parquet for the transient stage
only if it cuts the high-water mark >=3x or stage-3 wall time >=10%, AND the added
pyarrow/pandas dependency stays confined to `carafe_pred_to_cps.py` (never
`carafe_ms2_to_fi_mask.py`'s mask-from-cps path, which stays stdlib-only).

### 6.1 M1a RESULTS (measured 2026-08-22, this machine, `~/.carafe/.venv`)

One constraint discovered before any numbers: **`--fast` is all-or-nothing** --
`predict_rt()`/`predict_ms2()` read `--in_file` with `pd.read_parquet()` unconditionally in
fast mode, so the INPUT chunk must itself be parquet (a TSV input fails with "Parquet magic
bytes not found"). Input conversion is trivial (0.1s per 50K-row chunk via
pandas.to_parquet, zstd) and itself wins ~14x on size (4.06MB TSV -> 281KB parquet).

| File | chunk_00000 TSV -> parquet | chunk_01500 TSV -> parquet | reduction |
|---|---|---|---|
| ms2_df | 2.03MB -> 0.59MB | 5.70MB -> 0.69MB | 3.5-8.2x |
| ms2_pred | 13.08MB -> 3.00MB | 61.26MB -> 7.51MB | 4.4-8.2x |
| ms2_mz_df | 17.03MB -> 2.07MB | 114.24MB -> 10.74MB | 8.2-10.6x |
| rt_pred | 3.65MB -> 1.39MB | 7.17MB -> 1.46MB | 2.6-4.9x |
| **all outputs** | 35.78MB -> 7.05MB (**5.1x**) | 188.37MB -> 20.41MB (**9.2x**) | |

- Byte-weighted overall reduction ~8-9x (large chunks dominate total bytes): the real 386GB
  run would land around **~42-47GB** transient high-water mark.
- **Zero inference-time cost**: 25s vs the chunk's original 27s, and 104s vs 108s -- within
  ordinary run-to-run noise, matching the expectation that model inference (not
  serialization) dominates per-chunk wall time.
- Read-back (pandas): parquet 3-14x faster (biggest file: 0.06s vs 0.85s).

**Decision (per the pre-committed rule above): ADOPT parquet for the transient stage** --
the >=3x size threshold is exceeded ~3x over. pyarrow/pandas dependency confined to the
stage-3/4 boundary (`run_carafe_chunked.sh` gains a parquet mode that converts each input
chunk inline before invoking `ai_pred.py --fast`; `carafe_pred_to_cps.py` reads the parquet
outputs); the mask-from-cps path stays stdlib-only as planned.

## 7. Search-time budget: measure, then (likely) fix the load path

### 7.1 What to measure first (M3 -- no code changes until these numbers exist)

On this machine, against the real 4.9GB phospho masks and the real `phospho_withNL.idx` /
`phospho_noNL.idx`:

1. `CometPredictedMask::Load()` wall time + peak RSS delta at 124.8M entries (eager
   `std::vector<Entry>` load; `Entry` is ~48B aligned in RAM -> ~6GB expected).
2. Masked `GenerateFragmentIndex()` wall time vs unmasked, same `.idx` -- isolates the
   per-variant `Lookup()` binary-search cost (~27 compares x ~125M lookups over a 6GB
   array = cache-hostile; could be seconds or could be many minutes -- unknown until
   measured).
3. The same pair on the RTS-init path (`RealtimeSearch.exe --mask`), since RTS is the
   latency-sensitive consumer.
4. Same measurements with the 35x-smaller oxmet mask as a scaling sanity check against the
   already-known-good numbers.

### 7.2 Optimization options, in escalation order (apply only what the numbers demand)

1. **Nothing.** If load + masked build lands well inside the budget (plausible: 4.9GB
   sequential read at even 200MB/s is ~25s; the sort is already done on disk), record the
   numbers in this doc and stop.
2. **Merge-join instead of binary search.** `GenerateFragmentIndex()` iterates
   `g_vRawPeptides` in iWhichPeptide order and enumerates each peptide's mod combinations;
   mask entries are sorted by exactly that tuple. A per-thread cursor advancing through the
   entry array (two-pointer merge) replaces ~27 random-access compares per lookup with ~1
   sequential advance -- removes the cache-hostile term entirely. Moderate, contained change
   inside `CometPredictedMask` + its `AddFragments()` call site.
3. **mmap the mask file and search/merge in place.** Eliminates the parse + 6GB copy
   entirely; requires a v4 mask format with fixed-stride naturally-aligned entries (current
   v3 entries are 42B packed/unaligned) and a Windows/Linux mmap shim (`CreateFileMapping` /
   `mmap`). Bigger change; only if measurement shows load itself (not lookups) dominates and
   option 2 is insufficient.
4. **Coarser fallback** (only if all above fail the budget, not expected): persist the
   POST-mask FI to disk per (idx, mask) pair. Rejected as a first resort -- it re-couples the
   FI's in-memory format to a disk format, exactly what the current architecture avoids
   (doc Section 6.13's "FI is rebuilt fresh every start" finding).

### 7.3 Explicit non-goal

Speeding up Carafe inference itself is out of scope here. It is the ahead-of-time step by
design; 26h on a modest GPU is acceptable for a per-database offline job, and the GPU
benchmark doc already covers that axis.

### 7.4 M1b RESULTS (measured 2026-08-22): budget PASSED, escalation ladder ends at "nothing"

All measurements at full production scale: `phospho_noNL.idx` (full canonical human
target+decoy proteome, 40,908 proteins, tryptic 2mc, len 7-50, 700-5000 Da, M-ox + STY-
phospho max 3 -> 124,863,304 peptide-mod variants), the real 4.9GB / 124,863,304-entry
noNL mask, real spectra (`20170103_HelaQC_01.mzXML` for batch, `20170103_Hela_01.raw` for
RTS -- 40,302 MS2 spectra either way), 16 threads.

| Path | Unmasked total | Masked total | Mask overhead | FI entries (un/masked) | Peak RSS (un/masked) |
|---|---|---|---|---|---|
| Batch, Linux/WSL binary, run 1 | 2m 38.9s | 2m 52.6s | +13.7s | 3.540e9 / 1.161e9 | 20.2GB / 16.8GB |
| Batch, Linux/WSL binary, run 2 | 2m 06.2s | 2m 40.8s | +34.6s | identical | identical |
| Batch, Windows binary (native NTFS) | 1m 29.4s | 1m 52.5s | +23.1s | identical | 19.3 / 16.0GB (comet self-report) |
| **RTS init, Windows** (`RealtimeSearch.exe --mask`) | **init 79.2s** (total 85.3s) | **init 103.9s** (total 109.4s) | +24.7s init | identical | -- |

Key findings:

1. **The budget question is closed: worst observed masked startup is 2m 53s** -- ~2% of a
   30-minute budget -- and the latency-critical RTS-init path is under 2 minutes masked.
   **Section 7.2's escalation ladder ends at option 1 ("nothing")**: no merge-join, no mmap
   v4 format, no persisted-FI fallback. Milestone M5 is unnecessary.
2. **Mask load costs ~25s** (4.9GB sequential read + parse into the eager vector), the
   only material masked-vs-unmasked overhead. The feared per-variant binary-search cost is
   a non-issue: on Linux the masked FI populate is actually FASTER than unmasked (1m30 vs
   1m44 -- inserting 3.05x fewer entries more than pays for the lookups); on Windows it's
   mildly slower (1m28 vs 1m11) -- the balance tips per platform, but both are noise
   against the budget.
3. **Masking REDUCES peak memory** despite the ~6GB in-RAM mask vector: 16.8GB vs 20.2GB
   (batch, Linux), because dropping 2.38e9 FI entries saves more than the mask costs.
4. **Masking mildly speeds the search itself**: 1816 vs 1736 Hz (batch), 12,191 vs 11,513 Hz
   (RTS) -- consistent with Sections 6.15-6.19's smaller-scale findings, now confirmed at
   3.5-billion-entry scale.
5. **Cross-platform determinism datapoint**: FI entry counts (3.540e9 unmasked / 1.161e9
   masked, -67.2%) are bit-identical across the Linux and Windows binaries and across
   repeat runs.
6. **DrvFs variance is real, as predicted**: unmasked Linux totals swung 2m06-2m39 (21%)
   between two same-day runs with no code change; the Windows binary on native NTFS is
   ~40% faster than the same-source Linux binary on `/mnt/c`. Single-sample deltas smaller
   than ~30s on this setup are noise.

Measurement caveats, stated honestly: the two Linux runs were same-day, ~1h apart (the
plan asked for non-consecutive -- the 21% swing already visible between them makes the
point regardless); the Windows `Comet.exe` was built 2026-08-11 (2 days older than the
Linux binary -- predates the T28 fix, which is provably behavior-identical for this no-NL
config since dMaxNL=0.0 reduces the new break to the old one); `/usr/bin/time`'s RSS is
not meaningful for Windows-interop processes, so Windows memory numbers are comet's own
self-reported figures.

Incidental finding: the `RealtimeSearch.exe` copy sitting in `20260420-human-phosho/`
(2026-08-11 13:37) is a stale PRE-flag-conversion build (positional args, no `--mask`) --
the first RTS measurement attempt failed confusingly against it ("Invalid
index_search_type" swallowing the `--db` value). The current-interface binary lives at
`RealtimeSearch/bin/x64/Release/RealtimeSearch.exe` (2026-08-11 21:12). Stale tool copies
in scratch dirs are a recurring trap; the M4 driver script should always invoke tools by
repo path, never rely on copies.

## 8. Milestones

- **M1 -- Measurements, no code** -- **DONE 2026-08-22**:
  a. Parquet: Section 6.1. Decision: ADOPT for the transient stage (~8-9x size, no
     inference-time cost).
  b. Search-time: Section 7.4. Budget passed >10x on every path; the existing masks are
     production-usable as-is and **M5 is unnecessary**.
- **M2 -- `.cps` format + translator** -- **DONE 2026-08-22**:
  - Format implemented in `tools/carafe_cps.py` (writer/reader + `compute_variant_mask_from_cps()`,
    which shares the TSV path's own threshold/floor/pack helpers so divergence can only come
    from quantization); translator in `tools/carafe_pred_to_cps.py` (multiprocessing over the
    existing prediction chunks); 7 unit tests in `tests/unit/test_carafe_cps.py`, all passing.
  - **Quantization decided empirically (Section 5.4's experiment, 10 real chunks / 463,304
    variants, full write->read roundtrip, diffed bit-for-bit against the on-disk ground-truth
    noNL chunk masks): u8 diverges on 2.81% of variants -- too high; u16 on 0.0086%**
    (40 variants, each a single fragment within ~0.0015% of the arbitrary 10% threshold;
    kept-bit totals differ by +5 in 4.27M). **Decision: u16.** Notably u8's kept-bit totals
    were also nearly identical (-17 in 4.27M) -- its differences are which marginal fragment
    survives, not systematic mask shrinkage -- but 2.8% of variants differing is not
    "equivalent" and u16's 2x size cost is cheap at this scale.
  - Real full-scale translation: **124,863,304 rows in 25.4 min** (12 workers, ~97K rows/s
    sustained) -> `phospho_charge2_withNL.cps`, **31.1GB** (12.4x under the 386GB raw;
    larger than Section 5.3's 15-20GB u16 estimate because the variant-weighted mean peptide
    length is ~30 residues, not the ~15-20 guessed -- combinatorial mod-site enumeration
    skews variants long; Section 5.3's estimate is hereby corrected by measurement).
  - Final-store verification: header/provenance checks pass (head-CRC + row count vs the
    real out_tsv), and the rebuild-diff re-run THROUGH the final 31GB store (5 chunks /
    213,304 variants, global row_index addressing) reproduces the experiment's rate:
    **16 differ (0.0075%)**. Random access is fast: 213K variants read+rebuilt+diffed in 14s.
  - **A real memory bug was found and fixed during the first full-scale attempt**: workers
    returned each chunk's rows as Python object graphs (~150MB/chunk) to a parent that did
    all struct-packing itself -- the slow consumer let `Pool.imap`'s in-order result buffer
    accumulate dozens of chunks: **44.2GB parent RSS in 164s** (vs 54GB total; caught by
    watching `free` mid-run, killed before OOM). Fix: pack bytes in the workers via a shared
    `pack_row()` serializer (packed chunk ~7MB; parent write-only, no backlog possible;
    proven byte-identical to the per-row path by a dedicated unit test) + the writer's
    offsets ledger as `array.array('Q')` (1GB) instead of a Python int list (~4GB). Fixed-run
    parent RSS: **0.4-1.1GB end to end**. Same lesson class as Section 6.16 Fix 2 in the
    main Carafe doc: per-row Python object overhead is the recurring enemy at 1e8-row scale.
  - Raw prediction TSVs on this machine are now deletable (M6, user sign-off) -- but note
    the GPU-vs-CPU prediction diff (Section 9) wants the GPU machine's predictions
    translated through this same tool first, and the CPU raw TSVs are the only local
    fallback if that comparison surfaces questions -- reclaim the ~380GB only after the
    diff is done or explicitly waived.
  - Also survived, worth recording: the first fixed-run launch was killed ~45 min in by
    another spontaneous WSL restart (the same environment failure mode that killed the
    original 51h Carafe run on 2026-08-16) -- the two-phase writer left no misleading
    half-store, exactly as designed; the rerun completed clean.
- **M3 -- mask-from-cps** -- **DONE 2026-08-22**:
  - Implemented as its own CLI, **`tools/carafe_cps_to_fi_mask.py`** (deviation from the
    "--from-cps flag on carafe_ms2_to_fi_mask.py" sketch above: that script's 6 positional
    TSV arguments have no meaning in cps mode -- same capability, cleaner interface; all
    decision logic still shared via carafe_cps.compute_variant_mask_from_cps()).
    Scale design: the variant map is streamed in parallel byte ranges (never loaded whole
    -- the TSV builder's whole-file read would be tens of GB here), tuple-group boundary
    handling at arbitrary byte splits is covered by an exhaustive every-split-point unit
    test (which caught three real boundary bugs pre-run), and workers return entries
    packed to bytes (the M2 lesson). 10 unit tests total in test_carafe_cps.py, all green.
  - **A real assumption was falsified by the loud-abort check, exactly as Section 9
    hoped**: variant-map enumeration order is NOT globally key-ordered (observed:
    (1267306, 2458, ...) followed by (762231, 132169, ...) -- mod-variant enumeration
    does not nest inside peptide order; the chunked TSV builder never noticed because it
    re-sorted at every chunk write and again at merge). First full-scale run aborted
    loudly with no output file, as designed. Fix: workers sort their own ranges, parent
    k-way-merges the sorted runs (heapq.merge) while streaming the file out; the written
    file is then re-read and verified strictly increasing (sort order + key uniqueness).
  - **noNL rebuild, full scale**: 124,863,304 entries in **23m43s** (12 workers, peak RSS
    6.2GB), vs the chunked TSV path's ~5.5h build + 15min merge -- **~14x faster**, and
    the plan's "minutes" target is honestly "tens of minutes" (24), not single digits.
    Full-population streaming diff vs the chunk-built ground-truth mask
    (`phospho_charge2_noNL_ignoremodloss.fi_mask`): headers identical field-for-field,
    **0 key mismatches across all 124,863,304 entry pairs**, 11,512 masks differ
    (**0.0092%**, single-bit threshold-boundary flips), kept-bit delta **+62 of
    1,176,101,839 bits**. Consistent with Section 5.4's characterized u16 quantization
    rate (0.0075-0.0086% on samples).
  - **withNL mask built for the first time at full scale** (the chunked path had only
    ever built the noNL flavor): `phospho_charge2_withNL_fromcps.fi_mask`, 124,863,304
    entries in 24m57s. No chunk-built ground truth exists for it, so verification is a
    spot-check against the quantization-free reference (direct TSV-path
    compute_variant_mask() over the raw prediction chunks, has_modloss=True) on 3 sample
    chunks / 113,304 variants, keys looked up by binary search in the built file:
    **113,300 identical, 4 differ (0.0035%), 0 missing keys**.
  - Both real masks (`*_fromcps.fi_mask`) now exist under `20260420-human-phosho/`,
    regenerable at any threshold/floor from the 31GB store in ~24 min without the raw
    TSVs and without Carafe.
- **M4 -- `carafe_prerun.sh` driver** -- **DONE 2026-08-22**:
  - `tools/carafe_prerun.sh`: one command runs the whole ahead-of-time pipeline --
    per-flavor `.idx` build (`-i`) / variant export (`-x`) / `idx_to_carafe.py`, one
    Carafe prediction pass (via `run_carafe_chunked.sh`, chunk-resumable), `.cps`
    translation, and per-flavor mask builds. "Flavor" = one comet.params mod config
    (canonically the withNL/noNL pair); the first flavor's out_tsv feeds Carafe and the
    store. Each flavor gets its own FASTA copy (Comet writes `<database_name>.idx`, so two
    flavors of one FASTA would clobber -- Section 6.15's old near-miss, now structural).
  - **Two operator-error classes removed by construction**: (1) `--ignore-modloss` is
    auto-detected per flavor from its own variant map's VarModConfig (all neutral-loss
    deltas 0.0 -> general mode), never hand-specified; (2) a cross-flavor population
    identity check (every flavor's conversion row count must equal the primary's) makes a
    mismatched flavor pair fail loudly before any expensive stage. Note this is a
    *mask-format* auto-detection (whether the mask carries modloss channels), separate from
    `--carafe-mode` (default `phosphorylation`), which selects `ai_pred.py`'s own prediction
    mode and must still be set explicitly to `general` for a non-phospho mod space (e.g. an
    oxidized-Met-only search) -- confirmed in practice 2026-08-24, `docs/
    20260824_carafe_oxmet_fi_vs_fimask.md`.
  - Stage-level `.done` markers under `OUT/.prerun/` (delete one to re-run its stage;
    stage 4 additionally resumes at chunk granularity), per-stage logs, `--stop-after`
    for deliberate partial runs (e.g. prediction on the GPU machine), `--delete-raw`
    (default OFF) for reclaiming the transient prediction output post-verification.
  - End-to-end dry run: 100-protein fixture (subset100 target+decoy), phospho withNL/noNL
    params pair crafted for it, charge-2, decoys included -- full pipeline in **~66s**
    (51,980 variants; Carafe step 58s of it), producing both masks with correct headers
    verified (withNL: GeneralMode=0, 42,512/51,980 entries carrying nonzero modloss
    masks; noNL: GeneralMode=1, zero modloss bits; identical entry counts). Note the
    fixture is the 100-protein one, not the plan's "500-protein Phase 5 fixture" -- the
    Phase 5 fixture is oxmet-only, which cannot exercise the withNL/noNL flavor logic.
  - Resume tests: full re-run -> every stage skips; single deleted marker
    (`s6_mask_nonl.done`) -> exactly that stage re-runs; `--stop-after convert` on a
    fresh dir -> stops before prediction with only stages 1-3 outputs on disk.
  - **Deliberately NOT in this driver: the parquet transient mode** (M1a's adoption
    decision). The driver orchestrates the TSV pipeline exactly as validated end-to-end
    at full scale; wiring parquet through `run_carafe_chunked.sh` + the translator is a
    contained follow-up (tracked as M6's remaining pipeline item) whose payoff is the
    transient disk high-water mark (~390GB -> ~45GB during stages 4-5), not correctness
    or steady-state storage (the `.cps` already handles that).
- **M5 -- load-path work IF M1b demands it** (Section 7.2 options 2/3) -- **CANCELLED
  2026-08-22**: M1b (Section 7.4) showed the existing load path passes the budget >10x on
  every path; no optimization warranted.
- **M6 -- housekeeping** -- **mostly DONE 2026-08-22** (one item deliberately held):
  - **Parquet transient mode implemented and validated** (the M1a adoption, folded in
    here): `run_carafe_chunked.sh --parquet` (inline input-chunk conversion, cached;
    `ai_pred.py --fast`), `carafe_pred_to_cps.py` auto-detects parquet chunk outputs
    (pandas/pyarrow confined to the translator; requires the Carafe venv python -- system
    python3 has neither), `carafe_prerun.sh --parquet` passes both through. Validated two
    ways: (1) the same real chunk translated from its TSV outputs and from its M1a parquet
    outputs produces **byte-identical stores** -- across two SEPARATE inference runs, i.e.
    seeded-model determinism and exact float32 text round-trip both confirmed; (2) a full
    parquet-mode driver run on the M4 fixture produces masks whose **all 51,980 entries
    are bit-identical** to the TSV-mode run's (headers differ only in per-run provenance:
    SourceIdxPath and the .idx fingerprint each run's own build binds to -- by design).
    Transient size on the fixture: 80MB -> 17MB.
  - **T29** (`t29_carafe_python_suites`): the four standalone pure-Python Carafe test
    suites (`test_carafe_ms2_to_fi_mask/alignment/idx_to_carafe_dedup_key/carafe_cps.py`)
    -- previously wired into NOTHING, hand-run only -- now run in the default fast suite,
    once per invocation. Full suite after all M6 changes: 47 passed, 0 failed, 0 skipped.
    **Renamed `t38_carafe_python_suites` on 2026-08-24** (`docs/20260805_carafe.md` Section
    6.24) when this project's own T25-T29 tests were renumbered T34-T38 to resolve a
    collision with master's independently-added T26-T33 -- run `t38`, not `t29`, against
    the current suite.
  - **CLAUDE.md**: new "Carafe ahead-of-time pipeline" section (driver one-liner, tool
    roster, pointers to both docs, and the two hard-won invariants: variant-map order is
    not key order; pack worker results to bytes), T29 documented, suite range updated.
  - **Scratch hygiene**: the stale pre-flag-conversion `RealtimeSearch.exe`/
    `CometWrapper.dll` copies in `20260420-human-phosho/` (the Section 7.4 RTS
    measurement trap) replaced with the current build-tree binaries; ~17.3GB of
    regenerable intermediates deleted (the noNL chunked-mask-build split trees +
    per-chunk masks -- the MERGED chunk-built ground-truth mask is kept -- plus the M1a
    parquet scratch and the M4 driver-test run dirs).
  - **Raw prediction-tree deletion: DONE locally 2026-08-22** (user sign-off, explicitly
    waiving the GPU-diff gate: these predictions will be regenerated in the future with a
    more limited peptide search space, so the raw trees have no re-analysis value worth
    ~775GB). The local 395GB `carafe_chunked_phospho_c2_withNL/` tree (split input chunks
    + all per-chunk prediction output) deleted; every durable artifact retained (the
    31GB `.cps` store, all three full-scale `.fi_mask` files, both out_tsvs, both variant
    maps, both `.idx` files, all run logs). The GPU machine's ~395GB tree is outside this
    session's reach -- delete there at will. Consequence, stated plainly: the
    prediction-level GPU-vs-CPU diff (Section 9) is now only possible if the GPU
    machine's predictions are translated to a `.cps` before its own cleanup; with the
    planned regeneration under a new search space, that comparison is effectively
    retired rather than pending.

## 9. Risks / open questions

- **Quantization acceptability** (Section 5.4): resolved empirically in M2; u16 fallback
  costs 2x store size, still ~20x under raw.
- **Merge-join ordering assumption** (Section 7.2 option 2): requires confirming
  `GenerateFragmentIndex()`'s multi-threaded variant enumeration really visits tuples in
  nondecreasing key order per thread partition -- if partitioning is by peptide range
  (it is: threads take `g_vRawPeptides` ranges), per-thread cursors work; verify against
  the actual loop structure before building.
- **Windows parity**: every new tool must run from WSL against `/mnt/c` paths (the proven
  environment) AND the mask/`.cps` consumers must behave identically when `comet.exe` is
  the MSVC build -- M1b/M3 measurements should include one Windows-binary datapoint
  (`x64/Release/Comet.exe`), matching the test suite's dual-binary convention.
- **GPU-vs-CPU prediction equivalence: retired, not pending** (Section 6.21; update per
  M6's own resolution, Section 8, 2026-08-22). This item originally anticipated using M2's
  translator to diff the GPU and CPU prediction runs cheaply via their compact stores. That
  never happened: the local 395GB raw prediction tree was deleted (with explicit user
  sign-off waiving this gate) before the GPU machine's predictions were ever translated,
  and future Carafe predictions are planned to be regenerated under a smaller search space
  regardless -- making the original CPU-vs-GPU-on-this-exact-population comparison moot.
  The GPU machine's own ~395GB tree remains undeleted as of 2026-08-22 (outside this
  session's reach); if it's ever translated to a `.cps` before that cleanup, the comparison
  is still technically possible, but nothing in this project currently plans to do so.
- **`/mnt/c` I/O variance**: all timing measurements land on DrvFs, whose throughput has
  already shown session-to-session swings in this project (Section 6.21's CPU plateau,
  the ghost-file episodes). Each M1/M3/M5 timing should be run twice, non-consecutively,
  before being treated as a real number.
