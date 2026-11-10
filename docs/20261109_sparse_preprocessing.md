# Sparse spectrum preprocessing (design, 2026-11-09)

Status: phase 0 done (section 9); phases 1-3 not started. Branch `RTSchimeric`.

## 1. Problem

Before a spectrum is scored, preprocessing (`CometPreprocess::PreprocessSingleSpectrumCore()`
for RTS, `CometPreprocess::Preprocess()` for batch) turns its peak list into three sparse
matrices on the `Query` -- `ppfSparseFastXcorrData`, `ppfSparseFastXcorrDataNL` (only with
`use_NL_ions` and a/b/y ions) and `ppfSparseSpScoreData` -- plus `iMinXcorrHisto`. Scoring
reads only those. To build them, both functions sweep dense per-bin scratch arrays whose
length is the precursor mass plus a cushion divided by the bin width (`iArraySize`,
`ComputeClampedArraySize()`).

Measured on the first 5,000 MS2 scans of `20170103_HelaQC_01` (`docs/20261105_RTSchimeric.md`
section 10.8: `rts_repro`, one thread, the `RTS_TIMING` build's `PREPROCESS` columns,
`isotope_error` 2, `use_NL_ions` off):

| setting | preprocessing per call | share of search | of which dense sweeps |
|---|---|---|---|
| high resolution: `fragment_bin_tol` 0.02, offset 0, `theoretical_fragment_ions` 0 | 308 us | 52% of a plain search, 28% of a chimeric call | buffer clearing 48 us, `MakeCorrData()` 61 us, fast-xcorr pass 122 us, sparse-matrix build 71 us |
| low resolution: `fragment_bin_tol` 1.0005, offset 0.4, `theoretical_fragment_ions` 1 | 18 us | 2% | ~6 us in total |
| low resolution, `theoretical_fragment_ions` 0 | 20 us | 2% | ~7 us |

At 0.02 Da the array is ~65,000 bins for this file's median precursor (1,281 Da) and up to
~150,000 for large ones, while the median spectrum has 43 peaks (mean 60). A peak can only
influence the bins within the xcorr offset (+/- 75 bins, `iXcorrProcessingOffset`) of it, so
~10% of the array can hold a nonzero value; the other ~90% of the sweeping reads zeros, writes
zeros, and filters them back out. At 1.0005 Da the array is ~1,300 bins and the +/- 75-bin bands
of 40-60 peaks cover all of it: there is nothing to skip, and preprocessing is already cheap.

## 2. Goal and constraints

- Build the same three sparse matrices and `iMinXcorrHisto` from the peak list, touching only
  bins that can be nonzero, when the spectrum is sparse relative to its array.
- **Byte-identical output.** Every stored float, every allocated sparse block (a block is
  allocated exactly when one of its 100 bins is stored), and `iMinXcorrHisto` must equal the
  dense path's, for every setting. Xcorr, Sp, E-values and every output format then follow.
  No tolerance.
- **No cost at low resolution or for dense spectra.** Low-resolution settings (`fragment_bin_tol`
  1.0005) and peak-rich spectra keep the current dense code, chosen by an O(1) test that costs
  less than measurement noise. The sparse path must also not be slower than the dense one at
  the switch point.
- RTS and batch both, through one shared implementation (today they hold two copies of the same
  dense code).
- No new user parameter. An internal environment variable selects dense / sparse / verify for
  testing (section 5).
- Out of scope: the MS1 path (`QueryMS1`, dense `pfFastXcorrData`), and the E-value cost (below).

## 3. The current algorithm, stage by stage

Notation: `raw[b]`, `corr[b]`, `fast[b]` are the double scratch arrays `pdTmpRawData`,
`pdTmpCorrelationData`, `pdTmpFastXcorrData`; `pf[b]`, `nl[b]`, `sp[b]` the float arrays
`pfFastXcorrData`, `pfFastXcorrDataNL`, `pfSpScoreData`; `N = iArraySize`, `off = 75`,
`R = 2*off + 1 = 151`, `dTmp = 1/(R-1)`.

1. **Clear** `raw`, `corr`, `fast` up to `iZeroBound + off` and (RTS) `pf`, `nl`, `sp` up to
   `N` (`ComputeZeroBound()`; the bound covers peaks up to 50 Da past the precursor mass, the
   `docs/20260714_EvalueJitter.md` stale-buffer fix).
2. **Load peaks** (RTS: the inline loop; batch: `LoadIons()`). Each accepted peak below
   `dExpPepMass + 50` updates `iHighestIon` (even when its bin is `>= N`) and, if its bin is
   `< N`, sets `raw[b] = max(raw[b], sqrt(intensity))` and `dHighestIntensity`, subject to
   `remove_precursor_peak` (modes 0-4) and `clear_mz_range`. Only max operations, so the
   result does not depend on peak order.
