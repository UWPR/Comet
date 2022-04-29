### Comet parameter: digest_mass_range

- Defines the mass range of peptides to search (based on MH+ or the singly
protonated mass).
- This parameter has two decimal values.
- The first value is the lower mass cutoff and the second value is
the high mass cutoff.
- Only spectra with experimental MH+ masses within (or equal to) the defined
mass ranges are searched.
- Valid values are two decimal numbers where the first number must
be less or equal to the second number.
- The default value is "600.0 8000.0" if this parameter is missing.

Example:
```
digest_mass_range = 600.0 8000.0
digest_mass_range = 400.0 5000.0
```
