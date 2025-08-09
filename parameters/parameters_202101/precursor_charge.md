### Comet parameter: precursor_charge

- This parameter specifies the precursor charge range to search.
- This parameter expects to integer values as input.
- If the first input value is 0 then this parameter is ignored and all charge
states are searched
- Only in the case where a spectrum does not have a precursor charge will all charges
in the specified charge range be searched.
- If the first input value is not 0 then all charge states between (and inclusive of)
the first and second input values are searched.  Again, only for those spectra with no
specified precursor charge state. NOTE: one exception is that if there are no peaks
above the acquisition precursor m/z then the charge state will return 1+.
Set "[override_charge](override_charge.html) = 1" to ignore this exception.
- If a precursor charge is present for a particular spectrum, this parameter will
not override that charge state and that spectrum will always be searched.
- With the default "0 0" values and a spectrum with no precursor charge, Comet will
either search the spectrum as a 1+ or a 2+/3+.
- The default value is "0 0" if this parameter is missing.

Example:
```
precursor_charge = 0 0
precursor_charge = 0 2 (will search all charge ranges because the first entry is 0)
precursor_charge = 2 6
precursor_charge = 3 3
```
