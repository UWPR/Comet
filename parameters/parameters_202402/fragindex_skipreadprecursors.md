### Comet parameter: fragindex_skipreadprecursors

- This parameter controls whether or not Comet reads all precursors from the
  input files.  It usees this information to limit the peptides that
  are included in the fragment ion index.
- Typically it is advantageous to apply this parameter. However, for some
  large input files with many MS/MS spectra, parsing all precursors can take
  time and may not provide a meaningful reduction in the fragment ion index.
- 0 = skip reading precursors from the input ifle
- 1 = read precursors from the input file

Example:
```
fragindex_skipreadprecursors = 0
fragindex_skipreadprecursors = 1
```
