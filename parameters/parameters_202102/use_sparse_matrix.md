### Comet parameter: use_sparse_matrix

- Controls whether or not internal sparse matrix data representation is used.
- The sparse matrix data representation will use a significantly smaller amount
of memory/RAM for small
[fragment_bin_tol](fragment_bin_tol.html)
settings such as 0.05 or 0.01.  On the order tens of GB (gigabytes) down to a few hundred
megabytes (MB)!
- In this release, the sparse matrix searches will always be slower than the classical
data representation (i.e. use_sparse_matrix = 0).  So it should be used only when
memory is an issue.  Alternately, the
[spectrum_batch_size](spectrum_batch_size.html)
parameter can also be used to mitigate memory issues.
- Valid values are 0 and 1.
- To not use sparse matrix, set the value to 0.
- To use sparse matrix, set the value to 1.
- The default value is "0" if this parameter is missing.

Example:
```
use_sparse_matrix = 0
use_sparse_matrix = 1
```
