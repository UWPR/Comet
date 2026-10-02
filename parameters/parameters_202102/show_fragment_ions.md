### Comet parameter: show_fragment_ions

**Note: this parameter, together with .out file output, was removed in release 2025.03.0.**

- This parameter affects .out files only e.g. when
[output_outfiles](output_outfiles.html) is set to 1.
This parameter controls whether or not the theoretical
fragment ion masses for the top peptide hit are calculated
and displayed at the end of an .out file.
- Valid values are 0 and 1.
- The default value is "0" if this parameter is missing.

Example:
```
show_fragment_ions = 0
show_fragment_ions = 1
```
