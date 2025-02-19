### Comet parameter: remove_precursor_peak

- This parameter controls excluding/removing any precursor signals
from the input MS/MS spectrum.
- Valid values are 0, 1, 2, and 3.
- Set this parameter to 0 to not perform any precursor peak removal.
- Set this parameter to 1 to remove all peaks around the precursor m/z.
- Set this parameter to 2 to remove all charge reduced precursor peaks
as expected to be present for ETD/ECD spectra.
- Set this parameter to 3 to remove the HPO3 (-80) and H3PO4 (-98)
precursor phosphate neutral loss peaks.
- This parameter works in conjuction with
"[remove_precursor_tolerance](remove_precursor_tolerance.html)"
to specify the tolerance around each precuror m/z that will be removed.
- Valid values are 0, 1, and 2.
- The default value is "0" if this parameter is missing.

Example:
```
remove_precursor_peak = 0
remove_precursor_peak = 1
remove_precursor_peak = 2
remove_precursor_peak = 3
```
