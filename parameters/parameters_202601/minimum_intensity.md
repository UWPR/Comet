### Comet parameter: minimum_intensity

- A floating point number indicating the minimum intensity value
for input the input peaks.
- If an experimental MS/MS peak intensity is less than this value,
it will not be read in and used in the analysis.
- This is one mechanism to get rid of systemmatic background noise
that has a near contant peak intensity.
- If a peak does not pass this minimum intensity threshold, it will
also not be counted towards the [minimum_peaks](minimum_peaks.html)
parameter.
- This performs a similar function as the [percentage_base_peak](percentage_base_peak.html) parameter.
- Valid values are any floating point number.
- The default value is "0.0" if this parameter is missing.

Example:
```
minimum_intensity = 0.001
minimum_intensity = 100.0
```
