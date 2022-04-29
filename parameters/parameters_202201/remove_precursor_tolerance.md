### Comet parameter: remove_precursor_tolerance

- This parameter specifies the mass tolerance around each precursor m/z
that would be removed when the "[remove_precursor_peak](remove_precursor_peak.html)"
option is invoked.
- The mass tolerance units is in Da (or Th if you prefer).
- Any non-negative, non-zero floating point number is valid.
- The default value is "1.5" if this parameter is missing.

Example:
```
remove_precursor_tolerance = 0.75
remove_precursor_tolerance = 1.5
```
