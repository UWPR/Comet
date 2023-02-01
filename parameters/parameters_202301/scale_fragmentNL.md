### Comet parameter: scale_fragmentNL

- The fragment neutral loss field is the 8th entry in the [variable_mod0X](/Comet/parameters/parameters_202301/variable_mod01.html)
parameter.
- If this parameter is set to 0, any fragment ion that contains the modified
residue will also have the neutral loss mass subtracted from the fragment ion
and analyzed. This is irrespective of the number of modified residues contained
in the fragment ion.
- If this parameter is set to 1, any fragment ion that contains the modified
the modified residue will have _N_ times the neutral loss mass subtracted from
the fragment ion and analyzed where _N_ is the number of modified residues
contained in the fragment.  So this parameter controls whether or not to
scale/multiply the neutral loss mass by the nubmer of modified residues.
- Valid values are 0 and 1.
- The default value is "0" if this parameter is missing.
- This parameter was added in Comet release [2023.01.0](Comet/releases/release_202301.html).

Example:
```
scale_fragmentNL = 0
scale_fragmentNL = 1
```
