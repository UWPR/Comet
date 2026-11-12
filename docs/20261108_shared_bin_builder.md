# Shared bin builder (2026-11-08)

## 1. Problem

The bins `XcorrScoreI()` credits for a candidate (its fragment ions, their fragment neutral
losses, and the precursor neutral-loss peaks) were computed by three hand-maintained copies
of the same code:

- `SearchFragmentIndex()` (FI_DB);
- `AnalyzePeptideIndex()` for the target (PI_DB);
- `AnalyzePeptideIndex()` for the internal decoy (PI_DB).

The RTS result extraction in `SearchSingleSpectrumCore()` kept a fourth copy, a b/y
conversion table plus its own neutral-loss loop. That copy is the source of both the reported
matched fragments and the chimeric removal bins.

The fourth copy drifted from the scorer (branch code review, item 9):

- With `scale_fragmentNL` the scorer subtracts a fragment neutral loss once per loss-bearing
  mod in the fragment. Removal subtracted it once, so a doubly phosphorylated winner kept 6 of
  its 30 credited peaks.
- Removal never touched the `precursor_NL_ions` peaks the scorer credits.

The same class of drift had already been fixed one rule at a time: the `use_NL_ions`
water/ammonia peaks, and the z+1 offset (review item 4).

## 2. Design

Two shared functions in `CometSearch` (`CometSearch.h`):

- `BuildIonLadder()`: the 1+ b and y ladders of a peptide with its mods. It applies the peptide
  terminal mods and the static protein-terminal masses. When fragment neutral losses are on,
  it also fills the per-mod loss positions and, under fragment-index rules, the cumulative
  per-position loss counts.
- `BuildBinnedIons()`: from that ladder, the `uiBinnedIonMasses` / `uiBinnedPrecursorNL`
  arrays `XcorrScoreI()` reads. It uses the same two-pass clear/set over `pbDuplFragment`, so
  each bin is stored once. An optional visitor sees every stored bin with its charge, ion
  series, ladder position, m/z and total neutral loss.

### 2.1 FI and PI rules differ

The copies did not agree, and the builder keeps the difference explicit (`IonRules`) so both
paths stay byte-identical:

| | `FragmentIndex` (FI_DB) | `PeptideIndex` (PI_DB) |
|---|---|---|
| Loss positions | first position of every loss-bearing mod | only the first loss-bearing mod from each end |
| `scale_fragmentNL` | loss x number of such mods in the fragment | ignored, always one loss |
| Mods considered | first `FRAGINDEX_VMODS` | all `VMODS` |

The batch FASTA path (`CalcVarModIons()`, not touched here) follows the fragment-index rules
over all `VMODS`. So PI_DB departs from batch FASTA in two ways that look like
pre-existing PI scoring bugs: it ignores `scale_fragmentNL`, and it gives losses only to the
first loss-bearing mod. Changing either changes PI results; both were changed later the same day (section 5).

### 2.2 iFoundVariableMod

The copies set `iFoundVariableMod = 2` at different moments:

- the PI target set it when a neutral-loss bin was in range;
- FI and the PI decoy set it when such a bin was stored.

The builder uses "stored" for all three. The flag is read in two places, and neither can
see the difference:

- `XcorrScoreI()` reads the loss slots only when the flag is 2, and when no loss bin was
  stored every slot is 0, so reading them adds nothing;
- `StorePeptideI()` tests the flag only for zero.

## 3. Step 1: refactor, no change in behaviour (commit 1)

FI scoring, PI target scoring and PI decoy scoring call the builder. The net change in
`CometSearch.cpp` is 333 lines fewer, and dead code was removed (FI's unused
`iMaxFragmentCharge` cap and the loop counters it no longer needs).

Verification against the previous commit:

- `20170103_HelaQC_01`, `human.canonical.target-decoy.fasta`, `data/comet_phospho.params`:
  - **Batch, phospho defaults:** FI 27,813 and PI 39,469 rank-1 rows, byte-identical.
  - **Batch, first 15,000 scans, internal decoys (`decoy_search = 1`), `scale_fragmentNL = 1`,
    `precursor_NL_ions = 97.976896`:** FI 5,244 and PI 7,956 rows, byte-identical (995 and
    2,918 phospho rows; thousands of internal-decoy rows).
  - **RTS (`rts_repro`, first 3,000 scans, 8 threads):** FI byte-identical. PI byte-identical
    with `max_index_runtime = 0`. With the 200 ms budget rts_repro sets for plain RTS, 8 slow
    PI spectra timed out differently between the two runs, which is timing, not scoring.
