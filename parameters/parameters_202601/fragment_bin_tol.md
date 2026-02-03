### Comet parameter: fragment_bin_tol

- This parameter controls the bin size associated with fragment ions.
- The bin size defines the mass width associated with a single MS/MS peak
as it is stored internally in an array element.
- Although it's not explicitly a fragment ion tolerance, it's probably
easiest to think of it as such.
- Note, there is a direct correlation between the value selected for
the fragment_bin_tol and the memory used in a search.  The lower the
fragment_bin_tol setting, the more memory a search will use.  A test of
loading ~40K query spectra used roughly 8GB RAM with a 1.0005 fragment_bin_tol value,
12GB RAM with a 0.02 value, and 14 GB RAM with a 0.01 value.  For small
fragment_bin_tol values where computer memory becomes an issue, 
set a non-zero value in the [spectrum_batch_size](spectrum_batch_size.html)
parameter.
- The minimum allowed value is 0.01.
- The default value is "1.0005" if the parameter is missing.

Example:
```
fragment_bin_tol = 1.0005
fragment_bin_tol = 0.02
```
