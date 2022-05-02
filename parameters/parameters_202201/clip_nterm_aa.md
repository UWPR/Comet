### Comet parameter: clip_nterm_aa

- This parameter controls whether Comet will automatically remove
the N-terminal amino acid from every digest peptide before analysis.
- If set to 0, the sequence is analyzed as-is.
- If set to 1, peptides will be generated based on the enzyme digestion
rule and then the n-terminal amino acid will be removed before analysis.
- Valid values are 0 and 1.
- The default value is "0" if this parameter is missing.

Example:
```
clip_nterm_aa= 0
clip_nterm_aa= 1
```
