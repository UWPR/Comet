### Comet parameter: num_output_lines

- This parameter controls the number of search result
hits (peptides) that are reported for each spectrum query.
- If you are only interested in seeing one top hit each
per query, set this value to 1.
- This parameter value cannot be larger than the value
entered for "[num_results](num_results.html)"
which itself is limited to a maximum value of 100.
- Valid values are any positive integer 1 or greater.
- If a value less than 1 is entered, this parameter is set to 1.
- The default value is "5" if this parameter is missing.

Example:
```
num_output_lines = 1
num_output_lines = 5
num_output_lines = 10
```
