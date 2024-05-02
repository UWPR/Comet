### Comet parameter: peptide_mass_tolerance_upper

- This parameter controls the upper bound of the precursor mass tolerance value.
- The units of the mass tolerance is controlled by the parameter "[peptide_mass_units](peptide_mass_units.html)".
- Usually you want to specify a positive number for this upper bound tolerance.
- The default value is "3.0" if this parameter is missing.
- This parameter was implemented in version 2024.01.0.

Example:
```
peptide_mass_tolerance_upper = 3.0
peptide_mass_tolerance_upper = 20.0
```
