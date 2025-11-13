### Comet parameter: max_duplicate_proteins

- This parameter defines the maximum number of proteins (identifiers/accessions)
to report.  If a peptide is present in 6 total protein sequences, there is one
(first) reference protein and 5 additional duplicate proteins.  This parameter
controls how many of those 5 additional duplicate proteins are reported.
- If "[decoy_search](decoy_search.html) = 2"
is set to report separate target and decoy results, this parameter will be
applied to the target and decoy outputs separately.
- Valid values are any integer greater than or equal to 0.
- If set to "-1", there will be no limit on the number of reported additional proteins.
- The default value is "20" if this parameter is missing.  This means up to 21 proteins,
  one reference and twenty additional, will be reported for each search result.

Example:
```
max_duplicate_proteins = 0
max_duplicate_proteins = 10
```
