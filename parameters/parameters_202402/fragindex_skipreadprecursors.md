### Comet parameter: fragindex_skipreadprecursors

- This parameter controls whether or not Comet reads all precursors from the
  input files.  It uses this information to limit the peptides that
  are included in the fragment ion index.  Otherwise, every peptides within
  the mass range defined by [digest_mass_range](digest_mass_range.html)
  will be included in the fragment ion index (even though only those
  with precursor masses corresponding to experimental spectra would be
  used in the analysis).
- This parameter is meant to reduce the fragment ion index size, saving
  both memory use and run time in generating the index.
- Typically it is advantageous to apply this parameter. However, for some
  large input files with many MS/MS spectra, parsing all precursors can take
  a lot of time and may not provide a meaningful reduction in the fragment
  ion index.
- Valid values are 0 or 1.
- 0 = skip reading precursors from the input ifle
- 1 = read precursors from the input file

Example:
```
fragindex_skipreadprecursors = 0
fragindex_skipreadprecursors = 1
```
