### Comet parameter: num_enzyme_termini

- This parameter specifies the number of enzyme termini a peptide must have.
- For example, if trypsin were specified as the search enzyme, only 
fully tryptic peptides would be analyzed if "num_enzyme_termini = 2"
whereas semi-tryptic peptides would be analyzed if "num_enzyme_termini = 1".
- This parameter is unused if a no-enzyme search is specified.
- Valid values are 1, 2, 8, 9.
- Set this parameter to 1 for a semi-enzyme search.
- Set this parameter to 2 for a full-enzyme search.
- Set this parameter to 8 for a semi-enzyme search, unspecific cleavage on peptide's C-terminus.
The N-terminus of each peptide will be enzyme specific and the C-terminus can be enzyme unspecific.
- Set this parameter to 9 for a semi-enzyme search, unspecific cleavage on peptide's N-terminus.
The C-terminus of each peptide will be enzyme specific and the N-terminus can be enzyme unspecific.
- The default value is "2" if this parameter is missing.

Example:
```
num_enzyme_termini = 1
num_enzyme_termini = 2
num_enzyme_termini = 8
num_enzyme_termini = 9
```
