### Comet parameter: num_results

- This parameter controls the number of peptide search results that
are stored internally.
- Depending on what post-processing tools are used, one may want to
set this to the same value as [num_output_lines](num_output_lines.html).
- When this parameter is set to a value greater than
[num_output_lines](num_output_lines.html), it allows
the SpRank value to span a larger range which may be helpful for
tools like PeptideProphet or Percolator (not likely though).
- Valid values are any integer between 1 and 100.
- The default value is "100" if this parameter is missing.

Example:
```
num_results = 50
```
