### Comet parameter: fragindex_num_spectrumpeaks

- This parameter defines the number of mass/intensity pairs that would be
  queried against the fragment ion index.
- Raw peaks are parsed and stored. The mass dimension is binned per the
  [fragment_bin_tol](fragment_bin_tol.html) and the most intense binned peaks
  are used to query the fragment ion index.
- The larger this value is, the slower a search will be as each additional
  peak, from every query spectra, will need to be searched against the
  fragment ion index.
- Note that specifying too small a value can be detrimental as, for some
  spectra, correct peptides may not pass the fragment ion index filter
  if there are not enough peaks queried.
- Valid values are integers, 1 or larger.

Example:
```
fragindex_num_spectrumpeaks = 100
fragindex_num_spectrumpeaks = 150
fragindex_num_spectrumpeaks = 200
```
