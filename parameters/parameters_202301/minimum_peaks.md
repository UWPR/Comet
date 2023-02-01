### Comet parameter: minimum_peaks

- An integer value indicating the minimum number of m/z-intensity pairs
that must be present in a spectrum before it is searched.
- This parameter can be used to avoid searching nearly sparse spectra
that will not likely yield an indentification.
- This parameter is checked against the spectrum after [clear_mz_range](clear_mz_range.html)
is applied but before any other spectral processing occurs
(i.e.  [remove_precursor_peak](remove_precursor_peak.html)).
- Valid values are any integer number.
- The default value is "10" if this parameter is missing.

Example:
```
minimum_peaks = 10
```
