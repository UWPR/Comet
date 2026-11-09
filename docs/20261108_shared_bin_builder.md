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
first loss-bearing mod. Changing either changes PI results and is left for a separate
decision.

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
