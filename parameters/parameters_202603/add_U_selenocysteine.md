### Comet parameter: add_U_selenocysteine

- Specify a static modification to the residue U.
- The specified mass is added to the unmodified mass of U.
- The default value is "0.0" if this parameter is missing.
- Note that releases prior to 2026.03.0 did not apply this parameter; a static
modification to U was ignored. Use release 2026.03.0 or later for this parameter.

Example:
```
add_U_selenocysteine = 15.9949
```
