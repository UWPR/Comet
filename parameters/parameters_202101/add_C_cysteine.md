### Comet parameter: add_C_cysteine

- Specify a static modification to the residue C.
- The specified mass is added to the unmodified mass of C.
- The default value is "0.0" if this parameter is missing *except*
  if Comet is compiled with the
  [Crux]("http://crux.ms/">Crux</a> flag on.
  For Crux compilation, the default value is "57.021464" if this parameter is missing.

Example:
```
add_C_cysteine = 57.021464
```
