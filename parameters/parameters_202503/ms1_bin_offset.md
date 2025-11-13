### Comet parameter: ms1_bin_offset

- This parameter controls how each MS1 bin of size [ms1_bin_tol](ms1_bin_tol)
is defined in terms of where each mass bin starts.
- This parameter is only relevant for the real-time search interface and
is typically specified in the C# calling program.
- See [fragment_bin_tol](fragment_bin_tol) for details of what this parameter
does; that page describes its application in how MS/MS spectra are stored internally
but the same concept applies for MS1 scans.
- The default value is "0.4" if this parameter is missing.

Example:
```
ms1_bin_offset = 0.4
```
