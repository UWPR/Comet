Comet parameter: add_B_user_amino_acid

- This parameter allows users to define their own custom residue. Just
encode the letter 'B' in the input FASTA file and specify its mass here.
- The letter 'B' has no default mass.  So the mass entered here will
be its residue mass.
- The default value is "0.0" if this parameter is missing.  If any peptide
contains the letter 'B' while this parameter value is set to 0.0, that
peptide will not be analyzed.

Example:
```
add_B_user_amino_acid = 15.9949
```
