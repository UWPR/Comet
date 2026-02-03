### Comet parameter: max_variable_mods_in_peptide

- Specifies the total/maximum number of residues that can be modified in a peptide.
- As opposed to specifying the maximum number of variable modifications for each
of the 6 possible variable modifications, this entry limits the global number
of variable mods possible in each peptide.
- The default value is "5" if this parameter is missing.
- Valid values are "0" (no variable mods) and higher.

Example:
```
max_variable_mods_in_peptide = 6
max_variable_mods_in_peptide = 10
```
