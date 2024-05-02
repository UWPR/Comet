### Comet parameter: peff_format

- Specifies whether the database is a PEFF file or normal FASTA.
- Valid values are 0, 1, 2, 3, 4, 5.
- Set this parameter to 0 to search a normal FASTA file, ignoring any PEFF annotations if present.
- Set this parameter to 1 to search PEFF PSI-MOD modifications and amino acid variants.
- Set this parameter to 2 to search PEFF Unimod modifications and amino acid variants.
- Set this parameter to 3 to search PEFF PSI-MOD modifications, skipping amino acid variants.
- Set this parameter to 4 to search PEFF Unimod modifications, skipping amino acid variants.
- Set this parameter to 5 to search PEFF amino acid variants, skipping PEFF modifications.
- The default value is "0" if this parameter is missing.

Example:
```
peff_format = 0
peff_format = 1
peff_format = 5
```
