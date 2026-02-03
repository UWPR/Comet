### Comet parameter: xcorr_processing_offset

- This parameter controls the number of offset bins used in the cross-correlation calculation.
- The cross-correlation is calculated by taking the scalar dot product at zero offset and subtracting the average of the scalar dot product over a range of +/- offset bins.  This parameter allows one to control/change the number of +/- bins applied in this background subtraction.
- The default value is "75" if this parameter is missing.

Example:
```
xcorr_processing_offset = 100
```
