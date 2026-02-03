### Comet parameter: percentage_base_peak

- A floating point number between 0.0 and 1.0 which defines the intensity cutoff as a percentage of the base peak intensity.
- If an experimental MS/MS peak intensity is less than the intensity cutoff, it will not be read in and used in the analysis.
- This is one mechanism to get rid of systemmatic background noise that has a near contant peak intensity.
- If a peak does not pass this minimum intensity threshold, it will also not be counted towards the [minimum_peaks](minimum_peaks.html) parameter.
- This performs a similar function as the [minimum_intensity](minimum_intensity.html) parameter.
- Valid values are any floating point number between 0.0 and 1.0.  This parameter is ignored if the value is 0.0 or outside of this range.
- The default value is "0.0" if this parameter is missing.

Example:
```
percentage_base_peak = 0.05      # only consider peaks that are at least 5% of the base peak intensity
```
