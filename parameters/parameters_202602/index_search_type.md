### Comet parameter: index_search_type

- This parameter was introduced with release v2026.02.2, corresponding with
a unified .idx format for both peptide index and fragment ion index searches.
- This parameter applies only when the .idx file specified as the search database does
not exist yet and is generated automatically from its FASTA file. It then controls which
type of index is built and searched.
- A value of "0" builds a peptide index and runs a peptide index search.
- A value of "1" builds a fragment ion index and runs a fragment ion index search.
- If this parameter is missing, a fragment ion index is built.
- An existing .idx file records the index type it was built with (Comet's "-i" command
line option builds a fragment ion index and "-j" builds a peptide index), and a search
against it uses that type; this parameter is ignored.

Example:
```
index_search_type = 0      // peptide index
index_search_type = 1      // fragment ion index
```
