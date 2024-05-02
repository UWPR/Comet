### Comet parameter: max_precursor_charge

- This parameter defines the maximum precursor charge state that
will be analyzed.
- Only spectra with this number of precursor charges or less will be searched.
- Valid values are any integer greater than 1.
- The default value is "6" if this parameter is missing.  A maximum
allowed value of "9" is enforced for this parameter.

Example:
```
max_precursor_charge = 5
```
