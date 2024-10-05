### Comet parameter: fragindex_min_ions_report

- This parameter sets the minimum number fragment ions a peptide must match
  against the fragment ion index in order to report this peptide in the output.
- This parameter value could be different (typically same or larger) than the
  [fragindex_min_ions_score](https://uwpr.github.io/Comet/parameters/parameters_202402/fragindex_min_ions_score.html)
  parameter.
- Any peptide that passes this filter is a candidate to be reported in the output
  list, assuming it scores high enough.
- Valid values are integers, 1 or larger.

Example:
```
fragindex_min_ions_report = 3
```
