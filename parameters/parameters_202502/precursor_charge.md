### Comet parameter: precursor_charge

- This parameter specifies a precursor charge range.
- This parameter expects two integer values as input.
- If the first input value is 0 then this parameter is ignored, as is
"[override_charge](override_charge.html)".
- This charge range is only applied when "[override_charge](override_charge.html)" is set to 1, 2, or 3.
See that parameter for how the range is used with each setting.
- With "[override_charge](override_charge.html) = 0" (the default), this parameter has no effect:
  - A spectrum with a precursor charge is searched at that charge state.
  - A spectrum with no precursor charge is searched as 1+ if more than 95% of the spectrum's total peak intensity is below the precursor m/z (or the spectrum has no peaks), or as both 2+ and 3+ otherwise.
- To keep any precursor charge present in the input file and use this charge range
(instead of 2+/3+) for spectra with no precursor charge, set
"[override_charge](override_charge.html) = 3". The 1+ rule above still applies with that setting.
- Any charge state below [min_precursor_charge](min_precursor_charge.html) or above
[max_precursor_charge](max_precursor_charge.html) is not searched.
- The default value is "0 0" if this parameter is missing.

Example:
```
precursor_charge = 0 0
precursor_charge = 0 2 (parameter is ignored because the first entry is 0)
precursor_charge = 2 6
precursor_charge = 3 3
```
