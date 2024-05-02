### Comet parameter: max_permutations

- Some peptides with many potentially modified residues can generate A LOT of
modification combinations, adversely affecting search times.  This parameter
applies a limit to the maximum number of modification permutations that will
be tested for each peptide.
- The default value is "10000" if this parameter is missing.

Example:
```
max_permutations = 10000
max_permutations = 50000
```
