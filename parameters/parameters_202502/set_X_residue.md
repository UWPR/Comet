### Comet parameter: set_A_residue through set_Z_residue

- Specify the base mass of any residue (changing it from the defaults).
- The specified new base mass will apply to both the average and
monoisotopic masses for the residue in question.
- The default value is "0.0" if this parameter is missing.

Example to redefine the base mass of alanine and histine to their N15 heavy counterparts:
```
set_A_residue = 72.034145
set_H_residue = 140.05002
```
