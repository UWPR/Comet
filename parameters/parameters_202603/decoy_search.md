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
- Internal decoys are supported for regular FASTA database searches and for both
indexed database searches (.idx files):
  - Peptide index searches support internal decoys starting with release 2026.01.
  - Fragment ion index searches support internal decoys starting with release 2026.03.0.
    For fragment ion index searches with earlier releases, include decoy sequences in the
    input FASTA database prior to the index generation instead.
- For a search against an .idx file, the decoy mode is the value of this parameter
when the index was built; it is stored in the .idx file and the value in the search's
params file is ignored. To change the decoy mode, rebuild the index.

Example:
```
decoy_search = 0
decoy_search = 1
decoy_search = 2
```
