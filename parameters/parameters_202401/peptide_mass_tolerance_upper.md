### Comet parameter: peptide_mass_tolerance_upper

- This parameter controls the upper bound of the precursor mass tolerance value.
- The units of the mass tolerance is controlled by the parameter "[peptide_mass_units](peptide_mass_units.html)".
- Usually you want to specify a positive number for this upper bound tolerance.
- The default value is "3.0" if this parameter is missing.
- The mass error is defined as (experimental - theoretical). So if the upper tolerance
  is set to "3.0", this means that identified peptides can be up to "3.0" units smaller
  than the experimental mass.
- For a wide window, open mod search, you would want to set peptide_mass_tolerance_upper to
  something large, e.g. "200.0" Da, and [peptide_mass_tolerance_lower](peptide_mass_tolerance_lower.html)
  to something smaller, e.g. "-10.0" Da.
- This parameter was implemented in version 2024.01.0.

Example:
```
peptide_mass_tolerance_upper = 3.0
peptide_mass_tolerance_upper = 20.0
```
