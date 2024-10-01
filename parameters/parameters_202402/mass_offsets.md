### Comet parameter: mass_offsets

- This parameter allows the user to specify one or more "mass offsets" to apply.
- This value is effective subtracted from
each precursor mass such that peptides that are smaller than the precursor mass
by the offset value can still be matched to the respective spectrum.
The application of this parameter is for those uses cases where say a chemical
tag is applied and always falls off the peptide before/during fragmentation.
- Only positive numbers only are allowed.
- When this parameter is applied, one must add the offset "0.0" if you want
the search to also analyze peptides that match the base precursor mass.

Example:
```
mass_offsets = 42.0123 48.3812 82.030
mass_offsets = 0.0 42.0123 48.3812 82.030
```