3. **`MakeCorrData()`**: ten windows of `W = iHighestIon/10 + 1` bins; per window the maximum
   `raw` over bins `<= iHighestIon`; then `corr[b] = raw[b] * 50/max` for bins whose `raw`
   exceeds `0.05 * dHighestIntensity`. `corr` is nonzero only at peak bins.
4. **Fast-xcorr pass**: `dSum` starts as `corr[0] + ... + corr[off-1]` in index order; then for
   `i = off .. N+off-1`: track `max(corr[i])` for `iMinXcorrHisto`; `if (i < N) dSum += corr[i]`;
   `if (i >= R) dSum -= corr[i-R]`; `fast[i-off] = (dSum - corr[i-off]) * dTmp`.
5. **Float arrays**, `k = 1 .. N-1`: `pf[k] = (float)(corr[k] - fast[k])`; with
   `theoretical_fragment_ions = 0` plus `0.5*(corr - fast)` at `k-1` and (if `< N`) `k+1`; with
   NL on, `nl[k] = pf[k] + 0.2*(corr - fast)` at `k - iMinus17` and `k - iMinus18` (each if
   `>= 0`). `pf[0] = 0`.
6. **Sparse build**: every `k` with `|pf[k]| > FLOAT_ZERO` (1e-6) is stored, allocating its
   100-bin block on first use; the same for `nl`; `sp[k] = (float)(100*raw[k]/dHighestIntensity)`
   for `k = 0 .. N-1`, stored where `> FLOAT_ZERO`.

Stages 1, 3, 4, 5 and 6 are O(N) over the whole array; only stage 2 is O(peaks).

## 4. Design

### 4.1 Which bins can be nonzero

Let `P` be the sorted set of peak bins with `corr[b] != 0` (from stage 3). Then:

- `fast[k]` can differ from the running sum's rounding residue only if some `b` in `P` has
  `|k - b| <= off`.
- `pf[k]` reads `fast` and `corr` at `k`, and at `k +/- 1` with flanking peaks, so it can be
  non-negligible only inside the **band** `B = union over b in P of [b - off - f, b + off + f]`,
  `f = 1` when `theoretical_fragment_ions = 0`, else 0.
- `nl[k]` is `pf[k]` plus terms read at `k - iMinus17` and `k - iMinus18`: its support is
  `B union (B + iMinus17) union (B + iMinus18)`.
- `sp[k]` is nonzero only at `raw`'s nonzero bins, a superset of `P` (peaks below the 5% cut
  have `raw` but no `corr`).

Outside these sets the dense path computes values that are pure rounding residue (section 4.4)
and never stores them, because they are far below `FLOAT_ZERO`.

### 4.2 Peak list instead of a dense `raw` array

The sparse path runs the same peak-acceptance loop (same filters, same precursor removal, same
`iHighestIon` and `dHighestIntensity` updates) but writes `(bin, sqrt intensity)` into a
thread-local vector, then sorts it by bin and keeps the per-bin maximum. Because stage 2 is
max-only, this equals the dense `raw` exactly whatever the peak order. RTS and batch
(`LoadIons()`) each produce this list from their own loop, so batch's `sortIntensity()` order
does not matter either.

### 4.3 Windowed normalization from the list

The ten window maxima are taken over the list's bins (all `< N <= iArraySizeGlobal`, and all
`<= iHighestIon` because `iHighestIon` is their maximum or larger); `corr` is computed for the
listed bins only, with the same expression `raw * (50.0 / max)` evaluated in the same order of
operations. Identical by construction.

### 4.4 The fast-xcorr pass, exactly

`dSum` changes only at **events**: adding `corr[b]` at step `i = b` (or in the initial sum for
`b < off`) and subtracting it at step `i = b + R` (when that step is inside the loop). Between
events the dense loop adds and subtracts `0.0`, which leaves a double exactly unchanged. The
sparse pass therefore:

1. forms the initial sum from the list's bins `< off`, in increasing bin order (the dense order
   with the zeros removed);
2. walks the events in increasing step order, applying at each step the add before the
   subtract, as the dense loop does;
3. evaluates `fast`, `pf`, `nl` only at the indices in the support sets of 4.1, using the
   current `dSum` -- so each evaluated value is bit-identical to the dense one;
4. takes `iMinXcorrHisto`'s maximum over the list's `corr` at bins `>= off` (the dense loop
   starts at `i = off`, so bins below the offset never count -- reproduced deliberately).

