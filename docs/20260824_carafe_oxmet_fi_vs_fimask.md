# FI vs. FI+Carafe-mask RTS comparison: oxmet, Hela_01 (2026-08-24)

Replicates the methodology of
[uwpr.github.io/Comet/notes/20260729_RTS_2026021.html](https://uwpr.github.io/Comet/notes/20260729_RTS_2026021.html)
(a PI_DB-vs-FI_DB RTS comparison across 1/2/4/8/20 threads, plus PSM-quality-at-FDR tables)
but with the comparison axis changed: **FI_DB unmasked vs. FI_DB + Carafe predicted-fragment
mask**, both backends otherwise identical. Uses the same query raw file as that note's primary
comparison (`20240924_Hela_01.raw`) and the same target-decoy canonical-proteome FASTA, with
a Met-oxidation-only mod space (`comet.params.oxmet.7-35`) instead of that note's PI/FI
mod configs. This document follows the same structure and methodology as
`docs/20260824_carafe_phoshoresults.md` (the MM2 phospho FI-vs-FI-masked analysis) so the two
can be read/compared side by side -- see that document for fuller methods detail on tools,
FDR computation, and the same/different-ID analysis method; this document does not repeat
that detail, only what differs.

## 1. Summary

A masked FI_DB RTS search against `20240924_Hela_01.raw` cut the in-memory fragment index by
**59.0%** (1.305e8 -> 5.345e7 entries) and improved throughput at every thread count tested (1,
2, 4, 8, 20), while simultaneously *increasing* PSMs identified at 1% FDR by **+5.9%**
(xcorr-sorted: 19,015 -> 20,141) and **+3.2%** (e-value-sorted: 20,820 -> 21,494) at 20
threads. A scan-level comparison shows >99.9% agreement between the two search modes on
shared identifications, with the same directional pattern found in the independent MM2
phospho analysis: PSMs masking uniquely gains are shorter and lower-charge, PSMs masking
loses are longer and higher-charge. This is a structurally different mod space (single mod
type, no neutral-loss channel, `--carafe-mode general`) and a structurally different
acquisition (Hela whole-cell digest, not phospho-enriched) from the MM2/phospho analysis --
the same qualitative findings replicating here is independent evidence the effect is a
general property of predicted-fragment masking, not specific to phospho/NL scoring.

The ahead-of-time pipeline (index build through mask build) took **~1.5 hours total**,
dominated by ~1.42h of CPU-only Carafe inference over 5,581,921 peptide-index variants --
about 7x fewer variants, and correspondingly faster, than the MM2 phospho run's 39,466,180
(single mod type vs. two mod types with combinatorial overlap and neutral-loss channels).

## 2. What differs from the reference note, and from the MM2 phospho analysis

| | Reference note (PI vs. FI) | MM2 phospho analysis | This analysis |
|---|---|---|---|
| Comparison axis | PI_DB vs. FI_DB backend | FI_DB unmasked vs. masked | FI_DB unmasked vs. masked |
| Raw file | `20240924_Hela_01.raw` (primary table) | `MM2_R1.raw` / `MM2_R2.raw` | `20240924_Hela_01.raw` |
| Mod space | wide (M/STY, up to 3/residue) or narrow (up to 2/residue) | M-ox + STY-phospho w/ NL | M-ox only, no NL |
| Params file | `comet.params` / `data/comet_phospho.params` | `comet.params.phosphosmall` | `comet.params.oxmet.7-35` (edited this session -- see Section 3) |
| Comet version | v2026.02.0 vs. v2026.02.1 (also compared) | carafe branch, single version | carafe branch, two versions -- see note below |
| Thread counts | 1, 2, 4, 8, 20 | 20 only | 1, 2, 4, 8, 20 |

**Comet version note (added 2026-08-25).** The ahead-of-time pipeline (Section 4: `.idx`
build through mask build) ran at commit `40f93fb3bf1b29bf1d2358864c1cd9fc0067f8e3` (confirmed
via `git log` against the pipeline's own `.prerun/*.log` timestamps -- the same commit the MM2
phospho analysis's pipeline ran at). Section 5's masked RTS sweep was re-run on 2026-08-25 at
the current branch HEAD, `a71b701ebca5ce5c06ba0928fcead94b7010f6e9` (`a71b701e`) -- two commits
after `40f93fb3`/`7d4e6427` (`3a3d8d4b` then `a71b701e`), the same two `CometPredictedMask`
memory fixes documented in `docs/20260824_carafe_phoshoresults.md`'s Section 7 dated notes.
Both fixes change only search-time mask handling (freeing/shrinking `CometPredictedMask::
s_entries` after/during the one-time FI-build pass), never mask-file format or content, so the
existing `oxmet735.fasta.idx`/`oxmet735.fi_mask` artifacts from the original pipeline run
remain valid and were reused unmodified -- only the RTS search binaries were rebuilt (Windows
`RealtimeSearch.exe`/`CometWrapper.dll`, MSBuild Release/x64, Clean-then-Build) and only the
masked side of Section 5's sweep was re-run. The unmasked column is untouched by this update
and was not re-run: `CometPredictedMask::Load()` is a no-op whenever no mask file is
configured, so neither fix changes anything about an unmasked search. All five re-run masked
threads counts were verified to produce the same FI-entries figure (5.345e7) as the original
run, and the 20-thread run's PSM output was verified byte-identical to the original masked
run's (`tools/rts_out_to_txt.py` output, sorted, `diff`: 0 differing lines) -- confirming
Section 6's PSM-quality analysis is unaffected and needs no changes.

## 3. Input data and configuration

- **FASTA**: `20260420-human-phosho/human.canonical.target-decoy.fasta` -- identical to the
  MM2 phospho analysis (40,908 sequences, target+decoy concatenated).
- **Query spectra**: `20260420-human-phosho/20240924_Hela_01.raw` -- 89,593 total scans,
  62,576 MS2 spectra.
- **Params**: `20260420-human-phosho/comet.params.oxmet.7-35`. This file was found to be
  byte-identical to `comet.params.oxmet` at the start of this analysis (`peptide_length_range
  = 7 50` and `digest_mass_range = 700.0 5000.0`, despite the "7-35" filename), and was edited
  during this session to actually match its filename and the MM2 analysis's mass/length
  ranges: `peptide_length_range = 7 35`, `digest_mass_range = 700.0 3500.0`. Effective
  configuration otherwise: `variable_mod01 = 15.9949 M 0 3 -1 0 0 0.0` (Met oxidation only, up
  to 3 per peptide, no other variable mods active), `max_variable_mods_in_peptide = 3`,
  `decoy_search = 0` (decoys already in the FASTA), `search_enzyme_number = 1` (Trypsin),
  otherwise matching `comet.params.phosphosmall`'s runtime scoring settings (fragment/
  precursor tolerance, charge range 1-6, etc.).
- A pre-existing `human.canonical.target-decoy.oxmet.fasta.idx` and several oxmet-mod-space
  Carafe artifacts from 2026-08-12/13 (predating `tools/carafe_prerun.sh`, which didn't exist
  yet) were found in the same directory but were **not reused** -- both because their mass/
  length ranges no longer matched this session's edited params file, and because their
  provenance (several sibling files with `_fixed` reruns, suggesting earlier bugs) could not
  be fully verified. Everything in this analysis was built fresh.

## 4. Pipeline: exact commands and results

Same ahead-of-time driver and stage structure as the MM2 analysis (`docs/
20260824_carafe_phoshoresults.md` Section 4/5) -- only the flavor name, params file, and
`--carafe-mode` differ:

```bash
FASTA=/mnt/c/Work/Comet-master/20260420-human-phosho/human.canonical.target-decoy.fasta
OUT=/mnt/c/Work/Comet-master/20260420-human-phosho/carafe_oxmet735
COMET=/mnt/c/Work/Comet-master/comet.exe
PARAMS=/mnt/c/Work/Comet-master/20260420-human-phosho/comet.params.oxmet.7-35

tools/carafe_prerun.sh \
  --fasta "$FASTA" --out "$OUT" --comet "$COMET" \
  --flavor oxmet735="$PARAMS" \
  --charges 2 --include-decoys \
  --carafe-mode general \
  --stop-after mask
```

`--carafe-mode general` (vs. the phospho run's default `phosphorylation`) is correct here:
Met oxidation has no neutral-loss channel (`variable_mod01`'s trailing NL field is `0.0`), so
there are no modloss intensities for Carafe to predict, and the mask builder auto-detects
`--ignore-modloss` from the exported `VarModConfig` regardless (confirmed in the driver log:
`all neutral-loss deltas zero -> --ignore-modloss`).

| # | Stage | Wall time | Peak memory | Output |
|---|---|---|---|---|
| 1 | `.idx` build | 41s (comet-reported) | 1.1GB | `oxmet735.fasta.idx`, 235MB -- 3,961,583 unmodified peptides (identical raw-peptide count to the phospho run's `.idx`, since the digest config is now identical) |
| 2 | Variant export | 59s | -- | `oxmet735.variants_export.tsv`, 307MB -- **5,581,921** peptide-index variants (vs. phospho's 39,466,180 -- ~7.07x fewer, from having one mod type instead of two with combinatorial/NL overlap) |
| 3 | `idx_to_carafe.py` convert | 106s | -- | `oxmet735.carafe_peptides.tsv` (194MB) + `.variants.tsv` (141MB), 5,581,921 rows |
| 4 | Carafe inference | **5,109s (1.42h)**, 112 chunks x 50,000 rows (CPU, `--mode general`) | 0.86-1.44GB per chunk | `prediction/chunk_preds/*` |
| 5 | Compact store build | 24s | -- | `oxmet735.cps`, 821MB, u16-quantized |
| 6 | Mask build | 37s (36s self-reported), `--ignore-modloss` auto-detected | -- | `oxmet735.fi_mask`, 234MB, 5,581,921 entries (sorted+unique verified) |
| | **Total, `.idx` -> mask** | **~1.49 hours** | | |

Machine was confirmed idle (no other heavy process, per `ps`/`uptime`) before every timed
step, avoiding the CPU-contention artifact documented in the MM2 analysis's Section 6.1.

Estimating the Carafe-inference time before running it: after stages 1-3 completed (fast,
~3.5 minutes total) and gave the real variant count, the phospho run's own fitted per-chunk
cost model (`time_sec ~ length^0.852`, Section 6.2 of the MM2 analysis) was applied to this
run's 112 chunks (vs. 790 for phospho) rather than naively scaling by row count, since the
model's cost driver is chunk-average peptide length, not row count. That gave a ~1.7-hour
estimate; the actual run finished in 1.42h, about 20% faster than estimated -- plausibly
because a single-mod-type population's chunk-average lengths (at a given chunk index)
trend slightly shorter than the two-mod-type phospho population's, since fewer long peptides
generate enough modified-variant combinations to dominate the tail chunks the way phospho's
combinatorially richer population does.

## 5. RTS FI search: masked vs. unmasked, across thread counts

`RealtimeSearch.exe`, same raw file for `--query`/`--ms1ref`, `--ascorepro` default (1),
`--threads` swept over {1, 2, 4, 8, 20}. FI entries and peak memory are per-mask-state
(unchanged across thread counts, since threading doesn't change what's loaded into memory);
timing is reported per thread count.

**FI entries**: unmasked 1.305e8, masked 5.345e7 (**-59.0%**) -- constant across all five runs
per mask state, as expected (thread count doesn't change what gets loaded).

| Threads | Unmasked MS2 search | Masked MS2 search | Unmasked Hz | Masked Hz | Unmasked ms/spec | Masked ms/spec | Unmasked peak mem | Masked peak mem |
|---|---|---|---|---|---|---|---|---|
| 1 | 35.02s | 33.92s | 1,787 | 1,845 | 0.56 | 0.54 | 2.3GB | 2.1GB |
| 2 | 17.91s | 17.13s | 3,494 | 3,654 | 0.29 | 0.27 | 2.4GB | 2.1GB |
| 4 | 9.07s | 8.50s | 6,902 | 7,364 | 0.14 | 0.14 | 2.4GB | 2.1GB |
| 8 | 4.77s | 4.48s | 13,112 | 13,983 | 0.08 | 0.07 | 2.5GB | 2.2GB |
| 20 | 4.00s | 3.77s | 15,626 | 16,605 | 0.06 | 0.06 | 2.8GB | 2.5GB |

(62,576 MS2 spectra searched per run, all ten runs. Masked-column figures are the 2026-08-25
re-run at commit `a71b701e` -- see the Comet-version note in Section 2; unmasked columns are
the original, unaffected run.)

**Observations:**
- Masking improves throughput at **every** thread count tested, by +3.2% to +6.7% (Hz), with
  the largest relative gain at 4 threads (+6.7%) and the smallest at 1 thread (+3.2%) --
  broadly consistent, not concentrated at any one thread count.
- Both configurations show the expected diminishing-returns scaling past 8 threads (14-17K Hz
  at 8 vs. 20 threads is only a ~19% further gain for 2.5x more threads) -- this FI search,
  masked or not, is memory-bandwidth-bound at high thread counts on this machine, matching
  the reference note's own general finding for FI_DB RTS.
- Peak memory is now **consistently lower** under masking at every thread count tested
  (-8.7% to -12.5%: 2.3GB->2.1GB at 1 thread, 2.4GB->2.1GB at 2 and 4 threads, 2.5GB->2.2GB at
  8 threads, 2.8GB->2.5GB at 20 threads) -- a clearer, more uniform reduction than the original
  run found (which saw peak memory "nearly identical... at low thread counts" and only
  "modestly lower... at high thread counts", 2.8GB->2.7GB at 20 threads), because the two
  `CometPredictedMask` memory fixes (Section 2 note) free/shrink the mask's resident lookup
  table regardless of thread count -- unlike the original comparison, this reduction is not a
  function of how much *other* per-thread memory happens to be competing for the same peak.
  Still smaller in absolute terms than the ~1.1-2.1GB reductions seen in the MM2 phospho
  analysis, because this run's mask (5,581,921 entries, no neutral-loss channel) is about
  1/7th the size of the phospho run's (39,466,180 entries) to begin with.

## 6. PSM-quality comparison at 1% and 5% FDR (20 threads)

FDR computed identically to the MM2 analysis (`tools/qvalue.py`, target-decoy competition,
rank-1 PSMs, xcorr-descending and e-value-ascending separately).

| | Unmasked | Masked | Delta |
|---|---|---|---|
| Total MS2 hits reported | 41,606 | 35,617 | -14.4% |
| PSMs @ 1% FDR (xcorr-sorted) | 19,015 (cutoff >= 1.4070) | 20,141 (cutoff >= 1.3110) | **+1,126 (+5.9%)** |
| PSMs @ 1% FDR (e-value-sorted) | 20,820 (cutoff <= 1.54e-01) | 21,494 (cutoff <= 2.78e-01) | **+674 (+3.2%)** |
| PSMs @ 5% FDR (xcorr-sorted) | 25,013 (cutoff >= 1.0970) | 25,978 (cutoff >= 0.9510) | **+965 (+3.9%)** |
| PSMs @ 5% FDR (e-value-sorted) | 26,062 (cutoff <= 1.76) | 26,870 (cutoff <= 6.81) | **+808 (+3.1%)** |

The +5.9% gain at 1% FDR (xcorr-sorted) here is noticeably *larger* than either MM2 replicate's
gain (+2.8%/+4.0%) -- consistent with this being a distinct dataset/mod-space combination
rather than a fixed universal magnitude; see Section 8 for a mechanistic note on why a
single-mod-type, non-phospho search might see a larger relative benefit.

### 6.1 Which PSMs are the same, and which differ

Method identical to the MM2 analysis's Section 8.1 (scan-keyed comparison of each side's 1%
FDR xcorr-sorted passing set, using `tools/qvalue.py`'s own functions).

| | Value |
|---|---|
| Scans passing in both (shared) | 18,482 |
| Scans passing in unmasked only | 533 |
| Scans passing in masked only | 1,659 |
| Of shared scans, identical peptide call | 18,465 (99.91%) |
| Of shared scans, genuinely different peptide call | 17 (0.092%) |

(No "same backbone, shifted phospho-site" category applies here -- there is no phospho mod in
this search, so every disagreement on a shared scan is, by construction, a different peptide
call rather than a site-localization ambiguity.)

**Characteristics of the PSMs unique to each run:**

| | Unmasked-only (n=533) | Masked-only (n=1,659) | Shared (n=18,482, reference) |
|---|---|---|---|
| Mean xcorr | 1.83 | 1.36 | 2.17 |
| Median peptide length (residues) | 14.0 | 10.0 | 13.0 |
| Charge 3+ fraction | 94.6% | 23.3% | 33.2% |
| Met-oxidized fraction | 2.8% | 4.9% | 3.2% |

**Interpretation, replicating the MM2 phospho analysis's finding in an independent mod space
and acquisition:**

- Shared-scan agreement is again >99.9%, with genuinely-different-peptide disagreements at
  the same order of magnitude as MM2 (0.092% here vs. 0.026-0.055% in MM2's two replicates) --
  consistently a rare, near-tied-candidate phenomenon rather than a systematic effect.
- **PSMs unique to the unmasked run** are again longer (median 14 vs. 13 shared) and far more
  skewed toward higher charge states (94.6% charge 3+, vs. 33.2% of the shared population) --
  the same direction as MM2, though the length gap is much smaller here (14 vs. 13 residues,
  compared to MM2's 20-21 vs. 16) while the charge-state skew is comparably dramatic. Given
  there is no second modification type or neutral-loss channel to add complexity here, charge
  state appears to be the dominant driver of "hard-for-Carafe" peptides in this mod space,
  where in the phospho analysis both length and modification burden contributed.
- **PSMs unique to the masked run** are shorter (median 10 vs. 13 shared) and much more
  charge-2-dominated (76.7% charge 2 vs. 66.8% of the shared population) sitting near the
  FDR boundary (mean xcorr 1.36 vs. 2.17 shared) -- the same short/low-charge/boundary-
  adjacent signature found in both MM2 replicates, though the charge-2 skew relative to the
  shared population is much less pronounced here than in MM2 (where the shared population
  itself was closer to an even charge-2/charge-3 split).
- The **masked-only:unmasked-only ratio is 3.11x** (1,659 vs. 533) here, in the same range as
  MM2's 3.9x/4.9x -- three independent runs (one non-phospho mod space, two phospho
  replicates) all show masked-only PSM gains outnumbering masked-only losses by a factor of
  roughly 3-5x.

## 7. Comparison with the reference note's PI-vs-FI findings

The reference note found FI_DB dramatically faster than PI_DB (12,681 Hz vs. 1,259 Hz at 20
threads -- roughly a 10x difference) -- a *backend* change. This analysis's FI-vs-FI-masked
comparison is a much smaller relative effect (15,626 -> 16,605 Hz at 20 threads, +6.3%) --
expected, since masking only prunes candidate fragment-ion postings within the same backend
and data structure, not a fundamentally different search algorithm. The two effects are
complementary and multiplicative in principle: FI_DB's backend advantage over PI_DB, and
masking's further reduction of FI_DB's own memory footprint and marginal throughput gain, are
independent optimizations that could both be in effect simultaneously in production (as they
are in this analysis -- both runs here are FI_DB, one additionally masked). The more
consequential effect of masking, as in the MM2 analysis, is on **PSM yield at fixed FDR**, a
dimension the reference note's PI-vs-FI comparison did not examine (that note's own PSM
counts were reported as backend-invariant, i.e. PI_DB and FI_DB find the same PSMs -- masking
is the first FI-side change in this project's history to actually change which PSMs are found,
not just how fast they're found).

## 8. Limitations / caveats for a manuscript writeup

- **Single acquisition, single mod space.** Only `20240924_Hela_01.raw` with an oxidized-Met-
  only mod space was searched. This is a deliberately different dataset/mod-space combination
  from the MM2 phospho analysis (to test generality, per Section 1), not a replicate of it --
  the two together give two independent data points, not statistical power for either alone.
- **`comet.params.oxmet.7-35` was edited mid-session** (Section 3) to make its content match
  its filename and the MM2 analysis's mass/length ranges; the original file's actual
  `peptide_length_range` (7-50) and `digest_mass_range` (700-5000) were wider. This analysis
  used the edited (7-35/700-3500) version throughout; the original file's content was
  overwritten, not preserved under a separate name.
- **No cross-check against the reference note's own raw numbers.** This analysis reused the
  reference note's *raw data file* (`20240924_Hela_01.raw`) but not its *comparison* (PI vs.
  FI, wide/narrow mod spaces) -- the two documents' absolute Hz/timing numbers are not
  directly comparable beyond both establishing that this file has 62,576 MS2 spectra and
  benefits from higher thread counts up to a memory-bandwidth-bound plateau around 8 threads.
- **The Section 6.1 comparison script is not committed** (same caveat as the MM2 analysis) --
  a one-off snippet built on `tools/qvalue.py`'s existing functions.
- **The pre-existing 2026-08-12/13 oxmet artifacts were not used or independently validated**
  (Section 3) -- if they are later found to be correct and equivalent, this analysis's
  ~1.5-hour pipeline cost could have been avoided; this was not confirmed either way.
- **CPU-only Carafe inference**, as in the MM2 analysis -- no GPU comparison performed.
- **One FDR threshold pair (1%, 5%)** and one mask-threshold setting
  (`--min-relative-intensity 0.10 --min-kept-peaks 6`, the project's established defaults) --
  no sweep performed.

## 9. Artifact locations

All working files live under
`/mnt/c/Work/Comet-master/20260420-human-phosho/carafe_oxmet735/` (not committed to the git
repository, matching the MM2 analysis's convention):

- `oxmet735.fasta.idx` -- the FI_DB index (input to all ten RTS runs)
- `oxmet735.variants_export.tsv`, `oxmet735.carafe_peptides.tsv` / `.carafe_peptides.variants.tsv`
- `prediction/chunks/`, `prediction/chunk_preds/` -- per-chunk Carafe inference inputs/outputs
- `oxmet735.cps` -- compact prediction store, 821MB
- `oxmet735.fi_mask` -- the predicted-fragment mask, 234MB, 5,581,921 entries
- `.prerun/*.log`, `.prerun/*.done` -- per-stage driver logs and resume markers
- `rts_hela01_sweep.log` -- combined log of all ten RTS thread-sweep runs
- `rts_hela01_{unmasked,masked}_t{1,2,4,8,20}.out` -- per-run raw RTS output
- `rts_hela01_unmasked_t20.txt` / `rts_hela01_masked_t20.txt` -- qvalue.py-ready conversions
  of the 20-thread runs (the only pair converted/FDR-analyzed, since PSM identity is expected
  to be, and should be verified as, thread-count-invariant -- not independently re-verified in
  this analysis)
- `rts_hela01_masked_t{1,2,4,8,20}_a71b701e.out` / `rts_hela01_masked_t20_a71b701e.txt` /
  `rts_hela01_sweep_a71b701e.log` -- the 2026-08-25 masked-only re-run at commit `a71b701e`
  (Section 2's Comet-version note, Section 5's updated table) -- the `_unmasked_` files above
  were not re-run and remain the originals
