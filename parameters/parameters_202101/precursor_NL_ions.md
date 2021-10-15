### Comet parameter: precursor_NL_ions

- Controls whether or not precursor neutral loss peaks are considered in the
xcorr scoring.
- If left blank, this parameter is ignored.
- To consider precursor neutral loss peaks, add one or more neutral loss mass
value separated by a space.
- Each entered mass value will be subtracted from the experimentral precursor
mass and resulting neutral loss m/z values for all charge states (from 1 to
      precursor charge) will be analyzed.
- As these neutral loss peaks are analyzed along side fragment ion peaks, the
fragment tolerance settings (fragment_bin_tol, fragment_bin_offset,
      theoretical_fragment_ion) apply to the precursor neutral loss peaks.
- The default value is blank/unused if this parameter is missing.
- A value of "0" or "0.0" will caues Comet to consider the intact precursor
peaks (m/z's of the precursor in all fragment charge states) as ions to analyze
in the ms/ms scan.
- Negative mass values will be ignored.

Example:
```
precursor_NL_ions =                                     ... entry blank; unused
precursor_NL_ions = 79.96633 97.97689
```