The neighbours read by the flank and NL terms (`k +/- 1`, `k - iMinus17`, `k - iMinus18`) can
fall in a gap. There the dense `fast` value is `r * dTmp`, where `r` is `dSum`'s constant value
across that gap -- in exact arithmetic zero, in practice a rounding residue of order 1e-13 or
smaller. The sparse pass evaluates those neighbours from the tracked `dSum` too, so even residue
contributions are reproduced bit for bit. Gap bins themselves are skipped: their dense values
are residue-sized and never stored. As a proof obligation rather than a hope, the sparse pass
checks every gap it skips: if `|r| * dTmp * 2.4` (the largest combined weight, 1 + 0.5 + 0.5 +
0.2 + 0.2) could reach `FLOAT_ZERO`, it abandons the sparse build and runs the dense path for
that spectrum. This is not expected to trigger on real data; verify mode counts it.

Signed zero: the dense sum can only produce `-0.0` where the sparse one does too (same
operations on the same nonzero values), and a zero is never stored, so it cannot change output.

### 4.5 Sp matrix from the list

`sp` is computed for the listed bins only, `(float)(100.0 * raw / dHighestIntensity)`, the same
expression; stored where `> FLOAT_ZERO`. Identical by construction.

### 4.6 Choosing sparse or dense (the low-resolution guard)

Before any work, an O(1) bound on the support size:

```
support_bound = iNumPeaks * (2*off + 2*f + 1) * (NL on ? 3 : 1)
use sparse   = support_bound < N * kSparseFraction
```

`iNumPeaks` is the input peak count (an upper bound on `|P|`), so the test needs nothing from
the peak loop. With `kSparseFraction` around 0.5 (to be set by measurement, section 6):

- `fragment_bin_tol` 1.0005: `N` ~1,300 for a 1,300 Da precursor; 40 peaks already bound to
  ~6,000 > 650. **Always dense**, at the cost of one multiply and compare.
- 0.02 Da, median spectrum: 43 x 151 = ~6,500 against ~65,000 x 0.5. **Sparse.**
- 0.02 Da, very peak-rich spectra (several hundred peaks, or NL on with many peaks): the bound
  exceeds the threshold and the dense path runs, as today.

The threshold is set so that the sparse path is no slower than the dense one at the switch
point, measured (section 6); being an upper bound, the test errs toward dense.

### 4.7 One implementation for RTS and batch

A new internal builder, used by both `Preprocess()` and `PreprocessSingleSpectrumCore()` once
the peak list and `iHighestIon` / `dHighestIntensity` exist:

```
// CometPreprocess.cpp, file-local
struct SparseBlockAlloc { ... };   // RTS: g_rtsScratch.AllocSparseChild(); batch fused path:
                                   // FusedSparseArena / FusedPointerArena; batch otherwise: new[]
static bool BuildSparseMatricesFromPeaks(Query* pScoring,
                                         const std::vector<std::pair<int, double>>& vRawBins,
                                         int iHighestIon, double dHighestIntensity,
                                         SparseBlockAlloc& alloc);
```

It produces the same allocation pattern the dense code does (the pointer arrays sized
`N / SPARSE_MATRIX_SIZE + 1`, a child block on first stored value), with each path's existing
allocator and failure handling. The dense code stays where it is, used when 4.6 picks it.

### 4.8 Scratch buffers and the stale-data invariant

