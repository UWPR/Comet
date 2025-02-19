### Comet parameter: clear_mz_range

- This parameter is intended for iTRAQ/TMT type data where one might
want to remove the reporter ion signals in the MS/MS spectra prior to searching.
- Defines the m/z range to clear out in each MS/MS spectra
- This parameter has two decimal values.
- The first value is the lower mass cutoff and the second value is
the high mass cutoff.
- Valid values are two decimal numbers where the first number must
be less or equal to the second number.
- All peaks between the two decimal values are cleared out.
- The default value is "0.0 0.0" if this parameter is missing.

Example:
```
clear_mz_range = 0.0 0.0
clear_mz_range = 112.5 121.5 (iTRAQ 8-plux)
clear_mz_range = 125.5 131.5 (TMT)
```
