### Comet parameter: precursor_charge

- This parameter specifies a precursor charge range.
- This parameter expects two integer values as input.
- If the first input value is 0 then this parameter is ignored, as is
"[override_charge](override_charge.html)". A spectrum with a precursor charge is searched at that charge
state. A spectrum with no precursor charge is searched as 1+ if more than 95% of the spectrum's total peak intensity is below the precursor m/z (or the spectrum has no peaks), or as both 2+ and 3+ otherwise.
- If the first input value is not 0, then with the default
"[override_charge](override_charge.html) = 0":
  - A spectrum with a precursor charge is searched at that charge state; this
parameter does not override it.
  - A spectrum with no precursor charge is searched at all charge states between (and
inclusive of) the first and second input values.
- See "[override_charge](override_charge.html)" for how the range is applied with other settings.
- Any charge state above [max_precursor_charge](max_precursor_charge.html) is not searched.
- The default value is "0 0" if this parameter is missing.

Example:
```
precursor_charge = 0 0
precursor_charge = 0 2 (parameter is ignored because the first entry is 0)
precursor_charge = 2 6
precursor_charge = 3 3
```
