### Comet parameter: pinfile_protein_delimiter

- In the [Percolator pin output format](https://github.com/percolator/percolator/wiki/Interface#tab-delimited-file-format),
  the default delimiter for the protein field is a tab. This makes parsing that file difficult
  because tabs are also the delimiter between fields.
- This parameter allows one to specify a different character or string for
  the protein column delimiter.
- If this parameter is left blank or is missing, the default tab delimiter is used.

Example:
```
pinfile_protein_delimiter = :::       # use ::: as the delimiter between protein accessions
pinfile_protein_delimiter = ,         # use a comma as the delimiter between protein accessions
pinfile_protein_delimiter = ;         # use a semicolon as the delimiter between protein accessions
pinfile_protein_delimiter = xoxox     # use the string xoxox as the delimiter between protein accessions
```
