### Comet parameter: fragindex_min_ions_score

- This parameter sets the minimum number fragment ions a peptide must match
  against the fragment ion index in order to proceed to xcorr scoring.
- This parameter could be different (typically same or larger) than the
  [fragindex_min_ions_report](https://uwpr.github.io/Comet/parameters/parameters_202402/fragindex_min_ions_report.html)
  parameter.
- This parameter is intended to allow more candidate peptides to be scored, thus
  filling out the xcorr score histogram for a better E-value calculation.
  However, only peptides that pass the
  [fragindex_min_ions_report](https://uwpr.github.io/Comet/parameters/parameters_202402/fragindex_min_ions_report.html)
  filter will be reported in the output list.  
- Valid values are integers, 1 or larger.

Example:
```
fragindex_min_ions_score = 3
```
