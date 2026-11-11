# Sparse spectrum preprocessing (design, 2026-11-09)

Status: phases 0-3 done (section 9); the sparse build is the default where the section 4.6 bound picks it. Branch `RTSchimeric`.

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

1. **Clear** `raw`, `corr`, `fast` up to `iZeroBound` (no `+ off` padding; the priming sums of the fast-xcorr pass are bounded by `min(off, iZeroBound)`) and (RTS) `pf`, `nl`, `sp` up to
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

Before any work, an O(1) bound on the bins the sparse build can touch (`UseSparseBuild()`):

```
support_bound = iNumPeaks * (2*off + 2*f + 1) * (NL on ? 3 : 1)
use sparse   = support_bound < N
```

`iNumPeaks` is the input peak count (an upper bound on `|P|`), so the test needs nothing from
the peak loop. The threshold (1 x `N`) was set from the phase 3 sweep (section 9): synthetic
spectra of 10 to 4,000 random peaks at MH+ 1,300 (z 2, ~65,000 bins) and 2,600 (z 3, ~130,000),
dense and sparse preprocessing time per call compared:

- **Without NL ions** the sparse build was faster at every peak count tried -- 0.02-0.15x the
  dense time up to 80 peaks, 0.93x at 4,000 peaks, where the bound is ~9x `N`.
- **With NL ions** it breaks even near a bound of ~2x `N` (300 peaks at 1,300 Da, 600 at
  2,600 Da) and is up to 1.6x slower beyond. A threshold of 1x `N` keeps margin there (at 150
  peaks, 1,300 Da, NL on: 0.70x) while covering typical spectra (median 43 peaks) everywhere.
- **Low resolution** (`fragment_bin_tol` 1.0005, offset 0.4): `N` is ~1,300 at 1,300 Da, so 10
  peaks already bound to 1,510 and the dense build runs; at 2,600 Da (~2,600 bins) spectra of 17
  or fewer peaks without NL ions go sparse, and there the sparse build measured faster (6-8 vs
  10 us). With NL ions everything at low resolution stays dense (where sparse measured slower
  from ~15 peaks). Preprocessing at low resolution costs 3-40 us per call either way.
- 0.02 Da, very peak-rich spectra or NL with many peaks: the bound exceeds `N` and the dense
  build runs, as before.

Being an upper bound on the touched bins, the test errs toward dense.

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

### 4.9 The raw-intensity buffer the RTS result extraction reads

`SearchSingleSpectrumCore()` passes the thread-local raw buffer (`GetRtsRawDataBuffer()`,
`g_rtsScratch.pdTmpRawData`) to `VisitResultIon()`, which reads `raw[bin]` to attach an
intensity to every matched fragment it reports. So on the RTS path `raw` is an output of
preprocessing, not only scratch: the sparse path still writes each peak's `sqrt` intensity
into it, at the peak bins only, and records the bins it wrote. Instead of the dense path's
up-front `memset`, the buffer is kept clean incrementally: a sparse spectrum zeroes the bins
the previous sparse spectrum recorded and, after a dense spectrum, the range that spectrum
cleared and wrote (`iZeroBound + off`), once. A dense spectrum clears its own range as today.

