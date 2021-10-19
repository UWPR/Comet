### Comet parameter: output_percolatorfile

- Controls whether to output search results in a [Percolator's](http://per-colator.com)
tab-delimited input format.
- Valid values are 0 (do not output) or 1 (output).
- The default value is "0" if this parameter is missing.
- The created file will have a ".pin" file extension.
- This parameter replaces the now defunct "output_pinxmlfile".

Example:
```
output_percolatorfile = 0
output_percolatorfile = 1
```
