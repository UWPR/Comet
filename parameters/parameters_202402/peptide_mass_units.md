### Comet parameter: peptide_mass_units

- This parameter controls the units applied to the peptide mass tolerance
  parameters([1](peptide_mass_tolerance_lower.html))([2](peptide_mass_tolerance_upper.html)).
- Valid values are 0, 1, and 2.
- Set this parameter to 0 for amu. "amu" stands for "atomic mass unit" aka dalton.
- Set this parameter to 1 for mmu. "mmu" stands for "milli mass unit" and effectively divides the specified mass tolerance values by 1000.
- Set this parameter to 2 for ppm. "ppm" stands for "parts per million".  The applied tolerance would be (mass_tolerance * precursor_mass / 1000000).
- The default value is "0" if this parameter is missing.

Example:
```
peptide_mass_units = 0
peptide_mass_units = 1
peptide_mass_units = 2
```
