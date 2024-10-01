### Comet parameter: precursor_tolerance_type

- This parameter controls how the peptide mass tolerance
parameters([1](peptide_mass_tolerance_lower.html))([2](peptide_mass_tolerance_upper.html))
are applied.  The tolerances can be applied to the singly charged peptide mass or it can
be applied to the precursor m/z.
- Note that this parameter is applied only when amu or mmu tolerances are specified.  It is
ignored when ppm tolerances are specified.
- Valid values are 0 or 1.
- Set this parameter to 0 to specify that the mass tolerance is applied to the singly charged peptide mass.
- Set this parameter to 1 to specify that the mass tolerance is applied to the precursor m/z.
- The default value is "0" if this parameter is missing.

Example:
```
precursor_tolerance_type = 0
precursor_tolerance_type = 1
```

For example, assume a 1.0 Da [peptide_mass_tolerance](peptide_mass_tolerance.html) was
specified.  If "precursor_tolerance_type = 0" then a peptide with MH+ mass of 1250.4 will be queried
against peptide sequences with MH+ masses between 1249.4 to 1251.4.  If "precursor_tolerance_type = 1"
then say the 2+ m/z is 625.7 so the search mass range would be 624.7 m/z to 626.7 m/z which
corresponds to MH+ masses between 1248.4 and 1252.4, effectively scaling the mass tolerance by
the charge state.
