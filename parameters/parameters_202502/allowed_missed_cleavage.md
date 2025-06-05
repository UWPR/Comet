### Comet parameter: allowed_missed_cleavage

- Number of allowed missed enzyme cleavages in a peptide.
- Parameter is not applied of the no-enzyme option is specified
in the [search_enzyme_nubmer](search_enzyme_number.html) parameter.
- Set the parameter to "0" for no missed cleavages allowed.
- The default value is "2" if this parameter is missing.

Example:
```
allowed_missed_cleavage = 0
allowed_missed_cleavage = 2
```
