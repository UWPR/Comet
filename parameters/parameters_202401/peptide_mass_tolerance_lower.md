### Comet parameter: peptide_mass_tolerance_lower

- This parameter controls the lower bound of the precursor mass tolerance value.
- The units of the mass tolerance is controlled by the parameter "[peptide_mass_units](peptide_mass_units.html)".
- Usually you want to specify a negative number for this lower bound tolerance.
- The default value is "-3.0" if this parameter is missing.
- This parameter was implemented in version 2024.01.0.

Example:
```
peptide_mass_tolerance_lower = -3.0
peptide_mass_tolerance_lower = -20.0
```
