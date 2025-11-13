### Comet parameter: override_charge

- This parameter specifies whether or not to override existing precursor
charge state information when present in the files with the charge
range specified by the "[precursor_charge](precursor_charge.html)" parameter.
- Valid values are 0, 1, 2, and 3:
  - 0 = keep any known precursor charge state values in the input files
  - 1 = ignore known precursor charge state values in the input files 
        and instead use the charge state range specified by the
        "[precursor_charge](precursor_charge.html)" parameter.
  - 2 = only search precursor charge state values that are within the
        range specified by the 
        "[precursor_charge](precursor_charge.html)" parameter.
  - 3 = keep any known precursor charge state values. For unknown
        charge states, search as singly charged if there is no
        signal above the precursor m/z or use the
        "[precursor_charge](precursor_charge.html)" range otherwise

Example:
```
override_charge = 0
override_charge = 1
override_charge = 2
override_charge = 3
```
