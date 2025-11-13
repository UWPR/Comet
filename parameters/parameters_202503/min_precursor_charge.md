### Comet parameter: min_precursor_charge

- This parameter defines the minimum precursor charge state that will be analyzed.
- Only spectra with this number of precursor charges or more will be searched.
- Valid values are any integer greater than 1.
- The default value is "1" if this parameter is missing.  A maximum
allowed value of "9" is enforced for this parameter.
- This parameter works in conjunction with the
  [max_precursor_charge](max_precursor_charge.html) parameter to restrict
  the precursor charge range.
- This parameter was implemented in Comet version 2025.02.0.

Example:
```
min_precursor_charge = 3
```