- **Wall time (one run each, batch, 20 threads):** FI 25.4 s / 22.9 s, PI 34.5 s / 34.7 s, FI
  decoy settings 40.2 s / 40.1 s, PI decoy settings 19.7 s / 21.1 s. Within noise.
- **Tests:** unit 94 passed; T22 (4 variants) passed.

## 4. Step 2: RTS result extraction uses the builder (commit 2)

`SearchSingleSpectrumCore()` no longer keeps its own copy. For each reported result it
rebuilds the result's ladder and bins with `BuildIonLadder()` / `BuildBinnedIons()`. It uses
the rules of the index that scored the result: `FragmentIndex` on FI_DB, `PeptideIndex` on
PI_DB, the only two types RTS searches. The input is the stored peptide, `piVarModSites`
(including the two terminal slots), the flanks and `dPepMass`. A visitor (`VisitResultIon()`)
handles every stored bin:

- If the bin holds an observed peak, it becomes a matched `Fragment`. Its mass is the 1+ mass
  after any loss, and `neutralLossMass` is the total loss.
- For result 0 of a chimeric call, every bin adds its removal bins through
  `AddChimericRemovalBins()`: the flanks, the observed 13C isotopes, and the water/ammonia
  positions on PI_DB 1+ a/b/y ions under `use_NL_ions`. Precursor neutral-loss peaks are
  removed too.

Two removal rules stay outside the arrays because the scorer folds them into its matrices
rather than storing them as bins: the flanking bins and the water/ammonia losses. Isotope
removal stays as well; it was never a scoring rule.

The b/y conversion table (`ionMassesRelative`) and its neutral-loss loop are gone, 189 lines.

### 4.1 Behaviour changes

- **Chimeric removal now covers every credited bin:**
  - the `scale_fragmentNL` k x loss peaks of fragments with k loss-bearing mods;
  - the `precursor_NL_ions` peaks;
  - on PI_DB only, the loss peaks of the first loss-bearing mod, which are the only ones PI
    scores. The old loop also removed loss peaks of a second, uncredited mod type.
- **Matched fragments:**
  - listed in builder order (fragment charge, then ion series, then position), where the old
    order was position, then charge, then series;
  - each credited bin is listed once, so two ions sharing a bin give one entry;
  - k x loss fragments now appear with their true loss.
- On the HeLa benchmark (phospho params: `scale_fragmentNL` off, no precursor losses, one loss
  mod) nothing changed. The chimeric FI run on 3,000 scans (charges 1-4, +/- 1 m/z) gave 3,091
  rows, byte-identical. A plain RTS run gave the same 3,000 PSM lines and the same 8,376
  matched fragments as a set.

A correction to the review discussion: the "FI considers only `FRAGINDEX_VMODS` mods"
difference cannot surface in removal. An FI_DB winner never carries a mod beyond the first
`FRAGINDEX_VMODS`, because the index holds only those.

### 4.2 Tests

- **T76:** `LVS[80]ELT[80]EFAK`. On FI_DB with `scale_fragmentNL`, removal takes 30 of 30
  peaks (24 before). With precursor losses it takes 32 of 32 (24 before). On PI_DB it takes
  30 of 30. Matched fragments list the 2x-loss ions at ion - 2 x loss (none before).
- **Existing tests:** T67 (including the `use_NL_ions` 36/18 removal counts), T71, T72, T73
  and T75 unchanged.
- `rts_repro`'s `RTS_REPRO_PARAMS` gained a `v` type, comma-separated doubles, for
  `vector<double>` parameters such as `precursor_NL_ions`.
- The test parsers no longer split peptide strings on `.`, which broke on modified peptides:
  `_strip_flanks()`.
- **Results:** unit 95 passed; T22 (4 variants) passed.

## 5. PI_DB follows the FI / batch FASTA neutral-loss rules (2026-11-08)

