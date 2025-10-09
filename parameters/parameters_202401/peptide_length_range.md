### Comet parameter: peptide_length_range

- Defines the length range of peptides to search. 
- This parameter has two integer values.
- The first value is the minimum length cutoff and the second value is
the maximum length cutoff.
- Only peptides within the specified length range are analyzed.
- The maximum peptide length that Comet can analyze is 50.
  (Versions 2023.01.0 and older support a maximum peptide length of 63.)
- The default values are "1 50" if this parameter is missing.

Example:
```
peptide_length_range = 6 50
```
