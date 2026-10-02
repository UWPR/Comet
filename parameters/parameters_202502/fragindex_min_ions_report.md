### Comet parameter: fragindex_min_ions_report

- This parameter sets the minimum number of fragment ions a peptide must match
  against the fragment ion index in order to report this peptide in the output.
- This parameter value could be different (typically the same or smaller) than the
  [fragindex_min_ions_score](fragindex_min_ions_score.html)
  parameter.
- Any peptide that passes this filter is a candidate to be reported in the output
  list, assuming it scores high enough.
- Valid values are a positive integer, 1 or larger.

Example:
```
fragindex_min_ions_report = 3
```
