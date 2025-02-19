### Comet parameter: decoy_search

- This parameter controls whether or not an internal decoy search is performed.
- Comet generates decoys by reversing each target peptide sequence, keeping the
N-terminal or C-terminal amino acid in place (depending on the "sense" value of the
digestion enzyme specified by [search_enzyme_number](search_enzyme_number.html).
For example, peptide DIGSESTK becomes decoy peptide TSESGIDK for a tryptic search
  and peptide DVINHKGGA becomes DAGGKHNIV for an Asp-N search.
- Valid parameter values are 0, 1, or 2:
  - 0 = no decoy search (default)
  - 1 = concatenated decoy search.  Target and decoy entries will be scored against
        each other and a single result is returned for each spectrum query.
  - 2 = separate decoy search.  Target and decoy entries will be scored separately
        and separate target and decoy search results will be reported.
- The default value is "0" if this parameter is missing.
- This parameter is currently not supported by the index database generation and
searches against an indexed database. For a target-decoy search against an indexed
database, please include decoy sequences in the input FASTA database prior to
the index generation.

Example:
```
decoy_search = 0
decoy_search = 1
decoy_search = 2
```
