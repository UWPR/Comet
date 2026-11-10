### Comet parameter: override_charge

- This parameter controls how the charge range specified by the
"[precursor_charge](precursor_charge.html)" parameter is applied.
- This parameter has no effect if the first "[precursor_charge](precursor_charge.html)" value is 0.
- Valid values are 0, 1, 2, and 3:
  - 0 = search spectra with a known precursor charge at that charge state.
        Search spectra with no precursor charge at all charge states in the
        "[precursor_charge](precursor_charge.html)" range.
  - 1 = ignore known precursor charge state values in the input files
        and instead search all spectra at all charge states in the
        "[precursor_charge](precursor_charge.html)" range.
  - 2 = search spectra with a known precursor charge only if that charge
        state is within the "[precursor_charge](precursor_charge.html)" range; spectra with a precursor
        charge outside that range are not searched. Search spectra with no
        precursor charge at all charge states in the "[precursor_charge](precursor_charge.html)" range.
  - 3 = search spectra with a known precursor charge at that charge state,
        even if it is outside the "[precursor_charge](precursor_charge.html)" range. A spectrum with no
        precursor charge is searched as 1+ if more than 95% of the spectrum's total peak intensity is below the precursor m/z (or the spectrum has no peaks); otherwise it is searched at all
        charge states in the "[precursor_charge](precursor_charge.html)" range.
- The default value is "0" if this parameter is missing.

Example:
```
override_charge = 0
override_charge = 1
override_charge = 2
override_charge = 3
```
