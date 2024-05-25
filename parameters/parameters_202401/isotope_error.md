### Comet parameter: isotope_error

- This parameter controls whether the peptide mass tolerance
  parameters([1](peptide_mass_tolerance_lower.html))([2](peptide_mass_tolerance_upper.html))
takes into account possible isotope errors in the precursor mass measurement.
- It is possible that an accurately read precursor mass is not measured on the monoisotopic
peak of a precursor isotopic pattern. In these cases, the precursor mass is measured on the
first isotope peak (one C13 atom) or possibly even the second or third isotope peak. To address
this problem, this "isotope_error" parameter allows you to perform an accurate mass search
(say 10 ppm) even if the precursor mass measurement is off by one or more C13 offsets.
- Valid values are 0, 1, 2, 3, 4 and 5:
  - 0 analyzes no isotope offsets, just the given precursor mass
  - 1 searches 0, +1 isotope offsets
  - 2 searches 0, +1, +2 isotope offsets
  - 3 searches 0, +1, +2, +3 isotope offsets
  - 4 searches -1, 0, +1, +2, +3 isotope offsets
  - 5 searches -1, 0, +1 isotope offsets
  - 6 searches -3, -2, -1, 0, +1, +2, +3 isotope offsets
  - 7 searches -8, -4, 0, +4, +8 isotope offsets (for +4/+8 stable isotope labeling)
- The default value is "0" if this parameter is missing.
- The behavior for values 4 through 7 were changed in release 2024.01.0.

Example:
```
isotope_error = 0
isotope_error = 4
```