Section 2.1 recorded two places where PI_DB departed from FI_DB and batch FASTA. PI_DB now
follows the FI/FASTA rules. Every loss-bearing mod type present in a fragment adds its own
loss peak; types are never combined into one peak. With `scale_fragmentNL` on, that loss is
multiplied by the number of that type's residues in the fragment, and only that k x loss peak
is added. `IonRules` now differs only in which mods are considered: `FragmentIndex` considers
the first `FRAGINDEX_VMODS` (all the index holds) and `PeptideIndex` considers all `VMODS`.

The old PI rule was not "one loss per fragment". It was direction-dependent. For a peptide with
a phospho-S near the N-terminus and an oxidized M near the C-terminus:

- b ions got only the phospho loss, from the first loss-bearing mod scanning from the N-terminus;
- y ions got only the oxidation loss, from the first one scanning from the C-terminus;
- so b ions containing both mods lacked the oxidation loss, and y ions containing both lacked
  the phospho loss.

The code was labelled "matches batch path", and its `break` looks like an unintended
truncation of the per-mod-type scan.

### 5.1 Measurement

HeLa, batch, 20 threads, full file. "Before" is the previous commit; FI_DB is unchanged by this
change. "Parity" counts the spectra where PI_DB and FI_DB report the same rank-1 peptide and
asks how many have equal xcorr. Parity is measured with `print_ascorepro_score = 0`, because
AScorePro rewrites the reported modification sites (see below); the FDR counts are the same
with it on or off.

| Configuration | PI rows changed | Parity before | Parity after | PI 1% FDR, xcorr | PI 1% FDR, E-value |
|---|---|---|---|---|---|
| A: phospho defaults (one loss mod, no scaling) | none (byte-identical) | -- | -- | -- | -- |
| B: phospho, `scale_fragmentNL = 1` | 5,054 | 97.55% of 19,613 | **100% of 19,815** | 11,035 -> 11,026 | 12,969 -> 12,957 |
| C: phospho + oxidized M with loss 63.998285 | 3,954 | 98.09% of 19,489 | **100% of 19,441** | 11,005 -> 10,657 | 13,042 -> 13,026 |

FI_DB in the same configurations gives, by xcorr / E-value: B 11,423 / 13,068 and
C 11,306 / 13,148. Wall times are unchanged, about 35 s for PI and 24 s for FI.

A first measurement with AScorePro on (the `comet_phospho.params` default) seemed to leave
about 0.25% of spectra unmatched, and about 0.28% even at the phospho defaults. Those numbers
were an artifact: AScorePro rewrites the reported site. On scan 5476, PI scored
`TNDGKIT[79.9663]YPPGVK` at 0.797, while FI's index prefilter never scored that isoform (it does
not reach `fragindex_min_ions_score` 3; with the floor at 1 FI finds it, at 0.797). FI's best was
therefore `T[79.9663]NDGKITYPPGVK` at 0.676. AScorePro relocalized both outputs to that string,
so the two rows looked like the same peptide with different scores. With AScorePro off, PI and FI
agree on every spectrum where they report the same scored peptide: 19,777 of 19,777 at the
phospho defaults, and all of them in B and C after this change. There is no remaining PI/FI
scoring difference.

In C, the xcorr-ranked count drops by 3.2% while the E-value count barely moves. Crediting the
oxidation loss on every fragment that holds an oxidized M raises the xcorr of M-containing
targets and decoys alike. A global xcorr ranking mixes those with everything else, while the
per-spectrum E-value does not. B moves by less than 0.1%.

### 5.2 Tests

- **T76:** the PI removal case uses the scaled spectrum and removes 30 of 30 peaks (24 with
  the old rules).
- **T76 parity, doubly-phosphorylated peptide with `scale_fragmentNL`:** PI_DB and FI_DB give
  the same xcorr, 6.795 (old PI: 5.59).
- **T76 parity, `LVS[80]EGLEM[16]AK` (phospho S3, oxidized M8 with a loss):** the same xcorr,
  7.984 (old PI: 7.144).
- **Results:** unit 95 passed; T22 (4 variants) passed.

### 5.3 Release note

*"Peptide-index searches now score fragment neutral losses like fragment-ion-index and FASTA
searches: every loss-bearing modification in a fragment contributes its own neutral-loss peak,
and `scale_fragmentNL` is honored. Previously a peptide-index search ignored
`scale_fragmentNL`, and when a peptide carried two different loss-bearing modifications it
credited only one of them on each ion series. Results change only for searches with
`scale_fragmentNL = 1` or with more than one variable modification that has a neutral loss."*