The record belongs to the **buffer**, not to the thread (`RawBufferState`, looked up by the
buffer's address in a registry). The RTS scratch is thread-local, but the batch pool
(`ppdTmpRawDataArr`) gives a slot to whichever task finds it free, so one buffer is used by
different threads over time; a per-thread record (the first implementation) cleared only the
bins this thread had set and left the others' behind -- see the 2026-11-10 log row. A buffer
is held by one task at a time, so its state needs no lock once found; the registry is locked
for the lookup only (each thread caches its last buffer), is emptied when the batch pool is
freed, and an RTS scratch drops its buffer's entry when it frees it, since a later allocation
may reuse the address.

A buffer's first use clears all of it, whichever build comes first: `PrepareRawForSparse()` clears the
whole buffer, and `NoteDenseRawClear()` -- which a dense build calls after clearing its own range --
zeroes the rest (the RTS/fused scratch is allocated uninitialized, and a later sparse spectrum assumes
what it does not clear is zero; see the second 2026-11-10 log row).

### 4.10 Prior art: Telescope (reviewed 2026-11-09)

Telescope (github.com/SchweppeLab/Telescope, MIT licence, commit 559f865) is a fragment-ion-
index search engine from the Schweppe lab built for RTS, whose `src/FastXCorr.cpp` reimplements
Comet's xcorr preprocessing (via Kojak) in two forms:

- `XCorr()`, the dense form, with two differences from Comet: the sweep stops at the highest
  peak's bin + 75 (`iMax = iHighestIon + 75`) instead of running to the precursor-sized array
  end, and the flanking-peak term is computed with a rolling three-value register (`dTmp0`,
  `dTmp1`, `dTmp2`) instead of re-reading `fast[k-1]` and `fast[k+1]`. The output is a flat
  (bin, value) list, not Comet's 100-bin block matrix.
- `UltraXCorr()`, the sparse form, selected by a parameter and documented as "a fast
  approximation": peaks (sorted by bin) are grouped into runs whose +/- 76-bin windows overlap,
  and each run gets its own running sum, restarted from zero at the run's start. Normalization
  windows are taken from the peak list, as in 4.3. It still `memset`s the correlation array up
  to the highest peak + 75, and it divides by a constant `0.0066666667` (Comet computes
  `1/150`).

What this design takes from it, and what it does not:

- **Confirmation of the approach.** Grouping peaks into overlapping +/- 76-bin bands and
  computing only inside them is what 4.1-4.4 do; Telescope runs it as its fast mode.
- **Not the per-band restart.** Restarting the sum per band drops the rounding residue the
  dense sum carries from earlier bands, so values differ from Comet's in the last bits --
  acceptable for Telescope, not here (section 2). 4.4 carries one sum across all bands instead.
  Its normalize-to-100-first step and the `0.0066666667` constant would also break bit
  identity.
- **No `memset` at all** (4.8), where `UltraXCorr()` still clears a dense range.
- **The rolling register** for the flank term is a worthwhile micro-optimization inside the
  band loop (one `fast` evaluation per bin instead of three); adopted.
- **Stopping the dense sweep at the highest peak + offset** (+ the NL shift when NL ions are on)
  is an exact speed-up for the dense path too (beyond it every value is residue, as in 4.4), useful for the dense fallback on
  peak-rich high-resolution spectra. Not part of phase 1; a phase 3 candidate, measured first.
- NL ions and the Sp matrix are not handled by Telescope; nothing to borrow there.

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
| 2026-11-09 | 1 | `BuildSparseMatricesFromPeaks()` (`CometPreprocess.cpp`) on the RTS thread-local-pool path, taken when `COMET_PREPROCESS` is `sparse`, `verify` or `verify-selftest`; the default (`auto`) stays dense until phase 3 sets the switch point. The peak loop records each raw bin it sets; normalization, the event-replayed running sum (one sum carried across all bands), the flank and NL terms (a rolling register for the flank, as in Telescope's `FastXCorr`) and Sp are computed from the list; the raw buffer is kept clean incrementally (4.9) instead of `memset`; a gap-residue failure falls back to the dense build. Verify, sparse vs dense, on the 5,000 HeLa scans at 8 threads: plain search (4,732 spectra) with flanking peaks on and off, NL ions on with flanks on and off; chimeric (20,190) with and without NL ions; low resolution 1.0005 / 0.4 forced sparse with flanking peaks on and off; `remove_precursor_peak` 1-4 -- zero mismatches and zero gap-residue fallbacks everywhere; chimeric output with `sparse` identical to `dense`. T86 adds five synthetic edge-case spectra under ten settings, all identical. **Mutation test** (each planted in the builder, then T86 run): Telescope's `0.0066666667` constant for `1/150` -- caught (16 failed checks); the k-1 flank term dropped -- caught (15); the NL-shifted bands not evaluated -- caught (4, the NL settings); Telescope's per-band restart of the running sum (dropping the carried residue) -- **not** caught, by T86 or by 4,732 HeLa spectra in verify mode: the residue is ~1e-15 against values ~1 and disappears in the float conversion, so on this data the restart stores the same floats. The carried sum is kept (it costs nothing and keeps identity guaranteed rather than observed). Preliminary single-thread timing, 5,000 scans (phase 3 measures properly): plain search 2.55 s dense -> 1.28 s sparse (-50%); chimeric 24.8 s -> 17.6 s (-29%); outputs identical. |
| 2026-11-09 | 2 | Batch `Preprocess()` takes the same builder (`COMET_PREPROCESS=sparse`/`verify`; the verify shadow is a dense re-run without arenas). The builder moved ahead of `Preprocess()` and gained `SparseMatrixAllocator` (the RTS pool's child blocks; the fused path's `FusedSparseArena` / `FusedPointerArena`, each optional, setting `bSparseFromPool` / `bSparsePointerArraysFromPool` as the dense code does; otherwise `new[]`). `LoadIons()` records the raw bins it sets (`SetRawBinRecorded()`), and the raw-buffer state (4.9) is now per buffer (`RawBufferState`: the RTS/fused pool and the classic per-thread arrays are different buffers). **Bug found and fixed:** the first batch verify run (FI_DB, full HeLa file) reported 6 mismatches in 40,287 spectra, and a sparse-only run's output differed from dense on many scans. Cause: the RTS/fused scratch pool allocates the raw buffer uninitialized (`new double[...]`, harmless for the dense build, which clears before every use); `PrepareRawForSparse()` cleared only the current spectrum's range on first use, so a later, larger spectrum read leftover heap contents. Phase 1's RTS runs had passed only because fresh pages from the OS are zero. Fix: a buffer seen for the first time on a thread is cleared over its whole length (`iArraySizeGlobal + iXcorrProcessingOffset`). To keep verify from being fooled that way again, `COMET_PREPROCESS=verify` now poisons the scratch buffers with 7.0 when they are allocated; with the fix reverted, T86 then fails 26 checks. After the fix, verify on HeLa: batch FI_DB full file (40,287 spectra) at default settings, NL ions with `theoretical_fragment_ions` 1, and low resolution; PI_DB scans 1-8,000 (2,672 spectra); FASTA scans 1-3,000 (182, the classic non-fused path); RTS plain, NL, chimeric (20,190), low resolution, `remove_precursor_peak` 2 -- zero mismatches and zero fallbacks everywhere; batch FI output with `sparse` identical to `dense`. Preliminary batch FI throughput at 8 threads: 8,995 -> 13,200 spectra/s (one run each). |
| 2026-11-09 | 3 | **Switch point.** Synthetic sweep, single thread, `RTS_TIMING` preprocessing time per call (median of 150 spectra per point), 10-4,000 random peaks at MH+ 1,300 z 2 and 2,600 z 3: without NL ions the sparse build is faster at every point (e.g. 1,300 Da: 236 vs 5 us at 10 peaks, 265 vs 67 at 150, 553 vs 515 at 4,000); with NL ions it breaks even near a support bound of ~2x `N` (1,300 Da: 380 vs 362 us at 300 peaks; 2,600 Da: 796 vs 771 at 600). Low resolution (1.0005 / 0.4, 300 spectra per point): no NL, sparse faster or equal up to ~40-80 peaks (1,300 Da: 5 vs 3 us at 10 peaks, 13 vs 13 at 150); NL, slower from ~15 peaks. `UseSparseBuild()` now implements 4.6 with a threshold of 1x `N` and is the default (`auto`). On the 5,000 HeLa scans it routes, at 0.02 Da, every spectrum sparse without NL ions and 4,330 with; at 1.0005 Da, 433 (no NL) and 55 (NL), all large precursors with very few peaks. **End to end** (5,000 HeLa scans for RTS, single thread; the full file for batch; output identical to `dense` in every row): RTS plain 2.46 / 2.57 s -> 1.27 / 1.30 s (two runs); chimeric 24.3 -> 17.4 s; NL ions 2.90 -> 1.73 s; batch FI_DB 1 thread 1,246 -> 2,147 spectra/s, 8 threads 8,656 -> 14,337; batch PI_DB (scans 1-8,000) 8 threads 1,695 -> 1,815; batch FI_DB low resolution 936 -> 935; RTS low resolution 4.08 -> 4.20 s and with NL 4.22 -> 4.43 s in the first pass, then alternating repeats 4.24 / 4.16 / 4.68 (auto) vs 4.15 / 4.16 / 4.33 (dense), and with NL 4.43 / 4.35 / 4.52 vs 4.49 / 4.41 / 4.44 (means 4.44 vs 4.45 s): run-to-run scatter of +/-6%, and preprocessing is ~2% of a low-resolution search (18 of ~880 us per call), so no measurable change at low resolution, as intended. Not done: the dense-sweep trim from Telescope (4.10) -- the dense build now runs mostly at low resolution, where it costs 3-20 us per call. |
| 2026-11-10 | fix | **Threaded batch bug (code review of the branch).** The raw-buffer record (`tl_rawState`) was per thread, keyed by buffer address, but the batch pool hands each task the first free slot, so a buffer moves between threads: `PrepareRawForSparse()` then cleared only the bins *this* thread had recorded and left another thread's bins in the buffer. Classic (FASTA) batch with several threads gave wrong, run-to-run varying results: HeLa scans 10,000-13,000, `human.small.fasta`, 0.02 Da, 4 threads: 4,110 and 3,744 differing lines against the dense build in two runs (2,279 rows), 1 thread or dense identical. T86 did not see it (its batch runs used one thread) and verify mode saw 1 mismatch in 2,319. Fix: the state is per buffer (4.9). After the fix, 4 threads equal dense in two runs and verify reports 0 mismatches in 2,319 spectra. T86 gains a 4-thread FASTA case (the fixture's spectra repeated 80 times with fresh scan numbers; default build twice and verify vs dense, sorted output), which fails on the pre-fix code. |
| 2026-11-10 | fix | **Uninitialized scratch after a dense first use (found while timing the decoy gather).** Low-resolution FI_DB batch output at 8 threads varied from run to run (the pushed baseline differed from itself by 14-16 of 38,518 lines, and from dense by 6; 1 thread, forced dense, forced sparse and verify mode were deterministic). The RTS/fused scratch (`RtsScratch`) is allocated uninitialized; `NoteDenseRawClear()` recorded a first-seen buffer's dense-cleared range but never zeroed the rest, so a later *sparse* spectrum with peaks beyond that range (a large precursor with few peaks) read leftover heap contents -- `SetRawBinRecorded()` and the max-merge in `LoadIons()` take a nonzero leftover for a value. At 0.02 Da the arrays are megabytes (fresh zero pages); at low resolution they are small and come from recycled heap, and each thread's first spectrum is usually dense. Fix: `NoteDenseRawClear()` zeroes the remainder on first sight. After it, three 8-thread low-resolution runs of the full file are identical to each other and to dense (0 lines). T86 gains a low-resolution FI_DB case on a heap pre-filled with `MALLOC_PERTURB_` (dense scans first, then scans of a long peptide with 12 peaks, four of them above the dense scans' cleared range); it fails on the pre-fix binary and passes now. The MS1 spectral-library function (`PreprocessThreadProcMS1()`) clears and writes the same pool buffers; it now calls `NoteDenseRawClear()` after its memsets, so a later sparse spectrum clears what it left (a rarely used path, the batch library load drawing slots from the pool the MS2 sparse build uses; no test reaches it, so this is by construction only). |
| 2026-11-10 | cleanup | **The `iXcorrProcessingOffset` padding of the scratch arrays is gone (code review).** The dense fast-xcorr loops (`Preprocess()` and `PreprocessSingleSpectrumCore()`) took `dMinXcorrInten` over the correlation data for `i` in `[offset, iArraySize + offset)`, so they read up to 75 bins past the query's array. Those bins hold only zeros (raw is never written at or above `iArraySize`, and the dense setup clears `[0, iZeroBound + offset)`), so the max is the same over `i < iArraySize`, which is now what both loops do (as the sparse builder already did). With every access below `iArraySize` (and `MakeCorrData()`'s below `iZeroBound`), nothing needs the padding: the RTS scratch, the batch pool, the verify heap path and shadow buffer, and the raw-buffer clear/first-sight totals are all `iArraySizeGlobal` now, and the dense setup clears `[0, iZeroBound)`. The padding mattered only when a query's array ended within `xcorr_processing_offset` bins of the global one -- at 0.02 Da a precursor within 1.5 Da of the mass ceiling, at 1.0005 Da within 75 Da -- which is why earlier fixtures never touched it (the first session's arrays are sized from `MS1_MAX_MASS` 3000, above a 2000 Da digest ceiling). Checked under AddressSanitizer (`CometSearch` and `Comet.cpp` rebuilt with it): nine fixture tests (T67-T70, T74, T76, T77, T86, T87), batch FASTA and FI_DB on HeLa scans 8000-13000 at both resolutions with 4 threads, `rts_repro` on the 5,000-scan fixture (plain, chimeric, low resolution, low-resolution chimeric, NL), and a low-resolution search of a 3377 Da peptide under a 3400 Da ceiling in batch (T88) and RTS -- all clean. Negative control, guard removed but padding gone: both near-boundary searches report a heap-buffer-overflow (`PreprocessSingleSpectrumCore()` for RTS). T88 gains the batch case; the RTS case was run by hand (`rts_repro` is not built with a sanitizer by the tests). |
