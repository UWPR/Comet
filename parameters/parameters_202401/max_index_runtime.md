### Comet parameter: max_index_runtime

- This parameter sets the maximum indexed database search run time for a scan/query.
- Valid values are integers 0 or higher representing the maximum run time in milliseconds.
- As Comet loops through analyzing peptides from the database index file,
it checks the cummulative run time of that spectrum search after each
peptide is analyzed.  If the run time exceeds the value set for this
parameter, the search is aborted and the best peptide result analyzed
up to that point is returned.
- To have no maximum search time, set this parameter value to "0".
- The default value is "0" if this parameter is missing.

Example:
```
max_index_runtime = 0
max_index_runtime = 150
```
