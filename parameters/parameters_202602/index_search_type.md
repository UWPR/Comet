### Comet parameter: index_search_type

- This parameter was introduced with release v2026.02.2, corresponding with
a unified .idx format for both peptide index and fragment ion index searches.
- When an .idx file is chosen as the search database, this parameter controls
whether a peptide index or fragment ion index search is performed.
- A value of "0" runs a peptide index search.
- A value of "1" runs a fragment ion index search.
- If this parameter is missing, a fragment ion index search is perfomed if an
.idx database is specified as the search database.

Example:
```
index_search_type = 0      // peptide index
index_search_type = 1      // fragment ion index
```