The sparse path never reads or writes the dense scratch arrays: the peak list, `corr` and the
per-index evaluation are thread-local vectors sized by the peak count. So it needs no clearing
(stage 1's 48 us goes away for sparse spectra), and the `iZeroBound` invariant that protects the
dense path is untouched: the dense path still clears exactly what it reads before it reads it.
A thread that alternates dense and sparse spectra is therefore safe in both directions.

## 5. Verification

- **Internal switch** `COMET_PREPROCESS=dense|sparse|verify` (read once per process, like
  `COMET_EVALUE_TRACE`; not a parameter). `dense` forces today's code, `sparse` forces the
  sparse path even where 4.6 would pick dense (to exercise it at low resolution), `verify` runs
  both for every spectrum and compares the three matrices element by element as bit patterns,
  the allocated-block sets and `iMinXcorrHisto`, logging the first mismatch and a count. Default
  (unset): the 4.6 choice.
- **Unit test (next free ID, T86)**: synthetic spectra through `rts_repro` and batch `comet.exe`
  in verify mode, zero mismatches required, covering: `theoretical_fragment_ions` 0 and 1;
  `use_NL_ions` on with a/b/y; `remove_precursor_peak` 0-4; `clear_mz_range`; a peak in the
  `dExpPepMass + cushion .. + 50` gap (the jitter-fix case); peaks below bin 75 (the
  `iMinXcorrHisto` start); peaks within 151 bins of each other (overlapping bands) and far apart
  (gaps); one peak; peaks below the 5% normalization cut; a precursor at the top of the mass
  range (`N` clamped to `iArraySizeGlobal`); low resolution forced sparse.
- **Real data**: the full HeLa file in verify mode, RTS (`rts_repro`, plain and chimeric -- the
  chimeric loop feeds stripped spectra) and batch FI_DB, PI_DB and FASTA, at 0.02 / 0 /
  `theoretical_fragment_ions` 0 and 1, NL on and off, and at 1.0005 / 0.4 forced sparse; zero
  mismatches. Then the normal-mode outputs byte-compared against the pre-change build, and the
  existing T22-T24 parity suites, which must stay byte-identical.

## 6. Performance targets and measurement

Same 5,000-scan harness as section 1, before and after:

- High resolution: preprocessing from ~300 us to well under 100 us per call. Estimate: clearing
  48 -> 0, `MakeCorrData()` 61 -> ~1, fast-xcorr 122 -> ~15-25, sparse build 71 -> ~10-15. That
  would make a plain RTS search roughly 35-45% faster and a chimeric call roughly 20% faster.
  Batch FI and PI searches gain in proportion to their preprocessing share, to be measured.
- Low resolution (`fragment_bin_tol` 1.0005, offset 0.4, `theoretical_fragment_ions` 0 and 1):
  **no measurable change** -- the dense path runs; the 4.6 test is the only added work.
- Switch point: sweep spectra by peak count at 0.02 Da with `COMET_PREPROCESS=dense` and
  `=sparse` to find where the two cost the same; set `kSparseFraction` below that.

## 7. Phases

| phase | work | exit criterion |
|---|---|---|
| 0 | `COMET_PREPROCESS` switch and the verify comparator around the existing dense code (dense vs dense must match -- tests the comparator) | comparator in place |
| 1 | `BuildSparseMatricesFromPeaks()`; RTS path uses it under 4.6 | T86 and HeLa RTS verify: zero mismatches |
| 2 | batch `Preprocess()` uses it | batch verify on HeLa, all three search types: zero mismatches; T22-T24 byte-identical |
| 3 | threshold sweep and the section 6 measurements; docs (`docs/RealTimeSearch.md`, this file, `tests/tests.md`) | targets met; low resolution unchanged |

## 8. Risks and open points

- **Exactness depends on reproducing the operation order**, not just the arithmetic: the
  initial sum's order, add-before-subtract at a shared step, the `i < N` guard on adds, the
  `k + 1 < N` guard on the flank term, the `>= 0` guards on the NL reads, and the loop starting
  at `k = 1`. Verify mode exists to catch any slip; the unit cases target each guard.
- **The gap-residue guard** (4.4) turns the one theoretical non-identity into a per-spectrum
  dense fallback. If verify mode ever shows it firing on real data, that needs a closer look
  before phase 3.
- **Memory**: the dense scratch arrays stay allocated, because the dense path still needs them.
- **Low resolution, separately**: at 1.0005 Da the E-value step is 46% of the search (413 us per
  call, against 79 us at 0.02 Da) -- a different, unrelated cost worth its own look later.

## 9. Progress

| date | phase | result |
|---|---|---|
| 2026-11-09 | 0 | `COMET_PREPROCESS` (`auto` / `dense` / `sparse` / `verify` / `verify-selftest`) and the comparator in `CometPreprocess.cpp` (`GetPreprocessMode()`, `PreprocessedQueriesEqual()`, `VerifyPreprocessShadow()`). Verify rebuilds every preprocessed Query into a heap shadow -- RTS: `PreprocessSingleSpectrumThreadLocal()` re-runs the core on its heap path, which leaves the thread-local pool untouched; batch: `Preprocess()` re-runs itself on a shadow Query without arenas -- and compares the three sparse matrices bit for bit, the block-allocation pattern, the matrix sizes and `iMinXcorrHisto`. `sparse` is accepted but still runs dense (with a one-line notice). Dense vs dense on real data (HeLa, the section 1 index): RTS plain 4,732 / chimeric 20,190 / low resolution 4,732 spectra compared, batch FI_DB full file 40,287 -- zero mismatches everywhere; `verify-selftest` flags every comparison. With the variable unset the plain search time is unchanged (2.60-2.72 s against 2.81 s before, single runs) and the output identical. Test T86. |
