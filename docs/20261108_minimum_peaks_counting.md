# minimum_peaks counts only scored peaks (2026-11-08)

## 1. Change

`minimum_peaks` now counts only the peaks preprocessing actually scores. A peak counts when

- its m/z is a positive finite number,
- its intensity is finite and greater than 0, and
- its intensity is at or above the intensity cutoff: `minimum_intensity`, raised to
  `percentage_base_peak` x the base-peak intensity when 0 < `percentage_base_peak` <= 1.

Before, every peak in the spectrum counted and only `clear_mz_range` peaks were subtracted,
so a spectrum with too few real peaks was still searched when zero-intensity padding or
peaks below the intensity cutoff brought the total up to `minimum_peaks`.

The rule applies identically in batch and RTS. One set of helpers in
`CometSearch/CometPreprocess.cpp` serves every path, so they cannot drift apart:

- `IntensityCutoff()` computes the cutoff for every path.
- `PeakCountsTowardMinimum()` is the per-peak test.
- `CountPeaksTowardMinimum()` is the batch counter. Batch uses it in two places:
  `ApplySpectrumFilters()`, which serves the fused and read-ahead loops, and the
  `LoadAndPreprocessSpectra()` read loop.
- RTS applies the per-peak test in `PreprocessSingleSpectrumCore()`, where the cutoff is now
  computed before the `minimum_peaks` check instead of after it.

Related: RTS used to apply a `percentage_base_peak` above 1, which put the cutoff above the
base peak and left no peaks. Batch always ignored such a value, and RTS now does too.

## 2. Effect

Only spectra whose count drops below `minimum_peaks` are affected. They are no longer searched.

- On the HeLa benchmark (`20170103_HelaQC_01`, default `minimum_intensity` 0 and
  `percentage_base_peak` 0, batch FI search), all 27,813 rank-1 rows are identical. The file
  has no zero-intensity peaks.
- Data with zero-intensity padding, or searches with a non-zero intensity cutoff, can lose
  rows. Those spectra have fewer real peaks than `minimum_peaks` asks for, but they can still
  be correctly identified. In T74, an 8-peak `LVNELTEFAK` spectrum padded to 13 used to be
  searched and identified (xcorr 1.82) and is now skipped. Lower `minimum_peaks` to keep
  such spectra.

## 3. Tests

T74 (`t74_minimum_peaks_counting`) pads an 8-real-peak spectrum past 10 in three ways, each
tested in batch (`comet.exe`) and RTS (`rts_repro`):

- with zero-intensity peaks;
- with peaks below `minimum_intensity` 10;
- with peaks below `percentage_base_peak` 0.05 x the base peak.

Each padded spectrum is skipped, and the same spectrum with 12 real peaks is searched. Before
the change, all three padded spectra were searched.

## 4. Documentation to update

- `comet.params` template text (`Comet.cpp`): updated.
- **gh-pages**: `parameters/parameters_202603/minimum_peaks.md` (or the newest versioned
  folder at release time) says the check happens "after clear_mz_range is applied but before
  any other spectral processing (i.e. remove_precursor_peak)". That still holds -- the count
  runs before precursor removal, binning and normalization -- but it no longer says which
  peaks count. Suggested addition: "This parameter is checked after clear_mz_range is
  applied, and only peaks with an intensity greater than zero and at or above the
  minimum_intensity / percentage_base_peak cutoff are counted. It is checked before other
  spectral processing such as remove_precursor_peak."
  Consider cross-links from `minimum_intensity.md` and `percentage_base_peak.md`. Not done
  here: the gh-pages branch is edited separately.

## 5. Release note

*"`minimum_peaks` now counts only the peaks that are scored: peaks with a positive intensity
at or above the `minimum_intensity` / `percentage_base_peak` cutoff. Zero-intensity peaks and
peaks below the cutoff no longer count, in batch and real-time search alike, so a spectrum
with fewer real peaks than `minimum_peaks` is no longer searched. Lower `minimum_peaks` to
keep such spectra. A `percentage_base_peak` above 1 is now ignored in real-time search, as
it always was in batch."*
